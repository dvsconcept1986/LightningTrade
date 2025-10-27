#include "MarketDataFeed.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QDebug>

MarketDataFeed::MarketDataFeed(QObject* parent)
	: QObject(parent)
	, m_webSocket(new QWebSocket())
	, m_networkManager(new QNetworkAccessManager(this))
	, m_status(FeedStatus::Disconnected)
	, m_webSocketUrl("wss://ws.finnhub.io")  // Finnhub WebSocket URL
	, m_restApiUrl("https://finnhub.io/api/v1")  // Finnhub REST API
	, m_updateInterval(1000)
	, m_useSimulation(false)  // Try real data first
	, m_finnhubApiKey("d3vbvs9r01qt2ctp2tugd3vbvs9r01qt2ctp2tv0")  // ADD YOUR API KEY
	, m_reconnectTimer(new QTimer(this))
	, m_heartbeatTimer(new QTimer(this))
	, m_simulationTimer(new QTimer(this))
	, m_messagesReceived(0)
	, m_messagesProcessed(0)
{
	// Connect WebSocket signals
	connect(m_webSocket, &QWebSocket::connected, this, &MarketDataFeed::onWebSocketConnected);
	connect(m_webSocket, &QWebSocket::disconnected, this, &MarketDataFeed::onWebSocketDisconnected);
	connect(m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred),
		this, &MarketDataFeed::onWebSocketError);
	connect(m_webSocket, &QWebSocket::textMessageReceived,
		this, &MarketDataFeed::onWebSocketTextMessageReceived);
	connect(m_webSocket, &QWebSocket::binaryMessageReceived,
		this, &MarketDataFeed::onWebSocketBinaryMessageReceived);

	// Setup timers
	m_reconnectTimer->setInterval(5000);  // Reconnect every 5 seconds
	m_reconnectTimer->setSingleShot(true);
	connect(m_reconnectTimer, &QTimer::timeout, this, &MarketDataFeed::connectToFeed);

	m_heartbeatTimer->setInterval(30000);  // Heartbeat every 30 seconds
	connect(m_heartbeatTimer, &QTimer::timeout, this, [this]() {
		if (m_webSocket->isValid()) {
			// Finnhub WebSocket ping
			QJsonObject ping;
			ping["type"] = "ping";
			m_webSocket->sendTextMessage(QJsonDocument(ping).toJson(QJsonDocument::Compact));
		}
		});

	m_simulationTimer->setInterval(m_updateInterval);
	connect(m_simulationTimer, &QTimer::timeout, this, &MarketDataFeed::simulateMarketData);
}

MarketDataFeed::~MarketDataFeed()
{
	disconnectFromFeed();
	qDeleteAll(m_marketData);
	m_marketData.clear();
	delete m_webSocket;
}

void MarketDataFeed::connectToFeed()
{
	if (m_status == FeedStatus::Connected || m_status == FeedStatus::Connecting) {
		return;
	}

	emit logMessage("[FEED] Connecting to Finnhub market data feed...");
	setStatus(FeedStatus::Connecting);

	if (m_useSimulation) {
		// Use simulation mode
		emit logMessage("[FEED] Using simulation mode");
		startSimulation();
		setStatus(FeedStatus::Connected);
		emit connected();
	}
	else {
		// Connect to Finnhub WebSocket with API key
		QString url = QString("%1?token=%2").arg(m_webSocketUrl, m_finnhubApiKey);
		emit logMessage(QString("[FEED] Connecting to: %1").arg(m_webSocketUrl));
		m_webSocket->open(QUrl(url));
	}
}

void MarketDataFeed::disconnectFromFeed()
{
	emit logMessage("[FEED] Disconnecting from market data feed...");

	if (m_useSimulation) {
		stopSimulation();
	}
	else {
		m_webSocket->close();
	}

	m_reconnectTimer->stop();
	m_heartbeatTimer->stop();
	setStatus(FeedStatus::Disconnected);
}

void MarketDataFeed::subscribe(const QString& symbol)
{
	if (m_subscribedSymbols.contains(symbol)) {
		return;
	}

	m_subscribedSymbols.append(symbol);

	// Create market data object if it doesn't exist
	if (!m_marketData.contains(symbol)) {
		m_marketData[symbol] = new MarketData(symbol, MarketDataType::Trade);
	}

	emit logMessage(QString("[FEED] Subscribed to %1").arg(symbol));

	// Send subscription message to Finnhub if connected
	if (m_status == FeedStatus::Connected && !m_useSimulation) {
		QJsonObject msg;
		msg["type"] = "subscribe";
		msg["symbol"] = symbol;
		QString jsonMsg = QJsonDocument(msg).toJson(QJsonDocument::Compact);
		m_webSocket->sendTextMessage(jsonMsg);
		emit logMessage(QString("[FEED] Sent subscribe message: %1").arg(jsonMsg));
	}

	// Fetch initial snapshot from REST API
	if (!m_useSimulation) {
		fetchSnapshotData(symbol);
	}
}

void MarketDataFeed::unsubscribe(const QString& symbol)
{
	m_subscribedSymbols.removeAll(symbol);

	emit logMessage(QString("[FEED] Unsubscribed from %1").arg(symbol));

	// Send unsubscribe message to Finnhub if connected
	if (m_status == FeedStatus::Connected && !m_useSimulation) {
		QJsonObject msg;
		msg["type"] = "unsubscribe";
		msg["symbol"] = symbol;
		m_webSocket->sendTextMessage(QJsonDocument(msg).toJson(QJsonDocument::Compact));
	}
}

void MarketDataFeed::subscribeMultiple(const QStringList& symbols)
{
	for (const QString& symbol : symbols) {
		subscribe(symbol);
	}
}

QStringList MarketDataFeed::getSubscribedSymbols() const
{
	return m_subscribedSymbols;
}

MarketData* MarketDataFeed::getMarketData(const QString& symbol)
{
	if (m_marketData.contains(symbol)) {
		return m_marketData[symbol];
	}
	return nullptr;
}

QList<MarketData*> MarketDataFeed::getAllMarketData() const
{
	QList<MarketData*> dataList;
	for (auto it = m_marketData.begin(); it != m_marketData.end(); ++it) {
		dataList.append(it.value());
	}
	return dataList;
}

void MarketDataFeed::onWebSocketConnected()
{
	emit logMessage("[FEED] WebSocket connected to Finnhub");
	setStatus(FeedStatus::Connected);
	m_heartbeatTimer->start();
	emit connected();

	// Subscribe to all symbols
	for (const QString& symbol : m_subscribedSymbols) {
		QJsonObject msg;
		msg["type"] = "subscribe";
		msg["symbol"] = symbol;
		QString jsonMsg = QJsonDocument(msg).toJson(QJsonDocument::Compact);
		m_webSocket->sendTextMessage(jsonMsg);
		emit logMessage(QString("[FEED] Subscribed to %1").arg(symbol));
	}
}

void MarketDataFeed::onWebSocketDisconnected()
{
	emit logMessage("[FEED] WebSocket disconnected from Finnhub");
	m_heartbeatTimer->stop();
	setStatus(FeedStatus::Disconnected);
	emit disconnected();

	// Attempt reconnection
	if (!m_useSimulation) {
		setStatus(FeedStatus::Reconnecting);
		emit logMessage("[FEED] Attempting to reconnect in 5 seconds...");
		m_reconnectTimer->start();
	}
}

void MarketDataFeed::onWebSocketError(QAbstractSocket::SocketError error)
{
	QString errorMsg = QString("[FEED] WebSocket error: %1").arg(m_webSocket->errorString());
	emit logMessage(errorMsg);
	emit connectionError(errorMsg);
	setStatus(FeedStatus::Error);

	// Fall back to simulation mode
	if (!m_useSimulation) {
		emit logMessage("[FEED] Failed to connect to Finnhub - falling back to simulation mode");
		m_useSimulation = true;
		startSimulation();
		setStatus(FeedStatus::Connected);
	}
}

void MarketDataFeed::onWebSocketTextMessageReceived(const QString& message)
{
	m_messagesReceived++;
	m_lastMessageTime = QDateTime::currentDateTime();
	processWebSocketMessage(message);
}

void MarketDataFeed::onWebSocketBinaryMessageReceived(const QByteArray& message)
{
	m_messagesReceived++;
	m_lastMessageTime = QDateTime::currentDateTime();
}

void MarketDataFeed::onRestApiReplyFinished()
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
	if (!reply) return;

	reply->deleteLater();

	if (reply->error() == QNetworkReply::NoError) {
		QByteArray data = reply->readAll();
		processRestApiData(data);
	}
	else {
		emit logMessage(QString("[FEED] REST API error: %1").arg(reply->errorString()));
	}
}

void MarketDataFeed::simulateMarketData()
{
	for (const QString& symbol : m_subscribedSymbols) {
		generateRandomMarketData(symbol);
	}
}

void MarketDataFeed::setStatus(FeedStatus status)
{
	if (m_status != status) {
		m_status = status;
		emit statusChanged(status);
	}
}

void MarketDataFeed::processWebSocketMessage(const QString& message)
{
	QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
	if (doc.isNull() || !doc.isObject()) {
		return;
	}

	QJsonObject obj = doc.object();
	QString type = obj["type"].toString();

	// Handle Finnhub WebSocket response format
	if (type == "ping") {
		// Respond to ping
		return;
	}
	else if (type == "trade") {
		// Finnhub sends trade data in "data" array
		QJsonArray dataArray = obj["data"].toArray();

		for (const QJsonValue& val : dataArray) {
			QJsonObject trade = val.toObject();

			QString symbol = trade["s"].toString();  // Symbol
			double price = trade["p"].toDouble();    // Price
			double volume = trade["v"].toDouble();   // Volume
			qint64 timestamp = trade["t"].toVariant().toLongLong();  // Timestamp

			if (!m_marketData.contains(symbol)) {
				continue;
			}

			MarketData* data = m_marketData[symbol];
			data->updateTrade(price, volume);

			emit tradeReceived(symbol, price, volume);
			emit marketDataUpdated(symbol, data);
			m_messagesProcessed++;

			emit logMessage(QString("[FEED] Trade: %1 @ $%2 (Vol: %3)")
				.arg(symbol)
				.arg(price, 0, 'f', 2)
				.arg(volume));
		}
	}
	else {
		// Log unknown message types for debugging
		emit logMessage(QString("[FEED] Unknown message type: %1").arg(type));
	}
}

void MarketDataFeed::processRestApiData(const QByteArray& data)
{
	QJsonDocument doc = QJsonDocument::fromJson(data);
	if (doc.isNull() || !doc.isObject()) {
		return;
	}

	QJsonObject obj = doc.object();

	// Handle Finnhub quote response
	// {"c":183.14,"d":1.24,"dp":0.682,"h":184.50,"l":182.11,"o":182.90,"pc":181.90,"t":1640995200}
	double currentPrice = obj["c"].toDouble();  // Current price
	double change = obj["d"].toDouble();        // Change
	double changePercent = obj["dp"].toDouble(); // Change percent
	double high = obj["h"].toDouble();          // High
	double low = obj["l"].toDouble();           // Low
	double open = obj["o"].toDouble();          // Open
	double previousClose = obj["pc"].toDouble(); // Previous close

	// Extract symbol from the URL (we'll need to pass it through)
	// For now, this will be handled in fetchSnapshotData
}

void MarketDataFeed::fetchSnapshotData(const QString& symbol)
{
	// Finnhub REST API endpoint for quote data
	QString url = QString("%1/quote?symbol=%2&token=%3")
		.arg(m_restApiUrl, symbol, m_finnhubApiKey);

	emit logMessage(QString("[FEED] Fetching snapshot for %1").arg(symbol));

	QNetworkRequest request(url);
	QNetworkReply* reply = m_networkManager->get(request);

	// Store symbol in reply property so we can use it in the callback
	reply->setProperty("symbol", symbol);

	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		if (!reply) return;

		QString symbol = reply->property("symbol").toString();
		reply->deleteLater();

		if (reply->error() != QNetworkReply::NoError) {
			emit logMessage(QString("[FEED] Error fetching %1: %2")
				.arg(symbol, reply->errorString()));
			return;
		}

		QByteArray data = reply->readAll();
		QJsonDocument doc = QJsonDocument::fromJson(data);

		if (doc.isNull() || !doc.isObject()) {
			return;
		}

		QJsonObject obj = doc.object();

		if (!m_marketData.contains(symbol)) {
			return;
		}

		MarketData* marketData = m_marketData[symbol];

		double currentPrice = obj["c"].toDouble();
		double open = obj["o"].toDouble();
		double high = obj["h"].toDouble();
		double low = obj["l"].toDouble();
		double previousClose = obj["pc"].toDouble();

		if (currentPrice > 0) {
			marketData->setOpenPrice(open);
			marketData->setHighPrice(high);
			marketData->setLowPrice(low);
			// setPreviousClose doesn't exist in MarketData - skip it
			marketData->updateTrade(currentPrice, 0);  // Initialize with current price

			emit logMessage(QString("[FEED] Snapshot for %1: $%2 (Open: $%3, High: $%4, Low: $%5)")
				.arg(symbol)
				.arg(currentPrice, 0, 'f', 2)
				.arg(open, 0, 'f', 2)
				.arg(high, 0, 'f', 2)
				.arg(low, 0, 'f', 2));

			emit marketDataUpdated(symbol, marketData);
		}
		});
}

void MarketDataFeed::startSimulation()
{
	emit logMessage("[FEED] Starting market data simulation");
	m_simulationTimer->start();
}

void MarketDataFeed::stopSimulation()
{
	emit logMessage("[FEED] Stopping market data simulation");
	m_simulationTimer->stop();
}

void MarketDataFeed::generateRandomMarketData(const QString& symbol)
{
	if (!m_marketData.contains(symbol)) {
		return;
	}

	MarketData* data = m_marketData[symbol];

	// Generate random price movement
	double lastPrice = data->lastPrice();
	if (lastPrice == 0.0) {
		// Initialize with a base price
		if (symbol == "AAPL") lastPrice = 182.50;
		else if (symbol == "MSFT") lastPrice = 384.90;
		else if (symbol == "GOOGL") lastPrice = 149.34;
		else if (symbol == "TSLA") lastPrice = 253.80;
		else if (symbol == "AMZN") lastPrice = 151.94;
		else if (symbol == "NVDA") lastPrice = 722.48;
		else if (symbol == "META") lastPrice = 434.61;
		else if (symbol == "SPY") lastPrice = 469.50;
		else if (symbol == "QQQ") lastPrice = 395.80;
		else lastPrice = 100.0;

		data->setOpenPrice(lastPrice);
	}

	// Random walk with mean reversion
	double change = (QRandomGenerator::global()->bounded(200) - 100) / 10000.0 * lastPrice;
	double newPrice = lastPrice + change;
	double volume = QRandomGenerator::global()->bounded(1000) + 100;

	data->updateTrade(newPrice, volume);

	// Generate quote data
	double spread = newPrice * 0.001;  // 0.1% spread
	double bid = newPrice - spread / 2.0;
	double ask = newPrice + spread / 2.0;
	double bidSize = QRandomGenerator::global()->bounded(500) + 100;
	double askSize = QRandomGenerator::global()->bounded(500) + 100;

	data->updateQuote(bid, bidSize, ask, askSize);

	emit marketDataUpdated(symbol, data);
	emit tradeReceived(symbol, newPrice, volume);
}

void MarketDataFeed::setFinnhubApiKey(const QString& apiKey)
{
	m_finnhubApiKey = apiKey;
}