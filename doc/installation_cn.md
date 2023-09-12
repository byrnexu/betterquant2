# 安装
## 🔥 先执行第一步编译

&emsp;&emsp;编译的结果在 bin 和 lib 目录下，其中以 -d 结尾的都是 debug 版本，其他是 release 版本

## 🔥 制作images  

&emsp;&emsp;需修改 docker/setting.sh 中 DB_PASSWD 和 ROOT_DIR_OF_INSTALLATION 也就是配置、插件和日志及数据库的 docker 挂载目录。
```bash
   cd docker && bash make-images.sh
```
&emsp;&emsp;docker images 可以看到创建的 images  

[<img src="../assets/images.png" width="800"/>]()


## 🔥 运行

* 运行基本组件  

&emsp;&emsp;这里会启动行情、交易、风控、数据库和 IPC 服务
```bash
   bash run-base.sh
```
[<img src="../assets/containers.png" width="800"/>]()

&emsp;&emsp;docker ps 可以看到启动的几个服务。  

* 创建交易账号  

&emsp;&emsp;为了降低系统复杂度，每创建一个账号需要启动一个相应的交易网关，因此创建交易账号包括以下几个方面：  

1. 数据库表 acctInfo 中需要增加记录。  
1. 需要创建新账号对应的交易网关的配置文件。  
1. 修改 docker-compose-tdgw.yaml 增加新的交易网关的启动项目。  

&emsp;&emsp;为了方便使用，我写了一个脚本 create-acct.sh 在 docker 目录下，运行 create-acct.sh 即可完成上述三个步骤。  
```bash
   # 格式：
   bash create-acct.sh acctId marketCode symbolType acctName apiKey secKey phase
```
&emsp;&emsp;更详细的例子请参考：docker/create-acct-for-test.sh  

| 参数 | 说明 | 备注 |
| ------ | ------ | ------ |
| acctId | 需要创建的acctId | 区间 \[10001,20000)，其他账号系统预留 |
| marketCode | 交易市场 | 国内二级市场一般都是支持多交易所交易，因此填Others即可 |
| symbolType | 交易品种类型 | 国内二级市场一般都是支持多交易所交易，因此填Others即可 |
| acctName | 账户名称 | 给账户取一个有辨识度的名称 |
| apiKey | 从交易所申请 | 国内二级市场填空即可，加密货币请绑定IP关闭转账💣❗  |
| secKey | 从交易所申请 | 国内二级市场填空即可，加密货币请绑定IP关闭转账💣❗ |
| phase | 从交易所申请 | 国内二级市场填空即可，加密货币没有的话传""即可💣❗ |

&emsp;&emsp;注：关于acctId，用户使用的账号区间定义在\[10001,20000)之间，其他区间预留给测试账号、手拍单或其他账号。  
```bash
   # 例子：
   bash create-acct.sh 10001 Others Others XTPTest "" "" ""  
```
&emsp;&emsp;注：如果 acctId 已经存在，则会覆盖原有信息。  

* 运行交易网关（数据库必须有相应的账户配置并在 docker-compose-tdgw.yaml 增加相应项，见下面）
```bash
   bash run-tdgw.sh
```
&emsp;&emsp;docker ps 可以看到交易网关启动结果。

* 停止服务
```bash
   bash stop.sh
```

## 🔥 运行（不通过docker）

&emsp;&emsp;如果成功编译的话，也可以直接运行编译成功的目标文件，不使用 docker，开发和测试的都可以考虑这种方式，先安装启动数据库和 IPC 服务，其他没有顺序要求。
* 安装启动数据库
```shell
cd bqdb && bash run-db.sh & 
```
* 启动 IPC 组件
```shell
iox-roudi -m off &
```
* 运行以下服务
```shell
# 运行币安现货行情服务
./bqmd-xtp --conf=config/bqmd-xtp/bqmd-xtp.yaml &

# 运行风控子系统
./bqriskmgr --conf=config/bqriskmgr/bqriskmgr.yaml

# 运行交易服务
./bqtd-srv --conf=config/bqtd-srv/bqtd-srv.yaml

# 启动交易网关  
./bqtd-xtp --conf=config/bqtd-xtp/bqtd-xtp.yaml &

```
* 停止服务
```shell
# 先通过下面的命令得到pid，以现货行情网关为例： 
ps -ef|grep -i bqmd|grep spot'

# 然后再通过 kill -SIGINT pid 或 kill -SIGTERM pid 停止服务
# 切记不可用 kill -SIGKILL 或 kill -9
```
