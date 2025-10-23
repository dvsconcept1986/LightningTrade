#pragma once
#include <QtWidgets/QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QStatusBar>
#include <QTimer>
#include <QGroupBox>
#include <QListWidget>
#include <QTableWidget>
#include <QSplitter>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTabWidget>
#include "ui_MainWindow.h"
#include "OrderManager.h"
#include "OrderEntryWidget.h"
#include "OrderBlotterWidget.h"
#include "MarketDataFeed.h"
#include "MarketDataWidget.h"
#include "UserAccount.h"
#include "AccountWidget.h"
#include "AuthManager.h"
#include "LoginDialog.h"

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget* parent = nullptr);
	~MainWindow();

private slots:
	void refreshMarketData();
	void onNewsReplyFinished();
	void onPriceReplyFinished();
	void showAbout();

	// OMS slots
	void handleOrderRequest(const QString& symbol, OrderSide side, OrderType type,
		double quantity, double price);
	void handleCancelRequest(const QString& orderId);
	void handleModifyRequest(const QString& orderId);
	void onOrderStatusChanged(const QString& orderId, OrderStatus status);
	void onOrderManagerLog(const QString& message);
	void refreshOrderBlotter();

	// Market data slots
	void onMarketDataUpdated(const QString& symbol, MarketData* data);

	void onAccountDeposit(double amount);
	void onAccountWithdrawal(double amount);
	void updateAccountPositions();

	void onLogout();

private:
	void setupUI();
	void setupMenuBar();
	void setupStatusBar();
	void setupMarketDataArea();
	void setupNewsArea();
	void setupTradingArea();
	void setupOrderManagementArea();

	void loadMarketNews();
	void loadMarketPrices();
	void updatePriceDisplay(const QJsonObject& data);
	void updateNewsDisplay(const QJsonArray& articles);
	void loadDemoNews();
	void loadDemoPrices();

private:
	Ui::MainWindowClass ui;

	// Central widget and layouts
	QWidget* m_centralWidget;
	QSplitter* m_mainSplitter;
	QSplitter* m_rightSplitter;

	// Market data widgets
	QGroupBox* m_marketDataGroup;
	QTableWidget* m_priceTable;
	QPushButton* m_refreshButton;

	// News widgets
	QGroupBox* m_newsGroup;
	QListWidget* m_newsList;

	// Trading widgets  
	QGroupBox* m_tradingGroup;
	QTextEdit* m_orderBlotter;

	// OMS widgets
	QTabWidget* m_tradingTabs;
	OrderEntryWidget* m_orderEntryWidget;
	OrderBlotterWidget* m_orderBlotterWidget;

	// Market Data Feed
	MarketDataFeed* m_marketDataFeed;
	MarketDataWidget* m_marketDataWidget;

	// Network
	QNetworkAccessManager* m_networkManager;

	// Timers
	QTimer* m_refreshTimer;
	QTimer* m_clockTimer;

	// Status
	QLabel* m_statusLabel;
	QLabel* m_clockLabel;
	QLabel* m_orderStatsLabel;

	// Order Management System
	OrderManager* m_orderManager;

private:
	UserAccount* m_userAccount;
	AccountWidget* m_accountWidget;
	AuthManager* m_authManager;
};