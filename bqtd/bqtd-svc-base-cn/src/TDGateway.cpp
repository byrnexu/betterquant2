/*!
 * \file TDGateway.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/01
 *
 * \brief
 */

#include "TDGateway.hpp"

namespace bq::td::svc {

TDGateway::TDGateway(TDSvcOfCN* tdSvc) : tdSvc_(tdSvc) {}

}  // namespace bq::td::svc
