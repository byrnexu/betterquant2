/*!
 * \file FlowCtrlUtil.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/11
 *
 * \brief
 */

#include "FlowCtrlUtil.hpp"

#include "def/BQConst.hpp"
#include "def/ConditionConst.hpp"
#include "def/ConditionUtil.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/Def.hpp"
#include "def/Field.hpp"
#include "def/StatusCode.hpp"

namespace bq {

bool LimitTargetsAreStateful(FlowCtrlTarget target) {
  switch (target) {
    //! 单笔下单数量
    case FlowCtrlTarget::OrderSizeEachTime:
      return false;
    //! 累计下单数量
    case FlowCtrlTarget::OrderSizeTotal:
      return true;

    //! 单笔下单金额
    case FlowCtrlTarget::OrderAmtEachTime:
      return false;
    //! 累计下单金额
    case FlowCtrlTarget::OrderAmtTotal:
      return true;

    //! 累计下单笔数
    case FlowCtrlTarget::OrderTimesTotal:
      return true;
    //! 单位时间下单笔数
    case FlowCtrlTarget::OrderTimesWithinTime:
      return true;

    //! 累计撤单笔数
    case FlowCtrlTarget::CancelOrderTimesTotal:
      return true;
    //! 单位时间撤单笔数
    case FlowCtrlTarget::CancelOrderTimesWithinTime:
      return true;

    //! 累计被拒单笔数
    case FlowCtrlTarget::RejectOrderTimesTotal:
      return true;
    //! 单位时间被拒单笔数
    case FlowCtrlTarget::RejectOrderTimesWithinTime:
      return true;

    //! 累计成交数量
    case FlowCtrlTarget::HoldVolTotal:
      return true;
    //! 累计成交数量
    case FlowCtrlTarget::HoldAmtTotal:
      return true;

    //! 当日累计开仓
    case FlowCtrlTarget::OpenTDayTotal:
      return true;

    default:
      return true;
  }

  return true;
}

std::tuple<int, std::string, FlowCtrlLimitType> GetLimitType(
    FlowCtrlTarget target) {
  switch (target) {
    //! 单笔下单数量
    case FlowCtrlTarget::OrderSizeEachTime:
      return {0, "", FlowCtrlLimitType::NumLimitEachTime};
    //! 累计下单数量
    case FlowCtrlTarget::OrderSizeTotal:
      return {0, "", FlowCtrlLimitType::NumLimitTotal};

    //! 单笔下单金额
    case FlowCtrlTarget::OrderAmtEachTime:
      return {0, "", FlowCtrlLimitType::NumLimitEachTime};
    //! 累计下单金额
    case FlowCtrlTarget::OrderAmtTotal:
      return {0, "", FlowCtrlLimitType::NumLimitTotal};

    //! 累计下单笔数
    case FlowCtrlTarget::OrderTimesTotal:
      return {0, "", FlowCtrlLimitType::NumLimitTotal};
    //! 单位时间下单笔数
    case FlowCtrlTarget::OrderTimesWithinTime:
      return {0, "", FlowCtrlLimitType::NumLimitWithinTime};

    //! 累计撤单笔数
    case FlowCtrlTarget::CancelOrderTimesTotal:
      return {0, "", FlowCtrlLimitType::NumLimitTotal};
    //! 单位时间撤单笔数
    case FlowCtrlTarget::CancelOrderTimesWithinTime:
      return {0, "", FlowCtrlLimitType::NumLimitWithinTime};

    //! 累计被拒单笔数
    case FlowCtrlTarget::RejectOrderTimesTotal:
      return {0, "", FlowCtrlLimitType::NumLimitTotal};
    //! 单位时间被拒单笔数
    case FlowCtrlTarget::RejectOrderTimesWithinTime:
      return {0, "", FlowCtrlLimitType::NumLimitWithinTime};

    //! 累计成交数量
    case FlowCtrlTarget::HoldVolTotal:
      return {0, "", FlowCtrlLimitType::NumLimitTotal};
    //! 累计成交数量
    case FlowCtrlTarget::HoldAmtTotal:
      return {0, "", FlowCtrlLimitType::NumLimitTotal};

    //! 当日累计开仓
    case FlowCtrlTarget::OpenTDayTotal:
      return {0, "", FlowCtrlLimitType::NumLimitTotal};

    default: {
      const auto statusMsg =
          fmt::format("Get limit type failed because of invalid target {}.",
                      magic_enum::enum_name(target));
      return {-1, statusMsg, FlowCtrlLimitType::Others};
    }
  }
}

//! 10000 or 100/1000ms -> LimitValue
std::tuple<int, std::string, LimitValue> MakeLimitValue(
    FlowCtrlLimitType limitType, const std::string& limitValueInStrFmt) {
  LimitValue limitValue;

  switch (limitType) {
    //! 数量or金额限制
    case FlowCtrlLimitType::NumLimitEachTime:
    case FlowCtrlLimitType::NumLimitTotal: {
      //! 10000 -> LimitValue
      const auto v = CONV_OPT(Decimal, limitValueInStrFmt);
      if (v == boost::none) {
        const auto statusMsg = fmt::format(
            "Make risk ctrl value of limit failed "
            "because of invalid limit value {}.",
            limitValueInStrFmt);
        return {-1, statusMsg, limitValue};
      }
      limitValue.value_ = v.value();
    } break;

    //! 单位时间内的数量限制，这种情况下 limitValueInConf_ 用于生成
    //! ConditionValue2LimitValueGroup 的值的模板
    case FlowCtrlLimitType::NumLimitWithinTime: {
      //! 100/1000ms -> LimitValue
      std::vector<std::string> fieldGroup;
      boost::split(fieldGroup, limitValueInStrFmt,
                   boost::is_any_of(SEP_OF_LIMIT_VALUE));

      //! 合法性检查
      if (fieldGroup.size() != 2 || !boost::iends_with(fieldGroup[1], "ms")) {
        const auto statusMsg = fmt::format(
            "Make risk ctrl value of limit failed "
            "because of invalid limit value {}.",
            limitValueInStrFmt);
        return {-1, statusMsg, limitValue};
      }

      //! 合法性检查，然后获取时间间隔
      const auto tmp = boost::erase_tail_copy(fieldGroup[1], 2);
      auto v = CONV_OPT(std::uint32_t, tmp);
      if (v == boost::none) {
        const auto statusMsg = fmt::format(
            "Make risk ctrl value of limit failed "
            "because of invalid limit value {}.",
            limitValueInStrFmt);
        return {-1, statusMsg, limitValue};
      }
      limitValue.msInterval_ = v.value();

      //! 合法性检查，然后获取时间间隔内可以执行的任务数
      v = CONV_OPT(std::uint32_t, fieldGroup[0]);
      if (v == boost::none) {
        const auto statusMsg = fmt::format(
            "Make risk ctrl value of limit failed "
            "because of invalid limit value {}.",
            limitValueInStrFmt);
        return {-1, statusMsg, limitValue};
      }
      const auto queueSize = v.value();
      if (queueSize > MAX_TS_QUEUE_SIZE || queueSize == 0) {
        const auto statusMsg = fmt::format(
            "Make risk ctrl value of limit failed "
            "because of tsQue size {} greater than {} or equal to 0.",
            queueSize, MAX_TS_QUEUE_SIZE);
        return {-1, statusMsg, limitValue};
      }

      limitValue.tsQue_ =
          TSQue(boost::circular_buffer<std::uint64_t>(queueSize));
      //! 造queueSize个假的2000-01-01 08:00:00的时间戳放进队列
      limitValue.reset(queueSize);

    } break;

    default:
      const auto statusMsg = fmt::format(
          "Make risk ctrl value of limit failed "
          "because of invalid limit type {}.",
          magic_enum::enum_name(limitType));
      return {-1, statusMsg, limitValue};
  }

  return {0, "", limitValue};
}

std::string MakeRuleInfo(int statusCode) {
  rapidjson::StringBuffer strBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);

  writer.StartObject();

  writer.Key("no");
  writer.Uint(statusCode);

  const std::string statusMsg = GetStatusMsg(statusCode);
  writer.Key("msg");
  writer.String(statusMsg.c_str());

  writer.EndObject();
  return strBuf.GetString();
}

}  // namespace bq
