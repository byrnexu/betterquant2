/*!
 * \file PosMgrOfStgInst.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/02/28
 *
 * \brief
 */

#include "StgEngDef.hpp"
#include "def/BQConstIF.hpp"
#include "def/BQDefIF.hpp"
#include "def/ConstIF.hpp"
#include "def/DefIF.hpp"
#include "util/PchBase.hpp"

namespace bq {

struct StgInstInfo;
using StgInstInfoSPtr = std::shared_ptr<StgInstInfo>;

struct PosOfStgInst;
using PosOfStgInstSPtr = std::shared_ptr<PosOfStgInst>;

using PosOfStgInstGroup = std::vector<PosOfStgInstSPtr>;
using PosOfStgInstGroupSPtr = std::shared_ptr<PosOfStgInstGroup>;

}  // namespace bq

namespace bq::stg {

using PosOfStgInstTable = std::map<std::string, PosOfStgInstSPtr>;
using PosOfStgInstTableSPtr = std::shared_ptr<PosOfStgInstTable>;

class PosMgrOfStgInst {
 public:
  PosMgrOfStgInst(const PosMgrOfStgInst&) = delete;
  PosMgrOfStgInst& operator=(const PosMgrOfStgInst&) = delete;
  PosMgrOfStgInst(const PosMgrOfStgInst&&) = delete;
  PosMgrOfStgInst& operator=(const PosMgrOfStgInst&&) = delete;

  PosMgrOfStgInst(const StgInstInfoSPtr& stgInstInfo,
                  const StgPosMgrSPtr& posMgr, const StgOrdMgrSPtr& ordMgr);

  PosOfStgInstGroupSPtr get() const;

 private:
  PosOfStgInstTableSPtr posOfStgInstTable_{nullptr};
};

}  // namespace bq::stg
