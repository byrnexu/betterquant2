/*!
 * \file TDEngConnpool.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/24
 *
 * \brief
 */

#pragma once

#include "tdeng/TDEngParam.hpp"
#include "util/Pch.hpp"
#include "util/StdExt.hpp"

#ifdef __cplusplus
extern "C" {
#endif
using TAOS = void;
#ifdef __cplusplus
}
#endif

namespace bq::tdeng {

struct Conn {
  Conn(const Conn&) = delete;
  Conn& operator=(const Conn&) = delete;
  Conn(const Conn&&) = delete;
  Conn& operator=(const Conn&&) = delete;

  Conn() = default;
  Conn(int no, bool idle, TAOS* taos) : no_(no), idle_(idle), taos_(taos) {}

  int no_{0};
  bool idle_{true};
  TAOS* taos_{nullptr};
};
using ConnSPtr = std::shared_ptr<Conn>;

class TDEngConnpool {
 public:
  TDEngConnpool(const TDEngConnpool& p) = delete;
  TDEngConnpool& operator=(const TDEngConnpool& p) = delete;
  TDEngConnpool(const TDEngConnpool&&) = delete;
  TDEngConnpool& operator=(const TDEngConnpool&&) = delete;

  TDEngConnpool(const TDEngParamSPtr& tdEngParam);

 public:
  int init();
  void uninit();

 private:
  sql::ConnectOptionsMap makeConnProperties(
      const TDEngParamSPtr& tdEngParam) const;

 public:
  std::uint32_t getSize() const;
  ConnSPtr getIdleConn() const;
  void giveBackConn(const ConnSPtr& conn);

 private:
  std::vector<ConnSPtr> connGroup_;
  mutable std::ext::spin_mutex mtxConnGroup_;

  const TDEngParamSPtr tdEngParam_{nullptr};
};

}  // namespace bq::tdeng
