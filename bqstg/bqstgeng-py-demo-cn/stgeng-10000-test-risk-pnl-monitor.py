#!/usr/bin/python3

# -*- coding: utf-8 -*-

"""
功能：测试pnl监控
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

        # pnl 监控测试，制造 601398 亏损超过 1000
        self.order_id11 = 0
        self.order_id12 = 0
        self.status_code11 = -1
        self.status_code12 = -1

        # 卖出，再买入，卖出应该是正常的买入会被 pnl 风控阻挡
        self.order_id21 = 0
        self.order_id22 = 0
        self.order_id23 = 0
        self.status_code21 = -1
        self.status_code22 = -1
        self.status_code23 = -1

    def on_stg_start(self):
        # fmt: off
        ts = 0

        ts += 1 ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBUpdate1", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # pnl 监控测试，制造 601398 亏损超过 1000
        ts += 10; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="Test1", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 卖出，再买入，卖出应该是正常的买入会被 pnl 风控阻挡
        ts += 3 ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="Test2", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        ts += 1 ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="DBRestore1", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 检查最终测试结果
        ts += 3       ; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="CheckResult", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)
        # fmt: on

    def on_stg_inst_start(self, stg_inst_info):
        pass

    def on_stg_inst_timer(self, stg_inst_info, timer_name):
        if stg_inst_info.stg_inst_id == 1:
            if timer_name == "DBUpdate1":
                self.DBUpdate1(stg_inst_info)

            # pnl 监控测试，制造 601398 亏损超过 1000
            if timer_name == "Test1":
                self.Test1(stg_inst_info)

            # 卖出，再买入，卖出应该是正常的买入会被 pnl 风控阻挡
            if timer_name == "Test2":
                self.Test2(stg_inst_info)

            if timer_name == "DBRestore1":
                self.DBRestore1(stg_inst_info)

            # 检查最终测试结果
            if timer_name == "CheckResult":
                self.CheckResult()

    def DBUpdate1(self, stg_inst_info):
        mylogger.debug("===================== BEGIN TO TEST1")
        # 修改限制
        self.stg_eng.sync_exec_sql("DELETE FROM `riskPnlMonitorRange`; ")
        self.stg_eng.sync_exec_sql(
            "INSERT INTO `riskPnlMonitorRange`(`step`, `condition`, `pnlType`, `limitValue`, `name`) VALUE('acctId', 'acctId=10000&marketCode=SSE&symbolCode=601398', 'Loss', 1000, 'acctId=10000&marketCode=SSE&symbolCode=601398'); "
        )
        self.stg_eng.sync_exec_sql(
            "INSERT INTO `riskPnlMonitorRange`(`step`, `condition`, `pnlType`, `limitValue`, `name`) VALUE('acctId', 'acctId=10000&marketCode=SSE&symbolCode=601288', 'Profit', 1000, 'acctId=10000&marketCode=SSE&symbolCode=601288'); "
        )

    # pnl 监控测试，制造 601398 亏损超过 1000
    def Test1(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))

        # 制造亏损
        ret_of_order11 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.SSE, symbol_code="601398", side=Side.Bid, pos_direction=PosDirection.Open , order_price=110, order_size=200, trd_acct_id=100000, close_tday_stg=CloseTDayStg.AllowCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        ret_of_order12 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.SSE, symbol_code="601398", side=Side.Ask, pos_direction=PosDirection.Close, order_price=100, order_size=200, trd_acct_id=100000, close_tday_stg=CloseTDayStg.AllowCloseTDay, algo_id=0, simed_td_info=simed_td_info)

        # fmt: on

        self.order_id11 = ret_of_order11[1]
        mylogger.info(f"===> send order {self.order_id11}")
        self.order_id12 = ret_of_order12[1]
        mylogger.info(f"===> send order {self.order_id12}")

        return

    # 卖出，再买入，卖出应该是正常的买入会被 pnl 风控阻挡
    def Test2(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))

        ret_of_order21 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.SSE, symbol_code="601398", side=Side.Ask, pos_direction=PosDirection.Close, order_price=100.1, order_size=200, trd_acct_id=100000, close_tday_stg=CloseTDayStg.AllowCloseTDay, algo_id=0, simed_td_info=simed_td_info)

        # 这个订单买入会被阻挡
        ret_of_order22 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.SSE, symbol_code="601398", side=Side.Bid, pos_direction=PosDirection.Open , order_price=100.2, order_size=200, trd_acct_id=100000, close_tday_stg=CloseTDayStg.AllowCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        # 这个订单买入不会被阻挡，因为该代码没有被pnl监控
        ret_of_order23 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.SSE, symbol_code="600600", side=Side.Bid, pos_direction=PosDirection.Open , order_price=100.2, order_size=200, trd_acct_id=100000, close_tday_stg=CloseTDayStg.AllowCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        # fmt: on

        self.order_id21 = ret_of_order21[1]
        mylogger.info(f"===> send order {self.order_id21}")
        self.order_id22 = ret_of_order22[1]
        mylogger.info(f"===> send order {self.order_id22}")
        self.order_id23 = ret_of_order23[1]
        mylogger.info(f"===> send order {self.order_id23}")

        return

    def DBRestore1(self, stg_inst_info):
        self.stg_eng.sync_exec_sql("DELETE FROM `riskPnlMonitorRange`; ")

    def on_order_ret(self, stg_inst_info, order_info):
        mylogger.info(f"<===  on order ret {order_info.to_short_str()}")

        # pnl 监控测试，制造 601398 亏损超过 1000
        if order_info.order_id == self.order_id11:
            self.status_code11 = order_info.status_code

        if order_info.order_id == self.order_id12:
            self.status_code12 = order_info.status_code

        if order_info.order_id == self.order_id21:
            self.status_code21 = order_info.status_code

        if order_info.order_id == self.order_id22:
            self.status_code22 = order_info.status_code
            if (
                order_info.status_code != 18041
                and order_info.status_code != 18042
                and order_info.status_code != 18043
            ):
                mylogger.error(
                    f"invalid status code of order {order_info.order_id} {order_info.side} {order_info.symbol_code} {order_info.status_code}"
                )

        if order_info.order_id == self.order_id23:
            self.status_code23 = order_info.status_code
            if order_info.status_code != 0:
                mylogger.error(
                    f"invalid status code of order {order_info.order_id} {order_info.side} {order_info.symbol_code} {order_info.status_code}"
                )

    # 检查最终测试结果
    def CheckResult(self):
        if (
            self.status_code22 == 18041
            or self.status_code22 == 18042
            or self.status_code22 == 18043
        ) and self.status_code23 == 0:
            mylogger.info(
                f"Test1 success. [statusCode = {self.status_code22} {self.status_code23}]"
            )
        else:
            mylogger.error(
                f"Test1 failed. [statusCode = {self.status_code22} {self.status_code23}]"
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
