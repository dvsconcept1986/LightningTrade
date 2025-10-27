#pragma once
#include <QObject>
#include <QMap>
#include <QTimer>
#include <QWebSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "MarketData.h"

enum class FeedStatus {
	Disconnected,
	Connecting,
	Connected,
	Reconnecting,
	Error
};

class MarketDataFeed : public QObject
{
	Q_OBJECT

public:
	explicit MarketDataFeed(QObject* parent = nullptr);
	~MarketDataFeed();

	// Connection management
	void connectToFeed();
	void disconnectFromFeed();
	bool isConnected() const { return m_status == FeedStatus::Connected; }
	FeedStatus status() const { return m_status; }

	// Subscription management
	void subscribe(const QString& symbol);
	void unsubscribe(const QString& symbol);
	void subscribeMultiple(const QStringList& symbols);
	QStringList getSubscribedSymbols() const;

	// Data access
	MarketData* getMarketData(const QString& symbol);
	QList<MarketData*> getAllMarketData() const;

	// Configuration
	void setUpdateInterval(int milliseconds) { m_updateInterval = milliseconds; }
	void setWebSocketUrl(const QString& url) { m_webSocketUrl = url; }
	void setRestApiUrl(const QString& url) { m_restApiUrl = url; }
	void setFinnhubApiKey(const QString& apiKey);  // NEW: Set API key

signals:
	// Connection signals
	void connected();
	void disconnected();
	void connectionError(const QString& error);
	void statusChanged(FeedStatus status);

	// Data signals
	void marketDataUpdated(const QString& symbol, MarketData* data);
	void tradeReceived(const QString& symbol, double price, double volume);
	void quoteReceived(const QString& symbol, double bid, double ask);

	// System signals
	void logMessage(const QString& message);

private slots:
	void onWebSocketConnected();
	void onWebSocketDisconnected();
	void onWebSocketError(QAbstractSocket::SocketError error);
	void onWebSocketTextMessageReceived(const QString& message);
	void onWebSocketBinaryMessageReceived(const QByteArray& message);
	void onRestApiReplyFinished();
	void simulateMarketData();

private:
	void setStatus(FeedStatus status);
	void processWebSocketMessage(const QString& message);
	void processRestApiData(const QByteArray& data);
	void fetchSnapshotData(const QString& symbol);
	void startSimulation();
	void stopSimulation();
	void generateRandomMarketData(const QString& symbol);

private:
	// Connection
	QWebSocket* m_webSocket;
	QNetworkAccessManager* m_networkManager;
	FeedStatus m_status;

	// Configuration
	QString m_webSocketUrl;
	QString m_restApiUrl;
	int m_updateInterval;
	bool m_useSimulation;
	QString m_finnhubApiKey;  // NEW: Store Finnhub API key

	// Data storage
	QMap<QString, MarketData*> m_marketData;
	QStringList m_subscribedSymbols;

	// Timers
	QTimer* m_reconnectTimer;
	QTimer* m_heartbeatTimer;
	QTimer* m_simulationTimer;

	// Statistics
	int m_messagesReceived;
	int m_messagesProcessed;
	QDateTime m_lastMessageTime;
};