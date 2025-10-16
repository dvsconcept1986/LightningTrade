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
	, m_webSocketUrl("wss://stream.example.com/market")
	, m_restApiUrl("https://api.example.com/market")
	, m_updateInterval(1000)
	, m_useSimulation(true)  // Default to simulation mode
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
			m_webSocket->sendTextMessage("{\"type\":\"heartbeat\"}");
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

	emit logMessage("[FEED] Connecting to market data feed...");
	setStatus(FeedStatus::Connecting);

	if (m_useSimulation) {
		// Use simulation mode
		emit logMessage("[FEED] Using simulation mode");
		startSimulation();
		setStatus(FeedStatus::Connected);
		emit connected();
	}
	else {
		// Try real WebSocket connection
		m_webSocket->open(QUrl(m_webSocketUrl));
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

	// Send subscription message if connected
	if (m_status == FeedStatus::Connected && !m_useSimulation) {
		QJsonObject msg;
		msg["type"] = "subscribe";
		msg["symbol"] = symbol;
		m_webSocket->sendTextMessage(QJsonDocument(msg).toJson(QJsonDocument::Compact));
	}

	// Fetch initial snapshot
	if (!m_useSimulation) {
		fetchSnapshotData(symbol);
	}
}

void MarketDataFeed::unsubscribe(const QString& symbol)
{
	m_subscribedSymbols.removeAll(symbol);

	emit logMessage(QString("[FEED] Unsubscribed from %1").arg(symbol));

	// Send unsubscribe message if connected
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
	emit logMessage("[FEED] WebSocket connected");
	setStatus(FeedStatus::Connected);
	m_heartbeatTimer->start();
	emit connected();

	// Resubscribe to all symbols
	for (const QString& symbol : m_subscribedSymbols) {
		QJsonObject msg;
		msg["type"] = "subscribe";
		msg["symbol"] = symbol;
		m_webSocket->sendTextMessage(QJsonDocument(msg).toJson(QJsonDocument::Compact));
	}
}

void MarketDataFeed::onWebSocketDisconnected()
{
	emit logMessage("[FEED] WebSocket disconnected");
	m_heartbeatTimer->stop();
	setStatus(FeedStatus::Disconnected);
	emit disconnected();

	// Attempt reconnection
	if (!m_useSimulation) {
		setStatus(FeedStatus::Reconnecting);
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
		emit logMessage("[FEED] Falling back to simulation mode");
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
	// Handle binary protocol if needed
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
	QString symbol = obj["symbol"].toString();

	if (!m_marketData.contains(symbol)) {
		return;
	}

	MarketData* data = m_marketData[symbol];

	if (type == "trade") {
		double price = obj["price"].toDouble();
		double volume = obj["volume"].toDouble();
		data->updateTrade(price, volume);
		emit tradeReceived(symbol, price, volume);
		emit marketDataUpdated(symbol, data);
		m_messagesProcessed++;
	}
	else if (type == "quote") {
		double bid = obj["bid"].toDouble();
		double ask = obj["ask"].toDouble();
		double bidSize = obj["bidSize"].toDouble();
		double askSize = obj["askSize"].toDouble();
		data->updateQuote(bid, bidSize, ask, askSize);
		emit quoteReceived(symbol, bid, ask);
		emit marketDataUpdated(symbol, data);
		m_messagesProcessed++;
	}
}

void MarketDataFeed::processRestApiData(const QByteArray& data)
{
	QJsonDocument doc = QJsonDocument::fromJson(data);
	if (doc.isNull()) return;

	// Process REST API response (implementation depends on API format)
}

void MarketDataFeed::fetchSnapshotData(const QString& symbol)
{
	QString url = QString("%1/snapshot/%2").arg(m_restApiUrl).arg(symbol);
	QNetworkRequest request(url);
	QNetworkReply* reply = m_networkManager->get(request);
	connect(reply, &QNetworkReply::finished, this, &MarketDataFeed::onRestApiReplyFinished);
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