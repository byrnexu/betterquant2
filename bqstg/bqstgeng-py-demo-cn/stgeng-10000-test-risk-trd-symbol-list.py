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

        # 测试黑名单和白名单都为空
        self.order_id1 = 0
        self.status_code1 = -1

        # 测试一个白名单，然后对白名单里的600600和不在白名单里的603123下单
        self.order_id21 = 0
        self.order_id22 = 0
        self.status_code21 = -1
        self.status_code22 = -1

        # 测试两个白名单，然后对白名单里的600600和不在白名单里的603123下单
        self.order_id31 = 0
        self.order_id32 = 0
        self.status_code31 = -1
        self.status_code32 = -1

        # 测试两个白名单一个黑名单，600600和000002在白名单里，600519在黑名单里，分别对600600、000002、600519、603123下单
        self.order_id41 = 0
        self.order_id42 = 0
        self.order_id43 = 0
        self.order_id44 = 0
        self.status_code41 = -1
        self.status_code42 = -1
        self.status_code43 = -1
        self.status_code44 = -1

        # 测试一个黑名单
        self.order_id51 = 0
        self.order_id52 = 0
        self.status_code51 = -1
        self.status_code52 = -1

    def on_stg_start(self):
        # fmt: off
        ts = 0
        interval = 10

        # 测试一个黑名单
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate5", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += interval; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="Test5", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore5", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 测试两个白名单一个黑名单，600600和000002在白名单里，600519在黑名单里，分别对600600、000002、600519、603123下单
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate4", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += interval; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="Test4", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore4", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 测试两个白名单，然后对白名单里的600600和不在白名单里的603123下单
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate3", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += interval; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="Test3", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore3", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 测试一个白名单，然后对白名单里的600600和不在白名单里的603123下单
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate2", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += interval; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="Test2", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore2", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 测试黑名单和白名单都为空
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate1", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += interval; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="Test1", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore1", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 检查最终测试结果
        ts += 3       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="CheckResult", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        # fmt: on

    def on_stg_inst_start(self, stg_inst_info):
        pass

    def on_stg_inst_timer(self, stg_inst_info, timer_name):
        if stg_inst_info.stg_inst_id == 1:
            # 测试黑名单和白名单都为空
            if timer_name == "DBUpdate1":
                self.DBUpdate1(stg_inst_info)
            if timer_name == "Test1":
                self.Test1(stg_inst_info)
            if timer_name == "DBRestore1":
                self.DBRestore1(stg_inst_info)

            # 测试一个白名单，然后对白名单里的600600和不在白名单里的603123下单
            if timer_name == "DBUpdate2":
                self.DBUpdate2(stg_inst_info)
            if timer_name == "Test2":
                self.Test2(stg_inst_info)
            if timer_name == "DBRestore2":
                self.DBRestore2(stg_inst_info)

            # 测试两个白名单，然后对白名单里的600600和不在白名单里的603123下单
            if timer_name == "DBUpdate3":
                self.DBUpdate3(stg_inst_info)
            if timer_name == "Test3":
                self.Test3(stg_inst_info)
            if timer_name == "DBRestore3":
                self.DBRestore3(stg_inst_info)

            # 测试两个白名单一个黑名单，600600和000002在白名单里，600519在黑名单里，分别对600600、000002、600519、603123下单
            if timer_name == "DBUpdate4":
                self.DBUpdate4(stg_inst_info)
            if timer_name == "Test4":
                self.Test4(stg_inst_info)
            if timer_name == "DBRestore4":
                self.DBRestore3(stg_inst_info)

            # 测试一个黑名单
            if timer_name == "DBUpdate5":
                self.DBUpdate5(stg_inst_info)
            if timer_name == "Test5":
                self.Test5(stg_inst_info)
            if timer_name == "DBRestore5":
                self.DBRestore3(stg_inst_info)

            # 检查最终测试结果
            if timer_name == "CheckResult":
                self.CheckResult()

    # 测试黑名单和白名单都为空
    def DBUpdate1(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST1")
        # 修改限制
        self.stg_eng.sync_exec_sql("DELETE FROM riskTrdSymbolList;")

    # 测试黑名单和白名单都为空
    def Test1(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))

        ret_of_order = self.stg_eng.order(stg_inst_info, market_code=MarketCode.SSE, symbol_code="600600", side=Side.Bid, pos_direction=PosDirection.Both, order_price=100, order_size=101, trd_acct_id=100000, close_tday_stg=CloseTDayStg.RejectCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        # fmt: on

        status_code = ret_of_order[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id1 = ret_of_order[1]
        mylogger.info(f"===> send order {self.order_id1}")

        return

    # 测试黑名单和白名单都为空
    def DBRestore1(self, stg_inst_info):
        pass

    # 测试一个白名单，然后对白名单里的600600和不在白名单里的603123下单
    def DBUpdate2(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST2")
        # 修改限制
        self.stg_eng.sync_exec_sql(
            "INSERT INTO `riskTrdSymbolList`(`step`, `condition`, `marketCode`, `symbolType`, `symbolCode`, `trdListType`, `name`) VALUES('acctId-trdAcctId', 'acctId=10000&trdAcctId=100000', 'SSE', 'CN_MainBoard', '600600', 'White', 'acctId=10000&trdAcctId=100000');"
        )

    # 测试一个白名单，然后对白名单里的600600和不在白名单里的603123下单
    def Test2(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))

        ret_of_order21 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.SSE, symbol_code="600600", side=Side.Bid, pos_direction=PosDirection.Both, order_price=100, order_size=101, trd_acct_id=100000, close_tday_stg=CloseTDayStg.RejectCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        ret_of_order22 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.SSE, symbol_code="603123", side=Side.Bid, pos_direction=PosDirection.Both, order_price=100, order_size=101, trd_acct_id=100000, close_tday_stg=CloseTDayStg.RejectCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        # fmt: on

        status_code = ret_of_order21[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id21 = ret_of_order21[1]
        mylogger.info(f"===> send order {self.order_id21}")

        status_code = ret_of_order22[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id22 = ret_of_order22[1]
        mylogger.info(f"===> send order {self.order_id22}")

        return

    # 测试一个白名单，然后对白名单里的600600和不在白名单里的603123下单
    def DBRestore2(self, stg_inst_info):
        self.stg_eng.sync_exec_sql("DELETE FROM riskTrdSymbolList;")

    # 测试两个白名单，然后对白名单里的600600和不在白名单里的603123下单
    def DBUpdate3(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST3")
        # 修改限制
        self.stg_eng.sync_exec_sql(
            "INSERT INTO `riskTrdSymbolList`(`step`, `condition`, `marketCode`, `symbolType`, `symbolCode`, `trdListType`, `name`) VALUES('acctId-trdAcctId', 'acctId=10000&trdAcctId=100000', 'SSE', 'CN_MainBoard', '600600', 'White', 'acctId=10000&trdAcctId=100000');"
        )
        self.stg_eng.sync_exec_sql(
            "INSERT INTO `riskTrdSymbolList`(`step`, `condition`, `marketCode`, `symbolType`, `symbolCode`, `trdListType`, `name`) VALUES('acctId-trdAcctId', 'acctId=10000&trdAcctId=100000&marketCode=SZSE', 'SZSE', 'CN_MainBoard', '000002', 'White', 'acctId=10000&trdAcctId=100000');"
        )

    # 测试两个白名单，然后对白名单里的600600和不在白名单里的603123下单
    def Test3(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))

        ret_of_order31 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.SSE, symbol_code="600600", side=Side.Bid, pos_direction=PosDirection.Both, order_price=100, order_size=101, trd_acct_id=100000, close_tday_stg=CloseTDayStg.RejectCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        ret_of_order32 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.SSE, symbol_code="603123", side=Side.Bid, pos_direction=PosDirection.Both, order_price=100, order_size=101, trd_acct_id=100000, close_tday_stg=CloseTDayStg.RejectCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        # fmt: on

        status_code = ret_of_order31[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id31 = ret_of_order31[1]
        mylogger.info(f"===> send order {self.order_id31}")

        status_code = ret_of_order32[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id32 = ret_of_order32[1]
        mylogger.info(f"===> send order {self.order_id32}")

        return

    # 测试两个白名单，然后对白名单里的600600和不在白名单里的603123下单
    def DBRestore3(self, stg_inst_info):
        self.stg_eng.sync_exec_sql("DELETE FROM riskTrdSymbolList;")

    # 测试两个白名单一个黑名单，600600和000002在白名单里，600519在黑名单里，分别对600600、000002、600519、603123下单
    def DBUpdate4(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST4")
        # 修改限制
        self.stg_eng.sync_exec_sql(
            "INSERT INTO `riskTrdSymbolList`(`step`, `condition`, `marketCode`, `symbolType`, `symbolCode`, `trdListType`, `name`) VALUES('acctId-trdAcctId', 'acctId=10000&trdAcctId=100000', 'SSE', 'CN_MainBoard', '600600', 'White', 'acctId=10000&trdAcctId=100000'); "
        )
        self.stg_eng.sync_exec_sql(
            "INSERT INTO `riskTrdSymbolList`(`step`, `condition`, `marketCode`, `symbolType`, `symbolCode`, `trdListType`, `name`) VALUES('acctId-trdAcctId', 'acctId=10000&trdAcctId=100000&marketCode=SZSE', 'SZSE', 'CN_MainBoard', '000002', 'White', 'acctId=10000&trdAcctId=100000');"
        )
        self.stg_eng.sync_exec_sql(
            "INSERT INTO `riskTrdSymbolList`(`step`, `condition`, `marketCode`, `symbolType`, `symbolCode`, `trdListType`, `name`) VALUES('acctId-trdAcctId', 'acctId=10000&trdAcctId=100000', 'SSE', 'CN_MainBoard', '600519', 'Black', 'acctId=10000&trdAcctId=100000');"
        )

    # 测试两个白名单一个黑名单，600600和000002在白名单里，600519在黑名单里，分别对600600、000002、600519、603123下单
    def Test4(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))

        ret_of_order41 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.SSE , symbol_code="600600", side=Side.Bid, pos_direction=PosDirection.Both, order_price=100, order_size=101, trd_acct_id=100000, close_tday_stg=CloseTDayStg.RejectCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        ret_of_order42 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.SZSE, symbol_code="000002", side=Side.Bid, pos_direction=PosDirection.Both, order_price=100, order_size=101, trd_acct_id=100000, close_tday_stg=CloseTDayStg.RejectCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        ret_of_order43 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.SSE , symbol_code="600519", side=Side.Bid, pos_direction=PosDirection.Both, order_price=100, order_size=101, trd_acct_id=100000, close_tday_stg=CloseTDayStg.RejectCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        ret_of_order44 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.SSE , symbol_code="603123", side=Side.Bid, pos_direction=PosDirection.Both, order_price=100, order_size=101, trd_acct_id=100000, close_tday_stg=CloseTDayStg.RejectCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        # fmt: on

        self.order_id41 = ret_of_order41[1]
        mylogger.info(f"===> send order {self.order_id41}")
        self.order_id42 = ret_of_order42[1]
        mylogger.info(f"===> send order {self.order_id42}")
        self.order_id43 = ret_of_order43[1]
        mylogger.info(f"===> send order {self.order_id43}")
        self.order_id44 = ret_of_order44[1]
        mylogger.info(f"===> send order {self.order_id44}")

        return

    # 测试两个白名单一个黑名单，600600和000002在白名单里，600519在黑名单里，分别对600600、000002、600519、603123下单
    def DBRestore4(self, stg_inst_info):
        self.stg_eng.sync_exec_sql("DELETE FROM riskTrdSymbolList;")

    # 测试一个黑名单
    def DBUpdate5(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST5")
        # 修改限制
        self.stg_eng.sync_exec_sql(
            "INSERT INTO `riskTrdSymbolList`(`step`, `condition`, `marketCode`, `symbolType`, `symbolCode`, `trdListType`, `name`) VALUES('acctId-trdAcctId', 'acctId=10000&trdAcctId=100000', 'SSE', 'CN_MainBoard', '600519', 'Black', 'acctId=10000&trdAcctId=100000'); "
        )

    # 测试一个黑名单
    def Test5(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))

        ret_of_order51 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.SSE, symbol_code="600600", side=Side.Bid, pos_direction=PosDirection.Both, order_price=100, order_size=101, trd_acct_id=100000, close_tday_stg=CloseTDayStg.RejectCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        ret_of_order52 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.SSE, symbol_code="600519", side=Side.Bid, pos_direction=PosDirection.Both, order_price=100, order_size=101, trd_acct_id=100000, close_tday_stg=CloseTDayStg.RejectCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        # fmt: on

        status_code = ret_of_order51[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id51 = ret_of_order51[1]
        mylogger.info(f"===> send order {self.order_id51}")

        status_code = ret_of_order52[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id52 = ret_of_order52[1]
        mylogger.info(f"===> send order {self.order_id52}")

        return

    # 测试一个黑名单
    def DBRestore5(self, stg_inst_info):
        self.stg_eng.sync_exec_sql("DELETE FROM riskTrdSymbolList;")

    def on_order_ret(self, stg_inst_info, order_info):
        mylogger.info(f"<===  on order ret {order_info.to_short_str()}")

        # 测试黑名单和白名单都为空
        if (
            order_info.order_id == self.order_id1
            and order_info.order_status == OrderStatus.Filled
        ):
            self.status_code1 = order_info.status_code
            if order_info.status_code != 0:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        # 测试一个白名单，然后对白名单里的600600和不在白名单里的603123下单
        if (
            order_info.order_id == self.order_id21
            and order_info.order_status == OrderStatus.Filled
        ):
            self.status_code21 = order_info.status_code
            if order_info.status_code != 0:
                mylogger.error(f"invalid status code of order {order_info.status_code}")
        if (
            order_info.order_id == self.order_id22
            and order_info.order_status == OrderStatus.Failed
        ):
            self.status_code22 = order_info.status_code
            if order_info.status_code != 18031:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        # 测试两个白名单，然后对白名单里的600600和不在白名单里的603123下单
        if (
            order_info.order_id == self.order_id31
            and order_info.order_status == OrderStatus.Filled
        ):
            self.status_code31 = order_info.status_code
            if order_info.status_code != 0:
                mylogger.error(f"invalid status code of order {order_info.status_code}")
        if (
            order_info.order_id == self.order_id32
            and order_info.order_status == OrderStatus.Failed
        ):
            self.status_code32 = order_info.status_code
            if order_info.status_code != 18031:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        # 测试两个白名单一个黑名单，600600和000002在白名单里，600519在黑名单里，分别对600600、000002、600519、603123下单
        if (
            order_info.order_id == self.order_id41
            and order_info.order_status == OrderStatus.Filled
        ):
            self.status_code41 = order_info.status_code
            if order_info.status_code != 0:
                mylogger.error(f"invalid status code of order {order_info.status_code}")
        if (
            order_info.order_id == self.order_id42
            and order_info.order_status == OrderStatus.Filled
        ):
            self.status_code42 = order_info.status_code
            if order_info.status_code != 0:
                mylogger.error(f"invalid status code of order {order_info.status_code}")
        if (
            order_info.order_id == self.order_id43
            and order_info.order_status == OrderStatus.Failed
        ):
            self.status_code43 = order_info.status_code
            if order_info.status_code != 18032:
                mylogger.error(f"invalid status code of order {order_info.status_code}")
        if (
            order_info.order_id == self.order_id44
            and order_info.order_status == OrderStatus.Failed
        ):
            self.status_code44 = order_info.status_code
            if order_info.status_code != 18031:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        # 测试一个黑名单
        if (
            order_info.order_id == self.order_id51
            and order_info.order_status == OrderStatus.Filled
        ):
            self.status_code51 = order_info.status_code
            if order_info.status_code != 0:
                mylogger.error(f"invalid status code of order {order_info.status_code}")
        if (
            order_info.order_id == self.order_id52
            and order_info.order_status == OrderStatus.Failed
        ):
            self.status_code52 = order_info.status_code
            if order_info.status_code != 18032:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

    # 检查最终测试结果
    def CheckResult(self):
        if self.status_code1 == 0:
            mylogger.info(f"Test1 success.")
        else:
            mylogger.error(f"Test1 failed. [statusCode = {self.status_code1}]")

        if self.status_code21 == 0 and self.status_code22 == 18031:
            mylogger.info(f"Test2 success.")
        else:
            mylogger.error(
                f"Test2 failed. [statusCode21 = {self.status_code21}] [statusCode22 = {self.status_code22}]"
            )

        if self.status_code31 == 0 and self.status_code32 == 18031:
            mylogger.info(f"Test3 success.")
        else:
            mylogger.error(
                f"Test3 failed. [statusCode31 = {self.status_code31}] [statusCode33 = {self.status_code32}]"
            )

        if (
            self.status_code41 == 0
            and self.status_code42 == 0
            and self.status_code43 == 18032
            and self.status_code44 == 18031
        ):
            mylogger.info(f"Test4 success.")
        else:
            mylogger.error(
                f"Test4 failed. [statusCode41 = {self.status_code41}] [statusCode43 = {self.status_code42}]"
            )

        if self.status_code51 == 0 and self.status_code52 == 18032:
            mylogger.info(f"Test5 success.")
        else:
            mylogger.error(
                f"Test5 failed. [statusCode51 = {self.status_code51}] [statusCode52 = {self.status_code52}]"
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
