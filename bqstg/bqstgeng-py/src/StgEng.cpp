/*!
 * \file PosSnapshotImpl.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/09
 *
 * \brief
 */

#include "StgEng.hpp"

#include <boost/python/suite/indexing/map_indexing_suite.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/utility.hpp>
#include <chrono>
#include <cstring>
#include <memory>
#include <thread>

#include "CommonIPCData.hpp"
#include "PosMgrOfStgInst.hpp"
#include "SHMIPCUtil.hpp"
#include "StgEngImpl.hpp"
#include "StgEngPYUtil.hpp"
#include "StgInstTaskHandlerImpl.hpp"
#include "def/AssetInfo.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/DataStruOfMD.hpp"
#include "def/DataStruOfOthers.hpp"
#include "def/DataStruOfStg.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/Def.hpp"
#include "def/OrderInfo.hpp"
#include "def/PosInfo.hpp"
#include "def/StgInstInfo.hpp"
#include "def/SymbolCode.hpp"
#include "util/BQUtil.hpp"
#include "util/Logger.hpp"
#include "util/PosSnapshot.hpp"
#include "util/String.hpp"

namespace bq::stg {

StgEng::StgEng(const std::string& configFilename)
    : stgEngImpl_(std::make_shared<StgEngImpl>(configFilename)) {}

int StgEng::init(PyObject* stgInstTaskHandler) {
  StgInstTaskHandlerBundle stgInstTaskHandlerBundle;
  const auto stgInstTaskHandlerImpl = std::make_shared<StgInstTaskHandlerImpl>(
      stgEngImpl_.get(), stgInstTaskHandlerBundle);
  stgEngImpl_->installStgInstTaskHandler(stgInstTaskHandlerImpl);
  const auto ret = stgEngImpl_->init();
  if (ret != 0) {
    return ret;
  }
  installStgInstTaskHandler(stgInstTaskHandler);
  return 0;
}

StgInstInfoSPtr StgEng::getDftStgInstInfo() const {
  return stgEngImpl_->getDftStgInstInfo();
}

void StgEng::installStgInstTaskHandler(PyObject* value) {
  stgInstTaskHandler_ = value;

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onStgManualIntervention_ = [this](const StgInstInfoSPtr& stgInstInfo,
                                         const CommonIPCData* commonIPCData) {
    std::string pyerr;
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(
            stgInstTaskHandler_, "on_stg_manual_intervention", stgInstInfo,
            std::string(commonIPCData->data_));
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const std::string msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onPushTopic_ = [this](const StgInstInfoSPtr& stgInstInfo,
                             const CommonIPCData* commonIPCData) {
    std::string pyerr;
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_, "on_push_topic",
                                         stgInstInfo, commonIPCData->toJson());
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onOrderRet_ = [this](const StgInstInfoSPtr& stgInstInfo,
                            const OrderInfo* orderInfo) {
    std::string pyerr;
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_, "on_order_ret",
                                         stgInstInfo, orderInfo);
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onCancelOrderRet_ = [this](const StgInstInfoSPtr& stgInstInfo,
                                  const OrderInfo* orderInfo) {
    std::string pyerr;
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(
            stgInstTaskHandler_, "on_cancel_order_ret", stgInstInfo, orderInfo);
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onAlgoOrder_ = [this](const StgInstInfoSPtr& stgInstInfo,
                             const CommonIPCData* commonIPCData) {
    std::string pyerr;
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_, "on_algo_order",
                                         stgInstInfo, commonIPCData->toJson());
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onTrades_ = [this](const StgInstInfoSPtr& stgInstInfo,
                          const Trades* trades) {
    std::string pyerr;
    const auto marketData = trades->toJson();
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_, "on_trades",
                                         stgInstInfo, marketData);
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onOrders_ = [this](const StgInstInfoSPtr& stgInstInfo,
                          const Orders* orders) {
    std::string pyerr;
    const auto marketData = orders->toJson();
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_, "on_orders",
                                         stgInstInfo, marketData);
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()->getStgInstTaskHandlerBundle().onBooks_ =
      [this](const StgInstInfoSPtr& stgInstInfo, const Books* books) {
        //!
        //! 获取此 stgInstId 真正的档数
        //!
        std::uint32_t realDepthLevel{0};
        {
          std::lock_guard<std::mutex> guard(mtxStgInstId2RealDepthLevel_);
          realDepthLevel = stgInstId2RealDepthLevel_[stgInstInfo->stgInstId_];
        }

        std::string pyerr;
        const auto marketData = books->toJson(realDepthLevel);
        {
          std::lock_guard<std::mutex> guard(mtxPY_);
          try {
            boost::python::call_method<void>(stgInstTaskHandler_, "on_books",
                                             stgInstInfo, marketData);
          } catch (const boost::python::error_already_set& e) {
            if (PyErr_Occurred()) {
              const auto msg = handlePYErr();
              pyerr = fmt::format("Python interpreter error: \n {}", msg);
            }
            boost::python::handle_exception();
            PyErr_Clear();
          }
        }
        logPYErr(stgInstInfo, pyerr);
      };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onCandle_ = [this](const StgInstInfoSPtr& stgInstInfo,
                          const Candle* candle) {
    std::string pyerr;
    const auto marketData = candle->toJson();
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_, "on_candle",
                                         stgInstInfo, marketData);
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onTickers_ = [this](const StgInstInfoSPtr& stgInstInfo,
                           const Tickers* tickers) {
    std::string pyerr;
    const auto marketData = tickers->toJson();
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_, "on_tickers",
                                         stgInstInfo, marketData);
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onBid1Ask1_ = [this](const StgInstInfoSPtr& stgInstInfo,
                            const Bid1Ask1* bid1Ask1) {
    std::string pyerr;
    const auto marketData = bid1Ask1->toJson();
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_, "on_bid1_ask1",
                                         stgInstInfo, marketData);
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onLastPrice_ = [this](const StgInstInfoSPtr& stgInstInfo,
                             const LastPrice* lastPrice) {
    std::string pyerr;
    const auto marketData = lastPrice->toJson();
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_, "on_last_price",
                                         stgInstInfo, marketData);
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onDynCandle_ = [this](const StgInstInfoSPtr& stgInstInfo,
                             const Candle* candle) {
    std::string pyerr;
    const auto marketData = candle->toJson();
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_, "on_dyn_candle",
                                         stgInstInfo, marketData);
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onStgStart_ = [this]() {
    std::string pyerr;
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_, "on_stg_start");
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgEngImpl_->getDftStgInstInfo(), pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onStgInstStart_ = [this](const auto& stgInstInfo) {
    std::string pyerr;
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_,
                                         "on_stg_inst_start", stgInstInfo);
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onStgStop_ = [this]() {
    std::string pyerr;
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_, "on_stg_stop");
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgEngImpl_->getDftStgInstInfo(), pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onStgInstStop_ = [this](const auto& stgInstInfo) {
    std::string pyerr;
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_,
                                         "on_stg_inst_stop", stgInstInfo);
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onStgInstAdd_ = [this](const StgInstInfoSPtr& stgInstInfo) {
    std::string pyerr;
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_, "on_stg_inst_add",
                                         stgInstInfo);
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onStgInstDel_ = [this](const StgInstInfoSPtr& stgInstInfo) {
    std::string pyerr;
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_, "on_stg_inst_del",
                                         stgInstInfo);
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onStgInstChg_ = [this](const StgInstInfoSPtr& stgInstInfo) {
    std::string pyerr;
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_, "on_stg_inst_chg",
                                         stgInstInfo);
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onStgInstTimer_ = [this](const StgInstInfoSPtr& stgInstInfo,
                                const std::string& timerName) {
    std::string pyerr;
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(
            stgInstTaskHandler_, "on_stg_inst_timer", stgInstInfo, timerName);
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onPosUpdateOfAcctId_ = [this](const StgInstInfoSPtr& stgInstInfo,
                                     const PosSnapshotSPtr& posSnapshot) {
    std::string pyerr;
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_,
                                         "on_pos_update_of_acct_id",
                                         stgInstInfo, posSnapshot);
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onPosSnapshotOfAcctId_ = [this](const StgInstInfoSPtr& stgInstInfo,
                                       const PosSnapshotSPtr& posSnapshot) {
    std::string pyerr;
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_,
                                         "on_pos_snapshot_of_acct_id",
                                         stgInstInfo, posSnapshot);
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onPosUpdateOfStgId_ = [this](const StgInstInfoSPtr& stgInstInfo,
                                    const PosSnapshotSPtr& posSnapshot) {
    std::string pyerr;
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_,
                                         "on_pos_update_of_stg_id", stgInstInfo,
                                         posSnapshot);
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onPosSnapshotOfStgId_ = [this](const StgInstInfoSPtr& stgInstInfo,
                                      const PosSnapshotSPtr& posSnapshot) {
    std::string pyerr;
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_,
                                         "on_pos_snapshot_of_stg_id",
                                         stgInstInfo, posSnapshot);
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onPosUpdateOfStgInstId_ = [this](const StgInstInfoSPtr& stgInstInfo,
                                        const PosSnapshotSPtr& posSnapshot) {
    std::string pyerr;
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_,
                                         "on_pos_update_of_stg_inst_id",
                                         stgInstInfo, posSnapshot);
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onPosSnapshotOfStgInstId_ = [this](const StgInstInfoSPtr& stgInstInfo,
                                          const PosSnapshotSPtr& posSnapshot) {
    std::string pyerr;
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_,
                                         "on_pos_snapshot_of_stg_inst_id",
                                         stgInstInfo, posSnapshot);
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onAssetsUpdate_ = [this](const StgInstInfoSPtr& stgInstInfo,
                                const AssetsUpdateSPtr& assetsUpdate) {
    std::string pyerr;
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(
            stgInstTaskHandler_, "on_assets_update", stgInstInfo, assetsUpdate);
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };

  stgEngImpl_->getStgInstTaskHandler()
      ->getStgInstTaskHandlerBundle()
      .onAssetsSnapshot_ = [this](const StgInstInfoSPtr& stgInstInfo,
                                  const AssetsSnapshotSPtr& assetsSnapshot) {
    std::string pyerr;
    {
      std::lock_guard<std::mutex> guard(mtxPY_);
      try {
        boost::python::call_method<void>(stgInstTaskHandler_,
                                         "on_assets_snapshot", stgInstInfo,
                                         assetsSnapshot);
      } catch (const boost::python::error_already_set& e) {
        if (PyErr_Occurred()) {
          const auto msg = handlePYErr();
          pyerr = fmt::format("Python interpreter error: \n {}", msg);
        }
        boost::python::handle_exception();
        PyErr_Clear();
      }
    }
    logPYErr(stgInstInfo, pyerr);
  };
}

void StgEng::logPYErr(const StgInstInfoSPtr& stgInstInfo,
                      const std::string& pyerr) {
  if (!pyerr.empty()) {
    stgEngImpl_->logError(pyerr, stgInstInfo);
  }
}

int StgEng::run() { return stgEngImpl_->run(); }

void StgEng::installStgInstTimer(StgInstId stgInstId,
                                 const std::string& timerName,
                                 const std::string& execTime,
                                 std::uint32_t timeZone) {
  stgEngImpl_->installStgInstTimer(stgInstId, timerName, execTime, timeZone);
}

void StgEng::installStgInstTimer(StgInstId stgInstId,
                                 const std::string& timerName,
                                 ExecAtStartup execAtStartUp,
                                 std::uint32_t milliSecInterval,
                                 std::uint64_t maxExecTimes) {
  stgEngImpl_->installStgInstTimer(stgInstId, timerName, execAtStartUp,
                                   milliSecInterval, maxExecTimes);
}

void StgEng::uninstallStgInstTimer(StgInstId stgInstId,
                                   const std::string& timerName) {
  stgEngImpl_->uninstallStgInstTimer(stgInstId, timerName);
}

std::vector<OrderInfoSPtr> StgEng::getUnclosedOrderInfoGroup(
    const StgInstInfoSPtr& stgInstInfo) const {
  return stgEngImpl_->getUnclosedOrderInfoGroup(stgInstInfo);
}

PosOfStgInstGroupSPtr StgEng::getPosOfStgInst(
    const StgInstInfoSPtr& stgInstInfo) const {
  return stgEngImpl_->getPosOfStgInst(stgInstInfo);
}

std::tuple<int, OrderId> StgEng::order(const StgInstInfoSPtr& stgInstInfo,
                                       MarketCode marketCode,
                                       const std::string& symbolCode, Side side,
                                       PosDirection posDirection,
                                       Decimal orderPrice, Decimal orderSize,
                                       TrdAcctId trdAcctId,
                                       CloseTDayStg closeTDayStg, AlgoId algoId,
                                       const SimedTDInfoSPtr& simedTDInfo) {
  return stgEngImpl_->order(stgInstInfo, marketCode, symbolCode, side,
                            posDirection, orderPrice, orderSize, trdAcctId,
                            closeTDayStg, algoId, simedTDInfo);
}

std::tuple<int, OrderId> StgEng::order(const StgInstInfoSPtr& stgInstInfo,
                                       MarketCode marketCode,
                                       const std::string& symbolCode, Side side,
                                       Decimal orderPrice, Decimal orderSize,
                                       TrdAcctId trdAcctId,
                                       CloseTDayStg closeTDayStg, AlgoId algoId,
                                       const SimedTDInfoSPtr& simedTDInfo) {
  return stgEngImpl_->order(stgInstInfo, marketCode, symbolCode, side,
                            orderPrice, orderSize, trdAcctId, closeTDayStg,
                            algoId, simedTDInfo);
}

std::tuple<int, OrderId> StgEng::order(OrderInfoSPtr& orderInfo) {
  return stgEngImpl_->order(orderInfo);
}

int StgEng::cancelOrder(OrderId orderId) {
  return stgEngImpl_->cancelOrder(orderId);
}

std::vector<OrderId> StgEng::cancelAllOrderOfStg() {
  return stgEngImpl_->cancelAllOrderOfStg();
}

std::vector<OrderId> StgEng::cancelAllOrderOfStgInst(
    const StgInstInfoSPtr& stgInstInfo) {
  return stgEngImpl_->cancelAllOrderOfStgInst(stgInstInfo);
}

std::vector<OrderId> StgEng::cancelAllOrderOfStgInst(StgInstId stgInstId) {
  return stgEngImpl_->cancelAllOrderOfStgInst(stgInstId);
}

std::vector<OrderId> StgEng::cancelAllOrderOfAlgo(AlgoId algoId) {
  return stgEngImpl_->cancelAllOrderOfAlgo(algoId);
}

std::tuple<int, AlgoId> StgEng::algoOrder(
    const StgInstInfoSPtr& stgInstInfo, const std::string& algoType,
    const std::string& algoName, std::uint32_t lifetime,
    const std::string& algoParamsInJsonFmt) {
  return stgEngImpl_->algoOrder(stgInstInfo, algoType, algoName, lifetime,
                                algoParamsInJsonFmt);
}

int StgEng::cancelAlgoOrder(AlgoId algoId) {
  return stgEngImpl_->cancelAlgoOrder(algoId);
}

std::string StgEng::getProgressOfAlgoOrder(AlgoId algoId) {
  return stgEngImpl_->getProgressOfAlgoOrder(algoId);
}

std::tuple<int, OrderInfoSPtr> StgEng::getOrderInfo(OrderId orderId) const {
  return stgEngImpl_->getOrderInfo(orderId);
}

int StgEng::sub(StgInstId subscriber, const std::string& topic) {
  //!
  //! 如果订阅的是books，那么转换为最高档，同时保存实际档位
  //!
  //! MD@Binance@Perp@ADA-USDT-Perp@Books@10 ->
  //! MD@Binance@Perp@ADA-USDT-Perp@Books@400
  //!
  const auto internalTopic = convertTopic(topic);
  std::vector<std::string> fieldGroup;
  boost::algorithm::split(fieldGroup, internalTopic,
                          boost::is_any_of(SEP_OF_TOPIC));
  if (fieldGroup.size() == 6 && fieldGroup[4] == ENUM_TO_STR(MDType::Books) &&
      fieldGroup[0] == TOPIC_PREFIX_OF_MARKET_DATA) {
    {
      std::lock_guard<std::mutex> guard(mtxStgInstId2RealDepthLevel_);
      stgInstId2RealDepthLevel_[subscriber] =
          CONV(std::uint32_t, fieldGroup[5]);
    }
    fieldGroup.pop_back();
    const auto topicOfMaxDepthLevel =
        fmt::format("{}{}{}", boost::join(fieldGroup, SEP_OF_TOPIC),
                    SEP_OF_TOPIC, MAX_DEPTH_LEVEL);
    return stgEngImpl_->sub(subscriber, topicOfMaxDepthLevel);
  } else {
    return stgEngImpl_->sub(subscriber, internalTopic);
  }
}

int StgEng::unSub(StgInstId subscriber, const std::string& topic) {
  return stgEngImpl_->unSub(subscriber, topic);
}

std::tuple<int, std::string> StgEng::queryHisMDBetween2Ts(
    const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
    SymbolType symbolType, const std::string& symbolCode, MDType mdType,
    std::uint64_t tsBegin, std::uint64_t tsEnd, const std::string& ext) {
  return stgEngImpl_->queryHisMDBetween2Ts(stgInstInfo, marketCode, symbolType,
                                           symbolCode, mdType, tsBegin, tsEnd,
                                           ext);
}

std::tuple<int, std::string> StgEng::queryHisMDBetween2Ts(
    const StgInstInfoSPtr& stgInstInfo, const std::string& topic,
    std::uint64_t tsBegin, std::uint64_t tsEnd) {
  return stgEngImpl_->queryHisMDBetween2Ts(stgInstInfo, topic, tsBegin, tsEnd);
}

std::tuple<int, std::string> StgEng::querySpecificNumOfHisMDBeforeTs(
    const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
    SymbolType symbolType, const std::string& symbolCode, MDType mdType,
    std::uint64_t ts, int num, const std::string& ext) {
  return stgEngImpl_->querySpecificNumOfHisMDBeforeTs(
      stgInstInfo, marketCode, symbolType, symbolCode, mdType, ts, num, ext);
}

std::tuple<int, std::string> StgEng::querySpecificNumOfHisMDBeforeTs(
    const StgInstInfoSPtr& stgInstInfo, const std::string& topic,
    std::uint64_t ts, int num) {
  return stgEngImpl_->querySpecificNumOfHisMDBeforeTs(stgInstInfo, topic, ts,
                                                      num);
}

std::tuple<int, std::string> StgEng::querySpecificNumOfHisMDAfterTs(
    const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
    SymbolType symbolType, const std::string& symbolCode, MDType mdType,
    std::uint64_t ts, int num, const std::string& ext) {
  return stgEngImpl_->querySpecificNumOfHisMDAfterTs(
      stgInstInfo, marketCode, symbolType, symbolCode, mdType, ts, num, ext);
}

std::tuple<int, std::string> StgEng::querySpecificNumOfHisMDAfterTs(
    const StgInstInfoSPtr& stgInstInfo, const std::string& topic,
    std::uint64_t ts, int num) {
  return stgEngImpl_->querySpecificNumOfHisMDAfterTs(stgInstInfo, topic, ts,
                                                     num);
}

bool StgEng::saveStgPrivateData(StgInstId stgInstId,
                                const std::string& jsonStr) {
  return stgEngImpl_->saveStgPrivateData(stgInstId, jsonStr);
}

std::string StgEng::loadStgPrivateData(StgInstId stgInstId) {
  return stgEngImpl_->loadStgPrivateData(stgInstId);
}

std::tuple<int, std::string> StgEng::syncExecSql(const std::string& sql) {
  return stgEngImpl_->syncExecSql(sql);
}

void StgEng::saveToDB(const PnlSPtr& pnl) {
  if (!pnl) return;
  stgEngImpl_->saveToDB(pnl);
}

std::vector<std::string> StgEng::getArgGroup(const boost::python::list& args) {
  std::vector<std::string> ret;
  for (int i = 0; i < boost::python::len(args); ++i) {
    std::string item = boost::python::extract<std::string>(args[i]);
    ret.push_back(item);
  }
  return ret;
}

void StgEng::logTrace(const std::string& fmt, const boost::python::list& args,
                      const StgInstInfoSPtr& stgInstInfo,
                      NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logTrace(fmt, getArgGroup(args), stgInstInfo, notifyToTerminal);
}

void StgEng::logDebug(const std::string& fmt, const boost::python::list& args,
                      const StgInstInfoSPtr& stgInstInfo,
                      NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logDebug(fmt, getArgGroup(args), stgInstInfo, notifyToTerminal);
}

void StgEng::logInfo(const std::string& fmt, const boost::python::list& args,
                     const StgInstInfoSPtr& stgInstInfo,
                     NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logInfo(fmt, getArgGroup(args), stgInstInfo, notifyToTerminal);
}

void StgEng::logWarn(const std::string& fmt, const boost::python::list& args,
                     const StgInstInfoSPtr& stgInstInfo,
                     NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logWarn(fmt, getArgGroup(args), stgInstInfo, notifyToTerminal);
}

void StgEng::logError(const std::string& fmt, const boost::python::list& args,
                      const StgInstInfoSPtr& stgInstInfo,
                      NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logError(fmt, getArgGroup(args), stgInstInfo, notifyToTerminal);
}

void StgEng::logCritical(const std::string& fmt,
                         const boost::python::list& args,
                         const StgInstInfoSPtr& stgInstInfo,
                         NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logCritical(fmt, getArgGroup(args), stgInstInfo,
                           notifyToTerminal);
}

void StgEng::logTrace(const std::string& fmt,
                      const StgInstInfoSPtr& stgInstInfo,
                      NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logTrace(fmt, stgInstInfo, notifyToTerminal);
}

void StgEng::logDebug(const std::string& fmt,
                      const StgInstInfoSPtr& stgInstInfo,
                      NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logDebug(fmt, stgInstInfo, notifyToTerminal);
}

void StgEng::logInfo(const std::string& fmt, const StgInstInfoSPtr& stgInstInfo,
                     NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logInfo(fmt, stgInstInfo, notifyToTerminal);
}

void StgEng::logWarn(const std::string& fmt, const StgInstInfoSPtr& stgInstInfo,
                     NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logWarn(fmt, stgInstInfo, notifyToTerminal);
}

void StgEng::logError(const std::string& fmt,
                      const StgInstInfoSPtr& stgInstInfo,
                      NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logError(fmt, stgInstInfo, notifyToTerminal);
}

void StgEng::logCritical(const std::string& fmt,
                         const StgInstInfoSPtr& stgInstInfo,
                         NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logCritical(fmt, stgInstInfo, notifyToTerminal);
}

}  // namespace bq::stg
