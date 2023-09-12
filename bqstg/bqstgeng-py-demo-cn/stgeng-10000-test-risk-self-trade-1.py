#!/usr/bin/python3

# -*- coding: utf-8 -*-

"""
功能：测试黑白名单
注意：
原理：
"""

import sys
import getopt
import datetime
import mylogger
import time
import json
from stgeng import *
from datetime import datetime

sys.path.append(".")


class StgInstTaskHandler(StgInstTaskHandlerBase):
    def __init__(self, stg_eng):
        self.stg_eng = stg_eng

        # 先用100001下一个买单挂单，再下一个卖开和卖平全成，但是价格都高于买单，也就是不会触发自成交
        self.order_id11 = 0
        self.order_id12 = 0
        self.order_id13 = 0
        self.status_code11 = -1
        self.status_code12 = -1
        self.status_code13 = -1

        # 先用100001下一个卖单挂单，再下一个买开和买平全成，但是价格都低于卖单，也就是不会触发自成交
        self.order_id21 = 0
        self.order_id22 = 0
        self.order_id23 = 0
        self.status_code21 = -1
        self.status_code22 = -1
        self.status_code23 = -1

        # 先用100001下一个买单挂单，再下一个卖开和卖平全成，但是价格都低于买单，也就是会触发自成交，最后下一个全成的买单
        self.order_id31 = 0
        self.order_id32 = 0
        self.order_id33 = 0
        self.order_id34 = 0
        self.status_code31 = -1
        self.status_code32 = -1
        self.status_code33 = -1
        self.status_code34 = -1

        # 先用100001下一个卖单挂单，再下一个买开和买平全成，但是价格都高于卖单，也就是会触发自成交，最后下一个全成的卖单
        self.order_id41 = 0
        self.order_id42 = 0
        self.order_id43 = 0
        self.order_id44 = 0
        self.status_code41 = -1
        self.status_code42 = -1
        self.status_code43 = -1
        self.status_code44 = -1

        # 先用100002下一个买单挂单，再下一个卖开和卖平全成，但是价格都低于买单，但是因为100002没有设置自成交风控，所以不会触发自成交

        # 先用100002下一个卖单挂单，再下一个买开和买平全成，但是价格都高于卖单，但是因为100002没有设置自成交风控，所以不会触发自成交

    def on_stg_start(self):
        # fmt: off
        ts = 0
        interval = 10

        # 先用100001下一个卖单挂单，再下一个买开和买平全成，但是价格都高于卖单，也就是会触发自成交，最后下一个全成的卖单
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate4", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += interval; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="Test4", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore4", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 先用100001下一个买单挂单，再下一个卖开和卖平全成，但是价格都低于买单，也就是会触发自成交，最后下一个全成的买单
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate3", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += interval; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="Test3", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore3", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 先用100001下一个买单挂单，再下一个卖开和卖平全成，但是价格都高于买单，也就是不会触发自成交
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate1", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += interval; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="Test1", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore1", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 先用100001下一个卖单挂单，再下一个买开和买平全成，但是价格都低于卖单，也就是不会触发自成交
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate2", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += interval; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="Test2", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore2", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 检查最终测试结果
        ts += 3       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="CheckResult", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        # fmt: on

    def on_stg_inst_start(self, stg_inst_info):
        pass

    def on_stg_inst_timer(self, stg_inst_info, timer_name):
        if stg_inst_info.stg_inst_id == 1:
            # 先用100001下一个买单挂单，再下一个卖开和卖平全成，但是价格都高于买单，也就是不会触发自成交
            if timer_name == "DBUpdate1":
                self.DBUpdate1(stg_inst_info)
            if timer_name == "Test1":
                self.Test1(stg_inst_info)
            if timer_name == "DBRestore1":
                self.DBRestore1(stg_inst_info)

            # 先用100001下一个卖单挂单，再下一个买开和买平全成，但是价格都低于卖单，也就是不会触发自成交
            if timer_name == "DBUpdate2":
                self.DBUpdate2(stg_inst_info)
            if timer_name == "Test2":
                self.Test2(stg_inst_info)
            if timer_name == "DBRestore2":
                self.DBRestore2(stg_inst_info)

            # 先用100001下一个买单挂单，再下一个卖开和卖平全成，但是价格都低于买单，也就是会触发自成交，最后下一个全成的买单
            if timer_name == "DBUpdate3":
                self.DBUpdate3(stg_inst_info)
            if timer_name == "Test3":
                self.Test3(stg_inst_info)
            if timer_name == "DBRestore3":
                self.DBRestore3(stg_inst_info)

            # 先用100001下一个卖单挂单，再下一个买开和买平全成，但是价格都高于卖单，也就是会触发自成交，最后下一个全成的卖单
            if timer_name == "DBUpdate4":
                self.DBUpdate4(stg_inst_info)
            if timer_name == "Test4":
                self.Test4(stg_inst_info)
            if timer_name == "DBRestore4":
                self.DBRestore4(stg_inst_info)

            # 检查最终测试结果
            if timer_name == "CheckResult":
                self.CheckResult()

    # 先用100001下一个买单挂单，再下一个卖开和卖平全成，但是价格都高于买单，也就是不会触发自成交
    def DBUpdate1(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST1")
        # 修改限制
        self.stg_eng.sync_exec_sql(
            "INSERT INTO `riskSelfTradeCtrlRange`(`step`, `condition`, `name`) VALUES('acctId', 'acctId=10001', 'acctId=10001');"
        )

    # 先用100001下一个买单挂单，再下一个卖开和卖平全成，但是价格都高于买单，也就是不会触发自成交
    def Test1(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.ConfirmedByExch
        # 第一个是挂单
        ret_of_order11 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.DCE, symbol_code="a2403", side=Side.Bid, pos_direction=PosDirection.Open, order_price=100, order_size=101, trd_acct_id=100001, close_tday_stg=CloseTDayStg.AllowCloseTDay, algo_id=0, simed_td_info=simed_td_info)

        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))
        # 第二第三个是全成的订单
        ret_of_order12 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.DCE, symbol_code="a2403", side=Side.Ask, pos_direction=PosDirection.Open, order_price=101, order_size=101, trd_acct_id=100001, close_tday_stg=CloseTDayStg.AllowCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        ret_of_order13 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.DCE, symbol_code="a2403", side=Side.Ask, pos_direction=PosDirection.Close, order_price=102, order_size=101, trd_acct_id=100001, close_tday_stg=CloseTDayStg.AllowCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        # fmt: on

        self.order_id11 = ret_of_order11[1]
        mylogger.info(f"===> send order {self.order_id11}")
        self.order_id12 = ret_of_order12[1]
        mylogger.info(f"===> send order {self.order_id12}")
        self.order_id13 = ret_of_order13[1]
        mylogger.info(f"===> send order {self.order_id13}")

        # 撤销订单，避免对后续测试产生影响
        self.stg_eng.cancel_order(self.order_id11)
        return

    # 先用100001下一个买单挂单，再下一个卖开和卖平全成，但是价格都高于买单，也就是不会触发自成交
    def DBRestore1(self, stg_inst_info):
        self.stg_eng.sync_exec_sql(
            "DELETE FROM `riskSelfTradeCtrlRange` WHERE step = 'acctId' AND `condition` = 'acctId=10001' AND `name` = 'acctId=10001';"
        )

    # 先用100001下一个卖单挂单，再下一个买开和买平全成，但是价格都低于卖单，也就是不会触发自成交
    def DBUpdate2(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST2")
        # 修改限制
        self.stg_eng.sync_exec_sql(
            "INSERT INTO `riskSelfTradeCtrlRange`(`step`, `condition`, `name`) VALUES('acctId', 'acctId=10001', 'acctId=10001');"
        )

    # 先用100001下一个卖单挂单，再下一个买开和买平全成，但是价格都低于卖单，也就是不会触发自成交
    def Test2(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.ConfirmedByExch
        # 第一个是挂单
        ret_of_order21 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.DCE, symbol_code="a2403", side=Side.Ask, pos_direction=PosDirection.Open, order_price=102, order_size=101, trd_acct_id=100001, close_tday_stg=CloseTDayStg.AllowCloseTDay, algo_id=0, simed_td_info=simed_td_info)

        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))
        # 第二第三个是全成的订单
        ret_of_order22 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.DCE, symbol_code="a2403", side=Side.Bid, pos_direction=PosDirection.Open, order_price=101, order_size=101, trd_acct_id=100001, close_tday_stg=CloseTDayStg.AllowCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        ret_of_order23 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.DCE, symbol_code="a2403", side=Side.Bid, pos_direction=PosDirection.Close, order_price=100, order_size=101, trd_acct_id=100001, close_tday_stg=CloseTDayStg.AllowCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        # fmt: on

        self.order_id21 = ret_of_order21[1]
        mylogger.info(f"===> send order {self.order_id21}")
        self.order_id22 = ret_of_order22[1]
        mylogger.info(f"===> send order {self.order_id22}")
        self.order_id23 = ret_of_order23[1]
        mylogger.info(f"===> send order {self.order_id23}")

        # 撤销订单，避免对后续测试产生影响
        self.stg_eng.cancel_order(self.order_id21)
        return

    # 先用100001下一个卖单挂单，再下一个买开和买平全成，但是价格都低于卖单，也就是不会触发自成交
    def DBRestore2(self, stg_inst_info):
        self.stg_eng.sync_exec_sql(
            "DELETE FROM `riskSelfTradeCtrlRange` WHERE step = 'acctId' AND `condition` = 'acctId=10001' AND `name` = 'acctId=10001';"
        )

    # 先用100001下一个买单挂单，再下一个卖开和卖平全成，但是价格都高于买单，也就是不会触发自成交
    def DBUpdate3(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST3")
        # 修改限制
        self.stg_eng.sync_exec_sql(
            "INSERT INTO `riskSelfTradeCtrlRange`(`step`, `condition`, `name`) VALUES('acctId', 'acctId=10001', 'acctId=10001');"
        )

    # 先用100001下一个买单挂单，再下一个卖开和卖平全成，但是价格都高于买单，也就是不会触发自成交
    def Test3(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.ConfirmedByExch
        # 第一个是挂单
        ret_of_order31 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.DCE, symbol_code="a2403", side=Side.Bid, pos_direction=PosDirection.Open, order_price=100, order_size=101, trd_acct_id=100001, close_tday_stg=CloseTDayStg.AllowCloseTDay, algo_id=0, simed_td_info=simed_td_info)

        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))
        # 第二第三个是全成的订单
        ret_of_order32 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.DCE, symbol_code="a2403", side=Side.Ask, pos_direction=PosDirection.Open, order_price=100, order_size=101, trd_acct_id=100001, close_tday_stg=CloseTDayStg.AllowCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        ret_of_order33 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.DCE, symbol_code="a2403", side=Side.Ask, pos_direction=PosDirection.Close, order_price=99, order_size=101, trd_acct_id=100001, close_tday_stg=CloseTDayStg.AllowCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        ret_of_order34 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.DCE, symbol_code="a2403", side=Side.Bid, pos_direction=PosDirection.Open, order_price=99, order_size=101, trd_acct_id=100001, close_tday_stg=CloseTDayStg.AllowCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        # fmt: on

        self.order_id31 = ret_of_order31[1]
        mylogger.info(f"===> send order {self.order_id31}")
        self.order_id32 = ret_of_order32[1]
        mylogger.info(f"===> send order {self.order_id32}")
        self.order_id33 = ret_of_order33[1]
        mylogger.info(f"===> send order {self.order_id33}")
        self.order_id34 = ret_of_order34[1]
        mylogger.info(f"===> send order {self.order_id34}")

        # 撤销订单，避免对后续测试产生影响
        self.stg_eng.cancel_order(self.order_id31)
        return

    # 先用100001下一个买单挂单，再下一个卖开和卖平全成，但是价格都高于买单，也就是不会触发自成交
    def DBRestore3(self, stg_inst_info):
        self.stg_eng.sync_exec_sql(
            "DELETE FROM `riskSelfTradeCtrlRange` WHERE step = 'acctId' AND `condition` = 'acctId=10001' AND `name` = 'acctId=10001';"
        )

    # 先用100001下一个卖单挂单，再下一个买开和买平全成，但是价格都高于卖单，也就是会触发自成交，最后下一个全成的卖单
    def DBUpdate4(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST4")
        # 修改限制
        self.stg_eng.sync_exec_sql(
            "INSERT INTO `riskSelfTradeCtrlRange`(`step`, `condition`, `name`) VALUES('acctId', 'acctId=10001', 'acctId=10001');"
        )

    # 先用100001下一个卖单挂单，再下一个买开和买平全成，但是价格都高于卖单，也就是会触发自成交，最后下一个全成的卖单
    def Test4(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.ConfirmedByExch
        # 第一个是挂单
        ret_of_order41 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.DCE, symbol_code="a2403", side=Side.Ask, pos_direction=PosDirection.Open, order_price=100, order_size=101, trd_acct_id=100001, close_tday_stg=CloseTDayStg.AllowCloseTDay, algo_id=0, simed_td_info=simed_td_info)

        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))
        # 第二第三个是全成的订单
        ret_of_order42 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.DCE, symbol_code="a2403", side=Side.Bid, pos_direction=PosDirection.Open, order_price=100, order_size=101, trd_acct_id=100001, close_tday_stg=CloseTDayStg.AllowCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        ret_of_order43 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.DCE, symbol_code="a2403", side=Side.Bid, pos_direction=PosDirection.Close, order_price=101, order_size=101, trd_acct_id=100001, close_tday_stg=CloseTDayStg.AllowCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        ret_of_order44 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.DCE, symbol_code="a2403", side=Side.Ask, pos_direction=PosDirection.Close, order_price=101, order_size=101, trd_acct_id=100001, close_tday_stg=CloseTDayStg.AllowCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        # fmt: on

        self.order_id41 = ret_of_order41[1]
        mylogger.info(f"===> send order {self.order_id41}")
        self.order_id42 = ret_of_order42[1]
        mylogger.info(f"===> send order {self.order_id42}")
        self.order_id43 = ret_of_order43[1]
        mylogger.info(f"===> send order {self.order_id43}")
        self.order_id44 = ret_of_order44[1]
        mylogger.info(f"===> send order {self.order_id44}")

        # 撤销订单，避免对后续测试产生影响
        self.stg_eng.cancel_order(self.order_id41)
        return

    # 先用100001下一个卖单挂单，再下一个买开和买平全成，但是价格都高于卖单，也就是会触发自成交，最后下一个全成的卖单
    def DBRestore4(self, stg_inst_info):
        self.stg_eng.sync_exec_sql(
            "DELETE FROM `riskSelfTradeCtrlRange` WHERE step = 'acctId' AND `condition` = 'acctId=10001' AND `name` = 'acctId=10001';"
        )

    def on_order_ret(self, stg_inst_info, order_info):
        mylogger.info(f"<===  on order ret {order_info.to_short_str()}")

        # 先用100001下一个买单挂单，再下一个卖开和卖平全成，但是价格都高于买单，也就是不会触发自成交
        if order_info.order_id == self.order_id11:
            self.status_code11 = order_info.status_code
            if order_info.status_code != 0:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        if order_info.order_id == self.order_id12:
            self.status_code12 = order_info.status_code
            if order_info.status_code != 0:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        if order_info.order_id == self.order_id13:
            self.status_code13 = order_info.status_code
            if order_info.status_code != 0:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        # 先用100001下一个卖单挂单，再下一个买开和买平全成，但是价格都低于卖单，也就是不会触发自成交
        if order_info.order_id == self.order_id21:
            self.status_code21 = order_info.status_code
            if order_info.status_code != 0:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        if order_info.order_id == self.order_id22:
            self.status_code22 = order_info.status_code
            if order_info.status_code != 0:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        if order_info.order_id == self.order_id23:
            self.status_code23 = order_info.status_code
            if order_info.status_code != 0:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        # 先用100001下一个买单挂单，再下一个卖开和卖平全成，但是价格都低于买单，也就是会触发自成交，最后下一个全成的买单
        if order_info.order_id == self.order_id31:
            self.status_code31 = order_info.status_code
            if order_info.status_code != 0:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        if order_info.order_id == self.order_id32:
            self.status_code32 = order_info.status_code
            if order_info.status_code != 18022:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        if order_info.order_id == self.order_id33:
            self.status_code33 = order_info.status_code
            if order_info.status_code != 18022:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        if order_info.order_id == self.order_id34:
            self.status_code34 = order_info.status_code
            if order_info.status_code != 0:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        # 先用100001下一个卖单挂单，再下一个买开和买平全成，但是价格都高于卖单，也就是会触发自成交，最后下一个全成的卖单
        if order_info.order_id == self.order_id41:
            self.status_code41 = order_info.status_code
            if order_info.status_code != 0:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        if order_info.order_id == self.order_id42:
            self.status_code42 = order_info.status_code
            if order_info.status_code != 18021:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        if order_info.order_id == self.order_id43:
            self.status_code43 = order_info.status_code
            if order_info.status_code != 18021:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        if order_info.order_id == self.order_id44:
            self.status_code44 = order_info.status_code
            if order_info.status_code != 0:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

    # 检查最终测试结果
    def CheckResult(self):
        if (
            self.status_code11 == 0
            and self.status_code12 == 0
            and self.status_code13 == 0
        ):
            mylogger.info(f"Test1 success.")
        else:
            mylogger.error(
                f"Test1 failed. [statusCode = {self.status_code11} {self.status_code12} {self.status_code13} ]"
            )
        if (
            self.status_code21 == 0
            and self.status_code22 == 0
            and self.status_code23 == 0
        ):
            mylogger.info(f"Test2 success.")
        else:
            mylogger.error(
                f"Test2 failed. [statusCode = {self.status_code21} {self.status_code22} {self.status_code23} ]"
            )
        if (
            self.status_code31 == 0
            and self.status_code32 == 18022
            and self.status_code33 == 18022
            and self.status_code34 == 0
        ):
            mylogger.info(f"Test3 success.")
        else:
            mylogger.error(
                f"Test3 failed. [statusCode = {self.status_code31} {self.status_code32} {self.status_code33} {self.status_code34} ]"
            )
        if (
            self.status_code41 == 0
            and self.status_code42 == 18021
            and self.status_code43 == 18021
            and self.status_code44 == 0
        ):
            mylogger.info(f"Test4 success.")
        else:
            mylogger.error(
                f"Test4 failed. [statusCode = {self.status_code41} {self.status_code42} {self.status_code43} {self.status_code44} ]"
            )

    def on_stg_manual_intervention(self, stg_inst_info, stg_manual_intervention):
        mylogger.info(f"recv manual intervention {stg_manual_intervention}")
        data = json.dumps(stg_manual_intervention)

    def on_push_topic(self, stg_inst_info, topic_data):
        mylogger.info(f"stg inst {stg_inst_info.stg_inst_id} recv {topic_data}")

    def on_cancel_order_ret(self, stg_inst_info, order_info):
        mylogger.info(f"<===  on cancel order ret {order_info.to_short_str()}")

    def on_trades(self, stg_inst_info, trades):
        return
        market_data = json.loads(trades)
        mylogger.info(f"on trades of {market_data['mdHeader']['symbolCode']}")

    def on_orders(self, stg_inst_info, orders):
        return
        market_data = json.loads(orders)
        mylogger.info(f"on orders of {market_data['mdHeader']['symbolCode']}")

    def on_books(self, stg_inst_info, books):
        return
        market_data = json.dumps(books)
        mylogger.info(f"on books of {market_data['mdHeader']['symbolCode']}")

    def on_tickers(self, stg_inst_info, tickers):
        return
        market_data = json.loads(tickers)
        mylogger.info(f"on tickers of {market_data['mdHeader']['symbolCode']}")

    def on_candle(self, stg_inst_info, candle):
        return
        market_data = json.dumps(candle)
        mylogger.info(f"on candle of {market_data['mdHeader']['symbolCode']}")

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
        pass

    def on_pos_snapshot_of_acct_id(self, stg_inst_info, pos_snapshot):
        pass

    def on_pos_update_of_stg_id(self, stg_inst_info, pos_snapshot):
        pass

    def on_pos_snapshot_of_stg_id(self, stg_inst_info, pos_snapshot):
        pass

    def on_pos_update_of_stg_inst_id(self, stg_inst_info, pos_snapshot):
        pass

    def on_pos_snapshot_of_stg_inst_id(self, stg_inst_info, pos_snapshot):
        pass

    def on_assets_update(self, stg_inst_info, assets_update):
        pass

    def on_assets_snapshot(self, stg_inst_info, assets_snapshot):
        pass


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
