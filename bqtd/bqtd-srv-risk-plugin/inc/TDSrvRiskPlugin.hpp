/*!
 * \file TDSrvRiskPlugin.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "TDSrvRiskPluginDef.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq {
struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;
}  // namespace bq

namespace bq::td::srv {

class TDSrv;

class BOOST_SYMBOL_VISIBLE TDSrvRiskPlugin {
 public:
  TDSrvRiskPlugin(const TDSrvRiskPlugin&) = delete;
  TDSrvRiskPlugin& operator=(const TDSrvRiskPlugin&) = delete;
  TDSrvRiskPlugin(const TDSrvRiskPlugin&&) = delete;
  TDSrvRiskPlugin& operator=(const TDSrvRiskPlugin&&) = delete;

  explicit TDSrvRiskPlugin(TDSrv* tdSrv);

 public:
  int init(const std::string& configFilename);

 public:
  void onThreadStart(std::uint32_t combNo, std::uint32_t threadNo);

 private:
  virtual void doOnThreadStart(std::uint32_t combNo, std::uint32_t threadNo);

 public:
  void onThreadExit(std::uint32_t combNo, std::uint32_t threadNo);

 private:
  virtual void doOnThreadExit(std::uint32_t combNo, std::uint32_t threadNo);

 public:
  int load();

 private:
  virtual int doLoad() { return 0; }

 public:
  void unload();

 private:
  virtual void doUnload() {}

 public:
  std::string name() const { return name_; }
  bool enable() const { return enable_; }
  std::shared_ptr<spdlog::async_logger> logger() { return logger_; }

 public:
  boost::dll::fs::path location() { return getLocation(); }

 private:
  virtual boost::dll::fs::path getLocation() const = 0;

 public:
  TDSrv* getTDSrv() const;

 public:
  int onRiskCtrlConfChg(const std::string& conf, std::uint32_t combNo,
                        std::uint32_t threadNo);

  std::tuple<int, std::string> onOrder(const OrderInfoSPtr& order,
                                       std::uint32_t combNo,
                                       std::uint32_t threadNo);

  std::tuple<int, std::string> onCancelOrder(const OrderInfoSPtr& order,
                                             std::uint32_t combNo,
                                             std::uint32_t threadNo);

 private:
  virtual int doOnRiskCtrlConfChg(const std::string& step, const Doc& doc,
                                  const std::string& conf, std::uint32_t combNo,
                                  std::uint32_t threadNo) {
    return 0;
  }

  virtual std::tuple<int, std::string> doOnOrder(const OrderInfoSPtr& order,
                                                 std::uint32_t combNo,
                                                 std::uint32_t threadNo) {
    return {0, ""};
  }

  virtual std::tuple<int, std::string> doOnCancelOrder(
      const OrderInfoSPtr& order, std::uint32_t combNo,
      std::uint32_t threadNo) {
    return {0, ""};
  }

 public:
  std::tuple<int, std::string> onOrderRet(const OrderInfoSPtr& order,
                                          std::uint32_t combNo,
                                          std::uint32_t threadNo);

  std::tuple<int, std::string> onCancelOrderRet(const OrderInfoSPtr& order,
                                                std::uint32_t combNo,
                                                std::uint32_t threadNo);

 private:
  virtual std::tuple<int, std::string> doOnOrderRet(const OrderInfoSPtr& order,
                                                    std::uint32_t combNo,
                                                    std::uint32_t threadNo) {
    return {0, ""};
  }

  virtual std::tuple<int, std::string> doOnCancelOrderRet(
      const OrderInfoSPtr& order, std::uint32_t combNo,
      std::uint32_t threadNo) {
    return {0, ""};
  }

 public:
  std::string getMD5SumOfConf() const { return md5SumOfConf_; }

 public:
  void saveTriggerInfoToDB(const std::string& name, int statusCode,
                           const std::string& statusMsg,
                           const std::string& details);

 protected:
  TDSrv* tdSrv_;
  YAML::Node node_;

 private:
  std::string name_;
  bool enable_{false};

  std::string md5SumOfConf_;

 protected:
  std::shared_ptr<spdlog::async_logger> logger_;
};

struct LibraryHoldingDeleter {
  std::shared_ptr<boost::dll::shared_library> lib_;
  void operator()(TDSrvRiskPlugin* p) const { delete p; }
};

inline std::shared_ptr<TDSrvRiskPlugin> bind(TDSrvRiskPlugin* plugin) {
  boost::dll::fs::path location = plugin->location();
  std::shared_ptr<boost::dll::shared_library> lib =
      std::make_shared<boost::dll::shared_library>(location);

  LibraryHoldingDeleter deleter;
  deleter.lib_ = lib;

  return std::shared_ptr<TDSrvRiskPlugin>(plugin, deleter);
}

inline std::shared_ptr<TDSrvRiskPlugin> GetPlugin(boost::dll::fs::path path,
                                                  const char* funcName,
                                                  TDSrv* tdSrv) {
  typedef TDSrvRiskPlugin*(Func)(TDSrv*);
  boost::function<Func> creator = boost::dll::import_alias<Func>(
      path, funcName, boost::dll::load_mode::append_decorations);

  TDSrvRiskPlugin* plugin = creator(tdSrv);
  return bind(plugin);
}

}  // namespace bq::td::srv
