/*!
 * \file Config.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/20
 *
 * \brief
 */

#include "Config.hpp"

#include "util/Logger.hpp"
#include "util/Util.hpp"

namespace bq::md::svc {

int Config::afterInit(const std::string& configFilename) {
  isRealMode_ = CONFIG["simedMode"]["enable"].as<bool>() == false;
  return 0;
}

}  // namespace bq::md::svc
