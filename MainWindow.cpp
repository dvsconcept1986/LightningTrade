#include "MainWindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QMenuBar>
#include <QHeaderView>
#include <QDateTime>
#include <QNetworkRequest>
#include <QUrl>
#include <algorithm>

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
	, m_centralWidget(nullptr)
	, m_networkManager(new QNetworkAccessManager(this))
	, m_refreshTimer(new QTimer(this))
	, m_clockTimer(new QTimer(this))
	, m_orderManager(new OrderManager(this))
	, m_marketDataFeed(new MarketDataFeed(this))
{
	ui.setupUi(this);

	setupMenuBar();
	setupUI();
	setupStatusBar();

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
	m_newsGroup = new QGroupBox("Market News", this);

	QVBoxLayout* layout = new QVBoxLayout(m_newsGroup);

	m_newsList = new QListWidget(m_newsGroup);
	m_newsList->setAlternatingRowColors(true);

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
	QString orderId = m_orderManager->submitOrder(symbol, side, type, quantity, price);

	if (!orderId.isEmpty()) {
		m_orderBlotter->append(QString("[%1] Order submitted: %2 %3 %4 @ %5")
			.arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
			.arg(Order::sideToString(side))
			.arg(quantity)
			.arg(symbol)
			.arg(price));

		refreshOrderBlotter();
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
	// Market prices now come from the feed automatically

	m_refreshButton->setEnabled(true);
	m_statusLabel->setText("Market data updated");
}

void MainWindow::loadMarketNews()
{
	QString url = "https://api.rss2json.com/v1/api.json?rss_url=https://feeds.finance.yahoo.com/rss/2.0/headline";

	QNetworkRequest request;
	request.setUrl(QUrl(url));
	request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
	request.setRawHeader("Accept", "application/json");

	QNetworkReply* reply = m_networkManager->get(request);
	connect(reply, &QNetworkReply::finished, this, &MainWindow::onNewsReplyFinished);
}

void MainWindow::loadMarketPrices()
{
	// This is now handled by the MarketDataFeed
	// Keeping this function for compatibility but it's not used anymore
}

void MainWindow::onNewsReplyFinished()
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
	if (!reply) return;

	reply->deleteLater();

	if (reply->error() != QNetworkReply::NoError) {
		loadDemoNews();
		m_refreshButton->setEnabled(true);
		m_statusLabel->setText("Using demo news data");
		return;
	}

	QByteArray data = reply->readAll();
	QJsonDocument doc = QJsonDocument::fromJson(data);
	QJsonObject root = doc.object();

	if (root.contains("items")) {
		QJsonArray items = root["items"].toArray();
		if (items.size() > 0) {
			updateNewsDisplay(items);
		}
		else {
			loadDemoNews();
		}
	}
	else {
		loadDemoNews();
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

	int maxItems = std::min(10, static_cast<int>(articles.size()));
	for (int i = 0; i < maxItems; ++i) {
		QJsonObject article = articles[i].toObject();
		QString title = article["title"].toString();

		if (!title.isEmpty()) {
			m_newsList->addItem(title);
		}
	}

	if (m_newsList->count() == 0) {
		loadDemoNews();
	}
}

void MainWindow::updatePriceDisplay(const QJsonObject& data)
{
	// Not used anymore - prices come from MarketDataFeed
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
		"</ul>");
}

void MainWindow::loadDemoNews()
{
	m_newsList->clear();

	QStringList demoNews = {
		"?? S&P 500 reaches new all-time high amid tech rally",
		"?? Federal Reserve signals potential rate cuts in Q2",
		"?? Microsoft reports strong quarterly earnings, stock up 5%",
		"? Oil prices surge on OPEC+ production cut announcement",
		"?? Dollar weakens against Euro as ECB hints at policy shift",
		"?? Tesla stock jumps 8% on record delivery numbers",
		"?? Manufacturing PMI data exceeds expectations",
		"? Bitcoin breaks $50K resistance level",
		"?? Apple announces new AI chip, shares climb 3%",
		"?? Global markets rally on positive trade data"
	};

	for (const QString& news : demoNews) {
		m_newsList->addItem(news);
	}
}

void MainWindow::loadDemoPrices()
{
	// Not used anymore - prices come from MarketDataFeed
}