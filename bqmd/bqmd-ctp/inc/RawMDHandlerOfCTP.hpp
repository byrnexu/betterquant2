/*!
 * \file RawMDHandlerOfCTP.hpp
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

namespace bq::db::symbolInfo {
struct FieldGroupOfAll;
using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
}  // namespace bq::db::symbolInfo

namespace bq::md::svc::ctp {

class RawMDHandlerOfCTP;
using RawMDHandlerOfCTPSPtr = std::shared_ptr<RawMDHandlerOfCTP>;

class RawMDHandlerOfCTP : public RawMDHandler {
 public:
  RawMDHandlerOfCTP(const RawMDHandlerOfCTP&) = delete;
  RawMDHandlerOfCTP& operator=(const RawMDHandlerOfCTP&) = delete;
  RawMDHandlerOfCTP(const RawMDHandlerOfCTP&&) = delete;
  RawMDHandlerOfCTP& operator=(const RawMDHandlerOfCTP&&) = delete;

  using RawMDHandler::RawMDHandler;

 private:
  std::tuple<int, RawMDAsyncTaskSPtr> makeAsyncTask(
      const RawMDSPtr& task) final;

 private:
  void handleNewSymbol(RawMDAsyncTaskSPtr& asyncTask) final;
  bq::db::symbolInfo::RecordSPtr makeRecSymbolInfo(
      RawMDAsyncTaskSPtr& asyncTask);

  bool handleMDTickers(RawMDAsyncTaskSPtr& asyncTask) final;
};

}  // namespace bq::md::svc::ctp
