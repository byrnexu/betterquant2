/*!
 * \file TDSvcOfXTP.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/01
 *
 * \brief
 */

#pragma once

#include "TDSvcOfCN.hpp"

namespace bq::td::svc::xtp {

class TDSvcOfXTP : public TDSvcOfCN {
 public:
  using TDSvcOfCN::TDSvcOfCN;

 private:
  void doInitScheduleTaskBundle() final;

 private:
  int beforeInit() final;
};

}  // namespace bq::td::svc::xtp
