/*!
 * \file TDSrvRiskPluginMgr.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "TDSrvRiskPluginMgr.hpp"

#include "TDSrv.hpp"
#include "TDSrvRiskPlugin.hpp"
#include "def/StatusCode.hpp"
#include "util/File.hpp"
#include "util/Logger.hpp"

namespace bq::td::srv {

TDSrvRiskPluginMgr::TDSrvRiskPluginMgr(TDSrv* tdSrv, std::uint32_t no)
    : tdSrv_(tdSrv), no_(no) {}

int TDSrvRiskPluginMgr::load() {
  const auto [statusCode, no2LibPath] = initNo2LibPath();
  if (statusCode != 0) {
    return statusCode;
  }
  initPlugIn(no2LibPath);
  return 0;
}

std::tuple<int, No2LibPath> TDSrvRiskPluginMgr::initNo2LibPath() {
  No2LibPath no2LibPath;

  const auto tdSrvRiskPluginPath =
      CONFIG["riskCtrlModuleComb"][no_]["riskCtrlpluginPath"].as<std::string>();
  const auto [statusCode, fileGroup] =
      GetFileGroupFromPathRecursively(tdSrvRiskPluginPath);
  if (statusCode != 0) {
    LOG_W("[{}] Load tdSrv risk plugin from {} failed.", no_,
          tdSrvRiskPluginPath);
    return {statusCode, no2LibPath};
  }

  for (const auto& file : fileGroup) {
    LOG_T("[{}] Begin to check {}", no_, file.string());
    if (file.extension().string() != ".so") continue;

    const auto no = getNo(file);
    if (no < 0) {
      LOG_T("[{}] No of file {} is {}, less than 0, skip.", no_, file.string(),
            no);
      continue;
    }

    boost::dll::fs::path libPath(file.parent_path());
    libPath /= file.stem();

    boost::dll::library_info info(file);
    std::vector<std::string> exports = info.symbols("PlugIn");
    if (exports.empty()) {
      LOG_W("[{}] Get invalid file {} when load risk plugin.", no_,
            file.string());
      continue;
    }

    no2LibPath.emplace(no, libPath);
    LOG_T("[{}] Add {} - {} to no2LibPath.", no_, no, libPath.string());
  }

  return {0, no2LibPath};
}

int TDSrvRiskPluginMgr::getNo(const boost::filesystem::path& file) {
  boost::filesystem::path parentPathAndStem = file.parent_path();
  parentPathAndStem /= file.stem();

  std::vector<std::string> strGroup;
  boost::split(strGroup, parentPathAndStem.string(), boost::is_any_of("-"));

  boost::optional<std::size_t> optNo;
#ifndef NDEBUG
  if (strGroup.size() < 3) {
    LOG_W("[{}] Get invalid file {} when load risk plugin.", no_,
          file.string());
    return -1;
  }
  if (strGroup[strGroup.size() - 1] != "d") {
    return -1;
  }
  optNo = CONV_OPT(std::size_t, strGroup[strGroup.size() - 2]);
#else
  if (strGroup.size() < 2) {
    LOG_W("[{}] Get invalid file {} when load risk plugin.", no_,
          file.string());
    return -1;
  }
  if (strGroup[strGroup.size() - 1] == "d") {
    return -1;
  }
  optNo = CONV_OPT(std::size_t, strGroup[strGroup.size() - 1]);
#endif
  if (optNo == boost::none) {
    LOG_W("[{}] Get invalid file {} when load risk plugin.", no_,
          file.string());
    return -1;
  }

  const auto no = optNo.value();
  if (no >= MAX_TD_SRV_RISK_PLUGIN_NUM) {
    LOG_W("[{}] Get invalid file {} when load risk plugin.", no_,
          file.string());
    return -1;
  }

  return no;
}

void TDSrvRiskPluginMgr::initPlugIn(const No2LibPath& no2LibPath) {
  for (std::size_t no = 0; no < MAX_TD_SRV_RISK_PLUGIN_NUM; ++no) {
    const auto iter = no2LibPath.find(no);
    if (iter == std::end(no2LibPath)) {
      const auto oldPlugin = safeTDSrvRiskPluginGroup_[no].get();
      if (oldPlugin != nullptr) {
        oldPlugin->unload();
        safeTDSrvRiskPluginGroup_[no].set(nullptr);
        LOG_I("[{}] Unload risk plugin {} - {} success.", no_, no,
              oldPlugin->name());
      } else {
      }

    } else {
      const auto& libPath = iter->second;
      const auto newPlugin = createPlugin(no, libPath);
      if (!newPlugin) continue;

      const auto oldPlugin = safeTDSrvRiskPluginGroup_[no].get();
      if (oldPlugin != nullptr) {
        if (newPlugin->getMD5SumOfConf() != oldPlugin->getMD5SumOfConf()) {
          if (newPlugin->enable()) {
            if (oldPlugin->enable()) {
              oldPlugin->unload();
              const auto ret = newPlugin->load();
              if (ret == 0) {
                safeTDSrvRiskPluginGroup_[no].set(newPlugin);
                LOG_I("[{}] Update risk plugin {} - {} to {} success.", no_, no,
                      oldPlugin->name(), newPlugin->name());
              } else {
                LOG_W("[{}] Update risk plugin {} - {} to {} failed. [{} - {}]",
                      no_, no, oldPlugin->name(), newPlugin->name(), ret,
                      GetStatusMsg(ret));
              }

            } else {
              // newPlugin enable oldPlugin disable
              const auto ret = newPlugin->load();
              if (ret == 0) {
                safeTDSrvRiskPluginGroup_[no].set(newPlugin);
                LOG_I("[{}] Load risk plugin {} - {} success.", no_, no,
                      newPlugin->name());
              } else {
                LOG_W("[{}] Load risk plugin {} - {} failed. [{} - {}]", no_,
                      no, newPlugin->name(), ret, GetStatusMsg(ret));
              }
            }

          } else {
            if (oldPlugin->enable()) {
              // newPlugin disable old plugin enable
              oldPlugin->unload();
              safeTDSrvRiskPluginGroup_[no].set(nullptr);
              LOG_I("[{}] Unload risk plugin {} - {} success.", no_, no,
                    oldPlugin->name());

            } else {
              // newPlugin and oldPlugin are all disable
            }
          }
        } else {
        }

      } else {
        if (newPlugin->enable()) {
          const auto ret = newPlugin->load();
          if (ret == 0) {
            safeTDSrvRiskPluginGroup_[no].set(newPlugin);
            LOG_I("[{}] Load risk plugin {} - {} success.", no_, no,
                  newPlugin->name());
          } else {
            LOG_W("[{}] Load risk plugin {} - {} failed. [{} - {}]", no_, no,
                  newPlugin->name(), ret, GetStatusMsg(ret));
          }
        } else {
          // newPlugin disable and oldPlugin == nullptr
        }
      }
    }
  }
}

TDSrvRiskPluginSPtr TDSrvRiskPluginMgr::createPlugin(
    std::size_t no, const boost::filesystem::path& libPath) {
  auto plugin = GetPlugin(libPath, "CreatePlugin", tdSrv_);
  const auto configFilename = fmt::format("{}.yaml", libPath.string());
  const auto statusCode = plugin->init(configFilename);
  if (statusCode != 0) {
    LOG_I(
        "[{}] Load risk plugin {} - {} "
        "failed because of init by config {} failed",
        no_, no, libPath.stem().string(), configFilename);
    return nullptr;
  }
  return plugin;
}

void TDSrvRiskPluginMgr::onThreadStart(std::uint32_t combNo,
                                       std::uint32_t threadNo) {
  for (const auto& safePlugin : safeTDSrvRiskPluginGroup_) {
    auto plugin = safePlugin.get();
    if (!plugin) continue;
    plugin->onThreadStart(combNo, threadNo);
  }
}

void TDSrvRiskPluginMgr::onThreadExit(std::uint32_t combNo,
                                      std::uint32_t threadNo) {
  for (const auto& safePlugin : safeTDSrvRiskPluginGroup_) {
    auto plugin = safePlugin.get();
    if (!plugin) continue;
    plugin->onThreadExit(combNo, threadNo);
  }
}

int TDSrvRiskPluginMgr::onRiskCtrlConfChg(const std::string& conf,
                                          std::uint32_t combNo,
                                          std::uint32_t threadNo) {
  for (const auto& safePlugin : safeTDSrvRiskPluginGroup_) {
    auto plugin = safePlugin.get();
    if (!plugin) continue;
    const auto statusCode = plugin->onRiskCtrlConfChg(conf, combNo, threadNo);
    if (statusCode != 0) {
      return statusCode;
    }
  }
  return 0;
}

std::tuple<int, std::string> TDSrvRiskPluginMgr::onOrder(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  for (const auto& safePlugin : safeTDSrvRiskPluginGroup_) {
    auto plugin = safePlugin.get();
    if (!plugin) continue;
    const auto [statusCode, details] = plugin->onOrder(order, combNo, threadNo);
    if (statusCode != 0) {
      return {statusCode, details};
    }
  }
  return {0, ""};
}

std::tuple<int, std::string> TDSrvRiskPluginMgr::onCancelOrder(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  for (const auto& safePlugin : safeTDSrvRiskPluginGroup_) {
    auto plugin = safePlugin.get();
    if (!plugin) continue;
    const auto [statusCode, details] =
        plugin->onCancelOrder(order, combNo, threadNo);
    if (statusCode != 0) {
      return {statusCode, details};
    }
  }
  return {0, ""};
}

std::tuple<int, std::string> TDSrvRiskPluginMgr::onOrderRet(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  for (const auto& safePlugin : safeTDSrvRiskPluginGroup_) {
    auto plugin = safePlugin.get();
    if (!plugin) continue;
    const auto [statusCode, details] =
        plugin->onOrderRet(order, combNo, threadNo);
    if (statusCode != 0) {
      return {statusCode, details};
    }
  }
  return {0, ""};
}

std::tuple<int, std::string> TDSrvRiskPluginMgr::onCancelOrderRet(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  for (const auto& safePlugin : safeTDSrvRiskPluginGroup_) {
    auto plugin = safePlugin.get();
    if (!plugin) continue;
    const auto [statusCode, details] =
        plugin->onCancelOrderRet(order, combNo, threadNo);
    if (statusCode != 0) {
      return {statusCode, details};
    }
  }
  return {0, ""};
}

}  // namespace bq::td::srv
