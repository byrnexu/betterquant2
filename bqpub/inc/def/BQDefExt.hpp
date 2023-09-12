/*!
 * \file BQDefExt.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "def/BQConstIF.hpp"
#include "def/DefIF.hpp"
#include "util/PchBase.hpp"

namespace bq::web {
struct TaskFromSrv;
using TaskFromSrvSPtr = std::shared_ptr<TaskFromSrv>;
}  // namespace bq::web

namespace bq {

template <typename Task>
struct AsyncTask;

using WSCliAsyncTask = AsyncTask<bq::web::TaskFromSrvSPtr>;
using WSCliAsyncTaskSPtr = std::shared_ptr<WSCliAsyncTask>;

struct RawMD;
using RawMDSPtr = std::shared_ptr<RawMD>;
using RawMDAsyncTask = AsyncTask<RawMDSPtr>;
using RawMDAsyncTaskSPtr = std::shared_ptr<RawMDAsyncTask>;

std::string MakeTopic(const RawMDSPtr& rawMD);

}  // namespace bq
