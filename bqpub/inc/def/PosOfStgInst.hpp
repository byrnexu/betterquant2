/*!
 * \file PosOfStgInst.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/02/28
 *
 * \brief
 */

#include "def/BQConstIF.hpp"
#include "def/BQDefIF.hpp"
#include "def/ConstIF.hpp"
#include "def/DefIF.hpp"
#include "util/PchBase.hpp"

namespace bq {

struct PosInfo;
using PosInfoSPtr = std::shared_ptr<PosInfo>;

struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;

struct StgInstInfo;
using StgInstInfoSPtr = std::shared_ptr<StgInstInfo>;

struct PosOfStgInst;
using PosOfStgInstSPtr = std::shared_ptr<PosOfStgInst>;

struct PosOfStgInst {
  StgInstInfoSPtr stgInstInfo_{nullptr};

  AcctId acctId_{0};
  TrdAcctId trdAcctId_{0};
  AlgoId algoId_{0};

  MarketCode marketCode_{MarketCode::Others};
  SymbolType symbolType_{SymbolType::Others};
  std::string symbolCode_;

  Side side_{Side::Others};
  PosSide posSide_{PosSide::Others};

  std::uint32_t parValue_{0};
  std::string feeCurrency_;

  Decimal fee_{0};

  Decimal pos_{0};
  Decimal avgOpenPrice_{0};

  Decimal posUnreal_{0};
  Decimal avgOrderPrice_{0};

  std::string toStr() const;
  std::string getKey() const;
};

PosOfStgInstSPtr MakePosOfStgInst(const StgInstInfoSPtr& stgInstInfo,
                                  const PosInfoSPtr& posInfo);

PosOfStgInstSPtr MakePosOfStgInst(const StgInstInfoSPtr& stgInstInfo,
                                  const OrderInfoSPtr& orderInfo);

using PosOfStgInstGroup = std::vector<PosOfStgInstSPtr>;
using PosOfStgInstGroupSPtr = std::shared_ptr<PosOfStgInstGroup>;

}  // namespace bq
