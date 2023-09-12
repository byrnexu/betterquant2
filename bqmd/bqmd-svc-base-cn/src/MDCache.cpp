/*!
 * \file MDCache.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/27
 *
 * \brief
 */

#include "MDCache.hpp"

#include "Config.hpp"
#include "MDSvcOfCN.hpp"
#include "def/DataStruOfMD.hpp"
#include "def/Def.hpp"
#include "def/StatusCode.hpp"
#include "tdeng/TDEngUtil.hpp"
#include "util/BQMDHis.hpp"
#include "util/Datetime.hpp"
#include "util/Json.hpp"
#include "util/Logger.hpp"
#include "util/MarketDataCond.hpp"
#include "util/String.hpp"

namespace bq::md::svc {

int MDCache::start() {
  //! 每次缓存的行情的时间的长度
  secOfCacheMD_ = CONFIG["simedMode"]["secOfCacheMD"].as<std::uint32_t>(60);
  if (secOfCacheMD_ > MAX_SEC_OF_CACHE_MD_SIM) {
    LOG_W(
        "Cache a batch of his market data failed "
        "because of secOfCacheMD greater than {}.",
        MAX_SEC_OF_CACHE_MD_SIM);
    return -1;
  }

  //! 因为localTs和exchTs有偏移，所以回测的localTs最小值和最大值分别往下和往上偏移
  const auto usOffsetOfExchAndLocalTs =
      CONFIG["simedMode"]["secOffsetOfExchAndLocalTs"].as<std::uint64_t>() *
      1000000ULL;

  //! 获取回测起始的unix时间戳
  int statusCode = 0;
  const auto playbackDateTimeStart =
      CONFIG["simedMode"]["playbackDateTimeStart"].as<std::string>("");
  std::tie(statusCode, exchTsStart_) =
      ConvertISODatetimeToTs(playbackDateTimeStart);
  if (statusCode != 0) {
    LOG_W(
        "Cache a batch of his market data failed "
        "because of invalid playbackDateTimeStart {} in config.",
        playbackDateTimeStart);
    return -1;
  }

  //! 回测起始的localTs往小偏移
  tsStart_ = exchTsStart_ - usOffsetOfExchAndLocalTs;

  //! 获取回测结束的unix时间戳
  const auto playbackDateTimeEnd =
      CONFIG["simedMode"]["playbackDateTimeEnd"].as<std::string>("");
  std::tie(statusCode, exchTsEnd_) =
      ConvertISODatetimeToTs(playbackDateTimeEnd);
  if (statusCode != 0) {
    LOG_W(
        "Cache a batch of his market data failed "
        "because of invalid playbackDateTimeEnd {} in config.",
        playbackDateTimeEnd);
    return -1;
  }

  //! 回测结束的localTs往大偏移
  tsEnd_ = exchTsEnd_ + usOffsetOfExchAndLocalTs;

  if (exchTsEnd_ <= exchTsStart_) {
    LOG_W(
        "Cache a batch of his market data failed because of invalid "
        "playbackDateTimeEnd {} <= playbackDateTimeStart {} in config.",
        playbackDateTimeEnd, playbackDateTimeStart);
    return -1;
  }

  tsStartOfCurCache_ = tsStart_;

  keepRunning_.store(true);
  threadCacheMDHis_ = std::make_unique<std::thread>([this]() { cacheHisMD(); });

  return 0;
}

void MDCache::stop() {
  keepRunning_.store(false);
  if (threadCacheMDHis_->joinable()) {
    threadCacheMDHis_->join();
  }
}

Ts2MarketDataOfSimGroupSPtr MDCache::pop() {
  Ts2MarketDataOfSimGroupSPtr ret{nullptr};
  {
    std::lock_guard<std::mutex> guard(mtxMDCache_);
    if (!mdCache_.empty()) {
      ret = mdCache_.front();
      mdCache_.pop_front();
    }
  }
  return ret;
}

void MDCache::cacheHisMD() {
  //! 内存中准备用于回放的cache数量
  const auto cacheNumLimit =
      CONFIG["simedMode"]["cacheNumLimit"].as<std::uint32_t>(5);

  //! 每次检查内存中cache数量是否到达cacheNumLimit的时间间隔
  const auto msIntervalOfCheckCacheNum =
      CONFIG["simedMode"]["msIntervalOfCheckCacheNum"].as<std::uint32_t>(1);

  while (keepRunning_.load()) {
    //! 不停的检查批次是否到达cacheNumLimit，不够的话继续缓存
    while (true) {
      //! 如果已经缓存到了 tsEnd_ 就不再缓存。
      if (tsStartOfCurCache_ >= tsEnd_) {
        LOG_I("Has been cached to the md of {} in config, stop caching.",
              ConvertTsToPtime(tsEnd_));
        keepRunning_.store(false);
        break;
      }

      if (keepRunning_.load() == false) {
        break;
      }

      //! 获取当前实际的cache数量
      std::size_t cacheNum = 0;
      {
        std::lock_guard<std::mutex> guard(mtxMDCache_);
        cacheNum = mdCache_.size();
      }

      //! 如果 cache 中已经有 cacheNumLimit 批次的数据，那么暂停缓存
      if (cacheNum >= cacheNumLimit) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(msIntervalOfCheckCacheNum));
        break;
      }

      //! 如果 cache 的批次不足那么继续 cache
      cacheHisMDOf1Batch();
    }
  }
}

void MDCache::cacheHisMDOf1Batch() {
  //! 获取本次缓存的结束时间点 tsEndOfCurCache
  auto tsEndOfCurCache = tsStartOfCurCache_ + secOfCacheMD_ * 1000 * 1000;

  //! 如果本次缓存的结束时间点已经超过配置中的回放结束时间 tsEnd_，那么本次缓
  //! 存的而结束时间点置为 tsEnd_
  if (tsEndOfCurCache > tsEnd_) {
    tsEndOfCurCache = tsEnd_;
  }

  //! 从 tdengine 获取历史行情
  auto mdCacheOfCurBatch =
      makeMDCacheOfCurBatch(tsStartOfCurCache_, tsEndOfCurCache);

  //! 计算两条回放记录之间的时间差用于回放间隔的处理
  calcDelayBetweenAdjacentMD(mdCacheOfCurBatch);

  LOG_I("Cache {} numbers of market data between {} - {} success. ",
        mdCacheOfCurBatch->size(), ConvertTsToPtime(tsStartOfCurCache_),
        ConvertTsToPtime(tsEndOfCurCache));

  //! 将当前批次的 market data cache 插入 mdCache
  if (!mdCacheOfCurBatch->empty()) {
    std::lock_guard<std::mutex> guard(mtxMDCache_);
    mdCache_.emplace_back(mdCacheOfCurBatch);
  }

  //! 下一次缓存从 tsEndOfCurCache 开始
  tsStartOfCurCache_ = tsEndOfCurCache;
}

Ts2MarketDataOfSimGroupSPtr MDCache::makeMDCacheOfCurBatch(
    std::uint64_t startLocalTs, std::uint64_t endLocalTs) {
  //! 从数据库查询历史行情
  auto queryMD = [&, this]() {
    std::string sql;
    const auto prefixOfSql = "SELECT * FROM marketdata.origdata WHERE ";
    const auto condOfSql =
        CONFIG["simedMode"]["playbackMD"].as<std::string>("");
    if (!condOfSql.empty()) {
      sql.append(condOfSql).append(" ");
    }
    sql.append(fmt::format("AND localts >= {} AND localts < {} ", startLocalTs,
                           endLocalTs));
    sql.append(fmt::format("AND apiname = '{}'", mdSvc_->getApiName()));

    const auto [statusCode, statusMsg, recNum, recSet] =
        tdeng::QueryDataFromTDEng(mdSvc_->getTDEngConnpool(), sql, UINT32_MAX);
    if (statusCode != 0) {
      LOG_W("Make market data of cur batch failed. [{} - {}] {}", statusCode,
            statusMsg, sql);
      return std::string("");
    }
    return recSet;
  };

  auto parseMD = [this](const std::string& recSet) {
    auto ret = std::make_shared<Ts2MarketDataOfSimGroup>();

    Doc doc;
    doc.Parse(recSet.data());
    for (std::size_t i = 0; i < doc["recSet"].Size(); ++i) {
      const auto exchTs = doc["recSet"][i]["exchts"].GetUint64();
      if (exchTs < exchTsStart_ || exchTs > exchTsEnd_) continue;

      auto rec = std::make_shared<MarketDataOfSim>();
      rec->localTs_ = doc["recSet"][i]["localts"].GetUint64();
      rec->mdType_ =
          magic_enum::enum_cast<MDType>(doc["recSet"][i]["mdtype"].GetUint())
              .value();
      rec->data_ = Base64Decode(doc["recSet"][i]["data"].GetString());
      ret->emplace(rec->localTs_, rec);

      //! 这个循环时间很长，如果 keepRunning_ 为 false 跳出函数
      if (keepRunning_.load() == false) {
        return std::make_shared<Ts2MarketDataOfSimGroup>();
      }
    }

    return ret;
  };

  const auto recSet = queryMD();
  if (recSet.empty()) {
    return std::make_shared<Ts2MarketDataOfSimGroup>();
  }
  const auto ret = parseMD(recSet);
  return ret;
}

void MDCache::calcDelayBetweenAdjacentMD(
    Ts2MarketDataOfSimGroupSPtr& mdCacheOfCurBatch) {
  for (auto iter = std::begin(*mdCacheOfCurBatch);
       iter != std::end(*mdCacheOfCurBatch); ++iter) {
    const auto localTs = iter->first;
    auto& marketDataOfSim = iter->second;
    auto nextIter = std::next(iter, 1);
    if (nextIter != std::end(*mdCacheOfCurBatch)) {
      marketDataOfSim->delay_ = nextIter->second->localTs_ - localTs;
    } else {
      marketDataOfSim->delay_ = 0;
    }
  }
}

}  // namespace bq::md::svc
