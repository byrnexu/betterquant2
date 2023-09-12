/*!
 * \file ConfigBase.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "util/Pch.hpp"

namespace bq {

class ConfigBase {
 public:
  int init(const std::string& configFilename);

 private:
  virtual int beforeInit(const std::string& configFilename);
  virtual int doInit(const std::string& configFilename);
  //! 这里可以对配置数据进行加工得到自己想要的一些便于访问的数据结构
  virtual int afterInit(const std::string& configFilename);

 public:
  YAML::Node& get() { return node_; }

 protected:
  YAML::Node node_;
};

std::tuple<int, YAML::Node> InitConfig(const std::string& configFilename);

}  // namespace bq
