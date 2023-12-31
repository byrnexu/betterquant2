/*!
 * \file ConstIF.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "util/PchBase.hpp"

namespace bq {

enum class WriteLog { True = 1, False = 2 };
enum class OnlyModifyIsDel { True = 1, False = 2 };
enum class ExceedFlowCtrlRule { TryToResend = 1, NotSendAgain = 2 };
enum class SyncToDB { True = 1, False = 2 };
enum class DeepClone { True = 1, False = 2 };
enum class ExecAtStartup { True = 1, False = 2 };
enum class BlockType { Block, NoBlock };

}  // namespace bq
