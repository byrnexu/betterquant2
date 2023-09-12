/*!
 * \file Decimal.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2019/09/08
 *
 * \brief
 */

#pragma once

#include "util/Pch.hpp"

namespace bq {

class DEC {
  inline static const double EPSINON = 1e-6;

 public:
  inline static bool ZERO(double a) { return (fabs(a - 0.0) < EPSINON); }
  inline static bool EQ(double a, double b) { return (fabs(a - b) < EPSINON); }
  inline static bool GT(double a, double b) { return a - b > EPSINON; }
  inline static bool LT(double a, double b) { return b - a > EPSINON; }
  inline static bool GE(double a, double b) { return GT(a, b) || EQ(a, b); }
  inline static bool LE(double a, double b) { return LT(a, b) || EQ(a, b); }
};

}  // namespace bq
