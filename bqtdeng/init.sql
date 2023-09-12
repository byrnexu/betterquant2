  DROP DATABASE IF EXISTS marketData;  
	CREATE DATABASE IF NOT EXISTS marketData PRECISION "us";
	SHOW DATABASES;  
	USE marketData;
	
	DROP STABLE IF EXISTS trades;
	CREATE STABLE IF NOT EXISTS trades(
	  exchTs TIMESTAMP, localTs TIMESTAMP, tradeTime TIMESTAMP, tradeNo VARCHAR(32), price DOUBLE, 
	  size DOUBLE, side TINYINT UNSIGNED, bidOrderId VARCHAR(16), askOrderId VARCHAR(16), 
	  tradingDay VARCHAR(8)) 
	  TAGS(symbolType TINYINT UNSIGNED, marketCode SMALLINT UNSIGNED, symbolCode VARCHAR(16));
	DESCRIBE trades;

	DROP STABLE IF EXISTS orders;
	CREATE STABLE IF NOT EXISTS orders(
	  exchTs TIMESTAMP, localTs TIMESTAMP, orderTime TIMESTAMP, orderNo VARCHAR(32), price DOUBLE, 
	  size DOUBLE, side TINYINT UNSIGNED, tradingDay VARCHAR(8)) 
	  TAGS(symbolType TINYINT UNSIGNED, marketCode SMALLINT UNSIGNED, symbolCode VARCHAR(16));
	DESCRIBE orders;

	DROP STABLE IF EXISTS tickers;
	CREATE STABLE IF NOT EXISTS tickers(
	  exchTs TIMESTAMP, localTs TIMESTAMP, open DOUBLE, high DOUBLE, low DOUBLE, lastPrice DOUBLE, 
	  lastSize DOUBLE, upperLimitPrice DOUBLE, lowerLimitPrice DOUBLE, preClosePrice DOUBLE, 
	  preSettlementPrice DOUBLE, closePrice DOUBLE, settlementPrice DOUBLE, preOpenInterest DOUBLE, 
	  openInterest DOUBLE, vol DOUBLE, amt DOUBLE, askPrice DOUBLE, askSize DOUBLE, bidPrice DOUBLE, 
	  bidSize DOUBLE, tradingDay VARCHAR(8), asks VARCHAR(1024), bids VARCHAR(1024)) 
	  TAGS(symbolType TINYINT UNSIGNED, marketCode SMALLINT UNSIGNED, symbolCode VARCHAR(16));
	DESCRIBE tickers;

	DROP STABLE IF EXISTS books;
	CREATE STABLE IF NOT EXISTS books(exchTs TIMESTAMP, localTs TIMESTAMP, lastPrice DOUBLE, 
	totalVol DOUBLE, totalAmt DOUBLE, tradesCount BIGINT UNSIGNED, tradingDay VARCHAR(8), asks VARCHAR(1024), 
	bids VARCHAR(1024)) TAGS(symbolType TINYINT UNSIGNED, marketCode SMALLINT UNSIGNED, symbolCode VARCHAR(16));
	DESCRIBE books;

	DROP STABLE IF EXISTS origData;
	CREATE STABLE IF NOT EXISTS origData(localTs TIMESTAMP, exchTs TIMESTAMP, tradingDay VARCHAR(8), data BINARY(16000)) 
	TAGS(apiName VARCHAR(8), symbolType TINYINT UNSIGNED, marketCode SMALLINT UNSIGNED, symbolCode VARCHAR(16), mdType TINYINT UNSIGNED);
	DESCRIBE origData;
