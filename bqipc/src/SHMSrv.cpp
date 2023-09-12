/*!
 * \file SHMSrv.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "SHMSrv.hpp"

#include "SHMIPCConst.hpp"
#include "SHMIPCMsgId.hpp"
#include "SHMIPCUtil.hpp"
#include "util/Datetime.hpp"
#include "util/Logger.hpp"

namespace bq {

void SHMSrv::beforeInit() {
  {
    for (std::size_t i = 0; i < safePublisherGroup_.size(); ++i) {
      safePublisherGroup_[i] = std::make_shared<SafePublisher>();
    }
  }
}

void SHMSrv::beforeUninit() {
  {
    for (auto iter = std::begin(safePublisherGroup_);
         iter != std::end(safePublisherGroup_); ++iter) {
      if ((*iter)->publisher_ != nullptr) {
        delete (*iter)->publisher_;
        (*iter)->publisher_ = nullptr;
      }
    }
  }
}

void SHMSrv::sendRspWithZeroCopy(const FillSHMBufCallback& fillSHMBufCallback,
                                 const SHMHeader* reqHeader,
                                 std::size_t shmBufLen) {
  auto safePublisher = getSafePublisher(reqHeader->clientChannel_);
  {
    std::lock_guard<std::ext::spin_mutex> guard(
        safePublisher->mtxWriteRawDataToSHM_);
    safePublisher->publisher_->loan(shmBufLen)
        .and_then([&](auto& userPayload) {
          memset(userPayload, 0, shmBufLen);
          beforeSendRsp(reqHeader, userPayload);
          fillSHMBufCallback(userPayload);
          safePublisher->publisher_->publish(userPayload);
        })
        .or_else([&](auto& error) {
          std::ostringstream oss;
          oss << error;
          LOG_E("Unable to loan shm. {} [{}{}{}-{}{}{}] [{}]", appName_,
                service_, SEP_OF_SHM_SVC, instance_, reqHeader->clientChannel_,
                SEP_OF_SHM_SVC, event_, oss.str());
        });
  }
}

void SHMSrv::pushMsgWithZeroCopy(const FillSHMBufCallback& fillSHMBufCallback,
                                 ClientChannel clientChannel, MsgId msgId,
                                 std::size_t shmBufLen) {
  auto safePublisher = getSafePublisher(clientChannel);
  {
    std::lock_guard<std::ext::spin_mutex> guard(
        safePublisher->mtxWriteRawDataToSHM_);
    safePublisher->publisher_->loan(shmBufLen)
        .and_then([&](auto& userPayload) {
          memset(userPayload, 0, shmBufLen);
          beforePushMsg(clientChannel, msgId, userPayload);
          fillSHMBufCallback(userPayload);
          safePublisher->publisher_->publish(userPayload);
        })
        .or_else([&](auto& error) {
          std::ostringstream oss;
          oss << error;
          LOG_E("Unable to loan shm. {} [{}{}{}-{}{}{}] [{}]", appName_,
                service_, SEP_OF_SHM_SVC, instance_, clientChannel,
                SEP_OF_SHM_SVC, event_, oss.str());
        });
  }
}

void SHMSrv::sendRsp(const SHMHeader* reqHeader, void* data, std::size_t len) {
  const auto srcSHMHeader = static_cast<SHMHeader*>(data);
  auto safePublisher = getSafePublisher(reqHeader->clientChannel_);
  {
    std::lock_guard<std::ext::spin_mutex> guard(
        safePublisher->mtxWriteRawDataToSHM_);
    safePublisher->publisher_->loan(len)
        .and_then([&](auto& userPayload) {
          memset(userPayload, 0, len);
          beforeSendRsp(reqHeader, userPayload);
          auto shmHeader = static_cast<SHMHeader*>(userPayload);
          shmHeader->topicHash_ = srcSHMHeader->topicHash_;
          strncpy(shmHeader->topic_, srcSHMHeader->topic_,
                  sizeof(shmHeader->topic_) - 1);
          memcpy(static_cast<char*>(userPayload) + sizeof(SHMHeader),
                 static_cast<char*>(data) + sizeof(SHMHeader),
                 len - sizeof(SHMHeader));
          safePublisher->publisher_->publish(userPayload);
        })
        .or_else([this](auto& error) {
          std::ostringstream oss;
          oss << error;
          LOG_E("Unable to loan shm. {} [{}{}{}{}{}] [{}]", appName_, service_,
                SEP_OF_SHM_SVC, instance_, SEP_OF_SHM_SVC, event_, oss.str());
        });
  }
}

void SHMSrv::beforeSendRsp(const SHMHeader* reqHeader, void* data) {
  auto rspHeader = static_cast<SHMHeader*>(data);
  rspHeader->msgId_ = reqHeader->msgId_;
  rspHeader->clientChannel_ = reqHeader->clientChannel_;
  rspHeader->direction_ = Direction::Rsp;
  rspHeader->timestamp_ = reqHeader->timestamp_;
}

void SHMSrv::pushMsg(ClientChannel clientChannel, MsgId msgId, void* data,
                     std::size_t len) {
  const auto srcSHMHeader = static_cast<SHMHeader*>(data);
  auto safePublisher = getSafePublisher(clientChannel);
  {
    std::lock_guard<std::ext::spin_mutex> guard(
        safePublisher->mtxWriteRawDataToSHM_);
    safePublisher->publisher_->loan(len)
        .and_then([&](auto& userPayload) {
          memset(userPayload, 0, len);
          beforePushMsg(clientChannel, msgId, userPayload);
          auto shmHeader = static_cast<SHMHeader*>(userPayload);
          shmHeader->topicHash_ = srcSHMHeader->topicHash_;
          strncpy(shmHeader->topic_, srcSHMHeader->topic_,
                  sizeof(shmHeader->topic_) - 1);
          memcpy(static_cast<char*>(userPayload) + sizeof(SHMHeader),
                 static_cast<char*>(data) + sizeof(SHMHeader),
                 len - sizeof(SHMHeader));
          safePublisher->publisher_->publish(userPayload);
        })
        .or_else([this](auto& error) {
          std::ostringstream oss;
          oss << error;
          LOG_E("Unable to loan shm. {} [{}{}{}{}{}] [{}]", appName_, service_,
                SEP_OF_SHM_SVC, instance_, SEP_OF_SHM_SVC, event_, oss.str());
        });
  }
}

void SHMSrv::beforePushMsg(ClientChannel clientChannel, MsgId msgId,
                           void* data) {
  auto msgHeader = static_cast<SHMHeader*>(data);
  msgHeader->msgId_ = msgId;
  msgHeader->clientChannel_ = clientChannel;
  msgHeader->direction_ = Direction::Push;
  msgHeader->timestamp_ = GetTotalUSSince1970();
}

SafePublisherSPtr SHMSrv::getSafePublisher(ClientChannel clientChannel) {
  auto waitForSubscriberToBeVisible =
      [&](iox::popo::UntypedPublisher* publisher) {
        //!
        //! 行情推送公共频道第一次启动没有subscriber，所以直接返回
        //!
        //! 模拟行情服务端启动的时候会通过公共频道告知订阅者已经准备就绪，如果
        //! 不确定订阅者是否就绪的情况发送这个通知，订阅者将收不到这个通知，所
        //! 以以下代码注释掉了。
        //!
        //! if (clientChannel == 0) return;
        //!
        int i = 0;
        while (true) {
          if (++i > TIMES_OF_WAIT_FOR_SUBSCRIBER) {
            LOG_I("No subscriber found after {} times of attempts.", i);
            break;
          }
          if (publisher->hasSubscribers()) {
            LOG_D("Try {} times.", i);
            break;
          }
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
      };

  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxPubliserGroup_);
    if (safePublisherGroup_[clientChannel]->publisher_ == nullptr) {
      const auto instance = fmt::format("{}-{}", instance_, clientChannel);
      safePublisherGroup_[clientChannel]->publisher_ =
          new iox::popo::UntypedPublisher(
              {iox::capro::IdString_t(iox::cxx::TruncateToCapacity, service_),
               iox::capro::IdString_t(iox::cxx::TruncateToCapacity, instance),
               iox::capro::IdString_t(iox::cxx::TruncateToCapacity, event_)},
              makePublisherOptions());
      safePublisherGroup_[clientChannel]->publisher_->offer();
      waitForSubscriberToBeVisible(
          safePublisherGroup_[clientChannel]->publisher_);
      LOG_I("Server {} [{}] accept client [{}{}{}-{}{}{}].", appName_,
            subscriberName_, service_, SEP_OF_SHM_SVC, instance_, clientChannel,
            SEP_OF_SHM_SVC, event_);
    }
    return safePublisherGroup_[clientChannel];
  }
}

}  // namespace bq
