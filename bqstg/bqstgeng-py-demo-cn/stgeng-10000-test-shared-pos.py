#!/usr/bin/python3

# -*- coding: utf-8 -*-

"""
功能：准备借仓测试
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

        self.order_id11 = 0
        self.order_id12 = 0

        self.status_code11 = -1
        self.status_code12 = -1

    def on_stg_start(self):
        # fmt: off
        ts = 0

        # 账户内借仓
        ts += 1; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="Test1", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # 跨账户借仓
        ts += 3; self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="Test2", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * ts, max_exec_times=1)

        # fmt: on

    def on_stg_inst_start(self, stg_inst_info):
        pass

    def on_stg_inst_timer(self, stg_inst_info, timer_name):
        if stg_inst_info.stg_inst_id == 1:
            if timer_name == "Test1":
                self.Test1(stg_inst_info)
            if timer_name == "Test2":
                self.Test2(stg_inst_info)

    def Test1(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))
        ret_of_order11 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.DCE, symbol_code="c2403", side=Side.Bid, order_price=100, order_size=1, trd_acct_id=100001, close_tday_stg=CloseTDayStg.AllowCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        # fmt: on

        self.order_id11 = ret_of_order11[1]
        mylogger.info(f"===> send order {self.order_id11}")

    def Test2(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))
        ret_of_order12 = self.stg_eng.order(stg_inst_info, market_code=MarketCode.DCE, symbol_code="c2403", side=Side.Bid, order_price=100, order_size=1, trd_acct_id=100004, close_tday_stg=CloseTDayStg.AllowCloseTDay, algo_id=0, simed_td_info=simed_td_info)
        # fmt: on

        self.order_id12 = ret_of_order12[1]
        mylogger.info(f"===> send order {self.order_id12}")

    def on_order_ret(self, stg_inst_info, order_info):
        mylogger.info(f"<===  on order ret {order_info.to_short_str()}")

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
