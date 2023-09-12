SCODE_SUCCESS = 0

# BQPub模块中的状态码
SCODE_BQPUB_TOPIC_ALREADY_SUB = -1101
SCODE_BQPUB_TOPIC_NOT_SUB = -1102
SCODE_BQPUB_INVALID_TOPIC = -1103
SCODE_BQPUB_CALC_PRICE_FAILED = -1104
SCODE_BQPUB_INVALID_QRY_COND = -1111
SCODE_BQPUB_PNL_NOT_EXISTS = -1112
SCODE_BQPUB_POS_INFO_GROUP_NOT_EXISTS = -1113
SCODE_BQPUB_INVALID_FORMAT_OF_SIMED_TD_INFO = -1121
SCODE_BQPUB_INVALID_TRANS_DETAIL_IN_SIMED_TD_INFO = -1122
SCODE_BQPUB_INVALID_ORDER_STATUS_IN_SIMED_TD_INFO = -1123

# 交易网关自身的状态码
SCODE_TD_SVC_EXCEED_FLOW_CTRL = -3501
SCODE_TD_SVC_ORDER_NOT_SENT_TO_RMT_SRV = -3502
SCODE_TD_SVC_ORDER_FAILED_BY_API = -3503
SCODE_TD_SVC_ORDER_CANCEL_FAILED_BY_API = -3504
SCODE_TD_SVC_EXCH_ORDER_ID_IS_EMPTY = -3505
SCODE_TD_SVC_REAL_RECV_SIMED_ORDER = -3511
SCODE_TD_SVC_SIMED_RECV_REAL_ORDER = -3512
SCODE_TD_SVC_REAL_RECV_SIMED_ORDER_CANCEL = -3513
SCODE_TD_SVC_SIMED_RECV_REAL_ORDER_CANCEL = -3514
SCODE_TD_SVC_SIMED_ORDER_STATSU_FAILED = -3515

# 交易服务中的状态码
SCODE_TD_SRV_INSUFFICIENT_POS_FOR_BORROW = -4000
SCODE_TD_SRV_INVALID_POS_SIDE = -4001
SCODE_TD_SRV_TDGW_NOT_EXISTS = -4002

# 历史行情模块中的状态码
SCODE_HIS_MD_INVALID_TS = -4501
SCODE_HIS_MD_INVALID_NUM = -4502
SCODE_HIS_MD_RECORDS_LESS_THAN_NUM_OF_QURIES = -4503
SCODE_HIS_MD_NUM_OF_RECORDS_GREATER_THAN_LIMIT = -4504
SCODE_HIS_MD_MAKE_INDEX_GROUP_FAILED = -4511
SCODE_HIS_MD_GET_EXCH_TS_FAILED = -4512
SCODE_HIS_MD_SAVE_INDEX_GROUP_FAILED = -4513
SCODE_HIS_MD_LOAD_INDEX_GROUP_FAILED = -4514

# 数据库相关状态码
SCODE_DB_CAN_NOT_FIND_SYM_CODE = -5001
SCODE_DB_CAN_NOT_FIND_EXCH_SYM_CODE = -5002
SCODE_DB_CAN_NOT_FIND_STG_INST = -5003
SCODE_DB_CAN_NOT_FIND_ACCT_INFO = -5004
SCODE_DB_CAN_NOT_FIND_ACCT_ID = -5005
SCODE_DB_CAN_NOT_FIND_STG_GRP_ID = -5006
SCODE_DB_CAN_NOT_FIND_PRODUCT_GRP_ID = -5007

# TDEngine相关状态码
SCODE_TDENG_EXEC_SQL_FAILED = -5501

# 策略引擎相关状态码
SCODE_STG_MUST_HAVE_STG_INST_1 = -6002
SCODE_STG_INST_ID_MUST_START_FROM_1 = -6003
SCODE_STG_INVALID_SIMED_TD_INFO_SIZE = -6011
SCODE_STG_INVALID_MARKET_CODE = -6012
SCODE_STG_INVALID_SYMBOL_TYPE_IN_DB = -6013
SCODE_STG_INVALID_SIDE = -6014
SCODE_STG_INVALID_POS_DIRECTION = -6015
SCODE_STG_INVALID_SYMBOL_TYPE = -6016
SCODE_STG_INVALID_COMB_OF_MKT_AND_POS_DIR = -6017
SCODE_STG_ENG_INVALID_CONFIG_FILENAME = -6041
SCODE_STG_INST_TASK_HANDLER_NOT_INSTALL = -6051
SCODE_STG_SEND_HTTP_REQ_TO_QUERY_HIS_MD_FAILED = -6061
SCODE_STG_INVALID_TOPIC = -6071

# 算法单相关状态码
SCODE_ALGO_INVALID_ALGO_TYPE = -6501
SCODE_ALGO_INVALID_ALGO_PARAM = -6502
SCODE_ALGO_ID_NOT_EXISTS = -6503
SCODE_ALGO_INVALID_MARKET_CODE = -6504
SCODE_ALGO_INVALID_SPLIT_NUM = -6505
SCODE_ALGO_INVALID_PREC = -6506
SCODE_ALGO_INVALID_TOTAL_SIZE = -6507
SCODE_ALGO_BID1_ASK1_IS_NULL = -6551

# OrdMgr模块状态码
SCODE_ORD_MGR_ADD_ORDER_INFO_FAILED = -7001
SCODE_ORD_MGR_REMOVE_ORDER_INFO_FAILED = -7002
SCODE_ORD_MGR_CAN_NOT_FIND_ORDER = -7005

# 内置WebSrv服务状态码
SCODE_WEB_SRV_SESSION_TIMEOUT = -8001

SCODE_WEB_SRV_INVALID_BODY_IN_REQ = -8011
SCODE_WEB_SRV_EXEC_DB_CMD_FAILED = -8021
SCODE_WEB_SRV_REMOVE_SESSION_FAILED = -8022
SCODE_WEB_SRV_INVALID_USERNAME_OR_PASSWORD = -8023

# [-10001 - -18000) externalStatusCode 映射状态码

# [-18000 - -20000) 风控子系统计数相关错误码
SCODE_EXTERNAL_SYS_ORDER_REJECTED_MIN = -18000
SCODE_EXTERNAL_SYS_ORDER_REJECTED_MAX = -18100

# [10001 - 20000) 风控编号，[18000, 20000) 为不在数据库里的风控编号
SCODE_TD_SRV_RISK_EXCEED_FLOW_CTRL = 18001
SCODE_TD_SRV_RISK_EXISTS_OPEN_PENDING_ORDERS = 18011
SCODE_TD_SRV_RISK_EXISTS_OPEN_TDAY = 18012
SCODE_TD_SRV_RISK_SELF_TRADE_OF_BID = 18021
SCODE_TD_SRV_RISK_SELF_TRADE_OF_ASK = 18022
SCODE_TD_SRV_RISK_NOT_IN_WHITE_LIST = 18031
SCODE_TD_SRV_RISK_IN_BLACK_LIST = 18032
SCODE_TD_SRV_RISK_PNL_IS_NULL = 18041
SCODE_TD_SRV_RISK_PNL_IS_TIMEOUT = 18042
SCODE_TD_SRV_RISK_PNL_EXCEED_LIMIT = 18043

# 外部比如交易所的一些错误码，系统内部使用
SCODE_TD_SVC_PARSE_HTTP_RSP_OF_ORDER_FAILED = -21001
SCODE_TD_SVC_PARSE_HTTP_RSP_OF_CANCEL_ORDER_FAILED = -21002
SCODE_MD_SVC_SNAPSHOT_NOT_EXISTS = -23001
SCODE_MD_SVC_FINAL_UPDATE_ID_TOO_SMALL = -23002
SCODE_MD_SVC_FIRST_UPDATE_ID_TOO_LARGE = -23003
SCODE_MD_SVC_UPDATE_DATA_DISCONTINUOUS = -23004


def GetStatusMsg(statusCode):
    if statusCode == SCODE_SUCCESS:
        return "Success"
    elif statusCode == SCODE_BQPUB_TOPIC_ALREADY_SUB:
        return "Topic already sub"
    elif statusCode == SCODE_BQPUB_TOPIC_NOT_SUB:
        return "Topic not sub"
    elif statusCode == SCODE_BQPUB_INVALID_TOPIC:
        return "Invalid topic"
    elif statusCode == SCODE_BQPUB_CALC_PRICE_FAILED:
        return "Calc price failed"
    elif statusCode == SCODE_BQPUB_INVALID_QRY_COND:
        return "Invalid query condition"
    elif statusCode == SCODE_BQPUB_PNL_NOT_EXISTS:
        return "Pnl not exists"
    elif statusCode == SCODE_BQPUB_POS_INFO_GROUP_NOT_EXISTS:
        return "PosInfoGroup not exists"
    elif statusCode == SCODE_BQPUB_INVALID_FORMAT_OF_SIMED_TD_INFO:
        return "Invalid format of simed td info"
    elif statusCode == SCODE_BQPUB_INVALID_TRANS_DETAIL_IN_SIMED_TD_INFO:
        return "Invalid trans detail in simed td info"
    elif statusCode == SCODE_BQPUB_INVALID_ORDER_STATUS_IN_SIMED_TD_INFO:
        return "Invalid order status in simed td info"
    elif statusCode == SCODE_TD_SVC_EXCEED_FLOW_CTRL:
        return "Exceed flow control in td svc."
    elif statusCode == SCODE_TD_SVC_REAL_RECV_SIMED_ORDER:
        return "Real td mode recv simed td order in td svc."
    elif statusCode == SCODE_TD_SVC_SIMED_RECV_REAL_ORDER:
        return "Simed td mode recv real td order in td svc."
    elif statusCode == SCODE_TD_SVC_REAL_RECV_SIMED_ORDER_CANCEL:
        return "Real td mode recv simed td order cancel in td svc."
    elif statusCode == SCODE_TD_SVC_SIMED_RECV_REAL_ORDER_CANCEL:
        return "Simed td mode recv real td order cancel in td svc."
    elif statusCode == SCODE_TD_SVC_SIMED_ORDER_STATSU_FAILED:
        return " Simed order status failed by td svc."
    elif statusCode == SCODE_TD_SRV_INSUFFICIENT_POS_FOR_BORROW:
        return "Insufficient pos for borrow."
    elif statusCode == SCODE_TD_SRV_INVALID_POS_SIDE:
        return "Invalid pos side in order."
    elif statusCode == SCODE_TD_SRV_TDGW_NOT_EXISTS:
        return "TDGW not exists"
    elif statusCode == SCODE_HIS_MD_INVALID_TS:
        return "Invalid ts in query condition"
    elif statusCode == SCODE_HIS_MD_INVALID_NUM:
        return "Invalid num in query condition"
    elif statusCode == SCODE_HIS_MD_RECORDS_LESS_THAN_NUM_OF_QURIES:
        return "The number of records is less than the number of queries"
    elif statusCode == SCODE_HIS_MD_NUM_OF_RECORDS_GREATER_THAN_LIMIT:
        return "The number of returned records is greater than the limit"
    elif statusCode == SCODE_DB_CAN_NOT_FIND_SYM_CODE:
        return "Can not find symbolcode"
    elif statusCode == SCODE_DB_CAN_NOT_FIND_EXCH_SYM_CODE:
        return "Can not find exchange symbolcode"
    elif statusCode == SCODE_DB_CAN_NOT_FIND_STG_INST:
        return "Can not find stg inst"
    elif statusCode == SCODE_DB_CAN_NOT_FIND_ACCT_INFO:
        return "Can not find account info"
    elif statusCode == SCODE_DB_CAN_NOT_FIND_ACCT_ID:
        return "Can not find account id"
    elif statusCode == SCODE_DB_CAN_NOT_FIND_STG_GRP_ID:
        return "Can not find stg grp id"
    elif statusCode == SCODE_DB_CAN_NOT_FIND_PRODUCT_GRP_ID:
        return "Can not find product grp id"
    elif statusCode == SCODE_TDENG_EXEC_SQL_FAILED:
        return "Exec tdeng sql failed."
    elif statusCode == SCODE_STG_MUST_HAVE_STG_INST_1:
        return "Stg must have stg inst 1"
    elif statusCode == SCODE_STG_INST_ID_MUST_START_FROM_1:
        return "Stg inst id must start from 1"
    elif statusCode == SCODE_STG_INVALID_SIMED_TD_INFO_SIZE:
        return "Invalid simed td info size."
    elif statusCode == SCODE_STG_INVALID_MARKET_CODE:
        return "Invalid market code."
    elif statusCode == SCODE_STG_INVALID_SYMBOL_TYPE_IN_DB:
        return "Invalid symbol type in db."
    elif statusCode == SCODE_STG_INVALID_SIDE:
        return "Invalid side in order."
    elif statusCode == SCODE_STG_INVALID_POS_DIRECTION:
        return "Invalid pos direction in order."
    elif statusCode == SCODE_STG_INVALID_SYMBOL_TYPE:
        return "Invalid symbol type in order."
    elif statusCode == SCODE_STG_INVALID_COMB_OF_MKT_AND_POS_DIR:
        return "The current marketCode does not support closeTDay or closeYDay."
    elif statusCode == SCODE_STG_ENG_INVALID_CONFIG_FILENAME:
        return "Invalid config filename"
    elif statusCode == SCODE_STG_INST_TASK_HANDLER_NOT_INSTALL:
        return "StgInstTaskHandler not install"
    elif statusCode == SCODE_STG_SEND_HTTP_REQ_TO_QUERY_HIS_MD_FAILED:
        return "Stg send http request to query his market data failed"
    elif statusCode == SCODE_STG_INVALID_TOPIC:
        return "Invalid topic"
    elif statusCode == SCODE_ALGO_INVALID_ALGO_TYPE:
        return "Invalid type of algo order"
    elif statusCode == SCODE_ALGO_INVALID_ALGO_PARAM:
        return "Invalid params of algo order"
    elif statusCode == SCODE_ALGO_ID_NOT_EXISTS:
        return "Algo id not exists"
    elif statusCode == SCODE_ALGO_INVALID_MARKET_CODE:
        return "Invalid market code"
    elif statusCode == SCODE_ALGO_INVALID_SPLIT_NUM:
        return "Invalid split num"
    elif statusCode == SCODE_ALGO_INVALID_PREC:
        return "Invalid prec"
    elif statusCode == SCODE_ALGO_INVALID_TOTAL_SIZE:
        return "Invalid total size"
    elif statusCode == SCODE_ALGO_BID1_ASK1_IS_NULL:
        return "Bid1Ask1 is null"
    elif statusCode == SCODE_ORD_MGR_ADD_ORDER_INFO_FAILED:
        return "Add orderinfo failed"
    elif statusCode == SCODE_ORD_MGR_REMOVE_ORDER_INFO_FAILED:
        return "Remove order info failed"
    elif statusCode == SCODE_ORD_MGR_CAN_NOT_FIND_ORDER:
        return "Can not find orderinfo, order may have finished."
    elif statusCode == SCODE_TD_SVC_PARSE_HTTP_RSP_OF_ORDER_FAILED:
        return "Parse http rsp of order failed"
    elif statusCode == SCODE_TD_SVC_PARSE_HTTP_RSP_OF_CANCEL_ORDER_FAILED:
        return "Parse http rsp of cancel order failed"
    elif statusCode == SCODE_MD_SVC_SNAPSHOT_NOT_EXISTS:
        return "Snapshot not exists"
    elif statusCode == SCODE_MD_SVC_FINAL_UPDATE_ID_TOO_SMALL:
        return "Final update id too small"
    elif statusCode == SCODE_MD_SVC_FIRST_UPDATE_ID_TOO_LARGE:
        return "First update id too large"
    elif statusCode == SCODE_MD_SVC_UPDATE_DATA_DISCONTINUOUS:
        return "Update data discontinuous"
    elif statusCode == SCODE_WEB_SRV_SESSION_TIMEOUT:
        return "Session timed out or not logged in."
    elif statusCode == SCODE_WEB_SRV_INVALID_BODY_IN_REQ:
        return "Invalid body in request."
    elif statusCode == SCODE_WEB_SRV_EXEC_DB_CMD_FAILED:
        return "Execute database command failed."
    elif statusCode == SCODE_WEB_SRV_REMOVE_SESSION_FAILED:
        return "Remove session failed."
    elif statusCode == SCODE_WEB_SRV_INVALID_USERNAME_OR_PASSWORD:
        return "Invalid username or password."
    elif statusCode == SCODE_TD_SRV_RISK_EXCEED_FLOW_CTRL:
        return "Exceed flow control in risk ctrl of td srv."
    elif statusCode == SCODE_TD_SRV_RISK_EXISTS_OPEN_PENDING_ORDERS:
        return "There is already an open pending order for this contract today."
    elif statusCode == SCODE_TD_SRV_RISK_EXISTS_OPEN_TDAY:
        return "There is already a open order for this contract today."
    elif statusCode == SCODE_TD_SRV_RISK_SELF_TRADE_OF_BID:
        return "The current order may cause self trade of bid."
    elif statusCode == SCODE_TD_SRV_RISK_SELF_TRADE_OF_ASK:
        return "The current order may cause self trade of ask."
    elif statusCode == SCODE_TD_SRV_RISK_NOT_IN_WHITE_LIST:
        return "The symbol of current order not in white list."
    elif statusCode == SCODE_TD_SRV_RISK_IN_BLACK_LIST:
        return "The symbol of current order in black list."
    elif statusCode == SCODE_TD_SRV_RISK_PNL_IS_NULL:
        return "Pnl is null."
    elif statusCode == SCODE_TD_SRV_RISK_PNL_IS_TIMEOUT:
        return "Pnl is timeout."
    elif statusCode == SCODE_TD_SRV_RISK_PNL_EXCEED_LIMIT:
        return "Pnl exceed limit."
    elif statusCode == SCODE_TD_SVC_ORDER_NOT_SENT_TO_RMT_SRV:
        return "Order not sent to remote sever in td srv."
    elif statusCode == SCODE_TD_SVC_ORDER_FAILED_BY_API:
        return "Order failed by api."
    elif statusCode == SCODE_TD_SVC_ORDER_CANCEL_FAILED_BY_API:
        return "Order cancel failed by api."
    elif statusCode == SCODE_TD_SVC_EXCH_ORDER_ID_IS_EMPTY:
        return "Order cancel failed because exch order id is empty."
    elif statusCode > 10000 and statusCode < 18000:
        return "Trigger risk ctrl."
    else:
        return "N/A"
