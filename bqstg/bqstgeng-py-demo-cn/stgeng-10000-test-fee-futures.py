#!/usr/bin/python3

# -*- coding: utf-8 -*-

"""
功能：测试合约手续费
注意：测试失败有可能是SR2309/AP2404的合约乘数变了，目前是10
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

        # 测试费率表中有相关代码费率设置的(按金额)
        self.order_id1 = 0
        self.fee1 = -1

        # 测试费率表中有相关代码费率设置的(最低费率)
        self.order_id2 = 0
        self.fee2 = -1

        # 测试费率表有市场通用费率设置的
        self.order_id3 = 0
        self.fee3 = -1

        # 测试费率表有市场通用费率设置的(最低费率)
        self.order_id4 = 0
        self.fee4 = -1

        # 测试费率表中有相关代码费率设置的(按成交量)
        self.order_id5 = 0
        self.fee5 = -1

        # 平今测试先开今仓
        self.order_id6 = 0
        self.fee6 = -1

        # 平今测试再平今仓
        self.order_id7 = 0
        self.fee7 = -1

    def on_stg_start(self):
        # fmt: off

        self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="InitBaseFeeInfo", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * 3, max_exec_times=1,)

        # 测试费率表中有相关代码费率设置的(按金额)
        self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="TestOrder1", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * 13, max_exec_times=1,)

        # 测试费率表中有相关代码费率设置的(最低费率)
        self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="TestOrder2", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * 16, max_exec_times=1,)

        # 测试费率表有市场通用费率设置的
        self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="TestOrder3", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * 19, max_exec_times=1,)

        # 测试费率表有市场通用费率设置的(最低费率)
        self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="TestOrder4", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * 22, max_exec_times=1,)

        # 测试费率表中有相关代码费率设置的(按成交量)
        self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="TestOrder5", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * 25, max_exec_times=1,)

        # 平今测试先开今仓
        self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="TestOrder6", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * 28, max_exec_times=1,)

        # 平今测试再平今仓
        self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="TestOrder7", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * 31, max_exec_times=1,)

        # 检查测试结果
        self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="CheckResult", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * 34, max_exec_times=1,)

        self.stg_eng.install_stg_inst_timer( stg_inst_id=1, timer_name="ClearBaseFeeInfo", exec_at_startup=ExecAtStartup.IsFalse, millisec_interval=1000 * 37, max_exec_times=1,)

        # fmt: on

    def on_stg_inst_start(self, stg_inst_info):
        pass

    def on_stg_inst_timer(self, stg_inst_info, timer_name):
        if stg_inst_info.stg_inst_id == 1:

            if timer_name == "InitBaseFeeInfo":
                self.InitBaseFeeInfo()

            # 测试费率表中有相关代码费率设置的(按金额)
            if timer_name == "TestOrder1":
                self.TestOrder1(stg_inst_info)
            # 测试费率表中有相关代码费率设置的(最低费率)
            if timer_name == "TestOrder2":
                self.TestOrder2(stg_inst_info)
            # 测试费率表有市场通用费率设置的
            if timer_name == "TestOrder3":
                self.TestOrder3(stg_inst_info)
            # 测试费率表有市场通用费率设置的(最低费率)
            if timer_name == "TestOrder4":
                self.TestOrder4(stg_inst_info)
            # 测试费率表中有相关代码费率设置的(按成交量)
            if timer_name == "TestOrder5":
                self.TestOrder5(stg_inst_info)
            # 平今测试先开今仓
            if timer_name == "TestOrder6":
                self.TestOrder6(stg_inst_info)
            # 平今测试再平今仓
            if timer_name == "TestOrder7":
                self.TestOrder7(stg_inst_info)

            if timer_name == "CheckResult":
                self.CheckResult()

            if timer_name == "ClearBaseFeeInfo":
                self.ClearBaseFeeInfo()

    def InitBaseFeeInfo(self):
        self.stg_eng.sync_exec_sql("DELETE FROM baseFeeInfo;")
        self.stg_eng.sync_exec_sql(
            "INSERT  INTO `baseFeeInfo`(`id`,`acctId`,`trdAcctId`,`side`,`posDirection`,`marketCode`,`symbolType`,`symbolCode`,`feeType`,`commission`,`minCommission`,`stampDuty`,`minStampDuty`,`transFee`,`minTransFee`,`updateTime`,`isDel`) VALUES (1,10000,100000,'Bid','Open','SSE','CN_MainBoard','600600','ByAmt',0.000100000000000000,10.000000000000000000,0.000300000000000000,10.000000000000000000,0.000100000000000000,10.000000000000000000,'2023-05-05 00:12:15.647808',0);"
        )
        self.stg_eng.sync_exec_sql(
            "INSERT  INTO `baseFeeInfo`(`id`,`acctId`,`trdAcctId`,`side`,`posDirection`,`marketCode`,`symbolType`,`symbolCode`,`feeType`,`commission`,`minCommission`,`stampDuty`,`minStampDuty`,`transFee`,`minTransFee`,`updateTime`,`isDel`) VALUES (2,10000,100000,'Ask','Close','SSE','CN_MainBoard','600600','ByAmt',0.000100000000000000,10.000000000000000000,0.000300000000000000,10.000000000000000000,0.000100000000000000,10.000000000000000000,'2023-05-05 00:58:26.642108',0);"
        )
        self.stg_eng.sync_exec_sql(
            "INSERT  INTO `baseFeeInfo`(`id`,`acctId`,`trdAcctId`,`side`,`posDirection`,`marketCode`,`symbolType`,`symbolCode`,`feeType`,`commission`,`minCommission`,`stampDuty`,`minStampDuty`,`transFee`,`minTransFee`,`updateTime`,`isDel`) VALUES (3,10000,100000,'Bid','Open','SSE','CN_MainBoard','','ByAmt',0.000200000000000000,20.000000000000000000,0.000600000000000000,20.000000000000000000,0.000200000000000000,20.000000000000000000,'2023-05-05 01:00:12.400339',0);"
        )
        self.stg_eng.sync_exec_sql(
            "INSERT  INTO `baseFeeInfo`(`id`,`acctId`,`trdAcctId`,`side`,`posDirection`,`marketCode`,`symbolType`,`symbolCode`,`feeType`,`commission`,`minCommission`,`stampDuty`,`minStampDuty`,`transFee`,`minTransFee`,`updateTime`,`isDel`) VALUES (4,10000,100000,'Ask','Close','SSE','CN_MainBoard','','ByAmt',0.000200000000000000,20.000000000000000000,0.000400000000000000,20.000000000000000000,0.000200000000000000,20.000000000000000000,'2023-05-05 01:00:37.148151',0);"
        )
        self.stg_eng.sync_exec_sql(
            "INSERT  INTO `baseFeeInfo`(`id`,`acctId`,`trdAcctId`,`side`,`posDirection`,`marketCode`,`symbolType`,`symbolCode`,`feeType`,`commission`,`minCommission`,`stampDuty`,`minStampDuty`,`transFee`,`minTransFee`,`updateTime`,`isDel`) VALUES (5,10001,100001,'Bid','Open','CZCE','CN_Futures','SR2309','ByAmt',0.000100000000000000,20.000000000000000000,0.000000000000000000,0.000000000000000000,0.000000000000000000,0.000000000000000000,'2023-05-05 02:30:27.554944',0);"
        )
        self.stg_eng.sync_exec_sql(
            "INSERT  INTO `baseFeeInfo`(`id`,`acctId`,`trdAcctId`,`side`,`posDirection`,`marketCode`,`symbolType`,`symbolCode`,`feeType`,`commission`,`minCommission`,`stampDuty`,`minStampDuty`,`transFee`,`minTransFee`,`updateTime`,`isDel`) VALUES (6,10001,100001,'Bid','Open','CZCE','CN_Futures','','ByAmt',0.000150000000000000,25.000000000000000000,0.000000000000000000,0.000000000000000000,0.000000000000000000,0.000000000000000000,'2023-05-06 06:11:07.380369',0);"
        )
        self.stg_eng.sync_exec_sql(
            "INSERT  INTO `baseFeeInfo`(`id`,`acctId`,`trdAcctId`,`side`,`posDirection`,`marketCode`,`symbolType`,`symbolCode`,`feeType`,`commission`,`minCommission`,`stampDuty`,`minStampDuty`,`transFee`,`minTransFee`,`updateTime`,`isDel`) VALUES (8,10001,100001,'Ask','Close','CZCE','CN_Futures','','ByAmt',0.000250000000000000,45.000000000000000000,0.000000000000000000,0.000000000000000000,0.000000000000000000,0.000000000000000000,'2023-05-06 06:10:59.896104',0);"
        )
        self.stg_eng.sync_exec_sql(
            "INSERT  INTO `baseFeeInfo`(`id`,`acctId`,`trdAcctId`,`side`,`posDirection`,`marketCode`,`symbolType`,`symbolCode`,`feeType`,`commission`,`minCommission`,`stampDuty`,`minStampDuty`,`transFee`,`minTransFee`,`updateTime`,`isDel`) VALUES (9,10001,100001,'Ask','Close','CZCE','CN_Futures','SR2309','ByAmt',0.000200000000000000,40.000000000000000000,0.000000000000000000,0.000000000000000000,0.000000000000000000,0.000000000000000000,'2023-05-06 04:37:20.828935',0);"
        )
        self.stg_eng.sync_exec_sql(
            "INSERT  INTO `baseFeeInfo`(`id`,`acctId`,`trdAcctId`,`side`,`posDirection`,`marketCode`,`symbolType`,`symbolCode`,`feeType`,`commission`,`minCommission`,`stampDuty`,`minStampDuty`,`transFee`,`minTransFee`,`updateTime`,`isDel`) VALUES (10,10001,100001,'Bid','Open','CZCE','CN_Futures','WH2309','ByVol',100.000000000000000000,0.000000000000000000,0.000000000000000000,0.000000000000000000,0.000000000000000000,0.000000000000000000,'2023-05-06 04:42:35.782602',0);"
        )
        self.stg_eng.sync_exec_sql(
            "INSERT  INTO `baseFeeInfo`(`id`,`acctId`,`trdAcctId`,`side`,`posDirection`,`marketCode`,`symbolType`,`symbolCode`,`feeType`,`commission`,`minCommission`,`stampDuty`,`minStampDuty`,`transFee`,`minTransFee`,`updateTime`,`isDel`) VALUES (11,10001,100001,'Ask','Close','CZCE','CN_Futures','WH2309','ByVol',200.000000000000000000,0.000000000000000000,0.000000000000000000,0.000000000000000000,0.000000000000000000,0.000000000000000000,'2023-05-06 04:43:07.690377',0);"
        )
        self.stg_eng.sync_exec_sql(
            "INSERT  INTO `baseFeeInfo`(`id`,`acctId`,`trdAcctId`,`side`,`posDirection`,`marketCode`,`symbolType`,`symbolCode`,`feeType`,`commission`,`minCommission`,`stampDuty`,`minStampDuty`,`transFee`,`minTransFee`,`updateTime`,`isDel`) VALUES (16,10001,100001,'Ask','CloseTDay','CZCE','CN_Futures','WH2309','ByVol',250.000000000000000000,0.000000000000000000,0.000000000000000000,0.000000000000000000,0.000000000000000000,0.000000000000000000,'2023-05-06 06:11:43.134152',0);"
        )
        self.stg_eng.sync_exec_sql(
            "INSERT  INTO `baseFeeInfo`(`id`,`acctId`,`trdAcctId`,`side`,`posDirection`,`marketCode`,`symbolType`,`symbolCode`,`feeType`,`commission`,`minCommission`,`stampDuty`,`minStampDuty`,`transFee`,`minTransFee`,`updateTime`,`isDel`) VALUES (17,10001,100001,'Ask','CloseTDay','CZCE','CN_Futures','SR2309','ByAmt',0.000250000000000000,45.000000000000000000,0.000000000000000000,0.000000000000000000,0.000000000000000000,0.000000000000000000,'2023-05-06 06:11:53.387104',0);"
        )
        self.stg_eng.sync_exec_sql(
            "INSERT  INTO `baseFeeInfo`(`id`,`acctId`,`trdAcctId`,`side`,`posDirection`,`marketCode`,`symbolType`,`symbolCode`,`feeType`,`commission`,`minCommission`,`stampDuty`,`minStampDuty`,`transFee`,`minTransFee`,`updateTime`,`isDel`) VALUES (18,10001,100001,'Ask','CloseTDay','CZCE','CN_Futures','','ByAmt',0.000300000000000000,50.000000000000000000,0.000000000000000000,0.000000000000000000,0.000000000000000000,0.000000000000000000,'2023-05-06 06:12:08.903632',0);"
        )
        self.stg_eng.sync_exec_sql(
            "INSERT  INTO `baseFeeInfo`(`id`,`acctId`,`trdAcctId`,`side`,`posDirection`,`marketCode`,`symbolType`,`symbolCode`,`feeType`,`commission`,`minCommission`,`stampDuty`,`minStampDuty`,`transFee`,`minTransFee`,`updateTime`,`isDel`) VALUES (19,10001,100001,'Ask','Close','DCE','CN_Futures','a2403','ByAmt',0.000200000000000000,40.000000000000000000,0.000000000000000000,0.000000000000000000,0.000000000000000000,0.000000000000000000,'2023-05-09 23:52:57.498236',0);"
        )

    def ClearBaseFeeInfo(self):
        self.stg_eng.sync_exec_sql("DELETE FROM baseFeeInfo;")

    # 测试费率表中有相关代码费率设置的(按金额)
    def TestOrder1(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))
        # fmt: on

        ret_of_order = self.stg_eng.order(
            stg_inst_info,
            market_code=MarketCode.DCE,
            symbol_code="a2403",
            side=Side.Ask,
            pos_direction=PosDirection.Close,
            order_price=10000,
            order_size=100,
            trd_acct_id=100001,
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
        mylogger.info(f"===> test1 send order {self.order_id1}")
        return

    # 测试费率表中有相关代码费率设置的(最低费率)
    def TestOrder2(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))
        # fmt: on

        ret_of_order = self.stg_eng.order(
            stg_inst_info,
            market_code=MarketCode.DCE,
            symbol_code="a2403",
            side=Side.Ask,
            pos_direction=PosDirection.Close,
            order_price=10000,
            order_size=1,
            trd_acct_id=100001,
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
        mylogger.info(f"===> test2 send order {self.order_id2}")
        return

    # 测试费率表有市场通用费率设置的
    def TestOrder3(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))
        # fmt: on

        ret_of_order = self.stg_eng.order(
            stg_inst_info,
            market_code=MarketCode.CZCE,
            symbol_code="AP2404",
            side=Side.Ask,
            pos_direction=PosDirection.Close,
            order_price=10000,
            order_size=100,
            trd_acct_id=100001,
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
        mylogger.info(f"===> test3 send order {self.order_id3}")
        return

    # 测试费率表没中有相关代码费率设置的(最低费率)
    def TestOrder4(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))
        # fmt: on

        ret_of_order = self.stg_eng.order(
            stg_inst_info,
            market_code=MarketCode.CZCE,
            symbol_code="AP2404",
            side=Side.Ask,
            pos_direction=PosDirection.Close,
            order_price=10000,
            order_size=1,
            trd_acct_id=100001,
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
        mylogger.info(f"===> test4 send order {self.order_id4}")
        return

    # 测试费率表中有相关代码费率设置的(按成交量)
    def TestOrder5(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))
        # fmt: on

        ret_of_order = self.stg_eng.order(
            stg_inst_info,
            market_code=MarketCode.CZCE,
            symbol_code="WH2309",
            side=Side.Ask,
            pos_direction=PosDirection.Close,
            order_price=10000,
            order_size=2,
            trd_acct_id=100001,
            close_tday_stg=CloseTDayStg.RejectCloseTDay,
            algo_id=0,
            simed_td_info=simed_td_info,
        )

        status_code = ret_of_order[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id5 = ret_of_order[1]
        mylogger.info(f"===> test5 send order {self.order_id5}")
        return

    # 平今测试先开今仓
    def TestOrder6(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))
        # fmt: on

        ret_of_order = self.stg_eng.order(
            stg_inst_info,
            market_code=MarketCode.CZCE,
            symbol_code="SR2309",
            side=Side.Bid,
            pos_direction=PosDirection.Open,
            order_price=10000,
            order_size=100,
            trd_acct_id=100001,
            close_tday_stg=CloseTDayStg.RejectCloseTDay,
            algo_id=0,
            simed_td_info=simed_td_info,
        )

        status_code = ret_of_order[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id6 = ret_of_order[1]
        mylogger.info(f"===> test6 send order {self.order_id6}")
        return

    # 平今测试再平今仓
    def TestOrder7(self, stg_inst_info):
        # fmt: off
        simed_td_info = SimedTDInfo()
        simed_td_info.order_status = OrderStatus.Filled
        simed_td_info.trans_detail_group.append(TransDetail(slippage=0, filled_per=1, ld=LiquidityDirection.Maker))
        # fmt: on

        ret_of_order = self.stg_eng.order(
            stg_inst_info,
            market_code=MarketCode.CZCE,
            symbol_code="SR2309",
            side=Side.Ask,
            pos_direction=PosDirection.Close,
            order_price=10000,
            order_size=100,
            trd_acct_id=100001,
            close_tday_stg=CloseTDayStg.AllowCloseTDay,
            algo_id=0,
            simed_td_info=simed_td_info,
        )

        status_code = ret_of_order[0]
        if status_code != 0:
            status_msg = get_status_msg(status_code)
            mylogger.error(f"Create order failed. {status_code} - {status_msg}")
            return

        self.order_id7 = ret_of_order[1]
        mylogger.info(f"===> test7 send order {self.order_id7}")
        return

    def on_stg_manual_intervention(self, stg_inst_info, stg_manual_intervention):
        mylogger.info(f"recv manual intervention {stg_manual_intervention}")
        data = json.dumps(stg_manual_intervention)

    def on_push_topic(self, stg_inst_info, topic_data):
        mylogger.info(f"stg inst {stg_inst_info.stg_inst_id} recv {topic_data}")

    def on_order_ret(self, stg_inst_info, order_info):
        mylogger.info(f"<===  on order ret {order_info.to_short_str()}")

        # 测试费率表中有相关代码费率设置的(按金额)
        if order_info.order_id == self.order_id1:
            self.fee1 = order_info.fee
            if order_info.order_status == OrderStatus.Filled:
                if order_info.fee != 2000:
                    mylogger.error(f"invalid fee {order_info.fee}")

        # 测试费率表中有相关代码费率设置的(最低费率)
        if order_info.order_id == self.order_id2:
            self.fee2 = order_info.fee
            if order_info.order_status == OrderStatus.Filled:
                if order_info.fee != 40:
                    mylogger.error(f"invalid fee {order_info.fee}")

        # 测试费率表中有没相关代码费率设置的
        if order_info.order_id == self.order_id3:
            self.fee3 = order_info.fee
            if order_info.order_status == OrderStatus.Filled:
                if order_info.fee != 2500:
                    mylogger.error(f"invalid fee {order_info.fee}")

        # 测试费率表中有没相关代码费率设置的(最低费率)
        if order_info.order_id == self.order_id4:
            self.fee4 = order_info.fee
            if order_info.order_status == OrderStatus.Filled:
                if order_info.fee != 45:
                    mylogger.error(f"invalid fee {order_info.fee}")

        # 测试费率表中有相关代码费率设置的(按成交量)
        if order_info.order_id == self.order_id5:
            self.fee5 = order_info.fee
            if order_info.order_status == OrderStatus.Filled:
                if order_info.fee != 400:
                    mylogger.error(f"invalid fee {order_info.fee}")

        # 平今测试先开今仓
        if order_info.order_id == self.order_id6:
            self.fee6 = order_info.fee
            if order_info.order_status == OrderStatus.Filled:
                if order_info.fee != 1000:
                    mylogger.error(f"invalid fee {order_info.fee}")

        # 平今测试再平今仓
        if order_info.order_id == self.order_id7:
            self.fee7 = order_info.fee
            if order_info.order_status == OrderStatus.Filled:
                if order_info.fee != 2500:
                    mylogger.error(f"invalid fee {order_info.fee}")

    def CheckResult(self):
        if self.fee1 == 2000:
            mylogger.info(f"Test1 success.")
        else:
            mylogger.error(f"Test1 failed. [fee = {self.fee1}]")

        if self.fee2 == 40:
            mylogger.info(f"Test2 success.")
        else:
            mylogger.error(f"Test2 failed. [fee = {self.fee2}]")

        if self.fee3 == 2500:
            mylogger.info(f"Test3 success.")
        else:
            mylogger.error(f"Test3 failed. [fee = {self.fee3}]")

        if self.fee4 == 45:
            mylogger.info(f"Test4 success.")
        else:
            mylogger.error(f"Test4 failed. [fee = {self.fee4}]")

        if self.fee5 == 400:
            mylogger.info(f"Test5 success.")
        else:
            mylogger.error(f"Test5 failed. [fee = {self.fee5}]")

        if self.fee6 == 1000:
            mylogger.info(f"Test6 success.")
        else:
            mylogger.error(f"Test6 failed. [fee = {self.fee6}]")

        if self.fee7 == 2500:
            mylogger.info(f"Test7 success.")
        else:
            mylogger.error(f"Test7 failed. [fee = {self.fee7}]")

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
