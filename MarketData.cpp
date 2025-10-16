#include "MarketData.h"

MarketData::MarketData()
	: m_type(MarketDataType::Trade)
	, m_timestamp(QDateTime::currentDateTime())
	, m_lastPrice(0.0)
	, m_bidPrice(0.0)
	, m_askPrice(0.0)
	, m_openPrice(0.0)
	, m_highPrice(0.0)
	, m_lowPrice(0.0)
	, m_closePrice(0.0)
	, m_lastVolume(0.0)
	, m_bidVolume(0.0)
	, m_askVolume(0.0)
	, m_totalVolume(0.0)
{
}

MarketData::MarketData(const QString& symbol, MarketDataType type)
	: m_symbol(symbol)
	, m_type(type)
	, m_timestamp(QDateTime::currentDateTime())
	, m_lastPrice(0.0)
	, m_bidPrice(0.0)
	, m_askPrice(0.0)
	, m_openPrice(0.0)
	, m_highPrice(0.0)
	, m_lowPrice(0.0)
	, m_closePrice(0.0)
	, m_lastVolume(0.0)
	, m_bidVolume(0.0)
	, m_askVolume(0.0)
	, m_totalVolume(0.0)
{
}

double MarketData::changePercent() const
{
	if (m_openPrice <= 0.0) return 0.0;
	return ((m_lastPrice - m_openPrice) / m_openPrice) * 100.0;
}

void MarketData::updateTrade(double price, double volume)
{
	m_lastPrice = price;
	m_lastVolume = volume;
	m_totalVolume += volume;
	m_timestamp = QDateTime::currentDateTime();

	// Update high/low
	if (m_highPrice == 0.0 || price > m_highPrice) {
		m_highPrice = price;
	}
	if (m_lowPrice == 0.0 || price < m_lowPrice) {
		m_lowPrice = price;
	}

	// Set open price if not set
	if (m_openPrice == 0.0) {
		m_openPrice = price;
	}
}

void MarketData::updateQuote(double bidPrice, double bidVolume, double askPrice, double askVolume)
{
	m_bidPrice = bidPrice;
	m_bidVolume = bidVolume;
	m_askPrice = askPrice;
	m_askVolume = askVolume;
	m_timestamp = QDateTime::currentDateTime();
}

bool MarketData::isValid() const
{
	return !m_symbol.isEmpty() &&
		(m_lastPrice > 0.0 || m_bidPrice > 0.0 || m_askPrice > 0.0);
}

QString MarketData::typeToString(MarketDataType type)
{
	switch (type) {
	case MarketDataType::Trade: return "TRADE";
	case MarketDataType::Quote: return "QUOTE";
	case MarketDataType::Level2: return "LEVEL2";
	case MarketDataType::Summary: return "SUMMARY";
	default: return "UNKNOWN";
	}
}