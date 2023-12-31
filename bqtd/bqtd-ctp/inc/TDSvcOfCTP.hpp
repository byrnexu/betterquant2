/*!
 * \file TDSvcOfCTP.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/01
 *
 * \brief
 */

#pragma once

#include "TDSvcOfCN.hpp"

namespace bq::td::svc::ctp {

class TDSvcOfCTP : public TDSvcOfCN {
 public:
  using TDSvcOfCN::TDSvcOfCN;

 private:
  void doInitScheduleTaskBundle() final;

 private:
  int beforeInit() final;
};

}  // namespace bq::td::svc::ctp
