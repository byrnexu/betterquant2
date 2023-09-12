/*!
 * \file Topic.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/15
 *
 * \brief
 */

#pragma once

#include "util/PchBase.hpp"

namespace bq {

const static std::stirng TOPIC_RISK_PLUG_IN_TRIGGER_RISK_CTRL =
    "shm://RISK/PlugInChannel/Trade/TriggerRiskCrtl";

const static std::string TOPIC_MD_BINANCE_SPOT_SYMBOL_ADD =
    "shm://MD/Binance/Spot/SymbolAdd";
const static std::string TOPIC_MD_BINANCE_SPOT_SYMBOL_DEL =
    "shm://MD/Binance/Spot/SymbolDel";
const static std::string TOPIC_MD_BINANCE_SPOT_SYMBOL_CHG =
    "shm://MD/Binance/Spot/SymbolChg";

const static std::string TOPIC_MD_BINANCE_FUTURES_SYMBOL_ADD =
    "shm://MD/Binance/Futures/SymbolAdd";
const static std::string TOPIC_MD_BINANCE_FUTURES_SYMBOL_DEL =
    "shm://MD/Binance/Futures/SymbolDel";
const static std::string TOPIC_MD_BINANCE_FUTURES_SYMBOL_CHG =
    "shm://MD/Binance/Futures/SymbolChg";

const static std::string TOPIC_MD_BINANCE_CFUTURES_SYMBOL_ADD =
    "shm://MD/Binance/CFutures/SymbolAdd";
const static std::string TOPIC_MD_BINANCE_CFUTURES_SYMBOL_DEL =
    "shm://MD/Binance/CFutures/SymbolDel";
const static std::string TOPIC_MD_BINANCE_CFUTURES_SYMBOL_CHG =
    "shm://MD/Binance/CFutures/SymbolChg";

const static std::string TOPIC_MD_BINANCE_FUTURES_SYMBOL_ADD =
    "shm://MD/Binance/Perp/SymbolAdd";
const static std::string TOPIC_MD_BINANCE_FUTURES_SYMBOL_DEL =
    "shm://MD/Binance/Perp/SymbolDel";
const static std::string TOPIC_MD_BINANCE_FUTURES_SYMBOL_CHG =
    "shm://MD/Binance/Perp/SymbolChg";

const static std::string TOPIC_MD_BINANCE_CFUTURES_SYMBOL_ADD =
    "shm://MD/Binance/CPerp/SymbolAdd";
const static std::string TOPIC_MD_BINANCE_CFUTURES_SYMBOL_DEL =
    "shm://MD/Binance/CPerp/SymbolDel";
const static std::string TOPIC_MD_BINANCE_CFUTURES_SYMBOL_CHG =
    "shm://MD/Binance/CPerp/SymbolChg";

}  // namespace bq
