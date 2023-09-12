#!/usr/bin/python3

# -*- coding: utf-8 -*-
import sys
import getopt
import datetime
import time
import json
from stgeng import *
from datetime import datetime, timedelta

sys.path.append(".")


class StgInstTaskHandler(StgInstTaskHandlerBase):
    def __init__(self, stg_eng):
        self.stg_eng = stg_eng
        self.algo_order_id = 0

    def on_stg_start(self):
        self.stg_eng.log_warn(
            "Begin to test. {} {}",
            ["1", "2"],
            self.stg_eng.get_dft_stg_inst_info(),
            NotifyToTerminal.IsTrue,
        )

        # 安装一个定时器
        self.stg_eng.install_stg_inst_timer(
            stg_inst_id=1,
            timer_name="TestTimerOfAlgoOrderSmartOrder",
            exec_at_startup=ExecAtStartup.IsFalse,
            millisec_interval=1000 * 30,
            max_exec_times=1,
        )

        # 安装一个定时器
        self.stg_eng.install_stg_inst_timer(
            stg_inst_id=1,
            timer_name="TestTimerOfAlgoOrderTWAP",
            exec_at_startup=ExecAtStartup.IsFalse,
            millisec_interval=1000 * 1,
            max_exec_times=1,
        )

        # 安装一个定时器
        self.stg_eng.install_stg_inst_timer(
            stg_inst_id=1,
            timer_name="TestTimerOfCancelAlgoOrder",
            exec_at_startup=ExecAtStartup.IsFalse,
            millisec_interval=1000 * 3600 * 24,
            max_exec_times=1,
        )

        # 安装一个定时器，每隔1000毫秒触发一次，触发1000次后停止
        self.stg_eng.install_stg_inst_timer(
            stg_inst_id=1,
            timer_name="TestTimerOfIntervalTime",
            exec_at_startup=ExecAtStartup.IsFalse,
            millisec_interval=1000 * 3600 * 24,
            max_exec_times=0,
        )

        # 安装一个定时器，每天8点触发
        self.stg_eng.install_stg_inst_timer(
            stg_inst_id=1, timer_name="TestTimerOfFixedTime", exec_time="h08m00s00"
        )

        # 安装一个定时器，millisec_interval毫秒后触发
        self.stg_eng.install_stg_inst_timer(
            stg_inst_id=1,
            timer_name="TestGetUnclosedOrderInfoOfStgInst",
            exec_at_startup=ExecAtStartup.IsFalse,
            millisec_interval=1000 * 3600 * 24,
            max_exec_times=1,
        )

        # 安装一个定时器，millisec_interval毫秒后触发
        self.stg_eng.install_stg_inst_timer(
            stg_inst_id=1,
            timer_name="TestGetPosOfStgInst",
            exec_at_startup=ExecAtStartup.IsFalse,
            millisec_interval=1000 * 3600 * 24,
            max_exec_times=1,
        )

        # 安装一个定时器，millisec_interval毫秒后触发
        self.stg_eng.install_stg_inst_timer(
            stg_inst_id=1,
            timer_name="TestCancelAllOrderOfStgInst",
            millisec_interval=1000 * 3600 * 24,
            exec_at_startup=ExecAtStartup.IsFalse,
            max_exec_times=1,
        )

        # 安装一个定时器，millisec_interval毫秒后触发
        self.stg_eng.install_stg_inst_timer(
            stg_inst_id=1,
            timer_name="TestSelfTrade",
            exec_at_startup=ExecAtStartup.IsFalse,
            millisec_interval=1000 * 3600 * 24,
            max_exec_times=1,
        )

        # 安装一个定时器，millisec_interval毫秒后触发
        self.stg_eng.install_stg_inst_timer(
            stg_inst_id=1,
            timer_name="TestRealOrderSpot",
            exec_at_startup=ExecAtStartup.IsFalse,
            millisec_interval=1000 * 3600 * 24,
            max_exec_times=1,
        )

        # 安装一个定时器，millisec_interval毫秒后触发
        self.stg_eng.install_stg_inst_timer(
            stg_inst_id=1,
            timer_name="TestRealOrderFutures",
            exec_at_startup=ExecAtStartup.IsFalse,
            millisec_interval=1000 * 3600 * 24,
            max_exec_times=1,
        )

        # 安装一个定时器，millisec_interval毫秒后触发
        self.stg_eng.install_stg_inst_timer(
            stg_inst_id=1,
            timer_name="TestSimedOrder",
            exec_at_startup=ExecAtStartup.IsFalse,
            millisec_interval=1000 * 3600 * 24,
            max_exec_times=1,
        )

        # 安装一个定时器，millisec_interval毫秒后触发
        self.stg_eng.install_stg_inst_timer(
            stg_inst_id=1,
            timer_name="TestCancelOrder",
            exec_at_startup=ExecAtStartup.IsFalse,
            millisec_interval=1000 * 3600 * 24,
            max_exec_times=1,
        )

        self.__query_his_md()

    def __query_his_md(self):
        ret_of_qry = self.stg_eng.query_specific_num_of_his_md_after_ts(
            stg_inst_info=None,
            topic="MD@SZSE@Spot@000001@Tickers",
            ts=1672363452000000,
            num=2,
        )

        ret_of_qry = self.stg_eng.query_specific_num_of_his_md_before_ts(
            stg_inst_info=None,
            topic="MD@SZSE@Spot@000001@Tickers",
            ts=1672363452000000,
            num=2,
        )

        ret_of_qry = self.stg_eng.query_his_md_between_2_ts(
            stg_inst_info=self.stg_eng.get_dft_stg_inst_info(),
            topic="MD@SZSE@Spot@000001@Tickers",
            ts_begin=1672363452000000,
            ts_end=1672363470000000,
        )

        ret_of_qry = self.stg_eng.query_specific_num_of_his_md_after_ts(
            stg_inst_info=self.stg_eng.get_dft_stg_inst_info(),
            topic="MD@SSE@Spot@204001@Trades",
            ts=1672297393990036,
            num=2,
        )

        ret_of_qry = self.stg_eng.query_specific_num_of_his_md_before_ts(
            stg_inst_info=self.stg_eng.get_dft_stg_inst_info(),
            topic="MD@SSE@Spot@204001@Trades",
            ts=1672297393990036,
            num=2,
        )

        ret_of_qry = self.stg_eng.query_his_md_between_2_ts(
            stg_inst_info=self.stg_eng.get_dft_stg_inst_info(),
            topic="MD@SSE@Spot@204001@Trades",
            ts_begin=1672297393990036,
            ts_end=1672297393990046,
        )

        ret_of_qry = self.stg_eng.query_specific_num_of_his_md_after_ts(
            stg_inst_info=self.stg_eng.get_dft_stg_inst_info(),
            topic="MD@SZSE@Spot@000001@Orders",
            ts=1672362900500001,
            num=2,
        )

        ret_of_qry = self.stg_eng.query_specific_num_of_his_md_before_ts(
            stg_inst_info=self.stg_eng.get_dft_stg_inst_info(),
            topic="MD@SZSE@Spot@000001@Orders",
            ts=1672362900500003,
            num=2,
        )

        ret_of_qry = self.stg_eng.query_his_md_between_2_ts(
            stg_inst_info=self.stg_eng.get_dft_stg_inst_info(),
            topic="MD@SZSE@Spot@000001@Orders",
            ts_begin=1672362900500001,
            ts_end=1672362900500003,
        )

        print(f"===== {ret_of_qry[0]}")
        print(f"===== {ret_of_qry[1]}")

        return

    def on_stg_inst_start(self, stg_inst_info):
        if stg_inst_info.stg_inst_id == 1:
            self.stg_eng.sub(
                stg_inst_info.stg_inst_id, "shm://MD.SSE.Spot/603123/Tickers"
            )
            self.stg_eng.sub(
                stg_inst_info.stg_inst_id, "shm://MD.SSE.Spot/603123/Orders"
            )
            self.stg_eng.sub(
                stg_inst_info.stg_inst_id, "shm://MD.SSE.Spot/603123/Trades"
            )
            self.stg_eng.sub(
                stg_inst_info.stg_inst_id, "shm://MD.DCE.Futures/a2309/Tickers"
            )

            # self.stg_eng.sub(
            #     stg_inst_info.stg_inst_id, "shm://MD.SSE.Spot/603123/Bid1Ask1"
            # )
            # self.stg_eng.sub(
            #     stg_inst_info.stg_inst_id, "shm://MD.SZSE.Spot/000002/LastPrice"
            # )
            # self.stg_eng.sub(
            #     stg_inst_info.stg_inst_id, "shm://MD.DCE.Futures/a2309/Bid1Ask1"
            # )
            # self.stg_eng.sub(
            #     stg_inst_info.stg_inst_id, "shm://MD.DCE.Futures/a2309/LastPrice"
            # )

            tsStart = int(time.time() / 60) * 60 + 60
            self.stg_eng.sub(
                stg_inst_info.stg_inst_id,
                f"shm://MD.SZSE.Spot/000002/DynCandle/tsStart/{tsStart}/interval/15",
            )
            tsStart = int(time.time() / 60) * 60 + 10
            self.stg_eng.sub(
                stg_inst_info.stg_inst_id,
                f"shm://MD.SZSE.Spot/000002/DynCandle/tsStart/{tsStart}/interval/60",
            )
            tsStart = int(time.time() / 60) * 60 + 60
            self.stg_eng.sub(
                stg_inst_info.stg_inst_id,
                f"shm://MD.SZSE.Spot/002607/DynCandle/tsStart/{tsStart}/interval/15",
            )

            # sub pos info of stg id 10000, note that the topic is case sensitive.
            self.stg_eng.sub(
                stg_inst_info.stg_inst_id,
                "shm://RISK.PubChannel.Trade/PosInfo/StgId/10000",
            )

            # sub pos info of stg inst id 1, note that the topic is case sensitive.
            self.stg_eng.sub(
                stg_inst_info.stg_inst_id,
                "shm://RISK.PubChannel.Trade/PosInfo/StgId/10000/StgInstId/1",
            )

            # sub pos info of stg inst id 2, note that the topic is case sensitive.
            self.stg_eng.sub(
                stg_inst_info.stg_inst_id,
                "shm://RISK.PubChannel.Trade/PosInfo/StgId/10000/StgInstId/2",
            )

            self.stg_eng.sub(
                stg_inst_info.stg_inst_id, "MD@CFFEX@Futures@IC2302@Tickers"
            )

            self.stg_eng.sub(
                stg_inst_info.stg_inst_id, "MD@CZCE@Futures@SF2305@Tickers"
            )

            # sub trigger risk ctrl of plugin
            self.stg_eng.sub(
                stg_inst_info.stg_inst_id,
                "shm://RISK.PlugInChannel.Trade/TriggerRiskCrtl/StgId/10000/StgInstId/1",
            )

        if stg_inst_info.stg_inst_id == 2:
            # sub trigger risk ctrl of plugin
            self.stg_eng.sub(
                stg_inst_info.stg_inst_id,
                "shm://RISK.PlugInChannel.Trade/TriggerRiskCrtl/StgId/10000/StgInstId/2",
            )

    def on_stg_stop(self):
        print("on_stg_stop")

    def on_stg_inst_stop(self, stg_inst_info):
        print("on_stg_inst_stop")

    def on_stg_inst_timer(self, stg_inst_info, timer_name):
        if stg_inst_info.stg_inst_id == 1:

            if timer_name == "TestTimerOfAlgoOrderSmartOrder":
                print(f"Timer {timer_name} was triggered")
                for i in range(0, 1):
                    self.__test_algo_order_smart_order(stg_inst_info)

            if timer_name == "TestTimerOfAlgoOrderTWAP":
                print(f"Timer {timer_name} was triggered")
                for i in range(0, 1):
                    self.__test_algo_order_twap(stg_inst_info)

            if timer_name == "TestTimerOfCancelAlgoOrder":
                print(f"Timer {timer_name} was triggered")
                for i in range(0, 1):
                    self.__test_cancel_algo_order(stg_inst_info)

            if timer_name == "TestGetUnclosedOrderInfoOfStgInst":
                print(f"Timer {timer_name} was triggered")
                for i in range(0, 1):
                    self.__test_unclosed_order_info_of_stg_inst(stg_inst_info)

            if timer_name == "TestGetPosOfStgInst":
                print(f"Timer {timer_name} was triggered")
                for i in range(0, 1):
                    self.__test_get_pos_of_stg_inst(stg_inst_info)

            if timer_name == "TestCancelAllOrderOfStgInst":
                print(f"Timer {timer_name} was triggered")
                for i in range(0, 1):
                    self.__test_cancel_all_order_of_stg_inst(stg_inst_info)

            if timer_name == "TestSelfTrade":
                print(f"Timer {timer_name} was triggered")
                for i in range(0, 1):
                    self.__test_self_trade(stg_inst_info)

            if timer_name == "TestRealOrderSpot":
                print(f"Timer {timer_name} was triggered")
                for i in range(0, 1):
                    self.__test_real_order_spot(stg_inst_info)

            if timer_name == "TestRealOrderFutures":
                print(f"Timer {timer_name} was triggered")
                for i in range(0, 1):
                    self.__test_real_order_futures(stg_inst_info)

            if timer_name == "TestSimedOrder":
                print(f"Timer {timer_name} was triggered")
                for i in range(0, 1):
                    self.__test_simed_order(stg_inst_info)

            if timer_name == "TestCancelOrder":
                print(f"Timer {timer_name} was triggered")
                for i in range(0, 1):
                    self.stg_eng.cancel_order(8649250402108551045)

            if timer_name == "TestTimerOfFixedTime":
                print(f"{datetime.now()} TestTimerOfFixedTime")
                self.stg_eng.uninstall_stg_inst_timer(
                    stg_inst_info.stg_inst_id, "TestTimerOfIntervalTime"
                )
                return

            if timer_name == "TestTimerOfIntervalTime":
                return

            return

    def __test_algo_order_smart_order(self, stg_inst_info):
        # 获取当前时间
        current_time = datetime.now()

        # fmt: off
        params = {
            #
            # 现货例子
            #
            "trdAcctId"                         : 100000            ,  # 交易账号
            "marketCode"                        : "SZSE"            ,  # 市场
            "symbolCode"                        : "002230"          ,  # 代码
            "side"                              : "Bid"             ,  # 买卖
            "posDirection"                      : "Open"            ,  # 开平
            "orderSize"                         : 1000              ,  # 报单数量
            "initialUrgency"                    : "PriceOfMaker"    ,  # 每个子单初始的执行紧急度
            "tickerOffset"                      : 0                 ,  # 当initialUrgency为PriceOfMaker或者PriceOfTaker的时候，初始报单价格需要偏移的ticker数量
            "closeTDayStg"                      : "RejectCloseTDay" ,  # 如果是平今仓，是否拒绝，对于现货，此字段无效
            "priceRange"                        : [1.00, 5555.0]       # 实时价格在这个区间才报单，前后都是闭区间，无此参数表示无价格限制

            #
            # 合约例子
            #
            # "trdAcctId"                         : 100001            ,  # 交易账号
            # "marketCode"                        : "DCE"             ,  # 市场
            # "symbolCode"                        : "a2309"           ,  # 代码
            # "side"                              : "Bid"             ,  # 买卖
            # "posDirection"                      : "Open"            ,  # 开平
            # "orderSize"                         : 2                 ,  # 报单数量
            # "initialUrgency"                    : "PriceOfMaker"    ,  # 每个子单初始的执行紧急度
            # "tickerOffset"                      : 0                 ,  # 当initialUrgency为PriceOfMaker或者PriceOfTaker的时候，初始报单价格需要偏移的ticker数量
            # "closeTDayStg"                      : "RejectCloseTDay" ,  # 如果是平今仓，是否拒绝，对于现货，此字段无效
            # "priceRange"                        : [0, 10000000]        # 实时价格在这个区间才报单，前后都是闭区间，无此参数表示无价格限制
        }
        # fmt: on
        algoParamsInJsonFmt = json.dumps(params, indent=4)

        ret_of_algo_order = self.stg_eng.algo_order(
            stg_inst_info,
            "SmartOrder",
            "SmartOrderTest",
            3600 * 100,
            algoParamsInJsonFmt,
        )
        status_code = ret_of_algo_order[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            print(f"Create algo order failed. {status_code} - {status_msg}")
            return
        self.algo_order_id = ret_of_algo_order[1]
        print(f"Create algo order {self.algo_order_id}")

    def __test_algo_order_twap(self, stg_inst_info):
        # 获取当前时间
        current_time = datetime.now()

        # 计算下一分钟的第0秒
        next_minute = current_time + timedelta(minutes=1)
        next_minute = next_minute.replace(second=0, microsecond=0)

        # fmt: off
        params = {
            "sys": {
                # 算法单运行中遇到下面的错误尝试重新报单，否则的话就结束，并将状态置为ExecFailed
                "statusCodeGroupOfRetryOrder" : [
                  # 18012  # 拒绝平今
                  # -5001, # SCODE_DB_CAN_NOT_FIND_SYM_CODE
                  # -5003, # SCODE_DB_CAN_NOT_FIND_STG_INST
                  # -5004, # SCODE_DB_CAN_NOT_FIND_ACCT_INFO
                  # -5005, # SCODE_DB_CAN_NOT_FIND_ACCT_ID
                  # -6011, # SCODE_STG_INVALID_SIMED_TD_INFO_SIZE
                  # -6012, # SCODE_STG_INVALID_MARKET_CODE
                  # -6013, # SCODE_STG_INVALID_SYMBOL_TYPE_IN_DB
                  # -6014, # SCODE_STG_INVALID_SIDE
                  # -6016, # SCODE_STG_INVALID_SYMBOL_TYPE
                  # -7001  # SCODE_ORD_MGR_ADD_ORDER_INFO_FAILED
                ],
                # 尝试重新报单的间隔
                "msIntervalOfRetryOrder" : 3000,
                # 预订阅的 topic
                "preSubTopicGroup": [
                  # f"shm://MD.SSE.Spot/603123/DynCandle/tsStart/{int(next_minute.timestamp())}/interval/10",
                  # "shm://MD.SSE.Spot/600600/LastPrice",
                  # "shm://MD.SSE.Spot/600600/Bid1Ask1",
                ]
            },

            #
            # 现货例子
            #
            "trdAcctId"                         : 100000            ,  # 交易账号
            "marketCode"                        : "SZSE"            ,  # 市场
            "symbolCode"                        : "002230"          ,  # 代码
            "side"                              : "Bid"             ,  # 买卖
            "posDirection"                      : "Open"            ,  # 开平
            "totalSize"                         : 10000             ,  # 报单数量
            "splitNum"                          : 33                ,  # 子单数量
            "msIntervalOfSubOrder"              : 5000              ,  # 子单间隔
            "minMSIntervalOfOrderAndCancelOrder": 2000              ,  # 当挂单不在 bid/ask 之间会自动撤单，下单和撤单的最短间隔，避免频繁的报单撤单
            "cancelTimesOfUpgradeUrgency"       : 1                 ,  # 撤单次数超过当前值升级Urgency，如果不需要升级Urgency，那么将其设置为一个较大的值即可
            "initialUrgency"                    : "PriceOfMaker"    ,  # 每个子单初始的执行紧急度
            "tickerOffset"                      : 0                 ,  # 当initialUrgency为PriceOfMaker或者PriceOfTaker的时候，初始报单价格需要偏移的ticker数量
            "closeTDayStg"                      : "RejectCloseTDay" ,  # 如果是平今仓，是否拒绝，对于现货，此字段无效
            "priceRange"                        : [0.01, 10000]        # 实时价格在这个区间才报单，前后都是闭区间，无此参数表示无价格限制

            #
            # 合约例子
            #
            # "trdAcctId"                         : 100001            ,  # 交易账号
            # "marketCode"                        : "DCE"             ,  # 市场
            # "symbolCode"                        : "a2309"           ,  # 代码
            # "side"                              : "Bid"             ,  # 买卖
            # "posDirection"                      : "Open"            ,  # 开平
            # "totalSize"                         : 10                ,  # 报单数量
            # "splitNum"                          : 3                 ,  # 子单数量
            # "msIntervalOfSubOrder"              : 5000              ,  # 子单间隔
            # "minMSIntervalOfOrderAndCancelOrder": 2000              ,  # 当挂单不在 bid/ask 之间会自动撤单，下单和撤单的最短间隔，避免频繁的报单撤单
            # "cancelTimesOfUpgradeUrgency"       : 1                 ,  # 撤单次数超过当前值升级Urgency，如果不需要升级Urgency，那么将其设置为一个较大的值即可
            # "initialUrgency"                    : "PriceOfMaker"    ,  # 每个子单初始的执行紧急度
            # "tickerOffset"                      : 0                 ,  # 当initialUrgency为PriceOfMaker或者PriceOfTaker的时候，初始报单价格需要偏移的ticker数量
            # "closeTDayStg"                      : "RejectCloseTDay" ,  # 如果是平今仓，是否拒绝，对于现货，此字段无效
            # "priceRange"                        : [0, 100000000]       # 实时价格在这个区间才报单，前后都是闭区间，无此参数表示无价格限制
        }
        # fmt: on
        algoParamsInJsonFmt = json.dumps(params, indent=4)

        ret_of_algo_order = self.stg_eng.algo_order(
            stg_inst_info, "TWAP", "TWapTest", 3600 * 100, algoParamsInJsonFmt
        )
        status_code = ret_of_algo_order[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            print(f"Create algo order failed. {status_code} - {status_msg}")
            return
        self.algo_order_id = ret_of_algo_order[1]
        print(f"Create algo order {self.algo_order_id}")

    def __test_cancel_algo_order(self, stg_inst_info):
        self.stg_eng.cancel_algo_order(self.algo_order_id)
        print(f"Cancel algo order {self.algo_order_id}")

    def __test_unclosed_order_info_of_stg_inst(self, stg_inst_info):
        print(f"{datetime.now()} get unclosed order info of stg inst")
        order_info_group = self.stg_eng.get_unclosed_order_info_group(stg_inst_info)
        for i in range(len(order_info_group)):
            print(order_info_group[i].to_short_str())

    def __test_get_pos_of_stg_inst(self, stg_inst_info):
        print(f"{datetime.now()} get pos of stg inst")
        pos = self.stg_eng.get_pos_of_stg_inst(stg_inst_info)
        for i in range(len(pos)):
            print(pos[i].to_str())

    def __test_cancel_all_order_of_stg_inst(self, stg_inst_info):
        print(f"=====> {datetime.now()} cancel all order")
        self.stg_eng.cancel_all_order_of_stg_inst(stg_inst_info)

    def __test_self_trade(self, stg_inst_info):
        ret_of_order = self.stg_eng.order(
            stg_inst_info,
            market_code=MarketCode.CZCE,
            symbol_code="SR2309",
            side=Side.Bid,
            pos_direction=PosDirection.Both,
            order_price=5900,
            order_size=2,
            trd_acct_id=100000,
        )

        status_code = ret_of_order[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            print(f"Create bid order failed. {status_code} - {status_msg}")
            return

        ret_of_order = self.stg_eng.order(
            stg_inst_info,
            market_code=MarketCode.CZCE,
            symbol_code="SR2309",
            side=Side.Ask,
            pos_direction=PosDirection.Both,
            order_price=5900,
            order_size=2,
            trd_acct_id=100000,
        )

        status_code = ret_of_order[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            print(f"Create ask order failed. {status_code} - {status_msg}")
            return

        return

    def __test_real_order_spot(self, stg_inst_info):
        ret_of_order = self.stg_eng.order(
            stg_inst_info,
            market_code=MarketCode.SZSE,
            symbol_code="000002",
            side=Side.Bid,
            pos_direction=PosDirection.Both,
            order_price=15.1,
            order_size=100,
            trd_acct_id=100000,
        )

        status_code = ret_of_order[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            print(f"Create order failed. {status_code} - {status_msg}")
            return

        order_id = ret_of_order[1]
        print(f"=====> {datetime.now()} order {order_id}")

        ret_of_get_order_info = self.stg_eng.get_order_info(order_id)
        status_code = ret_of_get_order_info[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            print(f"Get order info failed. {status_code} - {status_msg}")
            return

        order_info = ret_of_get_order_info[1]
        print(order_info)
        return

    def __test_real_order_futures(self, stg_inst_info):
        return
        ret_of_order = self.stg_eng.order(
            stg_inst_info,
            market_code=MarketCode.CZCE,
            symbol_code="SR2309",
            side=Side.Ask,
            pos_direction=PosDirection.Close,
            order_price=5800,
            order_size=2,
            trd_acct_id=100001,
            close_tday_stg=CloseTDayStg.AllowCloseTDay,
        )

        status_code = ret_of_order[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            print(f"Create order failed. {status_code} - {status_msg}")
            return

        order_id = ret_of_order[1]

        ret_of_get_order_info = self.stg_eng.get_order_info(order_id)
        status_code = ret_of_get_order_info[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            print(f"Get order info failed. {status_code} - {status_msg}")
            return

        order_info = ret_of_get_order_info[1]
        print(order_info)
        return

    def __test_simed_order(self, stg_inst_info):
        td_info = SimedTDInfo()
        td_info.order_status = OrderStatus.PartialFilled
        td_info.trans_detail_group.append(
            TransDetail(slippage=0.1, filled_per=0.1, ld=LiquidityDirection.Maker)
        )
        td_info.trans_detail_group.append(
            TransDetail(slippage=0.1, filled_per=0.1, ld=LiquidityDirection.Maker)
        )

        ret_of_order = self.stg_eng.order(
            stg_inst_info,
            market_code=MarketCode.SZSE,
            symbol_code="000002",
            side=Side.Bid,
            pos_direction=PosDirection.Open,
            order_price=18,
            order_size=100,
            trd_acct_id=100000,
            close_tday_stg=CloseTDayStg.AllowCloseTDay,
            algo_id=0,
            simed_td_info=td_info,
        )

        status_code = ret_of_order[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            print(f"Create order failed. {status_code} - {status_msg}")
            return

        order_id = ret_of_order[1]

        ret_of_get_order_info = self.stg_eng.get_order_info(order_id)
        status_code = ret_of_get_order_info[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            print(f"Get order info failed. {status_code} - {status_msg}")
            return

        order_info = ret_of_get_order_info[1]
        self.stg_eng.cancel_order(order_id)
        return

    def __test_cancel_order(self, stg_inst_info):
        self.stg_eng.cancel_order(order_id)
        return

    def on_stg_manual_intervention(self, stg_inst_info, stg_manual_intervention):
        self.stg_eng.log_info(
            "Recv manual intervention. {} ",
            [stg_manual_intervention],
            stg_inst_info,
            NotifyToTerminal.IsTrue,
        )
        data = json.dumps(stg_manual_intervention)

    def on_push_topic(self, stg_inst_info, topic_data):
        print(f"stg inst {stg_inst_info.stg_inst_id} recv {topic_data}")

    def on_order_ret(self, stg_inst_info, order_info):
        print(f"<===== {datetime.now()} on order ret {order_info.to_short_str()}")
        return

    def on_cancel_order_ret(self, stg_inst_info, order_info):
        print(
            f"<===== {datetime.now()} on cancel order ret {order_info.to_short_str()}"
        )
        return

    def on_algo_order(self, stg_inst_info, algo_progress):
        print(f"{algo_progress}")
        return

    def on_trades(self, stg_inst_info, trades):
        return
        market_data = json.loads(trades)
        print(f"{datetime.now()} on trades of {market_data['mdHeader']['symbolCode']}")

    def on_orders(self, stg_inst_info, orders):
        return
        market_data = json.loads(orders)
        print(f"{datetime.now()} on orders of {market_data['mdHeader']['symbolCode']}")

    def on_books(self, stg_inst_info, books):
        market_data = json.dumps(books)

    def on_tickers(self, stg_inst_info, tickers):
        return
        market_data = json.loads(tickers)
        print(f"{datetime.now()} on tickers of {market_data['mdHeader']['symbolCode']}")

    def on_candle(self, stg_inst_info, candle):
        return
        market_data = json.dumps(candle)

    def on_bid1_ask1(self, stg_inst_info, bid1_ask1):
        return
        market_data = json.loads(bid1_ask1)
        print(
            f"{datetime.now()} on bid1_ask1 of {market_data['mdHeader']['symbolCode']}"
        )

    def on_last_price(self, stg_inst_info, last_price):
        return
        market_data = json.loads(last_price)
        print(
            f"{datetime.now()} on last_price of {market_data['mdHeader']['symbolCode']}"
        )

    def on_dyn_candle(self, stg_inst_info, candle):
        return
        market_data = json.loads(candle)
        print(
            f"{datetime.now()} on dyn candle of {market_data['mdHeader']['symbolCode']}"
        )

    def on_stg_inst_add(self, stg_inst_info):
        # Write strategy business code here
        pass

    def on_stg_inst_del(self, stg_inst_info):
        # Write strategy business code here
        pass

    def on_stg_inst_chg(self, stg_inst_info):
        # Write strategy business code here
        pass

    def on_pos_update_of_acct_id(self, stg_inst_info, pos_snapshot):
        return

    def on_pos_snapshot_of_acct_id(self, stg_inst_info, pos_snapshot):
        return

    def on_pos_update_of_stg_id(self, stg_inst_info, pos_snapshot):
        # query pnl of stgId=10000&stgInstId=1
        return
        if stg_inst_info.stg_inst_id == 1:
            ret_of_query_pnl = pos_snapshot.query_pnl(
                query_cond="stgId=10000&stgInstId=1",
                quote_currency_for_calc="CNY",
                quote_currency_for_conv="CNY",
            )
            status_code = ret_of_query_pnl[0]
            if status_code == 0:
                pnl = ret_of_query_pnl[1]
                print(f"pnl update = {pnl.to_str()}")
            else:
                status_msg = get_status_msg(status_code)
                print(f"{status_code} - {status_msg}")
        return

    def on_pos_snapshot_of_stg_id(self, stg_inst_info, pos_snapshot):
        return
        if stg_inst_info.stg_inst_id == 1:
            # 获取stgId=10000&stgInstId=1的仓位快照
            ret_of_query = pos_snapshot.query_pos_info_group("stgId=10000&stgInstId=1")
            status_code = ret_of_query[0]
            pos_info_group = ret_of_query[1]
            for i in range(len(pos_info_group)):
                print(f"{pos_info_group[i].to_str()}")

            # 按照stgId&stgInstId分组的仓位快照
            ret_of_query = pos_snapshot.query_pos_info_group_by("stgId&stgInstId")
            status_code = ret_of_query[0]
            key_to_pos_info_bundle = ret_of_query[1]
            for rec in key_to_pos_info_bundle:
                print(rec.key())
                pos_info_bundle = rec.data()
                for i in range(len(pos_info_bundle)):
                    print(f"pos info bundle {pos_info_bundle[i].to_str()}")

            # 获取stgId=10000&stgInstId=1的盈亏信息
            ret_of_query_pnl = pos_snapshot.query_pnl(
                query_cond="stgId=10000&stgInstId=1",
                quote_currency_for_calc="CNY",
                quote_currency_for_conv="CNY",
            )
            status_code = ret_of_query_pnl[0]
            if status_code != 0:
                status_msg = get_status_msg(status_code)
                print(f"{status_code} - {status_msg}")
            pnl = ret_of_query_pnl[1]
            print(f"pnl snapshot = {pnl.to_str()}")

            # 按照stgId&stgInstId分组的盈亏信息
            ret_of_query_pnl = pos_snapshot.query_pnl_group_by(
                group_cond="stgId&stgInstId",
                quote_currency_for_calc="CNY",
                quote_currency_for_conv="CNY",
            )
            status_code = ret_of_query_pnl[0]
            if status_code != 0:
                status_msg = get_status_msg(status_code)
                print(f"{status_code} - {status_msg}")
            key_to_pnl_group = ret_of_query_pnl[1]
            for rec in key_to_pnl_group:
                print(rec.key())
                pnl = rec.data()
                print(f"pnl rec {pnl.to_str()}")

        return

    def on_pos_update_of_stg_inst_id(self, stg_inst_info, pos_snapshot):
        # Write strategy business code here
        pass

    def on_pos_snapshot_of_stg_inst_id(self, stg_inst_info, pos_snapshot):
        # Write strategy business code here
        pass

    def on_assets_update(self, stg_inst_info, assets_update):
        return

    def on_assets_snapshot(self, stg_inst_info, assets_snapshot):
        return


def parse_argv(argv):
    conf = ""
    try:
        opts, args = getopt.getopt(argv[1:], "hc:", ["conf="])
    except getopt.GetoptError:
        print(f"{argv[0]} -c <configfile> or {argv[0]} --conf=<configfile>")
        sys.exit(2)
    for opt, arg in opts:
        if opt == "-h":
            print(f"{argv[0]} -c <configfile> or {argv[0]} --conf=<configfile>")
            sys.exit()
        elif opt in ("-c", "--conf"):
            conf = arg
    return conf


def main(argv):
    conf = parse_argv(argv)
    if len(conf) == 0:
        print(f"{argv[0]} -c <configfile> or {argv[0]} --conf=<configfile>")
        sys.exit(3)

    stg_eng = StgEng(conf)
    stg_inst_task_handler = StgInstTaskHandler(stg_eng)
    ret = stg_eng.init(stg_inst_task_handler)
    if ret != 0:
        sys.exit(4)
    stg_eng.run()


if __name__ == "__main__":
    main(sys.argv)
