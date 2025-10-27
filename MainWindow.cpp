#include "MainWindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QMenuBar>
#include <QHeaderView>
#include <QDateTime>
#include <QNetworkRequest>
#include <QUrl>
#include <QDesktopServices>
#include <QStatusBar>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>
#include "StockTickerWidget.h"

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
	, m_centralWidget(nullptr)
	, m_networkManager(new QNetworkAccessManager(this))
	, m_refreshTimer(new QTimer(this))
	, m_clockTimer(new QTimer(this))
	, m_orderManager(new OrderManager(this))
	, m_marketDataFeed(new MarketDataFeed(this))
	, m_authManager(new AuthManager(this))
	, m_userAccount(new UserAccount("trader001", "John Doe", "john@example.com"))
	, m_stockTicker(nullptr)
{
	// Show login dialog BEFORE setting up UI
	LoginDialog loginDialog(m_authManager, this);
	if (loginDialog.exec() != QDialog::Accepted) {
		// User cancelled login - exit application
		QTimer::singleShot(0, qApp, &QApplication::quit);
		return;
	}

	// Get the authenticated user's account
	m_userAccount = m_authManager->getCurrentUser();

	if (!m_userAccount) {
		QMessageBox::critical(this, "Error", "Failed to load user account");
		QTimer::singleShot(0, qApp, &QApplication::quit);
		return;
	}

	// Rest of your initialization code...
	setupMenuBar();
	setupUI();
	setupStatusBar();
	setupStockTicker();  // Setup the rolling stock ticker

	// Connect market data signals
	connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::refreshMarketData);
	connect(m_refreshTimer, &QTimer::timeout, this, &MainWindow::refreshMarketData);
	connect(m_clockTimer, &QTimer::timeout, this, [this]() {
		m_clockLabel->setText(QDateTime::currentDateTime().toString("hh:mm:ss AP"));
		});

	// Connect OMS signals
	connect(m_orderEntryWidget, &OrderEntryWidget::orderRequested,
		this, &MainWindow::handleOrderRequest);
	connect(m_orderBlotterWidget, &OrderBlotterWidget::cancelOrderRequested,
		this, &MainWindow::handleCancelRequest);
	connect(m_orderBlotterWidget, &OrderBlotterWidget::modifyOrderRequested,
		this, &MainWindow::handleModifyRequest);

	connect(m_orderManager, &OrderManager::orderStatusChanged,
		this, &MainWindow::onOrderStatusChanged);
	connect(m_orderManager, &OrderManager::logMessage,
		this, &MainWindow::onOrderManagerLog);

	// Connect market data feed signals
	connect(m_marketDataFeed, &MarketDataFeed::marketDataUpdated,
		this, &MainWindow::onMarketDataUpdated);
	connect(m_marketDataFeed, &MarketDataFeed::logMessage,
		this, &MainWindow::onOrderManagerLog);

	// Start timers
	m_refreshTimer->start(60000); // Refresh news every minute
	m_clockTimer->start(1000);    // Update clock every second

	// Auto-start market data feed
	QStringList defaultSymbols = { "AAPL", "MSFT", "GOOGL", "TSLA", "AMZN", "NVDA", "META", "SPY", "QQQ" };
	for (const QString& symbol : defaultSymbols) {
		m_marketDataFeed->subscribe(symbol);
	}
	m_marketDataFeed->connectToFeed();

	// Load initial data
	refreshMarketData();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupMenuBar()
{
	// File Menu
	QMenu* fileMenu = menuBar()->addMenu("&File");
	fileMenu->addAction("&Logout", this, &MainWindow::onLogout);
	fileMenu->addSeparator();
	fileMenu->addAction("&Exit", QKeySequence::Quit, this, &QWidget::close);

	// View Menu
	QMenu* viewMenu = menuBar()->addMenu("&View");
	viewMenu->addAction("&Refresh Market Data", QKeySequence::Refresh,
		this, &MainWindow::refreshMarketData);
	viewMenu->addAction("Refresh &Orders", this, &MainWindow::refreshOrderBlotter);

	// Trading Menu
	QMenu* tradingMenu = menuBar()->addMenu("&Trading");
	tradingMenu->addAction("&New Order", this, [this]() {
		m_tradingTabs->setCurrentIndex(0);
		});
	tradingMenu->addAction("Order &Blotter", this, [this]() {
		m_tradingTabs->setCurrentIndex(1);
		});

	// Help Menu
	QMenu* helpMenu = menuBar()->addMenu("&Help");
	helpMenu->addAction("&About", this, &MainWindow::showAbout);
}

void MainWindow::setupUI()
{
	m_centralWidget = new QWidget(this);
	setCentralWidget(m_centralWidget);

	// Main splitter (horizontal)
	m_mainSplitter = new QSplitter(Qt::Horizontal, m_centralWidget);

	// Right splitter (vertical)
	m_rightSplitter = new QSplitter(Qt::Vertical, m_mainSplitter);

	setupMarketDataArea();
	setupNewsArea();
	setupOrderManagementArea();
	setupTradingArea();

	// Add to splitters
	m_mainSplitter->addWidget(m_marketDataGroup);
	m_rightSplitter->addWidget(m_newsGroup);
	m_rightSplitter->addWidget(m_tradingTabs);
	m_rightSplitter->addWidget(m_tradingGroup);
	m_mainSplitter->addWidget(m_rightSplitter);

	// Set splitter proportions
	m_mainSplitter->setSizes({ 400, 800 });
	m_rightSplitter->setSizes({ 200, 300, 200 });

	// Main layout
	QHBoxLayout* mainLayout = new QHBoxLayout(m_centralWidget);
	mainLayout->addWidget(m_mainSplitter);
	mainLayout->setContentsMargins(5, 5, 5, 5);

	// Window properties
	setWindowTitle("Lightning Trade - Order Management System");
	setMinimumSize(1200, 800);
	resize(1600, 1000);
}

void MainWindow::setupMarketDataArea()
{
	m_marketDataGroup = new QGroupBox("Market Data", this);

	QVBoxLayout* layout = new QVBoxLayout(m_marketDataGroup);

	// Refresh button
	m_refreshButton = new QPushButton("Refresh Data", m_marketDataGroup);
	m_refreshButton->setMaximumWidth(120);

	// Price table
	m_priceTable = new QTableWidget(0, 5, m_marketDataGroup);
	m_priceTable->setHorizontalHeaderLabels({ "Symbol", "Price", "Change", "Change %", "Volume" });
	m_priceTable->horizontalHeader()->setStretchLastSection(true);
	m_priceTable->setAlternatingRowColors(true);
	m_priceTable->setSelectionBehavior(QAbstractItemView::SelectRows);

	layout->addWidget(m_refreshButton, 0, Qt::AlignLeft);
	layout->addWidget(m_priceTable);
}

void MainWindow::setupNewsArea()
{
	m_newsGroup = new QGroupBox("Market News - Finnhub", this);

	QVBoxLayout* layout = new QVBoxLayout(m_newsGroup);

	m_newsList = new QListWidget(m_newsGroup);
	m_newsList->setAlternatingRowColors(true);
	m_newsList->setCursor(Qt::PointingHandCursor);

	// Connect click handler to open articles in browser
	connect(m_newsList, &QListWidget::itemClicked,
		this, &MainWindow::onNewsItemClicked);

	layout->addWidget(m_newsList);
}

void MainWindow::setupOrderManagementArea()
{
	m_tradingTabs = new QTabWidget(this);

	// Order Entry Tab
	m_orderEntryWidget = new OrderEntryWidget(this);
	m_tradingTabs->addTab(m_orderEntryWidget, "Order Entry");

	// Order Blotter Tab
	m_orderBlotterWidget = new OrderBlotterWidget(this);
	m_tradingTabs->addTab(m_orderBlotterWidget, "Order Blotter");

	// Account Tab
	m_accountWidget = new AccountWidget(m_userAccount, this);
	m_tradingTabs->addTab(m_accountWidget, "Account");

	// Connect signals
	connect(m_accountWidget, &AccountWidget::depositRequested,
		this, &MainWindow::onAccountDeposit);
	connect(m_accountWidget, &AccountWidget::withdrawalRequested,
		this, &MainWindow::onAccountWithdrawal);
}

void MainWindow::setupTradingArea()
{
	m_tradingGroup = new QGroupBox("System Log", this);

	QVBoxLayout* layout = new QVBoxLayout(m_tradingGroup);

	m_orderBlotter = new QTextEdit(m_tradingGroup);
	m_orderBlotter->setReadOnly(true);
	m_orderBlotter->setMaximumHeight(150);
	m_orderBlotter->setPlainText("Lightning Trade System Ready\n"
		"Order Management System Initialized\n"
		"Connecting to market data feeds...\n");

	layout->addWidget(m_orderBlotter);
}

void MainWindow::setupStatusBar()
{
	m_statusLabel = new QLabel("Ready", this);
	m_orderStatsLabel = new QLabel("Orders: 0 Active | 0 Total", this);
	m_clockLabel = new QLabel(QDateTime::currentDateTime().toString("hh:mm:ss AP"), this);

	statusBar()->addWidget(m_statusLabel);
	statusBar()->addWidget(m_orderStatsLabel, 1);
	statusBar()->addPermanentWidget(m_clockLabel);
}

void MainWindow::handleOrderRequest(const QString& symbol, OrderSide side,
	OrderType type, double quantity, double price)
{
	// Check if user has sufficient funds
	double cost = quantity * price;

	if (side == OrderSide::Buy && cost > m_userAccount->cashBalance()) {
		QMessageBox::warning(this, "Insufficient Funds",
			QString("Insufficient cash. Required: $%1, Available: $%2")
			.arg(cost, 0, 'f', 2)
			.arg(m_userAccount->cashBalance(), 0, 'f', 2));
		return;
	}

	// Submit order
	QString orderId = m_orderManager->submitOrder(symbol, side, type, quantity, price);

	if (!orderId.isEmpty()) {
		// Deduct cash for buy orders
		if (side == OrderSide::Buy) {
			m_userAccount->addPosition(symbol, quantity, price);
		}

		m_orderBlotter->append(QString("[%1] Order submitted: %2 %3 %4 @ %5")
			.arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
			.arg(Order::sideToString(side))
			.arg(quantity)
			.arg(symbol)
			.arg(price));

		refreshOrderBlotter();
		m_accountWidget->updateDisplay();
	}
}

void MainWindow::handleCancelRequest(const QString& orderId)
{
	if (m_orderManager->cancelOrder(orderId)) {
		m_orderBlotter->append(QString("[%1] Cancel request sent for order: %2")
			.arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
			.arg(orderId.left(8)));
	}
}

void MainWindow::handleModifyRequest(const QString& orderId)
{
	// For now, just log - full modify dialog can be added later
	m_orderBlotter->append(QString("[%1] Modify requested for order: %2")
		.arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
		.arg(orderId.left(8)));
}

void MainWindow::onOrderStatusChanged(const QString& orderId, OrderStatus status)
{
	Order* order = m_orderManager->getOrder(orderId);
	if (order) {
		m_orderBlotterWidget->updateOrder(order);

		// Update statistics
		int activeCount = m_orderManager->getActiveOrderCount();
		int totalCount = m_orderManager->getTotalOrderCount();
		m_orderStatsLabel->setText(QString("Orders: %1 Active | %2 Total")
			.arg(activeCount).arg(totalCount));
	}
}

void MainWindow::onOrderManagerLog(const QString& message)
{
	m_orderBlotter->append(QString("[%1] %2")
		.arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
		.arg(message));
}

void MainWindow::onMarketDataUpdated(const QString& symbol, MarketData* data)
{
	if (!data) return;

	// Find or create row for this symbol
	int row = -1;
	for (int i = 0; i < m_priceTable->rowCount(); ++i) {
		QTableWidgetItem* item = m_priceTable->item(i, 0);
		if (item && item->text() == symbol) {
			row = i;
			break;
		}
	}

	// Create new row if not found
	if (row < 0) {
		row = m_priceTable->rowCount();
		m_priceTable->insertRow(row);
		m_priceTable->setItem(row, 0, new QTableWidgetItem(symbol));
	}

	// Update price
	double price = data->lastPrice();
	QTableWidgetItem* priceItem = new QTableWidgetItem(QString("$%1").arg(price, 0, 'f', 2));

	// Update change
	double change = data->changeAmount();
	QString changeStr = QString("%1%2").arg(change >= 0 ? "+" : "").arg(change, 0, 'f', 2);
	QTableWidgetItem* changeItem = new QTableWidgetItem(changeStr);

	// Update change %
	double changePercent = data->changePercent();
	QString percentStr = QString("%1%2%").arg(changePercent >= 0 ? "+" : "").arg(changePercent, 0, 'f', 2);
	QTableWidgetItem* percentItem = new QTableWidgetItem(percentStr);

	// Color code based on change
	QColor color;
	if (change > 0) {
		color = QColor(0, 200, 0);  // Green
	}
	else if (change < 0) {
		color = QColor(255, 100, 100);  // Red
	}
	else {
		color = QColor(255, 255, 255);  // White
	}

	priceItem->setForeground(color);
	changeItem->setForeground(color);
	percentItem->setForeground(color);

	m_priceTable->setItem(row, 1, priceItem);
	m_priceTable->setItem(row, 2, changeItem);
	m_priceTable->setItem(row, 3, percentItem);

	// Update volume
	double volume = data->totalVolume();
	QString volumeStr;
	if (volume >= 1000000) {
		volumeStr = QString("%1M").arg(volume / 1000000.0, 0, 'f', 2);
	}
	else if (volume >= 1000) {
		volumeStr = QString("%1K").arg(volume / 1000.0, 0, 'f', 1);
	}
	else {
		volumeStr = QString::number(volume, 'f', 0);
	}
	m_priceTable->setItem(row, 4, new QTableWidgetItem(volumeStr));

	// Update account position prices
	if (m_userAccount->hasPosition(symbol)) {
		m_userAccount->updatePositionPrice(symbol, data->lastPrice());
		m_accountWidget->updatePositions();
	}
}

void MainWindow::refreshOrderBlotter()
{
	m_orderBlotterWidget->clearOrders();

	QList<Order*> orders = m_orderManager->getAllOrders();
	for (Order* order : orders) {
		m_orderBlotterWidget->addOrder(order);
	}

	m_orderBlotter->append(QString("[%1] Order blotter refreshed (%2 orders)")
		.arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
		.arg(orders.size()));
}

void MainWindow::refreshMarketData()
{
	m_statusLabel->setText("Refreshing market data...");
	m_refreshButton->setEnabled(false);

	loadMarketNews();

	m_refreshButton->setEnabled(true);
	m_statusLabel->setText("Market data updated");
}

void MainWindow::loadMarketNews()
{
	QString apiKey = "d3vbvs9r01qt2ctp2tugd3vbvs9r01qt2ctp2tv0";

	QString url = QString("https://finnhub.io/api/v1/news?category=general&token=%1").arg(apiKey);

	QNetworkRequest request;
	request.setUrl(QUrl(url));
	request.setRawHeader("User-Agent", "Lightning Trade/1.0");

	QNetworkReply* reply = m_networkManager->get(request);
	connect(reply, &QNetworkReply::finished, this, &MainWindow::onNewsReplyFinished);
}

void MainWindow::loadMarketPrices()
{
	// This is now handled by the MarketDataFeed
}

void MainWindow::onNewsReplyFinished()
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
	if (!reply) return;

	reply->deleteLater();

	if (reply->error() != QNetworkReply::NoError) {
		qDebug() << "Finnhub API Error:" << reply->errorString();

		m_newsList->clear();
		QListWidgetItem* errorItem = new QListWidgetItem("❌ Failed to load news from Finnhub");
		errorItem->setForeground(QColor("#ff6464"));
		m_newsList->addItem(errorItem);

		QListWidgetItem* errorDetail = new QListWidgetItem(
			QString("Error: %1").arg(reply->errorString()));
		errorDetail->setForeground(QColor("#888"));
		m_newsList->addItem(errorDetail);

		m_refreshButton->setEnabled(true);
		m_statusLabel->setText("News feed unavailable");
		return;
	}

	QByteArray data = reply->readAll();
	QJsonDocument doc = QJsonDocument::fromJson(data);

	if (doc.isArray()) {
		QJsonArray articles = doc.array();
		if (articles.size() > 0) {
			updateNewsDisplay(articles);
			m_statusLabel->setText("Market data updated");
		}
		else {
			m_newsList->clear();
			QListWidgetItem* noNewsItem = new QListWidgetItem("📰 No news articles available");
			noNewsItem->setForeground(QColor("#888"));
			m_newsList->addItem(noNewsItem);
			m_statusLabel->setText("No news available");
		}
	}
	else {
		m_newsList->clear();
		QListWidgetItem* errorItem = new QListWidgetItem("❌ Invalid response from Finnhub");
		errorItem->setForeground(QColor("#ff6464"));
		m_newsList->addItem(errorItem);
		m_statusLabel->setText("News feed error");
	}

	m_refreshButton->setEnabled(true);
	m_statusLabel->setText("Market data updated");
}

void MainWindow::onPriceReplyFinished()
{
	// Not used anymore - prices come from MarketDataFeed
}

void MainWindow::updateNewsDisplay(const QJsonArray& articles)
{
	m_newsList->clear();

	int maxItems = std::min(20, static_cast<int>(articles.size()));

	for (int i = 0; i < maxItems; ++i) {
		QJsonObject article = articles[i].toObject();

		QString headline = article["headline"].toString();
		QString url = article["url"].toString();
		QString source = article["source"].toString();
		qint64 timestamp = article["datetime"].toVariant().toLongLong();

		if (!headline.isEmpty() && !url.isEmpty()) {
			QString displayText = QString("📰 [%1] %2").arg(source, headline);

			QListWidgetItem* item = new QListWidgetItem(displayText);
			item->setData(Qt::UserRole, url);

			QDateTime dt = QDateTime::fromSecsSinceEpoch(timestamp);
			item->setToolTip(QString("%1\n\nSource: %2\nPublished: %3\n\nClick to open in browser")
				.arg(headline, source, dt.toString("MMM dd, yyyy hh:mm AP")));

			QFont font = item->font();
			font.setUnderline(true);
			item->setFont(font);
			item->setForeground(QColor("#2a82da"));

			m_newsList->addItem(item);
		}
	}

	if (m_newsList->count() == 0) {
		QListWidgetItem* noNewsItem = new QListWidgetItem("📰 No valid news articles found");
		noNewsItem->setForeground(QColor("#888"));
		m_newsList->addItem(noNewsItem);
	}
}

void MainWindow::updatePriceDisplay(const QJsonObject& data)
{
	// Not used anymore - prices come from MarketDataFeed
}

void MainWindow::onNewsItemClicked(QListWidgetItem* item)
{
	if (!item) return;

	QString url = item->data(Qt::UserRole).toString();

	if (!url.isEmpty()) {
		bool success = QDesktopServices::openUrl(QUrl(url));

		if (success) {
			m_orderBlotter->append(QString("[%1] Opened news article in browser")
				.arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
		}
		else {
			QMessageBox::warning(this, "Error",
				"Failed to open URL in default browser.\n\nURL: " + url);
		}
	}
}

void MainWindow::showAbout()
{
	QMessageBox::about(this, "About Lightning Trade",
		"<h3>Lightning Trade</h3>"
		"<p>Version 1.0.0</p>"
		"<p>Ultra-low latency trading platform for institutional use.</p>"
		"<p>Built with Qt 6 and Visual Studio 2022</p>"
		"<p><b>Features:</b></p>"
		"<ul>"
		"<li>Order Management System</li>"
		"<li>Real-time Market Data</li>"
		"<li>Risk Management</li>"
		"<li>Trade Execution</li>"
		"<li>Live News Feed (Finnhub)</li>"
		"</ul>");
}

void MainWindow::onAccountDeposit(double amount)
{
	m_orderBlotter->append(QString("[%1] Account deposit: $%2")
		.arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
		.arg(amount, 0, 'f', 2));
}

void MainWindow::onAccountWithdrawal(double amount)
{
	m_orderBlotter->append(QString("[%1] Account withdrawal: $%2")
		.arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
		.arg(amount, 0, 'f', 2));
}

void MainWindow::updateAccountPositions()
{
	// Update position prices with current market data
	QList<Position> positions = m_userAccount->getAllPositions();
	for (const Position& pos : positions) {
		MarketData* data = m_marketDataFeed->getMarketData(pos.symbol());
		if (data && data->lastPrice() > 0) {
			m_userAccount->updatePositionPrice(pos.symbol(), data->lastPrice());
		}
	}
	m_accountWidget->updateDisplay();
}

void MainWindow::onLogout()
{
	QMessageBox::StandardButton reply = QMessageBox::question(
		this, "Logout",
		"Are you sure you want to logout?",
		QMessageBox::Yes | QMessageBox::No
	);

	if (reply == QMessageBox::Yes) {
		qDebug() << "User confirmed logout";

		// Perform logout
		m_authManager->logout();

		// Hide the main window during re-authentication
		hide();

		// Show login dialog again
		LoginDialog loginDialog(m_authManager, nullptr);

		if (loginDialog.exec() != QDialog::Accepted) {
			qDebug() << "User cancelled re-login after logout - closing application";
			QApplication::quit();
			return;
		}

		// Get new user account
		m_userAccount = m_authManager->getCurrentUser();

		if (!m_userAccount) {
			QMessageBox::critical(nullptr, "Error", "Failed to load user account");
			QApplication::quit();
			return;
		}

		// Update window title with new username
		setWindowTitle(QString("Lightning Trade - %1").arg(m_authManager->getCurrentUsername()));

		// Recreate the account widget with new user data
		int accountTabIndex = m_tradingTabs->indexOf(m_accountWidget);
		if (accountTabIndex != -1) {
			m_tradingTabs->removeTab(accountTabIndex);
		}
		delete m_accountWidget;

		// Create new account widget with new user
		m_accountWidget = new AccountWidget(m_userAccount, this);
		m_tradingTabs->insertTab(accountTabIndex, m_accountWidget, "Account");

		// Reconnect signals
		connect(m_accountWidget, &AccountWidget::depositRequested,
			this, &MainWindow::onAccountDeposit);
		connect(m_accountWidget, &AccountWidget::withdrawalRequested,
			this, &MainWindow::onAccountWithdrawal);

		// Clear order blotter for new user
		m_orderBlotterWidget->clearOrders();

		// Log the new login
		m_orderBlotter->clear();
		m_orderBlotter->append("Lightning Trade System Ready");
		m_orderBlotter->append("Order Management System Initialized");
		m_orderBlotter->append("Connecting to market data feeds...");
		m_orderBlotter->append(QString("[%1] User %2 logged in")
			.arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
			.arg(m_authManager->getCurrentUsername()));

		// Show the window again
		show();

		qDebug() << "User re-logged in successfully:" << m_authManager->getCurrentUsername();
	}
}

// Replace your setupStockTicker() with this version
// This uses the REAL StockTickerWidget class (now that it's fixed)

void MainWindow::setupStockTicker()
{
	qDebug() << "=== SETTING UP STOCK TICKER (REAL WIDGET) ===";

	m_stockTicker = new StockTickerWidget(m_marketDataFeed, this);

	qDebug() << "Ticker widget created:" << m_stockTicker;
	qDebug() << "Ticker visible:" << m_stockTicker->isVisible();
	qDebug() << "Ticker size:" << m_stockTicker->size();

	// Add stocks
	QStringList tickerSymbols = {
		"AAPL", "MSFT", "GOOGL", "AMZN", "TSLA", "META", "NVDA",
		"JPM", "BAC", "V", "MA", "WMT", "JNJ", "PG", "HD"
	};

	for (const QString& symbol : tickerSymbols) {
		m_stockTicker->addSymbol(symbol);
	}

	qDebug() << "Added" << tickerSymbols.size() << "symbols";

	// Connect click handler
	connect(m_stockTicker, &StockTickerWidget::symbolClicked,
		this, &MainWindow::onTickerSymbolClicked);

	// Get the current central widget
	QWidget* oldCentralWidget = m_centralWidget;

	if (!oldCentralWidget) {
		qDebug() << "ERROR: m_centralWidget is NULL!";
		m_orderBlotter->append("[ERROR] Cannot add ticker - central widget is NULL");
		return;
	}

	// Create new container widget
	QWidget* newCentralWidget = new QWidget(this);
	QVBoxLayout* mainLayout = new QVBoxLayout(newCentralWidget);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);

	// Add ticker at top, then the original content below
	mainLayout->addWidget(m_stockTicker);
	mainLayout->addWidget(oldCentralWidget);

	// Set as new central widget
	setCentralWidget(newCentralWidget);
	m_centralWidget = newCentralWidget;

	// Force visibility
	m_stockTicker->show();
	newCentralWidget->show();
	oldCentralWidget->show();

	qDebug() << "After setup:";
	qDebug() << "  Ticker visible:" << m_stockTicker->isVisible();
	qDebug() << "  Ticker size:" << m_stockTicker->size();
	qDebug() << "  Ticker width:" << m_stockTicker->width();
	qDebug() << "  Ticker height:" << m_stockTicker->height();

	m_orderBlotter->append(QString("[%1] Stock ticker initialized with %2 symbols")
		.arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
		.arg(tickerSymbols.size()));

	qDebug() << "=== TICKER SETUP COMPLETE ===";
}

//void MainWindow::setupStockTicker()
//{
//	qDebug() << "=== SETTING UP STOCK TICKER ===";
//
//	m_stockTicker = new StockTickerWidget(m_marketDataFeed, this);
//
//	// Make ticker VERY visible with bright styling for debugging
//	m_stockTicker->setStyleSheet(
//		"StockTickerWidget {"
//		"    background-color: #FF0000;"  // Bright red - can't miss it!
//		"    border: 3px solid #FFFF00;"  // Yellow border
//		"}"
//	);
//	m_stockTicker->setMinimumHeight(60);
//	m_stockTicker->setMaximumHeight(60);
//
//	qDebug() << "Ticker created. Size:" << m_stockTicker->size();
//
//	// Add popular stocks to the ticker
//	QStringList tickerSymbols = {
//		"AAPL", "MSFT", "GOOGL", "AMZN", "TSLA", "META", "NVDA",
//		"JPM", "BAC", "V", "MA", "WMT", "JNJ", "PG", "HD",
//		"DIS", "NFLX", "PYPL", "CSCO", "INTC", "AMD", "CRM"
//	};
//
//	for (const QString& symbol : tickerSymbols) {
//		m_stockTicker->addSymbol(symbol);
//	}
//
//	qDebug() << "Added" << tickerSymbols.size() << "symbols";
//
//	// Connect click handler
//	connect(m_stockTicker, &StockTickerWidget::symbolClicked,
//		this, &MainWindow::onTickerSymbolClicked);
//
//	// Customize scroll speed
//	m_stockTicker->setScrollSpeed(50);  // pixels per second
//
//	// Place ticker below menu bar as standalone widget
//	// Get the current central widget
//	QWidget* oldCentralWidget = m_centralWidget;
//	qDebug() << "Old central widget:" << oldCentralWidget;
//
//	// Create new container widget
//	QWidget* newCentralWidget = new QWidget(this);
//	QVBoxLayout* mainLayout = new QVBoxLayout(newCentralWidget);
//	mainLayout->setContentsMargins(0, 0, 0, 0);
//	mainLayout->setSpacing(0);
//
//	qDebug() << "Created new container widget";
//
//	// Add ticker at top, then the original content below
//	mainLayout->addWidget(m_stockTicker);
//	mainLayout->addWidget(oldCentralWidget);
//
//	qDebug() << "Added widgets to layout";
//
//	// Set as new central widget
//	setCentralWidget(newCentralWidget);
//
//	// Update reference to point to the container
//	m_centralWidget = newCentralWidget;
//
//	// Force show everything
//	m_stockTicker->show();
//	m_stockTicker->setVisible(true);
//	newCentralWidget->show();
//	oldCentralWidget->show();
//
//	qDebug() << "After setup:";
//	qDebug() << "  Ticker visible:" << m_stockTicker->isVisible();
//	qDebug() << "  Ticker size:" << m_stockTicker->size();
//	qDebug() << "  Ticker width:" << m_stockTicker->width();
//	qDebug() << "  Ticker height:" << m_stockTicker->height();
//	qDebug() << "  New central widget:" << newCentralWidget;
//
//	m_orderBlotter->append(QString("[%1] Stock ticker initialized with %2 symbols - LOOK FOR RED BAR AT TOP!")
//		.arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
//		.arg(tickerSymbols.size()));
//
//	qDebug() << "=== TICKER SETUP COMPLETE ===";
//}

void MainWindow::onTickerSymbolClicked(const QString& symbol)
{
	MarketData* data = m_marketDataFeed->getMarketData(symbol);
	if (!data) {
		QMessageBox::information(this, "Symbol Info",
			QString("No data available for %1").arg(symbol));
		return;
	}

	// Create detailed info with HTML formatting
	QString changeColor = data->changeAmount() >= 0 ? "#00c800" : "#ff6464";
	QString changeSymbol = data->changeAmount() >= 0 ? "▲" : "▼";

	QString info = QString(
		"<div style='font-family: Arial;'>"
		"<h2 style='color: #2a82da; margin-bottom: 10px;'>%1</h2>"
		"<table style='width: 100%; border-collapse: collapse;'>"
		"<tr style='border-bottom: 1px solid #ccc;'>"
		"  <td style='padding: 8px; font-weight: bold;'>Last Price:</td>"
		"  <td style='padding: 8px; text-align: right; font-size: 14pt; font-weight: bold;'>$%2</td>"
		"</tr>"
		"<tr style='border-bottom: 1px solid #ccc;'>"
		"  <td style='padding: 8px; font-weight: bold;'>Change:</td>"
		"  <td style='padding: 8px; text-align: right; color: %7; font-weight: bold;'>%8 %3%4 (%5%6%)</td>"
		"</tr>"
		"<tr style='border-bottom: 1px solid #ccc;'>"
		"  <td style='padding: 8px;'>Open:</td>"
		"  <td style='padding: 8px; text-align: right;'>$%9</td>"
		"</tr>"
		"<tr style='border-bottom: 1px solid #ccc;'>"
		"  <td style='padding: 8px;'>High:</td>"
		"  <td style='padding: 8px; text-align: right;'>$%10</td>"
		"</tr>"
		"<tr style='border-bottom: 1px solid #ccc;'>"
		"  <td style='padding: 8px;'>Low:</td>"
		"  <td style='padding: 8px; text-align: right;'>$%11</td>"
		"</tr>"
		"<tr>"
		"  <td style='padding: 8px;'>Volume:</td>"
		"  <td style='padding: 8px; text-align: right;'>%12</td>"
		"</tr>"
		"</table>"
		"</div>"
	)
		.arg(symbol)
		.arg(data->lastPrice(), 0, 'f', 2)
		.arg(data->changeAmount() >= 0 ? "+" : "")
		.arg(data->changeAmount(), 0, 'f', 2)
		.arg(data->changePercent() >= 0 ? "+" : "")
		.arg(data->changePercent(), 0, 'f', 2)
		.arg(changeColor)
		.arg(changeSymbol)
		.arg(data->openPrice(), 0, 'f', 2)
		.arg(data->highPrice(), 0, 'f', 2)
		.arg(data->lowPrice(), 0, 'f', 2)
		.arg(data->totalVolume(), 0, 'f', 0);

	QMessageBox msgBox(this);
	msgBox.setWindowTitle(QString("%1 - Market Data").arg(symbol));
	msgBox.setTextFormat(Qt::RichText);
	msgBox.setText(info);
	msgBox.setIcon(QMessageBox::Information);
	msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Close);
	msgBox.setDefaultButton(QMessageBox::Ok);

	// Add custom buttons for actions
	QPushButton* buyButton = msgBox.addButton("Place Buy Order", QMessageBox::ActionRole);
	QPushButton* sellButton = msgBox.addButton("Place Sell Order", QMessageBox::ActionRole);
	QPushButton* chartButton = msgBox.addButton("View Chart", QMessageBox::ActionRole);

	msgBox.exec();

	// Handle button clicks
	if (msgBox.clickedButton() == buyButton) {
		m_tradingTabs->setCurrentIndex(0);  // Switch to Order Entry
		m_orderBlotter->append(QString("[%1] Ready to place buy order for %2 at $%3")
			.arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
			.arg(symbol)
			.arg(data->lastPrice(), 0, 'f', 2));
	}
	else if (msgBox.clickedButton() == sellButton) {
		m_tradingTabs->setCurrentIndex(0);  // Switch to Order Entry
		m_orderBlotter->append(QString("[%1] Ready to place sell order for %2 at $%3")
			.arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
			.arg(symbol)
			.arg(data->lastPrice(), 0, 'f', 2));
	}
	else if (msgBox.clickedButton() == chartButton) {
		// Open Yahoo Finance chart
		QString url = QString("https://finance.yahoo.com/quote/%1").arg(symbol);
		QDesktopServices::openUrl(QUrl(url));
		m_orderBlotter->append(QString("[%1] Opened chart for %2")
			.arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
			.arg(symbol));
	}

	// Log the ticker click
	m_orderBlotter->append(QString("[%1] Clicked %2 in ticker (Price: $%3)")
		.arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
		.arg(symbol)
		.arg(data->lastPrice(), 0, 'f', 2));
}

void MainWindow::loadDemoPrices()
{
	// Not used anymore - prices come from MarketDataFeed
}