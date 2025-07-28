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
{
    ui.setupUi(this);

    setupMenuBar();
    setupUI();
    setupStatusBar();

    // Connect signals
    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::refreshMarketData);
    connect(m_refreshTimer, &QTimer::timeout, this, &MainWindow::refreshMarketData);
    connect(m_clockTimer, &QTimer::timeout, this, [this]() {
        m_clockLabel->setText(QDateTime::currentDateTime().toString("hh:mm:ss AP"));
        });

    // Start timers
    m_refreshTimer->start(60000); // Refresh every minute
    m_clockTimer->start(1000);    // Update clock every second

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
    viewMenu->addAction("&Refresh", QKeySequence::Refresh, this, &MainWindow::refreshMarketData);

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
    setupTradingArea();

    // Add to splitters
    m_mainSplitter->addWidget(m_marketDataGroup);
    m_rightSplitter->addWidget(m_newsGroup);
    m_rightSplitter->addWidget(m_tradingGroup);
    m_mainSplitter->addWidget(m_rightSplitter);

    // Set splitter proportions
    m_mainSplitter->setSizes({ 400, 600 });
    m_rightSplitter->setSizes({ 300, 300 });

    // Main layout
    QHBoxLayout* mainLayout = new QHBoxLayout(m_centralWidget);
    mainLayout->addWidget(m_mainSplitter);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    // Window properties
    setWindowTitle("Lightning Trade");
    setMinimumSize(1000, 700);
    resize(1400, 900);
}

void MainWindow::setupMarketDataArea()
{
    m_marketDataGroup = new QGroupBox("Market Data", this);

    QVBoxLayout* layout = new QVBoxLayout(m_marketDataGroup);

    // Refresh button
    m_refreshButton = new QPushButton("Refresh Data", m_marketDataGroup);
    m_refreshButton->setMaximumWidth(120);

    // Price table
    m_priceTable = new QTableWidget(0, 4, m_marketDataGroup);
    m_priceTable->setHorizontalHeaderLabels({ "Symbol", "Price", "Change", "Volume" });
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

void MainWindow::setupTradingArea()
{
    m_tradingGroup = new QGroupBox("Trading Console", this);

    QVBoxLayout* layout = new QVBoxLayout(m_tradingGroup);

    m_orderBlotter = new QTextEdit(m_tradingGroup);
    m_orderBlotter->setReadOnly(true);
    m_orderBlotter->setMaximumHeight(200);
    m_orderBlotter->setPlainText("Trading console ready...\nConnecting to market data feeds...\n");

    layout->addWidget(m_orderBlotter);
}

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel("Ready", this);
    m_clockLabel = new QLabel(QDateTime::currentDateTime().toString("hh:mm:ss AP"), this);

    statusBar()->addWidget(m_statusLabel);
    statusBar()->addPermanentWidget(m_clockLabel);
}

void MainWindow::refreshMarketData()
{
    m_statusLabel->setText("Refreshing market data...");
    m_refreshButton->setEnabled(false);

    loadMarketNews();
    loadMarketPrices();
}

void MainWindow::loadMarketNews()
{
    // Try multiple news sources for better reliability

    // Option 1: RSS2JSON with Yahoo Finance (sometimes blocked)
    QString url = "https://api.rss2json.com/v1/api.json?rss_url=https://feeds.finance.yahoo.com/rss/2.0/headline";

    // Option 2: Alternative - NewsAPI (requires free API key)
    // QString url = "https://newsapi.org/v2/top-headlines?country=us&category=business&apiKey=YOUR_API_KEY";

    // Option 3: Direct RSS feed (fallback)
    // QString url = "https://feeds.finance.yahoo.com/rss/2.0/headline";

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    request.setRawHeader("Accept", "application/json");

    m_orderBlotter->append(QString("[%1] Requesting news from: %2")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
        .arg(url));

    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &MainWindow::onNewsReplyFinished);
}

void MainWindow::loadMarketPrices()
{
    // Using a free API for demo prices (replace with real market data)
    QString url = "https://api.coindesk.com/v1/bpi/currentprice.json";

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("User-Agent", "LightningTrade/1.0");

    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &MainWindow::onPriceReplyFinished);
}

void MainWindow::onNewsReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    reply->deleteLater();

    // Debug: Check for network errors
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = QString("Network Error: %1").arg(reply->errorString());
        m_orderBlotter->append(QString("[%1] %2")
            .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
            .arg(errorMsg));

        // Load demo news on error
        loadDemoNews();
        m_refreshButton->setEnabled(true);
        m_statusLabel->setText("Using demo news data");
        return;
    }

    QByteArray data = reply->readAll();

    // Debug: Log the response
    m_orderBlotter->append(QString("[%1] News API Response: %2 bytes received")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
        .arg(data.size()));

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();

    if (root.contains("items")) {
        QJsonArray items = root["items"].toArray();
        if (items.size() > 0) {
            updateNewsDisplay(items);
        }
        else {
            m_orderBlotter->append(QString("[%1] No news items found in response")
                .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
            loadDemoNews();
        }
    }
    else {
        m_orderBlotter->append(QString("[%1] Invalid news response format")
            .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
        loadDemoNews();
    }

    m_refreshButton->setEnabled(true);
    m_statusLabel->setText("Market data updated");
}

void MainWindow::onPriceReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        m_orderBlotter->append(QString("[%1] Price API Error: %2")
            .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
            .arg(reply->errorString()));
        loadDemoPrices();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    // Try to update with real data, fallback to demo
    if (doc.object().contains("bpi")) {
        updatePriceDisplay(doc.object());
    }
    else {
        loadDemoPrices();
    }
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

    // Add some demo trading news if no articles
    if (m_newsList->count() == 0) {
        m_newsList->addItem("?? Markets open higher on positive economic data");
        m_newsList->addItem("?? Federal Reserve maintains current interest rates");
        m_newsList->addItem("?? Technology sector shows strong performance");
        m_newsList->addItem("??? Oil prices stabilize after recent volatility");
        m_newsList->addItem("?? Dollar strengthens against major currencies");
    }
}

void MainWindow::updatePriceDisplay(const QJsonObject& data)
{
    // Clear existing data
    m_priceTable->setRowCount(0);

    // Add Bitcoin price if available
    if (data.contains("bpi")) {
        QJsonObject bpi = data["bpi"].toObject();
        QJsonObject usd = bpi["USD"].toObject();

        m_priceTable->insertRow(0);
        m_priceTable->setItem(0, 0, new QTableWidgetItem("BTC/USD"));
        m_priceTable->setItem(0, 1, new QTableWidgetItem(usd["rate"].toString()));
        m_priceTable->setItem(0, 2, new QTableWidgetItem("+2.34%"));
        m_priceTable->setItem(0, 3, new QTableWidgetItem("1.2B"));

        m_orderBlotter->append(QString("[%1] Real Bitcoin price loaded: %2")
            .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
            .arg(usd["rate"].toString()));
    }

    // Always add demo stock data for a complete display
    loadDemoPrices();
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, "About Lightning Trade",
        "<h3>Lightning Trade</h3>"
        "<p>Version 1.0.0</p>"
        "<p>Ultra-low latency trading platform for institutional use.</p>"
        "<p>Built with Qt 6 and Visual Studio 2022</p>");
}

void MainWindow::loadDemoNews()
{
    m_newsList->clear();

    // Add realistic trading news with timestamps
    QStringList demoNews = {
        "?? S&P 500 reaches new all-time high amid tech rally",
        "?? Federal Reserve signals potential rate cuts in Q2",
        "?? Microsoft reports strong quarterly earnings, stock up 5%",
        "??? Oil prices surge on OPEC+ production cut announcement",
        "?? Dollar weakens against Euro as ECB hints at policy shift",
        "?? Tesla stock jumps 8% on record delivery numbers",
        "?? Manufacturing PMI data exceeds expectations",
        "?? Bitcoin breaks $50K resistance level",
        "?? Apple announces new AI chip, shares climb 3%",
        "?? Global markets rally on positive trade data"
    };

    for (const QString& news : demoNews) {
        m_newsList->addItem(news);
    }

    m_orderBlotter->append(QString("[%1] Loaded demo news data (%2 items)")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
        .arg(demoNews.size()));
}

void MainWindow::loadDemoPrices()
{
    m_priceTable->setRowCount(0);

    // Demo stock data with realistic prices
    struct StockData {
        QString symbol;
        QString price;
        QString change;
        QString volume;
    };

    QList<StockData> stocks = {
        {"AAPL", "$182.52", "+1.25%", "52.3M"},
        {"MSFT", "$384.90", "+2.15%", "28.7M"},
        {"GOOGL", "$149.34", "-0.85%", "24.1M"},
        {"TSLA", "$253.80", "+4.32%", "89.5M"},
        {"AMZN", "$151.94", "-1.05%", "35.2M"},
        {"NVDA", "$722.48", "+3.67%", "41.8M"},
        {"META", "$434.61", "+0.92%", "18.9M"},
        {"BTC/USD", "$47,234", "+2.84%", "1.2B"},
        {"SPY", "$487.23", "+0.67%", "67.4M"},
        {"QQQ", "$391.45", "+1.23%", "45.6M"}
    };

    for (int i = 0; i < stocks.size(); ++i) {
        m_priceTable->insertRow(i);
        m_priceTable->setItem(i, 0, new QTableWidgetItem(stocks[i].symbol));
        m_priceTable->setItem(i, 1, new QTableWidgetItem(stocks[i].price));
        m_priceTable->setItem(i, 2, new QTableWidgetItem(stocks[i].change));
        m_priceTable->setItem(i, 3, new QTableWidgetItem(stocks[i].volume));
    }

    m_orderBlotter->append(QString("[%1] Loaded demo price data (%2 symbols)")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
        .arg(stocks.size()));
}