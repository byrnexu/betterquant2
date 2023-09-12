/*!
 * \file TDEngUtil.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/29
 *
 * \brief
 */

#include "tdeng/TDEngUtil.hpp"

#include "def/StatusCode.hpp"
#include "taos.h"
#include "tdeng/TDEngConnpool.hpp"
#include "util/Logger.hpp"

typedef int16_t VarDataLenT;

#define TSDB_NCHAR_SIZE sizeof(int32_t)
#define VARSTR_HEADER_SIZE sizeof(VarDataLenT)

#define GET_FLOAT_VAL(x) (*(float *)(x))
#define GET_DOUBLE_VAL(x) (*(double *)(x))

#define varDataLen(v) ((VarDataLenT *)(v))[0]

namespace bq::tdeng {

std::tuple<int, std::uint32_t, std::string> GetJsonDataFromRes(
    TAOS_RES *res, std::uint32_t maxRecNum) {
  int statusCode = 0;
  std::uint32_t recNum = 0;

  const auto fields = taos_fetch_fields(res);
  const auto fieldsNum = taos_num_fields(res);
  std::vector<std::string> fieldGroup;
  for (auto i = 0; i < fieldsNum; ++i) {
    fieldGroup.emplace_back(fields[i].name);
  }

  rapidjson::StringBuffer strBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);
  writer.StartObject();

  writer.Key("recSet");
  writer.StartArray();

  TAOS_ROW row = nullptr;
  while ((row = taos_fetch_row(res))) {
    recNum += 1;
    if (recNum > maxRecNum) {
      statusCode = SCODE_HIS_MD_NUM_OF_RECORDS_GREATER_THAN_LIMIT;
      break;
    }

    writer.StartObject();

    for (std::size_t i = 0; i < fieldGroup.size(); ++i) {
      switch (fields[i].type) {
        case TSDB_DATA_TYPE_TINYINT:
          writer.Key(fieldGroup[i].c_str());
          writer.Int(*((int8_t *)row[i]));
          break;

        case TSDB_DATA_TYPE_UTINYINT:
          writer.Key(fieldGroup[i].c_str());
          writer.Uint(*((uint8_t *)row[i]));
          break;

        case TSDB_DATA_TYPE_SMALLINT:
          writer.Key(fieldGroup[i].c_str());
          writer.Int(*((int16_t *)row[i]));
          break;

        case TSDB_DATA_TYPE_USMALLINT:
          writer.Key(fieldGroup[i].c_str());
          writer.Uint(*((uint16_t *)row[i]));
          break;

        case TSDB_DATA_TYPE_INT:
          writer.Key(fieldGroup[i].c_str());
          writer.Int(*((int32_t *)row[i]));
          break;

        case TSDB_DATA_TYPE_UINT:
          writer.Key(fieldGroup[i].c_str());
          writer.Uint(*((uint32_t *)row[i]));
          break;

        case TSDB_DATA_TYPE_BIGINT:
          writer.Key(fieldGroup[i].c_str());
          writer.Int64(*((int64_t *)row[i]));
          break;

        case TSDB_DATA_TYPE_UBIGINT:
          writer.Key(fieldGroup[i].c_str());
          writer.Uint64(*((uint64_t *)row[i]));
          break;

        case TSDB_DATA_TYPE_FLOAT: {
          writer.Key(fieldGroup[i].c_str());
          float fv = 0;
          fv = GET_FLOAT_VAL(row[i]);
          writer.Double(fv);
        } break;

        case TSDB_DATA_TYPE_DOUBLE: {
          writer.Key(fieldGroup[i].c_str());
          double dv = 0;
          dv = GET_DOUBLE_VAL(row[i]);
          writer.Double(dv);
        } break;

        case TSDB_DATA_TYPE_BINARY:
        case TSDB_DATA_TYPE_NCHAR: {
          writer.Key(fieldGroup[i].c_str());
          int32_t charLen = varDataLen((char *)row[i] - VARSTR_HEADER_SIZE);
          std::string v((char *)row[i], charLen);
          writer.String(v.c_str());
        } break;

        case TSDB_DATA_TYPE_TIMESTAMP:
          writer.Key(fieldGroup[i].c_str());
          writer.Uint64(*(uint64_t *)row[i]);
          break;

        case TSDB_DATA_TYPE_BOOL:
          writer.Key(fieldGroup[i].c_str());
          writer.Int(*((int8_t *)row[i]));

        default:
          break;
      }
    }
    writer.EndObject();
  }

  writer.EndArray();
  writer.EndObject();

  const std::string recSet = strBuf.GetString();
  return {statusCode, recNum, recSet};
}

std::tuple<int, std::string, int, std::string> QueryDataFromTDEng(
    const TDEngConnpoolSPtr &tdEngConnpool, const std::string &sql,
    std::uint32_t maxRecNum) {
  int statusCode = 0;
  std::string statusMsg;
  int recNum = 0;
  std::string recSet;

  const auto conn = tdEngConnpool->getIdleConn();
  const auto res = taos_query(conn->taos_, sql.c_str());
  const auto errorCode = taos_errno(res);
  if (errorCode == 0) {
    std::tie(statusCode, recNum, recSet) =
        tdeng::GetJsonDataFromRes(res, maxRecNum);
    //! 返回的结果集过大情况下清空recSet
    if (statusCode != 0) recSet = "";
    statusMsg = GetStatusMsg(statusCode);
  } else {
    statusCode = SCODE_TDENG_EXEC_SQL_FAILED;
    statusMsg = fmt::format("Exec sql of tdeng failed. [{} - {}]", errorCode,
                            taos_errno(res));
    LOG_W(statusMsg);
  }
  taos_free_result(res);
  tdEngConnpool->giveBackConn(conn);

  return {statusCode, statusMsg, recNum, recSet};
}

}  // namespace bq::tdeng
