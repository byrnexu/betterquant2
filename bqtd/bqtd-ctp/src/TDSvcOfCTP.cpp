/*!
 * \file TDSvcOfCTP.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/01
 *
 * \brief
 */

#include "TDSvcOfCTP.hpp"

#include "Config.hpp"
#include "TDGatewayOfCTP.hpp"
#include "TDSvcDef.hpp"
#include "TDSvcUtil.hpp"
#include "util/Literal.hpp"
#include "util/Logger.hpp"
#include "util/ScheduleTaskBundle.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::td::svc::ctp {

int TDSvcOfCTP::beforeInit() {
  const auto tdGateway = std::make_shared<TDGatewayOfCTP>(this);
  setTDGateway(tdGateway);
  return 0;
}

void TDSvcOfCTP::doInitScheduleTaskBundle() {
  getScheduleTaskBundle()->emplace_back(std::make_shared<ScheduleTask>(
      "testOrder",
      [this]() {
        auto asyncTask = MakeTDSrvSignal(MSG_ID_ON_TEST_ORDER);
        const auto ret = getTDSrvTaskDispatcher()->dispatch(asyncTask);
        return ret == 0 ? true : false;
      },
      ExecAtStartup::False, MilliSecInterval(1000), ExecTimes(0)));

  getScheduleTaskBundle()->emplace_back(std::make_shared<ScheduleTask>(
      "testCancelOrder",
      [this]() {
        auto asyncTask = MakeTDSrvSignal(MSG_ID_ON_TEST_CANCEL_ORDER);
        const auto ret = getTDSrvTaskDispatcher()->dispatch(asyncTask);
        return ret == 0 ? true : false;
      },
      ExecAtStartup::False, MilliSecInterval(5000), ExecTimes(0)));
}

}  // namespace bq::td::svc::ctp
