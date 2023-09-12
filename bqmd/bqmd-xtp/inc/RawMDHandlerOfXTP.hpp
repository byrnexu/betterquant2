/*!
 * \file RawMDHandlerOfXTP.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/16
 *
 * \brief
 */

#pragma once

#include "RawMDHandler.hpp"
#include "def/BQMDDef.hpp"
#include "util/Pch.hpp"

namespace bq::md::svc {
class MDSvcOfCN;
}

namespace bq::md::svc::xtp {

class RawMDHandlerOfXTP;
using RawMDHandlerOfXTPSPtr = std::shared_ptr<RawMDHandlerOfXTP>;

class RawMDHandlerOfXTP : public RawMDHandler {
 public:
  RawMDHandlerOfXTP(const RawMDHandlerOfXTP&) = delete;
  RawMDHandlerOfXTP& operator=(const RawMDHandlerOfXTP&) = delete;
  RawMDHandlerOfXTP(const RawMDHandlerOfXTP&&) = delete;
  RawMDHandlerOfXTP& operator=(const RawMDHandlerOfXTP&&) = delete;

  using RawMDHandler::RawMDHandler;

 private:
  std::tuple<int, RawMDAsyncTaskSPtr> makeAsyncTask(
      const RawMDSPtr& task) final;

 private:
  void handleNewSymbol(RawMDAsyncTaskSPtr& asyncTask) final;
  bool handleMDTickers(RawMDAsyncTaskSPtr& asyncTask) final;
  bool handleMDTrades(RawMDAsyncTaskSPtr& asyncTask) final;
  bool handleMDOrders(RawMDAsyncTaskSPtr& asyncTask) final;
  bool handleMDBooks(RawMDAsyncTaskSPtr& asyncTask) final;
};

}  // namespace bq::md::svc::xtp
