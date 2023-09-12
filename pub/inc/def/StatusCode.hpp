/*!
 * \file StatusCode.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

const static int SCODE_SUCCESS = 0;

//! BQPub模块中的状态码
const static int SCODE_BQPUB_TOPIC_ALREADY_SUB = -1101;
const static int SCODE_BQPUB_TOPIC_NOT_SUB = -1102;
const static int SCODE_BQPUB_INVALID_TOPIC = -1103;
const static int SCODE_BQPUB_CALC_PRICE_FAILED = -1104;
const static int SCODE_BQPUB_INVALID_QRY_COND = -1111;
const static int SCODE_BQPUB_PNL_NOT_EXISTS = -1112;
const static int SCODE_BQPUB_POS_INFO_GROUP_NOT_EXISTS = -1113;
const static int SCODE_BQPUB_INVALID_FORMAT_OF_SIMED_TD_INFO = -1121;
const static int SCODE_BQPUB_INVALID_TRANS_DETAIL_IN_SIMED_TD_INFO = -1122;
const static int SCODE_BQPUB_INVALID_ORDER_STATUS_IN_SIMED_TD_INFO = -1123;

//! 交易网关自身的状态码
const static int SCODE_TD_SVC_EXCEED_FLOW_CTRL = -3501;
const static int SCODE_TD_SVC_ORDER_NOT_SENT_TO_RMT_SRV = -3502;
const static int SCODE_TD_SVC_ORDER_FAILED_BY_API = -3503;
const static int SCODE_TD_SVC_ORDER_CANCEL_FAILED_BY_API = -3504;
const static int SCODE_TD_SVC_EXCH_ORDER_ID_IS_EMPTY = -3505;
const static int SCODE_TD_SVC_REAL_RECV_SIMED_ORDER = -3511;
const static int SCODE_TD_SVC_SIMED_RECV_REAL_ORDER = -3512;
const static int SCODE_TD_SVC_REAL_RECV_SIMED_ORDER_CANCEL = -3513;
const static int SCODE_TD_SVC_SIMED_RECV_REAL_ORDER_CANCEL = -3514;
const static int SCODE_TD_SVC_SIMED_ORDER_STATSU_FAILED = -3515;

//! 交易服务中的状态码
const static int SCODE_TD_SRV_INSUFFICIENT_POS_FOR_BORROW = -4000;
const static int SCODE_TD_SRV_INVALID_POS_SIDE = -4001;
const static int SCODE_TD_SRV_TDGW_NOT_EXISTS = -4002;

//! 历史行情模块中的状态码
const static int SCODE_HIS_MD_INVALID_TS = -4501;
const static int SCODE_HIS_MD_INVALID_NUM = -4502;
const static int SCODE_HIS_MD_RECORDS_LESS_THAN_NUM_OF_QURIES = -4503;
const static int SCODE_HIS_MD_NUM_OF_RECORDS_GREATER_THAN_LIMIT = -4504;
const static int SCODE_HIS_MD_MAKE_INDEX_GROUP_FAILED = -4511;
const static int SCODE_HIS_MD_GET_EXCH_TS_FAILED = -4512;
const static int SCODE_HIS_MD_SAVE_INDEX_GROUP_FAILED = -4513;
const static int SCODE_HIS_MD_LOAD_INDEX_GROUP_FAILED = -4514;

//! 数据库相关状态码
const static int SCODE_DB_CAN_NOT_FIND_SYM_CODE = -5001;
const static int SCODE_DB_CAN_NOT_FIND_EXCH_SYM_CODE = -5002;
const static int SCODE_DB_CAN_NOT_FIND_STG_INST = -5003;
const static int SCODE_DB_CAN_NOT_FIND_ACCT_INFO = -5004;
const static int SCODE_DB_CAN_NOT_FIND_ACCT_ID = -5005;
const static int SCODE_DB_CAN_NOT_FIND_STG_GRP_ID = -5006;
const static int SCODE_DB_CAN_NOT_FIND_PRODUCT_GRP_ID = -5007;

//! TDEngine相关状态码
const static int SCODE_TDENG_EXEC_SQL_FAILED = -5501;

//! 策略引擎相关状态码
const static int SCODE_STG_MUST_HAVE_STG_INST_1 = -6002;
const static int SCODE_STG_INST_ID_MUST_START_FROM_1 = -6003;
const static int SCODE_STG_INVALID_SIMED_TD_INFO_SIZE = -6011;
const static int SCODE_STG_INVALID_MARKET_CODE = -6012;
const static int SCODE_STG_INVALID_SYMBOL_TYPE_IN_DB = -6013;
const static int SCODE_STG_INVALID_SIDE = -6014;
const static int SCODE_STG_INVALID_POS_DIRECTION = -6015;
const static int SCODE_STG_INVALID_SYMBOL_TYPE = -6016;
const static int SCODE_STG_INVALID_COMB_OF_MKT_AND_POS_DIR = -6017;
const static int SCODE_STG_ENG_INVALID_CONFIG_FILENAME = -6041;
const static int SCODE_STG_INST_TASK_HANDLER_NOT_INSTALL = -6051;
const static int SCODE_STG_SEND_HTTP_REQ_TO_QUERY_HIS_MD_FAILED = -6061;
const static int SCODE_STG_INVALID_TOPIC = -6071;

//! 算法单相关状态码
const static int SCODE_ALGO_INVALID_ALGO_TYPE = -6501;
const static int SCODE_ALGO_INVALID_ALGO_PARAM = -6502;
const static int SCODE_ALGO_ID_NOT_EXISTS = -6503;
const static int SCODE_ALGO_INVALID_MARKET_CODE = -6504;
const static int SCODE_ALGO_INVALID_SPLIT_NUM = -6505;
const static int SCODE_ALGO_INVALID_PREC = -6506;
const static int SCODE_ALGO_INVALID_TOTAL_SIZE = -6507;
const static int SCODE_ALGO_BID1_ASK1_IS_NULL = -6551;

//! OrdMgr模块状态码
const static int SCODE_ORD_MGR_ADD_ORDER_INFO_FAILED = -7001;
const static int SCODE_ORD_MGR_REMOVE_ORDER_INFO_FAILED = -7002;
const static int SCODE_ORD_MGR_CAN_NOT_FIND_ORDER = -7005;

//! 内置WebSrv服务状态码
const static int SCODE_WEB_SRV_SESSION_TIMEOUT = -8001;

const static int SCODE_WEB_SRV_INVALID_BODY_IN_REQ = -8011;
const static int SCODE_WEB_SRV_EXEC_DB_CMD_FAILED = -8021;
const static int SCODE_WEB_SRV_REMOVE_SESSION_FAILED = -8022;
const static int SCODE_WEB_SRV_INVALID_USERNAME_OR_PASSWORD = -8023;

const static int SCODE_START_STG_FAILED = -8051;
const static int SCODE_STOP_STG_FAILED = -8052;
const static int SCODE_STG_ALREADY_STARTED = -8053;
const static int SCODE_STG_NOT_START = -8054;

//! [-10001 - -18000) externalStatusCode 映射状态码

//! [-18000 - -20000) 风控子系统计数相关错误码
const static int SCODE_EXTERNAL_SYS_ORDER_REJECTED_MIN = -18000;
const static int SCODE_EXTERNAL_SYS_ORDER_REJECTED_MAX = -18100;

//! [10001 - 20000) 风控编号，[18000, 20000) 为不在数据库里的风控编号
const static int SCODE_TD_SRV_RISK_EXCEED_FLOW_CTRL = 18001;
const static int SCODE_TD_SRV_RISK_EXISTS_OPEN_PENDING_ORDERS = 18011;
const static int SCODE_TD_SRV_RISK_EXISTS_OPEN_TDAY = 18012;
const static int SCODE_TD_SRV_RISK_SELF_TRADE_OF_BID = 18021;
const static int SCODE_TD_SRV_RISK_SELF_TRADE_OF_ASK = 18022;
const static int SCODE_TD_SRV_RISK_NOT_IN_WHITE_LIST = 18031;
const static int SCODE_TD_SRV_RISK_IN_BLACK_LIST = 18032;
const static int SCODE_TD_SRV_RISK_PNL_IS_NULL = 18041;
const static int SCODE_TD_SRV_RISK_PNL_IS_TIMEOUT = 18042;
const static int SCODE_TD_SRV_RISK_PNL_EXCEED_LIMIT = 18043;

//! 外部比如交易所的一些错误码，系统内部使用
const static int SCODE_TD_SVC_PARSE_HTTP_RSP_OF_ORDER_FAILED = -21001;
const static int SCODE_TD_SVC_PARSE_HTTP_RSP_OF_CANCEL_ORDER_FAILED = -21002;
const static int SCODE_MD_SVC_SNAPSHOT_NOT_EXISTS = -23001;
const static int SCODE_MD_SVC_FINAL_UPDATE_ID_TOO_SMALL = -23002;
const static int SCODE_MD_SVC_FIRST_UPDATE_ID_TOO_LARGE = -23003;
const static int SCODE_MD_SVC_UPDATE_DATA_DISCONTINUOUS = -23004;

inline std::string GetStatusMsg(int statusCode) {
  if (statusCode == SCODE_SUCCESS) {
    return "Success";
  } else if (statusCode == SCODE_BQPUB_TOPIC_ALREADY_SUB) {
    return "Topic already sub";
  } else if (statusCode == SCODE_BQPUB_TOPIC_NOT_SUB) {
    return "Topic not sub";
  } else if (statusCode == SCODE_BQPUB_INVALID_TOPIC) {
    return "Invalid topic";
  } else if (statusCode == SCODE_BQPUB_CALC_PRICE_FAILED) {
    return "Calc price failed";
  } else if (statusCode == SCODE_BQPUB_INVALID_QRY_COND) {
    return "Invalid query condition";
  } else if (statusCode == SCODE_BQPUB_PNL_NOT_EXISTS) {
    return "Pnl not exists";
  } else if (statusCode == SCODE_BQPUB_POS_INFO_GROUP_NOT_EXISTS) {
    return "PosInfoGroup not exists";
  } else if (statusCode == SCODE_BQPUB_INVALID_FORMAT_OF_SIMED_TD_INFO) {
    return "Invalid format of simed td info";
  } else if (statusCode == SCODE_BQPUB_INVALID_TRANS_DETAIL_IN_SIMED_TD_INFO) {
    return "Invalid trans detail in simed td info";
  } else if (statusCode == SCODE_BQPUB_INVALID_ORDER_STATUS_IN_SIMED_TD_INFO) {
    return "Invalid order status in simed td info";
  } else if (statusCode == SCODE_TD_SVC_EXCEED_FLOW_CTRL) {
    return "Exceed flow control in td svc.";
  } else if (statusCode == SCODE_TD_SVC_REAL_RECV_SIMED_ORDER) {
    return "Real td mode recv simed td order in td svc.";
  } else if (statusCode == SCODE_TD_SVC_SIMED_RECV_REAL_ORDER) {
    return "Simed td mode recv real td order in td svc.";
  } else if (statusCode == SCODE_TD_SVC_REAL_RECV_SIMED_ORDER_CANCEL) {
    return "Real td mode recv simed td order cancel in td svc.";
  } else if (statusCode == SCODE_TD_SVC_SIMED_RECV_REAL_ORDER_CANCEL) {
    return "Simed td mode recv real td order cancel in td svc.";
  } else if (statusCode == SCODE_TD_SVC_SIMED_ORDER_STATSU_FAILED) {
    return " Simed order status failed by td svc.";
  } else if (statusCode == SCODE_TD_SRV_INSUFFICIENT_POS_FOR_BORROW) {
    return "Insufficient pos for borrow.";
  } else if (statusCode == SCODE_TD_SRV_INVALID_POS_SIDE) {
    return "Invalid pos side in order.";
  } else if (statusCode == SCODE_TD_SRV_TDGW_NOT_EXISTS) {
    return "TDGW not exists";
  } else if (statusCode == SCODE_HIS_MD_INVALID_TS) {
    return "Invalid ts in query condition";
  } else if (statusCode == SCODE_HIS_MD_INVALID_NUM) {
    return "Invalid num in query condition";
  } else if (statusCode == SCODE_HIS_MD_RECORDS_LESS_THAN_NUM_OF_QURIES) {
    return "The number of records is less than the number of queries";
  } else if (statusCode == SCODE_HIS_MD_NUM_OF_RECORDS_GREATER_THAN_LIMIT) {
    return "The number of returned records is greater than the limit";
  } else if (statusCode == SCODE_DB_CAN_NOT_FIND_SYM_CODE) {
    return "Can not find symbolcode";
  } else if (statusCode == SCODE_DB_CAN_NOT_FIND_EXCH_SYM_CODE) {
    return "Can not find exchange symbolcode";
  } else if (statusCode == SCODE_DB_CAN_NOT_FIND_STG_INST) {
    return "Can not find stg inst";
  } else if (statusCode == SCODE_DB_CAN_NOT_FIND_ACCT_INFO) {
    return "Can not find account info";
  } else if (statusCode == SCODE_DB_CAN_NOT_FIND_ACCT_ID) {
    return "Can not find account id";
  } else if (statusCode == SCODE_DB_CAN_NOT_FIND_STG_GRP_ID) {
    return "Can not find stg grp id";
  } else if (statusCode == SCODE_DB_CAN_NOT_FIND_PRODUCT_GRP_ID) {
    return "Can not find product grp id";
  } else if (statusCode == SCODE_TDENG_EXEC_SQL_FAILED) {
    return "Exec tdeng sql failed.";
  } else if (statusCode == SCODE_STG_MUST_HAVE_STG_INST_1) {
    return "Stg must have stg inst 1";
  } else if (statusCode == SCODE_STG_INST_ID_MUST_START_FROM_1) {
    return "Stg inst id must start from 1";
  } else if (statusCode == SCODE_STG_INVALID_SIMED_TD_INFO_SIZE) {
    return "Invalid simed td info size.";
  } else if (statusCode == SCODE_STG_INVALID_MARKET_CODE) {
    return "Invalid market code.";
  } else if (statusCode == SCODE_STG_INVALID_SYMBOL_TYPE_IN_DB) {
    return "Invalid symbol type in db.";
  } else if (statusCode == SCODE_STG_INVALID_SIDE) {
    return "Invalid side in order.";
  } else if (statusCode == SCODE_STG_INVALID_POS_DIRECTION) {
    return "Invalid pos direction in order.";
  } else if (statusCode == SCODE_STG_INVALID_SYMBOL_TYPE) {
    return "Invalid symbol type in order.";
  } else if (statusCode == SCODE_STG_INVALID_COMB_OF_MKT_AND_POS_DIR) {
    return "The current marketCode does not support closeTDay or closeYDay.";
  } else if (statusCode == SCODE_STG_ENG_INVALID_CONFIG_FILENAME) {
    return "Invalid config filename";
  } else if (statusCode == SCODE_STG_INST_TASK_HANDLER_NOT_INSTALL) {
    return "StgInstTaskHandler not install";
  } else if (statusCode == SCODE_STG_SEND_HTTP_REQ_TO_QUERY_HIS_MD_FAILED) {
    return "Stg send http request to query his market data failed";
  } else if (statusCode == SCODE_STG_INVALID_TOPIC) {
    return "Invalid topic";
  } else if (statusCode == SCODE_ALGO_INVALID_ALGO_TYPE) {
    return "Invalid type of algo order";
  } else if (statusCode == SCODE_ALGO_INVALID_ALGO_PARAM) {
    return "Invalid params of algo order";
  } else if (statusCode == SCODE_ALGO_ID_NOT_EXISTS) {
    return "Algo id not exists";
  } else if (statusCode == SCODE_ALGO_INVALID_MARKET_CODE) {
    return "Invalid market code";
  } else if (statusCode == SCODE_ALGO_INVALID_SPLIT_NUM) {
    return "Invalid split num";
  } else if (statusCode == SCODE_ALGO_INVALID_PREC) {
    return "Invalid prec";
  } else if (statusCode == SCODE_ALGO_INVALID_TOTAL_SIZE) {
    return "Invalid total size";
  } else if (statusCode == SCODE_ALGO_BID1_ASK1_IS_NULL) {
    return "Bid1Ask1 is null";
  } else if (statusCode == SCODE_ORD_MGR_ADD_ORDER_INFO_FAILED) {
    return "Add orderinfo failed";
  } else if (statusCode == SCODE_ORD_MGR_REMOVE_ORDER_INFO_FAILED) {
    return "Remove order info failed";
  } else if (statusCode == SCODE_ORD_MGR_CAN_NOT_FIND_ORDER) {
    return "Can not find orderinfo, order may have finished.";
  } else if (statusCode == SCODE_TD_SVC_PARSE_HTTP_RSP_OF_ORDER_FAILED) {
    return "Parse http rsp of order failed";
  } else if (statusCode == SCODE_TD_SVC_PARSE_HTTP_RSP_OF_CANCEL_ORDER_FAILED) {
    return "Parse http rsp of cancel order failed";
  } else if (statusCode == SCODE_MD_SVC_SNAPSHOT_NOT_EXISTS) {
    return "Snapshot not exists";
  } else if (statusCode == SCODE_MD_SVC_FINAL_UPDATE_ID_TOO_SMALL) {
    return "Final update id too small";
  } else if (statusCode == SCODE_MD_SVC_FIRST_UPDATE_ID_TOO_LARGE) {
    return "First update id too large";
  } else if (statusCode == SCODE_MD_SVC_UPDATE_DATA_DISCONTINUOUS) {
    return "Update data discontinuous";
  } else if (statusCode == SCODE_WEB_SRV_SESSION_TIMEOUT) {
    return "Session timed out or not logged in.";
  } else if (statusCode == SCODE_WEB_SRV_INVALID_BODY_IN_REQ) {
    return "Invalid body in request.";
  } else if (statusCode == SCODE_WEB_SRV_EXEC_DB_CMD_FAILED) {
    return "Execute database command failed.";
  } else if (statusCode == SCODE_WEB_SRV_REMOVE_SESSION_FAILED) {
    return "Remove session failed.";
  } else if (statusCode == SCODE_WEB_SRV_INVALID_USERNAME_OR_PASSWORD) {
    return "Invalid username or password.";
  } else if (statusCode == SCODE_START_STG_FAILED ) {
    return "Start stg failed.";
  } else if (statusCode == SCODE_STOP_STG_FAILED ) {
    return "Stop stg failed.";
  } else if (statusCode == SCODE_STG_ALREADY_STARTED ) {
    return "Stg already started.";
  } else if (statusCode == SCODE_STG_NOT_START ) {
    return "Stg not start.";
  } else if (statusCode == SCODE_TD_SRV_RISK_EXCEED_FLOW_CTRL) {
    return "Exceed flow control in risk ctrl of td srv.";
  } else if (statusCode == SCODE_TD_SRV_RISK_EXISTS_OPEN_PENDING_ORDERS) {
    return "There is already an open pending order for this contract today.";
  } else if (statusCode == SCODE_TD_SRV_RISK_EXISTS_OPEN_TDAY) {
    return "There is already a open order for this contract today.";
  } else if (statusCode == SCODE_TD_SRV_RISK_SELF_TRADE_OF_BID) {
    return "The current order may cause self trade of bid.";
  } else if (statusCode == SCODE_TD_SRV_RISK_SELF_TRADE_OF_ASK) {
    return "The current order may cause self trade of ask.";
  } else if (statusCode == SCODE_TD_SRV_RISK_NOT_IN_WHITE_LIST) {
    return "The symbol of current order not in white list.";
  } else if (statusCode == SCODE_TD_SRV_RISK_IN_BLACK_LIST) {
    return "The symbol of current order in black list.";
  } else if (statusCode == SCODE_TD_SRV_RISK_PNL_IS_NULL) {
    return "Pnl is null.";
  } else if (statusCode == SCODE_TD_SRV_RISK_PNL_IS_TIMEOUT) {
    return "Pnl is timeout.";
  } else if (statusCode == SCODE_TD_SRV_RISK_PNL_EXCEED_LIMIT) {
    return "Pnl exceed limit.";
  } else if (statusCode == SCODE_TD_SVC_ORDER_NOT_SENT_TO_RMT_SRV) {
    return "Order not sent to remote sever in td srv.";
  } else if (statusCode == SCODE_TD_SVC_ORDER_FAILED_BY_API) {
    return "Order failed by api.";
  } else if (statusCode == SCODE_TD_SVC_ORDER_CANCEL_FAILED_BY_API) {
    return "Order cancel failed by api.";
  } else if (statusCode == SCODE_TD_SVC_EXCH_ORDER_ID_IS_EMPTY) {
    return "Order cancel failed because exch order id is empty.";
  } else if (statusCode > 10000 && statusCode < 18000) {
    return "Trigger risk ctrl.";
  } else {
    return "N/A";
  }
  return "N/A";
}
