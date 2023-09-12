/*!
 * \file ReqParser.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "util/Pch.hpp"

namespace bq::md::svc {

class MDSvc;

class ReqParser {
 public:
  ReqParser(const ReqParser&) = delete;
  ReqParser& operator=(const ReqParser&) = delete;
  ReqParser(const ReqParser&&) = delete;
  ReqParser& operator=(const ReqParser&&) = delete;

  ReqParser(MDSvc* mdSvc) : mdSvc_(mdSvc) {}

 public:
  std::vector<std::string> getTopicGroupForSubOrUnSubAgain(
      const std::string& req) {
    return doGetTopicGroupForSubOrUnSubAgain(req);
  }

 private:
  //! 因为超流控或者其他原因订阅失败时解析订阅请求中的代码以便重新发起或者取消订阅
  virtual std::vector<std::string> doGetTopicGroupForSubOrUnSubAgain(
      const std::string& req) = 0;

 protected:
  MDSvc* mdSvc_{nullptr};
};

}  // namespace bq::md::svc
