#!/usr/bin/python3

# -*- coding: utf-8 -*-

"""
功能：测试风控
注意：考虑到tdSrv重新加载风控配置需要时间，interval值设为10，如果tdSrv重新加载配置的时间改动的话,interval也要做相应修改
原理：修改流控配置limitValue，触发风控，测试完毕恢复limitValue
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

        # 测试单笔下单数量
        self.order_id1 = 0
        self.status_code1 = -1

        # 测试下单总数
        self.order_id2 = 0
        self.status_code2 = -1

        # 测试单笔下单金额
        self.order_id3 = 0
        self.status_code3 = -1

        # 测试累计下单金额
        self.order_id4 = 0
        self.status_code4 = -1

        # 测试累计下单次数
        self.order_id7 = 0
        self.status_code7 = -1

        # 测试单位时间下单次数
        self.order_id81 = 0
        self.order_id82 = 0
        self.status_code8 = -1

        # 测试单位时间撤单次数
        self.order_id91 = 0
        self.order_id92 = 0
        self.status_code9 = -1

        # 测试累计撤单次数
        self.order_id10 = 0
        self.status_code10 = -1

        # 测试单位时间拒单次数
        self.order_id111 = 0
        self.order_id112 = 0
        self.order_id113 = 0
        self.status_code11 = -1

        # 测试累计拒单次数
        self.order_id12 = 0
        self.status_code12 = -1

        # 测试累计持仓数量
        self.order_id131 = 0
        self.order_id132 = 0
        self.status_code13 = -1

        # 测试累计持仓金额
        self.order_id141 = 0
        self.order_id142 = 0
        self.status_code14 = -1

        # 测试当日开仓(单独品种)
        self.order_id151 = 0
        self.order_id152 = 0
        self.status_code15 = -1

        # 测试当日开仓(IC系列)
        self.order_id161 = 0
        self.order_id162 = 0
        self.status_code16 = -1

    def on_stg_start(self):
        # fmt: off
        ts = 0
        interval = 10

        # 测试当日开仓(IC系列)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate16", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += interval; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="TestOrder16", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore16", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 测试当日开仓(单独品种)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate15", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += interval; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="TestOrder15", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore15", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 测试单笔下单数量
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate1", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += interval; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="TestOrder1", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore1", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 测试下单总数
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate2", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += interval; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="TestOrder2", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore2", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 测试单笔下单金额
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate3", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += interval; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="TestOrder3", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore3", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 测试累计下单金额
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate4", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += interval; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="TestOrder4", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore4", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 测试累计下单次数
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate7", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += interval; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="TestOrder7", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore7", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 测试单位时间下单次数
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate8", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += interval; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="TestOrder8", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore8", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 测试单位时间撤单次数
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate9", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += interval; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="TestOrder9", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore9", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 测试累计撤单次数
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate10", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += interval; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="TestOrder10", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore10", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 测试单位时间拒单次数 连续下两个拒单，再下一个正常单的测试有问题，因为拒单的tsQue队列还没形成，所以不会触发风控
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate11", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += interval; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="TestOrder11", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="TestOrderForTrigger11", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore11", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 测试累计拒单次数
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate12", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += interval; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="TestOrder12", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore12", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 测试累计持仓数量
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate13", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += interval; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="TestOrder13", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore13", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 测试累计持仓金额
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate14", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += interval; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="TestOrder14", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        ts += 1       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore14", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 检查最终测试结果
        ts += 3       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="CheckResult", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        # fmt: on

    def on_stg_inst_start(self, stg_inst_info):
        pass

    def on_stg_inst_timer(self, stg_inst_info, timer_name):
        if stg_inst_info.stg_inst_id == 1:
            # 测试单笔下单数量
            if timer_name == "DBUpdate1":
                self.DBUpdate1(stg_inst_info)
            if timer_name == "TestOrder1":
                self.TestOrder1(stg_inst_info)
            if timer_name == "DBRestore1":
                self.DBRestore1(stg_inst_info)

            # 测试下单总数
            if timer_name == "DBUpdate2":
                self.DBUpdate2(stg_inst_info)
            if timer_name == "TestOrder2":
                self.TestOrder2(stg_inst_info)
            if timer_name == "DBRestore2":
                self.DBRestore2(stg_inst_info)

            # 测试单笔下单金额
            if timer_name == "DBUpdate3":
                self.DBUpdate3(stg_inst_info)
            if timer_name == "TestOrder3":
                self.TestOrder3(stg_inst_info)
            if timer_name == "DBRestore3":
                self.DBRestore3(stg_inst_info)

            # 测试累计下单金额
            if timer_name == "DBUpdate4":
                self.DBUpdate4(stg_inst_info)
            if timer_name == "TestOrder4":
                self.TestOrder4(stg_inst_info)
            if timer_name == "DBRestore4":
                self.DBRestore4(stg_inst_info)

            # 测试累计下单次数
            if timer_name == "DBUpdate7":
                self.DBUpdate7(stg_inst_info)
            if timer_name == "TestOrder7":
                self.TestOrder7(stg_inst_info)
            if timer_name == "DBRestore7":
                self.DBRestore7(stg_inst_info)

            # 测试单位时间下单次数
            if timer_name == "DBUpdate8":
                self.DBUpdate8(stg_inst_info)
            if timer_name == "TestOrder8":
                self.TestOrder8(stg_inst_info)
            if timer_name == "DBRestore8":
                self.DBRestore8(stg_inst_info)

            # 测试单位时间撤单次数
            if timer_name == "DBUpdate9":
                self.DBUpdate9(stg_inst_info)
            if timer_name == "TestOrder9":
                self.TestOrder9(stg_inst_info)
            if timer_name == "DBRestore9":
                self.DBRestore9(stg_inst_info)

            # 测试单位时间撤单次数
            if timer_name == "DBUpdate10":
                self.DBUpdate10(stg_inst_info)
            if timer_name == "TestOrder10":
                self.TestOrder10(stg_inst_info)
            if timer_name == "DBRestore10":
                self.DBRestore10(stg_inst_info)

            # 测试单位时间拒单次数
            if timer_name == "DBUpdate11":
                self.DBUpdate11(stg_inst_info)
            if timer_name == "TestOrder11":
                self.TestOrder11(stg_inst_info)
            if timer_name == "TestOrderForTrigger11":
                self.TestOrderForTrigger11(stg_inst_info)
            if timer_name == "DBRestore11":
                self.DBRestore11(stg_inst_info)

            # 测试累计拒单次数
            if timer_name == "DBUpdate12":
                self.DBUpdate12(stg_inst_info)
            if timer_name == "TestOrder12":
                self.TestOrder12(stg_inst_info)
            if timer_name == "DBRestore12":
                self.DBRestore12(stg_inst_info)

            # 测试累计持仓数量
            if timer_name == "DBUpdate13":
                self.DBUpdate13(stg_inst_info)
            if timer_name == "TestOrder13":
                self.TestOrder13(stg_inst_info)
            if timer_name == "DBRestore13":
                self.DBRestore13(stg_inst_info)

            # 测试累计持仓金额
            if timer_name == "DBUpdate14":
                self.DBUpdate14(stg_inst_info)
            if timer_name == "TestOrder14":
                self.TestOrder14(stg_inst_info)
            if timer_name == "DBRestore14":
                self.DBRestore14(stg_inst_info)

            # 测试当日开仓(单独品种)
            if timer_name == "DBUpdate15":
                self.DBUpdate15(stg_inst_info)
            if timer_name == "TestOrder15":
                self.TestOrder15(stg_inst_info)
            if timer_name == "DBRestore15":
                self.DBRestore15(stg_inst_info)

            # 测试当日开仓(IC系列)
            if timer_name == "DBUpdate16":
                self.DBUpdate16(stg_inst_info)
            if timer_name == "TestOrder16":
                self.TestOrder16(stg_inst_info)
            if timer_name == "DBRestore16":
                self.DBRestore16(stg_inst_info)

            # 检查最终测试结果
            if timer_name == "CheckResult":
                self.CheckResult()

    # 测试单笔下单数量
    def DBUpdate1(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST 10001")
        # 修改限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = 100 where no = 10001;"
        )
        ret = self.stg_eng.sync_exec_sql(
            "SELECT no, limitValue from riskFlowCtrlRule where no = 10001;"
        )
        mylogger.info(f"Update limit value {ret[1]}")

    # 测试单笔下单数量
    def TestOrder1(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))
        # fmt: on

        ret_of_order = self.stg_eng.order(
            stg_inst_info,
            market_code=MarketCode.SSE,
            symbol_code="600600",
            side=Side.Bid,
            pos_direction=PosDirection.Both,
            order_price=100,
            order_size=101,
            trd_acct_id=100000,
            close_tday_stg=CloseTDayStg.RejectCloseTDay,
            algo_id=0,
            simed_td_info=simed_td_info,
        )

        status_code = ret_of_order[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id1 = ret_of_order[1]
        mylogger.info(f"===> send order {self.order_id1}")

        return

    # 测试单笔下单数量
    def DBRestore1(self, stg_inst_info):
        # 恢复限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = 10000000000 where no = 10001;"
        )

    # 测试下单总数
    def DBUpdate2(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST 10002")
        # 修改限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = 100 where no = 10002;"
        )
        ret = self.stg_eng.sync_exec_sql(
            "SELECT no, limitValue from riskFlowCtrlRule where no = 10002;"
        )
        mylogger.info(f"Update limit value {ret[1]}")

    # 测试下单总数
    def TestOrder2(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))
        # fmt: on

        ret_of_order = self.stg_eng.order(
            stg_inst_info,
            market_code=MarketCode.SSE,
            symbol_code="600600",
            side=Side.Bid,
            pos_direction=PosDirection.Both,
            order_price=100,
            order_size=101,
            trd_acct_id=100000,
            close_tday_stg=CloseTDayStg.RejectCloseTDay,
            algo_id=0,
            simed_td_info=simed_td_info,
        )

        status_code = ret_of_order[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id2 = ret_of_order[1]
        mylogger.info(f"===> send order {self.order_id2}")

    # 测试下单总数
    def DBRestore2(self, stg_inst_info):
        # 恢复限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = 10000000000 where no = 10002;"
        )

    # 测试单笔下单金额
    def DBUpdate3(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST 10003")
        # 修改限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = 10000 where no = 10003;"
        )
        ret = self.stg_eng.sync_exec_sql(
            "SELECT no, limitValue from riskFlowCtrlRule where no = 10003;"
        )
        mylogger.info(f"Update limit value {ret[1]}")

    # 测试单笔下单金额
    def TestOrder3(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))
        # fmt: on

        ret_of_order = self.stg_eng.order(
            stg_inst_info,
            market_code=MarketCode.SSE,
            symbol_code="600600",
            side=Side.Bid,
            pos_direction=PosDirection.Both,
            order_price=100,
            order_size=101,
            trd_acct_id=100000,
            close_tday_stg=CloseTDayStg.RejectCloseTDay,
            algo_id=0,
            simed_td_info=simed_td_info,
        )

        status_code = ret_of_order[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id3 = ret_of_order[1]
        mylogger.info(f"===> send order {self.order_id3}")

    # 测试单笔下单金额
    def DBRestore3(self, stg_inst_info):
        # 恢复限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = 10000000000 where no = 10003;"
        )

    # 测试累计下单金额
    def DBUpdate4(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST 10004")
        # 修改限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = 10000 where no = 10004;"
        )
        ret = self.stg_eng.sync_exec_sql(
            "SELECT no, limitValue from riskFlowCtrlRule where no = 10004;"
        )
        mylogger.info(f"Update limit value {ret[1]}")

    # 测试累计下单金额
    def TestOrder4(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))
        # fmt: on

        ret_of_order = self.stg_eng.order(
            stg_inst_info,
            market_code=MarketCode.SSE,
            symbol_code="600600",
            side=Side.Bid,
            pos_direction=PosDirection.Both,
            order_price=100,
            order_size=101,
            trd_acct_id=100000,
            close_tday_stg=CloseTDayStg.RejectCloseTDay,
            algo_id=0,
            simed_td_info=simed_td_info,
        )

        status_code = ret_of_order[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id4 = ret_of_order[1]
        mylogger.info(f"===> send order {self.order_id4}")

    # 测试累计下单金额
    def DBRestore4(self, stg_inst_info):
        # 恢复限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = 10000000000 where no = 10004;"
        )

    # 测试累计下单次数
    def DBUpdate7(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST 10007")
        # 修改限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = 1 where no = 10007;"
        )
        ret = self.stg_eng.sync_exec_sql(
            "SELECT no, limitValue from riskFlowCtrlRule where no = 10007;"
        )
        mylogger.info(f"Update limit value {ret[1]}")

    # 测试累计下单次数
    def TestOrder7(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))

        ret_of_order = self.stg_eng.order( stg_inst_info, market_code=MarketCode.SSE, symbol_code="600600", side=Side.Bid, pos_direction=PosDirection.Both, order_price=100, order_size=101, trd_acct_id=100000, close_tday_stg=CloseTDayStg.RejectCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        ret_of_order = self.stg_eng.order( stg_inst_info, market_code=MarketCode.SSE, symbol_code="600600", side=Side.Bid, pos_direction=PosDirection.Both, order_price=100, order_size=101, trd_acct_id=100000, close_tday_stg=CloseTDayStg.RejectCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        # fmt: on

        status_code = ret_of_order[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id7 = ret_of_order[1]
        mylogger.info(f"===> send order {self.order_id7}")

    # 测试累计下单次数
    def DBRestore7(self, stg_inst_info):
        # 恢复限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = 10000000000 where no = 10007;"
        )

    # 测试单位时间下单次数
    def DBUpdate8(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST 10008")
        # 修改限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = '1/1000ms' where no = 10008;"
        )
        ret = self.stg_eng.sync_exec_sql(
            "SELECT no, limitValue from riskFlowCtrlRule where no = 10008;"
        )
        mylogger.info(f"Update limit value {ret[1]}")

    # 测试单位时间下单次数
    def TestOrder8(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))

        ret_of_order81 = self.stg_eng.order( stg_inst_info, market_code=MarketCode.SSE, symbol_code="600600", side=Side.Bid, pos_direction=PosDirection.Both, order_price=100, order_size=101, trd_acct_id=100000, close_tday_stg=CloseTDayStg.RejectCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        ret_of_order82 = self.stg_eng.order( stg_inst_info, market_code=MarketCode.SSE, symbol_code="600600", side=Side.Bid, pos_direction=PosDirection.Both, order_price=100, order_size=101, trd_acct_id=100000, close_tday_stg=CloseTDayStg.RejectCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        # fmt: on

        status_code = ret_of_order81[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id81 = ret_of_order81[1]
        mylogger.info(f"===> send order {self.order_id81}")

        status_code = ret_of_order82[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id82 = ret_of_order82[1]
        mylogger.info(f"===> send order {self.order_id82}")

    # 测试单位时间下单次数
    def DBRestore8(self, stg_inst_info):
        # 恢复限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = '1000/10ms'where no = 10008;"
        )

    # 测试单位时间撤单次数
    def DBUpdate9(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST 10009")
        # 修改限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = '1/1000ms' where no = 10009;"
        )
        ret = self.stg_eng.sync_exec_sql(
            "SELECT no, limitValue from riskFlowCtrlRule where no = 10009;"
        )
        mylogger.info(f"Update limit value {ret[1]}")

    # 测试单位时间撤单次数
    def TestOrder9(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.ConfirmedByExch

        ret_of_order91 = self.stg_eng.order( stg_inst_info, market_code=MarketCode.SSE, symbol_code="600600", side=Side.Bid, pos_direction=PosDirection.Both, order_price=100, order_size=101, trd_acct_id=100000, close_tday_stg=CloseTDayStg.RejectCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        ret_of_order92 = self.stg_eng.order( stg_inst_info, market_code=MarketCode.SSE, symbol_code="600600", side=Side.Bid, pos_direction=PosDirection.Both, order_price=100, order_size=101, trd_acct_id=100000, close_tday_stg=CloseTDayStg.RejectCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        # fmt: on

        status_code = ret_of_order91[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id91 = ret_of_order91[1]
        mylogger.info(f"===> send order {self.order_id91}")

        status_code = ret_of_order92[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id92 = ret_of_order92[1]
        mylogger.info(f"===> send order {self.order_id92}")
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")

        self.stg_eng.cancel_order(self.order_id91)
        self.stg_eng.cancel_order(self.order_id92)

    # 测试单位时间撤单次数
    def DBRestore9(self, stg_inst_info):
        # 恢复限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = '1000/10ms'where no = 10009;"
        )

    # 测试累计撤单次数
    def DBUpdate10(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST 10010")
        # 修改限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = 1 where no = 10010;"
        )
        ret = self.stg_eng.sync_exec_sql(
            "SELECT no, limitValue from riskFlowCtrlRule where no = 10010;"
        )
        mylogger.info(f"Update limit value {ret[1]}")

    # 测试累计撤单次数
    def TestOrder10(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.ConfirmedByExch

        ret_of_order10 = self.stg_eng.order( stg_inst_info, market_code=MarketCode.SSE, symbol_code="600600", side=Side.Bid, pos_direction=PosDirection.Both, order_price=100, order_size=101, trd_acct_id=100000, close_tday_stg=CloseTDayStg.RejectCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        # fmt: on

        status_code = ret_of_order10[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id10 = ret_of_order10[1]
        mylogger.info(f"===> send order {self.order_id10}")

        self.stg_eng.cancel_order(self.order_id10)

    # 测试累计撤单次数
    def DBRestore10(self, stg_inst_info):
        # 恢复限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = 10000000000 where no = 10010;"
        )

    # 测试单位时间拒单次数
    def DBUpdate11(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST 10011")
        # 修改限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = '1/10000ms' where no = 10011;"
        )
        ret = self.stg_eng.sync_exec_sql(
            "SELECT no, limitValue from riskFlowCtrlRule where no = 10011;"
        )
        mylogger.info(f"Update limit value {ret[1]}")

    # 测试单位时间拒单次数
    def TestOrder11(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Failed

        # 下两个拒单，1秒内产生2个拒单
        ret_of_order111 = self.stg_eng.order( stg_inst_info, market_code=MarketCode.SSE, symbol_code="600600", side=Side.Bid, pos_direction=PosDirection.Both, order_price=100, order_size=101, trd_acct_id=100000, close_tday_stg=CloseTDayStg.RejectCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        ret_of_order112 = self.stg_eng.order( stg_inst_info, market_code=MarketCode.SSE, symbol_code="600600", side=Side.Bid, pos_direction=PosDirection.Both, order_price=100, order_size=101, trd_acct_id=100000, close_tday_stg=CloseTDayStg.RejectCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        # fmt: on

        status_code = ret_of_order111[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id111 = ret_of_order111[1]
        mylogger.info(f"===> send order {self.order_id111}")

        status_code = ret_of_order112[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id112 = ret_of_order112[1]
        mylogger.info(f"===> send order {self.order_id112}")

    # 测试单位时间拒单次数
    def TestOrderForTrigger11(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))
        # 下一个正常的单子，因为前面产生2个超流控拒单，所以这个单子会触发风控
        ret_of_order113 = self.stg_eng.order( stg_inst_info, market_code=MarketCode.SSE, symbol_code="600600", side=Side.Bid, pos_direction=PosDirection.Both, order_price=100, order_size=101, trd_acct_id=100000, close_tday_stg=CloseTDayStg.RejectCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        # fmt: on

        status_code = ret_of_order113[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id113 = ret_of_order113[1]
        mylogger.info(f"===> send order {self.order_id113}")

    # 测试单位时间拒单次数
    def DBRestore11(self, stg_inst_info):
        # 恢复限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = '1000/10ms' where no = 10011;"
        )

    # 测试累计拒单次数
    def DBUpdate12(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST 10012")
        # 修改限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = 1 where no = 10012;"
        )
        ret = self.stg_eng.sync_exec_sql(
            "SELECT no, limitValue from riskFlowCtrlRule where no = 10012;"
        )
        mylogger.info(f"Update limit value {ret[1]}")

    # 测试累计拒单次数
    def TestOrder12(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))
        # fmt: on

        ret_of_order = self.stg_eng.order(
            stg_inst_info,
            market_code=MarketCode.SSE,
            symbol_code="600600",
            side=Side.Bid,
            pos_direction=PosDirection.Both,
            order_price=100,
            order_size=101,
            trd_acct_id=100000,
            close_tday_stg=CloseTDayStg.RejectCloseTDay,
            algo_id=0,
            simed_td_info=simed_td_info,
        )

        status_code = ret_of_order[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id12 = ret_of_order[1]
        mylogger.info(f"===> send order {self.order_id12}")

    # 测试累计拒单次数
    def DBRestore12(self, stg_inst_info):
        # 恢复限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = 10000000000 where no = 10012;"
        )

    # 测试累计持仓数量
    def DBUpdate13(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST 10013")
        # 修改限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = 1 where no = 10013;"
        )
        ret = self.stg_eng.sync_exec_sql(
            "SELECT no, limitValue from riskFlowCtrlRule where no = 10013;"
        )
        mylogger.info(f"Update limit value {ret[1]}")

    # 测试累计持仓数量
    def TestOrder13(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))

        ret_of_order131 = self.stg_eng.order( stg_inst_info, market_code=MarketCode.SSE, symbol_code="600600", side=Side.Bid, pos_direction=PosDirection.Both, order_price=100, order_size=101, trd_acct_id=100000, close_tday_stg=CloseTDayStg.RejectCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        ret_of_order132 = self.stg_eng.order( stg_inst_info, market_code=MarketCode.SSE, symbol_code="600600", side=Side.Bid, pos_direction=PosDirection.Both, order_price=100, order_size=101, trd_acct_id=100000, close_tday_stg=CloseTDayStg.RejectCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        # fmt: on

        status_code = ret_of_order131[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id131 = ret_of_order131[1]
        mylogger.info(f"===> send order {self.order_id131}")

        status_code = ret_of_order132[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id132 = ret_of_order132[1]
        mylogger.info(f"===> send order {self.order_id132}")

    # 测试累计持仓数量
    def DBRestore13(self, stg_inst_info):
        # 恢复限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = 10000000000 where no = 10013;"
        )

    # 测试累计持仓金额
    def DBUpdate14(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST 10014")
        # 修改限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = 1 where no = 10014;"
        )
        ret = self.stg_eng.sync_exec_sql(
            "SELECT no, limitValue from riskFlowCtrlRule where no = 10014;"
        )
        mylogger.info(f"Update limit value {ret[1]}")

    # 测试累计持仓金额
    def TestOrder14(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))
        # fmt: on

        ret_of_order141 = self.stg_eng.order(
            stg_inst_info,
            market_code=MarketCode.SSE,
            symbol_code="600600",
            side=Side.Bid,
            pos_direction=PosDirection.Both,
            order_price=100,
            order_size=101,
            trd_acct_id=100000,
            close_tday_stg=CloseTDayStg.RejectCloseTDay,
            algo_id=0,
            simed_td_info=simed_td_info,
        )
        ret_of_order142 = self.stg_eng.order(
            stg_inst_info,
            market_code=MarketCode.SSE,
            symbol_code="600600",
            side=Side.Bid,
            pos_direction=PosDirection.Both,
            order_price=100,
            order_size=101,
            trd_acct_id=100000,
            close_tday_stg=CloseTDayStg.RejectCloseTDay,
            algo_id=0,
            simed_td_info=simed_td_info,
        )

        status_code = ret_of_order141[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id141 = ret_of_order141[1]
        mylogger.info(f"===> send order {self.order_id141}")

        status_code = ret_of_order142[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id142 = ret_of_order142[1]
        mylogger.info(f"===> send order {self.order_id142}")

    # 测试累计持仓金额
    def DBRestore14(self, stg_inst_info):
        # 恢复限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = 10000000000 where no = 10014;"
        )

    # 测试当日开仓(单独品种)
    def DBUpdate15(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST 10015")
        # 修改限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = 1 where no = 10015;"
        )
        ret = self.stg_eng.sync_exec_sql(
            "SELECT no, limitValue from riskFlowCtrlRule where no = 10015;"
        )
        mylogger.info(f"Update limit value {ret[1]}")

    # 测试当日开仓(单独品种)
    def TestOrder15(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))
        # fmt: on

        ret_of_order151 = self.stg_eng.order(
            stg_inst_info,
            market_code=MarketCode.CFFEX,
            symbol_code="IF2312",
            side=Side.Ask,
            pos_direction=PosDirection.Open,
            order_price=100,
            order_size=1,
            trd_acct_id=100001,
            close_tday_stg=CloseTDayStg.RejectCloseTDay,
            algo_id=0,
            simed_td_info=simed_td_info,
        )
        ret_of_order152 = self.stg_eng.order(
            stg_inst_info,
            market_code=MarketCode.CFFEX,
            symbol_code="IF2312",
            side=Side.Ask,
            pos_direction=PosDirection.Open,
            order_price=100,
            order_size=1,
            trd_acct_id=100001,
            close_tday_stg=CloseTDayStg.RejectCloseTDay,
            algo_id=0,
            simed_td_info=simed_td_info,
        )

        status_code = ret_of_order151[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id151 = ret_of_order151[1]
        mylogger.info(f"===> send order {self.order_id151}")

        status_code = ret_of_order152[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id152 = ret_of_order152[1]
        mylogger.info(f"===> send order {self.order_id152}")

    # 测试当日开仓(单独品种)
    def DBRestore15(self, stg_inst_info):
        # 恢复限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = 10000000000 where no = 10015;"
        )

    # 测试当日开仓(IC系列)
    def DBUpdate16(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST 10016")
        # 修改限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = 1 where no = 10016;"
        )
        ret = self.stg_eng.sync_exec_sql(
            "SELECT no, limitValue from riskFlowCtrlRule where no = 10016;"
        )
        mylogger.info(f"Update limit value {ret[1]}")

    # 测试当日开仓(IC系列)
    def TestOrder16(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))
        # fmt: on

        ret_of_order161 = self.stg_eng.order(
            stg_inst_info,
            market_code=MarketCode.CFFEX,
            symbol_code="IC2312",
            side=Side.Ask,
            pos_direction=PosDirection.Open,
            order_price=100,
            order_size=1,
            trd_acct_id=100001,
            close_tday_stg=CloseTDayStg.RejectCloseTDay,
            algo_id=0,
            simed_td_info=simed_td_info,
        )
        ret_of_order162 = self.stg_eng.order(
            stg_inst_info,
            market_code=MarketCode.CFFEX,
            symbol_code="IC2309",
            side=Side.Ask,
            pos_direction=PosDirection.Open,
            order_price=100,
            order_size=1,
            trd_acct_id=100001,
            close_tday_stg=CloseTDayStg.RejectCloseTDay,
            algo_id=0,
            simed_td_info=simed_td_info,
        )

        status_code = ret_of_order161[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id161 = ret_of_order161[1]
        mylogger.info(f"===> send order {self.order_id161}")

        status_code = ret_of_order162[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id162 = ret_of_order162[1]
        mylogger.info(f"===> send order {self.order_id162}")

    # 测试当日开仓(IC系列)
    def DBRestore16(self, stg_inst_info):
        # 恢复限制
        self.stg_eng.sync_exec_sql(
            "UPDATE BetterQuant.riskFlowCtrlRule SET limitValue = 10000000000 where no = 10016;"
        )

    # 检查最终测试结果
    def CheckResult(self):
        if self.status_code1 == 10001:
            mylogger.info(f"Test 10001 success.")
        else:
            mylogger.error(f"Test 10001 failed. [statusCode = {self.status_code1}]")

        if self.status_code2 == 10002:
            mylogger.info(f"Test 10002 success.")
        else:
            mylogger.error(f"Test 10002 failed. [statusCode = {self.status_code2}]")

        if self.status_code3 == 10003:
            mylogger.info(f"Test 10003 success.")
        else:
            mylogger.error(f"Test 10003 failed. [statusCode = {self.status_code3}]")

        if self.status_code4 == 10004:
            mylogger.info(f"Test 10004 success.")
        else:
            mylogger.error(f"Test 10004 failed. [statusCode = {self.status_code4}]")

        if self.status_code7 == 10007:
            mylogger.info(f"Test 10007 success.")
        else:
            mylogger.error(f"Test 10007 failed. [statusCode = {self.status_code7}]")

        if self.status_code8 == 10008:
            mylogger.info(f"Test 10008 success.")
        else:
            mylogger.error(f"Test 10008 failed. [statusCode = {self.status_code8}]")

        if self.status_code9 == 10009:
            mylogger.info(f"Test 10009 success.")
        else:
            mylogger.error(f"Test 10009 failed. [statusCode = {self.status_code9}]")

        if self.status_code10 == 10010:
            mylogger.info(f"Test 10010 success.")
        else:
            mylogger.error(f"Test 10010 failed. [statusCode = {self.status_code10}]")

        if self.status_code11 == 10011:
            mylogger.info(f"Test 10011 success.")
        else:
            mylogger.error(f"Test 10011 failed. [statusCode = {self.status_code11}]")

        if self.status_code12 == 10012:
            mylogger.info(f"Test 10012 success.")
        else:
            mylogger.error(f"Test 10012 failed. [statusCode = {self.status_code12}]")

        if self.status_code13 == 10013:
            mylogger.info(f"Test 10013 success.")
        else:
            mylogger.error(f"Test 10013 failed. [statusCode = {self.status_code13}]")

        if self.status_code14 == 10014:
            mylogger.info(f"Test 10014 success.")
        else:
            mylogger.error(f"Test 10014 failed. [statusCode = {self.status_code14}]")

        if self.status_code15 == 10015:
            mylogger.info(f"Test 10015 success.")
        else:
            mylogger.error(f"Test 10015 failed. [statusCode = {self.status_code15}]")

        if self.status_code16 == 10016:
            mylogger.info(f"Test 10016 success.")
        else:
            mylogger.error(f"Test 10016 failed. [statusCode = {self.status_code16}]")

    def on_stg_manual_intervention(self, stg_inst_info, stg_manual_intervention):
        mylogger.info(f"recv manual intervention {stg_manual_intervention}")
        data = json.dumps(stg_manual_intervention)

    def on_push_topic(self, stg_inst_info, topic_data):
        mylogger.info(f"stg inst {stg_inst_info.stg_inst_id} recv {topic_data}")

    def on_order_ret(self, stg_inst_info, order_info):
        mylogger.info(f"<===  on order ret {order_info.to_short_str()}")

        # 测试单笔下单数量
        if order_info.order_id == self.order_id1:
            self.status_code1 = order_info.status_code
            if order_info.status_code != 10001:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        # 测试下单总数
        if order_info.order_id == self.order_id2:
            self.status_code2 = order_info.status_code
            if order_info.status_code != 10002:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        # 测试单笔下单金额
        if order_info.order_id == self.order_id3:
            self.status_code3 = order_info.status_code
            if order_info.status_code != 10003:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        # 测试累计下单金额
        if order_info.order_id == self.order_id4:
            self.status_code4 = order_info.status_code
            if order_info.status_code != 10004:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        # 测试累计下单次数
        if order_info.order_id == self.order_id7:
            self.status_code7 = order_info.status_code
            if order_info.status_code != 10007:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        # 测试单位时间下单次数
        if (
            order_info.order_id == self.order_id81
            and order_info.order_status == OrderStatus.Filled
        ):
            if order_info.status_code != 0:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        if order_info.order_id == self.order_id82:
            self.status_code8 = order_info.status_code
            if order_info.status_code != 10008:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        # 测试单位时间拒单次数
        if order_info.order_id == self.order_id113:
            self.status_code11 = order_info.status_code
            if order_info.status_code != 0 and order_info.status_code != 10011:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        # 测试累计拒单次数
        if order_info.order_id == self.order_id12:
            self.status_code12 = order_info.status_code
            if order_info.status_code != 10012:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        # 测试累计持仓数量
        if order_info.order_id == self.order_id132:
            self.status_code13 = order_info.status_code
            if order_info.status_code != 10013:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        # 测试累计持仓金额
        if order_info.order_id == self.order_id142:
            self.status_code14 = order_info.status_code
            if order_info.status_code != 10014:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        # 测试当日开仓(单独品种)
        if order_info.order_id == self.order_id152:
            self.status_code15 = order_info.status_code
            if order_info.status_code != 10015:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        # 测试当日开仓(单独品种)
        if order_info.order_id == self.order_id162:
            self.status_code16 = order_info.status_code
            if order_info.status_code != 10016:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

    def on_cancel_order_ret(self, stg_inst_info, order_info):
        mylogger.info(f"<===  on cancel order ret {order_info.to_short_str()}")

        # 测试单位时间撤单次数
        if (
            order_info.order_id == self.order_id91
            and order_info.order_status == OrderStatus.Canceled
        ):
            pass

        if order_info.order_id == self.order_id92:
            self.status_code9 = order_info.status_code
            if order_info.status_code != 0 and order_info.status_code != 10009:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

        # 测试累计撤单次数
        if order_info.order_id == self.order_id10:
            self.status_code10 = order_info.status_code
            if order_info.status_code != 0 and order_info.status_code != 10010:
                mylogger.error(f"invalid status code of order {order_info.status_code}")

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
