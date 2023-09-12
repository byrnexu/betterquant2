/*!
 * \file SHMIPCUtil.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "SHMIPCUtil.hpp"

#include <cassert>

#include "CommonIPCData.hpp"
#include "SHMIPCMsgId.hpp"
#include "SHMSrv.hpp"
#include "util/Logger.hpp"

namespace bq {

std::once_flag& GetOnceFlagOfAssignAppName() {
  static std::once_flag ret;
  return ret;
}

void PubTopic(const SHMSrvSPtr& shmSrv, const std::string& topicName,
              const std::string& topic, std::string data) {
  assert(shmSrv != nullptr && "shmSrv != nullptr");

  std::string dataWithTopic;
  if (!data.empty()) {
    std::string str;
    str = fmt::format(R"({{"topicName":"{}",)", topicName);
    dataWithTopic.append(str);
    str = fmt::format(R"("topic":"{}")", topic);
    dataWithTopic.append(str);
    data[0] = ',';
    dataWithTopic.append(data);
  } else {
    std::string str;
    str = fmt::format(R"({{"topicName":"{}",)", topicName);
    dataWithTopic.append(str);
    str = fmt::format(R"("topic":{}}})", topic);
    dataWithTopic.append(str);
  }

  LOG_I("PUB topic {}", dataWithTopic);

  const auto topicHash = XXH3_64bits(topic.data(), topic.size());
  shmSrv->pushMsgWithZeroCopy(
      [&](void* shmBuf) {
        auto commonIPCData = static_cast<CommonIPCData*>(shmBuf);
        strncpy(commonIPCData->shmHeader_.topic_, topic.c_str(),
                MAX_TOPIC_LEN - 1);
        commonIPCData->shmHeader_.topicHash_ = topicHash;
        commonIPCData->dataLen_ = dataWithTopic.size();
        strncpy(commonIPCData->data_, dataWithTopic.c_str(),
                dataWithTopic.size() - 1);
      },
      PUB_CHANNEL, MSG_ID_ON_PUSH_TOPIC,
      sizeof(CommonIPCData) + dataWithTopic.size() + 1);
}

}  // namespace bq
