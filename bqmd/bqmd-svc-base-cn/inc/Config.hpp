/*!
 * \file Config.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/20
 *
 * \brief
 */

#pragma once

#include "util/ConfigBase.hpp"
#include "util/Pch.hpp"

namespace bq::md::svc {

class Config : public ConfigBase,
               public boost::serialization::singleton<Config> {
 private:
  int afterInit(const std::string& configFilename) final;

 public:
  bool isRealMode() const { return isRealMode_; }
  bool isSimedMode() const { return !isRealMode(); }

 private:
  bool isRealMode_{false};
};

}  // namespace bq::md::svc
