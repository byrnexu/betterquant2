/*!
 * \file bq_v1_QueryHisMD.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/25
 *
 * \brief
 */

#pragma once

#include <drogon/HttpController.h>

#include "def/BQDef.hpp"

using namespace drogon;

namespace bq {
namespace v1 {

class QueryHisMD : public drogon::HttpController<QueryHisMD> {
 public:
  METHOD_LIST_BEGIN
  //! http://192.168.19.115/v1/QueryHisMD/between/Binance/Spot/BTC-USDT/Trades?tsBegin=1668989747663000&tsEnd=1668989747697000
  //! http://192.168.19.115/v1/QueryHisMD/between/Binance/Spot/BTC-USDT/Books?tsBegin=1669032414507000&tsEnd=1669032415008000
  //! http://192.168.19.115/v1/QueryHisMD/between/Binance/Spot/BTC-USDT/Candle?tsBegin=1669032414507000&tsEnd=1669032415008000&freq=1m
  ADD_METHOD_TO(QueryHisMD::queryBetween2Ts,
                "/v1/QueryHisMD/between/{marketCode}/{symbolType}/{symbolCode}/"
                "{mdType}?tsBegin={tsBegin}&tsEnd={tsEnd}&freq={freq}",
                Get, "bq::LoginFilter");

  ADD_METHOD_TO(QueryHisMD::queryBeforeTs,
                "/v1/QueryHisMD/before/{marketCode}/{symbolType}/{symbolCode}/"
                "{mdType}?ts={ts}&num={num}&freq={freq}",
                Get, "bq::LoginFilter");

  ADD_METHOD_TO(QueryHisMD::queryAfterTs,
                "/v1/QueryHisMD/after/{marketCode}/{symbolType}/{symbolCode}/"
                "{mdType}?ts={ts}&num={num}&freq={freq}",
                Get, "bq::LoginFilter");

  METHOD_LIST_END

  void queryBetween2Ts(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback,
                       std::string &&marketCode, std::string &&symbolType,
                       std::string &&symbolCode, std::string &&mdType,
                       std::uint64_t tsBegin, std::uint64_t tsEnd,
                       std::string &&freq);

  void queryBeforeTs(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback,
                     std::string &&marketCode, std::string &&symbolType,
                     std::string &&symbolCode, std::string &&mdType,
                     std::uint64_t ts, int num, std::string &&freq) const;

  void queryAfterTs(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    std::string &&marketCode, std::string &&symbolType,
                    std::string &&symbolCode, std::string &&mdType,
                    std::uint64_t ts, int num, std::string &&freq) const;

 private:
  std::string makeBody(int statusCode, const std::string &statusMsg,
                       std::string recSet) const;

  HttpResponsePtr makeHttpResponse(const std::string &body) const;
};

}  // namespace v1
}  // namespace bq
