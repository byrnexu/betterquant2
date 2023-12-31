/*!
 * \file File.hpp
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

bool AppendStrToFile(const std::string& filename, const std::string& filecont);
bool OverwriteStrToFile(const std::string& filename,
                        const std::string& filecont);
std::string LoadFileContToStr(const std::string& filename);
int ClearFilecont(const std::string& filename);

std::tuple<int, std::vector<std::string>> LoadFileContToVec(
    const std::string& filename);
int SaveDataToFile(const std::string& filename,
                   const std::vector<std::string>& lineGroup);

std::tuple<int, std::vector<boost::filesystem::path>>
GetFileGroupFromPathRecursively(const std::string& path);

}  // namespace bq
