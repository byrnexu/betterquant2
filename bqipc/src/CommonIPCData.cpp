/*!
 * \file CommonIPCData.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/25
 *
 * \brief
 */

#include "CommonIPCData.hpp"

namespace bq {

std::string CommonIPCData::toJson() const {
  std::string ret;
  ret = R"({"shmHeader":)" + shmHeader_.toJson();
  if (dataLen_ != 0) {
    //! 如果使用toJson那么data_必须是json格式，不然组成的json不对，会有问题
    ret.append(R"(,"data":)").append(data_);
  }
  ret.append("}");
  return ret;
}

CommonIPCDataSPtr MakeCommonIPCData(const std::string& str) {
  void* buff = malloc(sizeof(CommonIPCData) + str.size() + 1);
  std::shared_ptr<CommonIPCData> ret(static_cast<CommonIPCData*>(buff));
  //! 需要memset不然toJson中的topic有可能不是utf8，传递到python会报错
  memset(&ret->shmHeader_, 0, sizeof(ret->shmHeader_));
  ret->dataLen_ = str.size();
  memcpy(ret->data_, str.c_str(), str.size());
  *(const_cast<char*>(ret->data_) + ret->dataLen_) = '\0';
  return ret;
}

CommonIPCData* MakeCommonIPCData(const std::string& str, MsgId msgId) {
  void* buff = malloc(sizeof(CommonIPCData) + str.size() + 1);
  auto ret = static_cast<CommonIPCData*>(buff);
  //! 需要memset不然toJson中的topic有可能不是utf8，传递到python会报错
  memset(&ret->shmHeader_, 0, sizeof(ret->shmHeader_));
  ret->shmHeader_.msgId_ = msgId;
  ret->dataLen_ = str.size();
  memcpy(ret->data_, str.c_str(), str.size());
  *(const_cast<char*>(ret->data_) + ret->dataLen_) = '\0';
  return ret;
}

}  // namespace bq
