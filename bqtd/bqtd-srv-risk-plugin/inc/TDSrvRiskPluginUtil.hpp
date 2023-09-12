/*!
 * \file TDSrvRiskPluginUtil.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/20
 *
 * \brief
 */

#include "util/Pch.hpp"

namespace bq {

struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;

std::string MakeRiskCtrlTriggerDetails(const std::string& name, int statusCode,
                                       const std::string& statusMsg,
                                       const OrderInfoSPtr& orderInfo);

std::string MakeSqlOfRiskCtrlTriggerInfo(const std::string& name,
                                         int statusCode,
                                         const std::string& statusMsg,
                                         const std::string& details);

std::string MakeSqlOfRiskCtrlTriggerInfo(const std::string& name,
                                         int statusCode,
                                         const std::string& statusMsg,
                                         const OrderInfoSPtr& orderInfo);

}  // namespace bq
