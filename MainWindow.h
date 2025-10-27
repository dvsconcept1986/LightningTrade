#pragma once
#include <QMainWindow>
#include <QWidget>
#include <QSplitter>
#include <QGroupBox>
#include <QTableWidget>
#include <QListWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTabWidget>
#include "OrderManager.h"
#include "OrderEntryWidget.h"
#include "OrderBlotterWidget.h"
#include "MarketDataFeed.h"
#include "AuthManager.h"
#include "AccountWidget.h"
#include "LoginDialog.h"
#include "StockTickerWidget.h"

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow();

private slots:
	// Order management slots
	void handleOrderRequest(const QString& symbol, OrderSide side,
		OrderType type, double quantity, double price);
	void handleCancelRequest(const QString& orderId);
	void handleModifyRequest(const QString& orderId);
	void onOrderStatusChanged(const QString& orderId, OrderStatus status);
	void onOrderManagerLog(const QString& message);

	// Market data slots
	void onMarketDataUpdated(const QString& symbol, MarketData* data);

	// UI interaction slots
	void refreshOrderBlotter();
	void refreshMarketData();
	void showAbout();

	// News slots
	void onNewsReplyFinished();
	void onPriceReplyFinished();
	void onNewsItemClicked(QListWidgetItem* item);

	// Account slots
	void onAccountDeposit(double amount);
	void onAccountWithdrawal(double amount);

	// Auth slots
	void onLogout();

	// Stock ticker slot
	void onTickerSymbolClicked(const QString& symbol);

private:
	void setupMenuBar();
	void setupUI();
	void setupMarketDataArea();
	void setupNewsArea();
	void setupOrderManagementArea();
	void setupTradingArea();
	void setupStatusBar();
	void setupStockTicker();

	void loadMarketNews();
	void loadMarketPrices();
	void updateNewsDisplay(const QJsonArray& articles);
	void updatePriceDisplay(const QJsonObject& data);
	void updateAccountPositions();
	void loadDemoPrices();

private:
	// Central widget and layout
	QWidget* m_centralWidget;
	QSplitter* m_mainSplitter;
	QSplitter* m_rightSplitter;

	// Market data area
	QGroupBox* m_marketDataGroup;
	QTableWidget* m_priceTable;
	QPushButton* m_refreshButton;

	// News area
	QGroupBox* m_newsGroup;
	QListWidget* m_newsList;

	// Order management area
	QTabWidget* m_tradingTabs;
	OrderEntryWidget* m_orderEntryWidget;
	OrderBlotterWidget* m_orderBlotterWidget;
	AccountWidget* m_accountWidget;

	// Trading area (system log)
	QGroupBox* m_tradingGroup;
	QTextEdit* m_orderBlotter;

	// Status bar
	QLabel* m_statusLabel;
	QLabel* m_orderStatsLabel;
	QLabel* m_clockLabel;

	// Network
	QNetworkAccessManager* m_networkManager;

	// Timers
	QTimer* m_refreshTimer;
	QTimer* m_clockTimer;

	// Order management
	OrderManager* m_orderManager;

	// Market data feed
	MarketDataFeed* m_marketDataFeed;

	// Authentication
	AuthManager* m_authManager;
	UserAccount* m_userAccount;

	// Stock ticker
	StockTickerWidget* m_stockTicker;
};