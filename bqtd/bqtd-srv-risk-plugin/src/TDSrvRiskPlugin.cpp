/*!
 * \file TDSrvRiskPlugin.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "TDSrvRiskPlugin.hpp"

#include "RiskCtrlModule.hpp"
#include "TDSrv.hpp"
#include "TDSrvRiskPluginUtil.hpp"
#include "db/DBE.hpp"
#include "util/File.hpp"
#include "util/Logger.hpp"
#include "util/String.hpp"

namespace bq::td::srv {

TDSrvRiskPlugin::TDSrvRiskPlugin(TDSrv* tdSrv) : tdSrv_(tdSrv) {}

int TDSrvRiskPlugin::init(const std::string& configFilename) {
  try {
    node_ = YAML::LoadFile(configFilename);
  } catch (const std::exception& e) {
    const auto statusMsg = fmt::format("Init config by file {} failed. [{}]",
                                       configFilename, e.what());
    std::cerr << statusMsg << std::endl;
    return -1;
  }

  logger_ = makeLogger(configFilename);
  if (logger_ == nullptr) {
    const auto statusMsg = fmt::format(
        "Init risk plug in {} failed "
        "because of init logger by file {} failed. ",
        name(), configFilename);
    std::cerr << statusMsg << std::endl;
    return -1;
  }

  name_ = fmt::format("{}-v{}",  //
                      node_["name"].as<std::string>(),
                      node_["version"].as<std::string>());

  enable_ = node_["enable"].as<bool>(false);

  const auto configFileCont = LoadFileContToStr(configFilename);
  md5SumOfConf_ = MD5Sum(configFileCont);

  return 0;
}

void TDSrvRiskPlugin::onThreadStart(std::uint32_t combNo,
                                    std::uint32_t threadNo) {
  L_I(logger(), "Risk plugin {} thread {}-{} start.", name(), combNo, threadNo);
  doOnThreadStart(combNo, threadNo);
}

void TDSrvRiskPlugin::doOnThreadStart(std::uint32_t combNo,
                                      std::uint32_t threadNo) {}

void TDSrvRiskPlugin::onThreadExit(std::uint32_t combNo,
                                   std::uint32_t threadNo) {
  L_I(logger(), "Risk plugin {} thread {}-{} exit.", name(), combNo, threadNo);
  doOnThreadExit(combNo, threadNo);
}

void TDSrvRiskPlugin::doOnThreadExit(std::uint32_t combNo,
                                     std::uint32_t threadNo) {}

int TDSrvRiskPlugin::load() {
  L_I(logger(), "Load risk plugin {}.", name());
  return doLoad();
}

void TDSrvRiskPlugin::unload() {
  L_I(logger(), "Unload risk plugin {}.", name());
  doUnload();
}

TDSrv* TDSrvRiskPlugin::getTDSrv() const { return tdSrv_; }

int TDSrvRiskPlugin::onRiskCtrlConfChg(const std::string& conf,
                                       std::uint32_t combNo,
                                       std::uint32_t threadNo) {
  const auto& stepInConf = getTDSrv()
                               ->getRiskCtrlModuleComb()[combNo]
                               ->getTDSrvTaskDispatcherConf()
                               ->step_;

  Doc doc;
  doc.Parse(conf.data());
  if (!doc.HasMember("data") || !doc["data"].IsObject()) {
    L_W(logger(),
        "[{}] Chg info of conf has no member of data. [threadNo: {}-{}] {}",
        name(), combNo, threadNo, conf);
    return 0;
  }

  if (!doc["data"].HasMember("step") || !doc["data"]["step"].IsString()) {
    L_W(logger(),
        "[{}] Chg info of conf has no member of step. [threadNo: {}-{}] {}",
        name(), combNo, threadNo, conf);
    return 0;
  }

  const std::string stepInDB = doc["data"]["step"].GetString();
  if (stepInConf != stepInDB) {
    return 0;
  }

  if (!doc.HasMember("pluginName") || !doc["pluginName"].IsString()) {
    L_W(logger(),
        "[{}] Chg info of conf has no member of pluginName. "
        "[threadNo: {}-{}] {}",
        name(), combNo, threadNo, conf);
    return 0;
  }

  // pluginName=risk-plugin-close-tday; name=risk-plugin-close-tday-v1.0.0
  const auto pluginName = doc["pluginName"].GetString();
  const auto prefix = fmt::format("{}-v", pluginName);
  if (!boost::starts_with(name(), prefix)) {
    LOG_T("name={}; pluginName={}", name(), pluginName);
    return 0;
  }

  L_I(logger(),
      "[{}] On risk ctrl config changed in step {}. [threadNo: {}-{}] {}",
      name(), stepInConf, combNo, threadNo, conf);

  return doOnRiskCtrlConfChg(stepInConf, doc, conf, combNo, threadNo);
}

std::tuple<int, std::string> TDSrvRiskPlugin::onOrder(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  L_T(logger(), "[{}] On order req. [threadNo: {}-{}]", name(), combNo,
      threadNo);
  return doOnOrder(order, combNo, threadNo);
}

std::tuple<int, std::string> TDSrvRiskPlugin::onCancelOrder(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  L_T(logger(), "[{}] On cancel order req. [threadNo: {}-{}]", name(), combNo,
      threadNo);
  return doOnCancelOrder(order, combNo, threadNo);
}

std::tuple<int, std::string> TDSrvRiskPlugin::onOrderRet(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  L_T(logger(), "[{}] On order ret. [threadNo: {}-{}]", name(), combNo,
      threadNo);
  return doOnOrderRet(order, combNo, threadNo);
}

std::tuple<int, std::string> TDSrvRiskPlugin::onCancelOrderRet(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  L_T(logger(), "[{}] On cancel order ret. [threadNo: {}-{}]", name(), combNo,
      threadNo);
  return doOnCancelOrderRet(order, combNo, threadNo);
}

void TDSrvRiskPlugin::saveTriggerInfoToDB(const std::string& name,
                                          int statusCode,
                                          const std::string& statusMsg,
                                          const std::string& details) {
  const auto identity = GET_RAND_STR();
  const auto sql =
      MakeSqlOfRiskCtrlTriggerInfo(name, statusCode, statusMsg, details);
  const auto [ret, execRet] = getTDSrv()->getDBEng()->asyncExec(identity, sql);
  if (ret != 0) {
    L_W(logger(), "Insert risk ctrl trigger info to db failed. [{}]", sql);
  }
}

}  // namespace bq::td::srv
