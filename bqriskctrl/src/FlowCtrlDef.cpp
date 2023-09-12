/*!
 * \file FlowCtrlDef.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/14
 *
 * \brief
 */

#include "FlowCtrlDef.hpp"

namespace bq {

//! 风控触发 PubTopic 中用到
std::string FlowCtrlRule::toJson() const {
  if (!jsonStr_.empty()) return jsonStr_;

  rapidjson::StringBuffer strBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);
  writer.StartObject();
  writer.Key("no");
  writer.Uint(no_);
  writer.Key("name");
  writer.String(name_.c_str());
  writer.Key("step");
  writer.String(step_.c_str());
  writer.Key("target");
  writer.String(ENUM_TO_STR(target_).c_str());
  writer.Key("condition");
  writer.String(condition_.c_str());
  writer.Key("limitType");
  writer.String(ENUM_TO_STR(limitType_).c_str());

  switch (limitType_) {
    case FlowCtrlLimitType::NumLimitEachTime:
    case FlowCtrlLimitType::NumLimitTotal:
      writer.Key("limitValueInConf");
      writer.Double(limitValueInConf_.value_);
      break;
    case FlowCtrlLimitType::NumLimitWithinTime:
      writer.Key("limitValueInConf");
      writer.Uint(limitValueInConf_.tsQue_.size());
      writer.Key("msInterval");
      writer.String(fmt::format("{}ms", limitValueInConf_.msInterval_).c_str());
      break;
  }

  writer.EndObject();
  return strBuf.GetString();
}

}  // namespace bq
