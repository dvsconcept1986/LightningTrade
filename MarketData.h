#pragma once
#include <QString>
#include <QDateTime>

enum class MarketDataType {
	Trade,
	Quote,
	Level2,
	Summary
};

class MarketData {
public:
	MarketData();
	MarketData(const QString& symbol, MarketDataType type);

	// Getters
	QString symbol() const { return m_symbol; }
	MarketDataType type() const { return m_type; }
	QDateTime timestamp() const { return m_timestamp; }

	// Price data
	double lastPrice() const { return m_lastPrice; }
	double bidPrice() const { return m_bidPrice; }
	double askPrice() const { return m_askPrice; }
	double openPrice() const { return m_openPrice; }
	double highPrice() const { return m_highPrice; }
	double lowPrice() const { return m_lowPrice; }
	double closePrice() const { return m_closePrice; }

	// Volume data
	double lastVolume() const { return m_lastVolume; }
	double bidVolume() const { return m_bidVolume; }
	double askVolume() const { return m_askVolume; }
	double totalVolume() const { return m_totalVolume; }

	// Calculated fields
	double midPrice() const { return (m_bidPrice + m_askPrice) / 2.0; }
	double spread() const { return m_askPrice - m_bidPrice; }
	double changePercent() const;
	double changeAmount() const { return m_lastPrice - m_openPrice; }

	// Setters
	void setSymbol(const QString& symbol) { m_symbol = symbol; }
	void setType(MarketDataType type) { m_type = type; }
	void setTimestamp(const QDateTime& timestamp) { m_timestamp = timestamp; }

	void setLastPrice(double price) { m_lastPrice = price; }
	void setBidPrice(double price) { m_bidPrice = price; }
	void setAskPrice(double price) { m_askPrice = price; }
	void setOpenPrice(double price) { m_openPrice = price; }
	void setHighPrice(double price) { m_highPrice = price; }
	void setLowPrice(double price) { m_lowPrice = price; }
	void setClosePrice(double price) { m_closePrice = price; }

	void setLastVolume(double volume) { m_lastVolume = volume; }
	void setBidVolume(double volume) { m_bidVolume = volume; }
	void setAskVolume(double volume) { m_askVolume = volume; }
	void setTotalVolume(double volume) { m_totalVolume = volume; }

	// Update methods
	void updateTrade(double price, double volume);
	void updateQuote(double bidPrice, double bidVolume, double askPrice, double askVolume);

	// Validation
	bool isValid() const;

	// String conversion
	static QString typeToString(MarketDataType type);

private:
	QString m_symbol;
	MarketDataType m_type;
	QDateTime m_timestamp;

	// Price data
	double m_lastPrice;
	double m_bidPrice;
	double m_askPrice;
	double m_openPrice;
	double m_highPrice;
	double m_lowPrice;
	double m_closePrice;

	// Volume data
	double m_lastVolume;
	double m_bidVolume;
	double m_askVolume;
	double m_totalVolume;
};