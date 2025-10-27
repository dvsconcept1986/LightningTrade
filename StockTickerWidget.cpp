#include "StockTickerWidget.h"
#include <QDebug>

StockTickerWidget::StockTickerWidget(MarketDataFeed* dataFeed, QWidget* parent)
	: QWidget(parent)
	, m_dataFeed(dataFeed)
	, m_tickerContainer(nullptr)
	, m_tickerLayout(nullptr)
	, m_scrollTimer(new QTimer(this))
	, m_updateTimer(new QTimer(this))
	, m_scrollSpeed(50)
	, m_scrollPosition(0)
	, m_containerWidth(0)
	, m_isPaused(false)
{
	qDebug() << "[Ticker] Constructor START";

	// Super simple setup - just like the bypass that works
	setStyleSheet("background-color: #2a2a2a;");  // Removed green border
	setMinimumHeight(50);
	setMaximumHeight(50);

	// Create layout directly on this widget
	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setContentsMargins(5, 5, 5, 5);
	layout->setSpacing(10);
	setLayout(layout);

	// Connect to market data feed for updates
	connect(m_dataFeed, &MarketDataFeed::marketDataUpdated,
		this, &StockTickerWidget::onMarketDataUpdated);

	// Setup update timer (no scrolling for now)
	m_updateTimer->setInterval(1000);
	connect(m_updateTimer, &QTimer::timeout, this, &StockTickerWidget::updateDisplay);
	m_updateTimer->start();

	qDebug() << "[Ticker] Constructor COMPLETE";
}

StockTickerWidget::~StockTickerWidget()
{
	qDebug() << "[Ticker] Destructor";
}

void StockTickerWidget::addSymbol(const QString& symbol)
{
	qDebug() << "[Ticker] addSymbol:" << symbol;

	if (m_symbols.contains(symbol)) {
		qDebug() << "[Ticker] Symbol already exists, skipping";
		return;
	}

	m_symbols.append(symbol);

	// Add separator if not first
	if (layout()->count() > 0) {
		QLabel* separator = new QLabel("│", this);
		separator->setStyleSheet("color: #888888; font-size: 8pt;");  // Smaller (was 9pt)
		m_separatorLabels[symbol] = separator;
		layout()->addWidget(separator);
		qDebug() << "[Ticker] Added separator";
	}

	// Create stock label
	QLabel* label = new QLabel(this);
	label->setStyleSheet(
		"QLabel {"
		"    color: #FFFFFF;"
		"    font-size: 8pt;"           // Even smaller font (was 9pt)
		"    font-weight: bold;"
		"    padding: 2px 5px;"
		"}"
		"QLabel:hover {"
		"    background-color: #3a3a3a;"
		"}"
	);

	// Enable click handler
	label->setCursor(Qt::PointingHandCursor);
	label->installEventFilter(this);
	label->setProperty("symbol", symbol);

	m_tickerLabels[symbol] = label;
	layout()->addWidget(label);

	// Subscribe to market data
	m_dataFeed->subscribe(symbol);

	// Set initial text
	MarketData* data = m_dataFeed->getMarketData(symbol);
	if (data) {
		label->setText(formatStockDisplay(symbol, data));
		qDebug() << "[Ticker] Set text from data:" << label->text();
	}
	else {
		label->setText(symbol + ": Loading...");
		qDebug() << "[Ticker] Set loading text";
	}

	label->show();
	qDebug() << "[Ticker] Label added and shown. Visible:" << label->isVisible();
}

void StockTickerWidget::removeSymbol(const QString& symbol)
{
	if (!m_symbols.contains(symbol)) {
		return;
	}

	m_symbols.removeAll(symbol);

	if (m_tickerLabels.contains(symbol)) {
		QLabel* label = m_tickerLabels[symbol];
		layout()->removeWidget(label);
		label->deleteLater();
		m_tickerLabels.remove(symbol);
	}

	if (m_separatorLabels.contains(symbol)) {
		QLabel* separator = m_separatorLabels[symbol];
		layout()->removeWidget(separator);
		separator->deleteLater();
		m_separatorLabels.remove(symbol);
	}

	m_dataFeed->unsubscribe(symbol);
}

void StockTickerWidget::clearSymbols()
{
	QStringList symbolsCopy = m_symbols;
	for (const QString& symbol : symbolsCopy) {
		removeSymbol(symbol);
	}
}

void StockTickerWidget::setScrollSpeed(int pixelsPerSecond)
{
	m_scrollSpeed = pixelsPerSecond;
	qDebug() << "[Ticker] Scroll speed set to:" << pixelsPerSecond;
}

void StockTickerWidget::setUpdateInterval(int milliseconds)
{
	m_updateTimer->setInterval(milliseconds);
}

void StockTickerWidget::onMarketDataUpdated(const QString& symbol, MarketData* data)
{
	if (!m_tickerLabels.contains(symbol)) {
		return;
	}

	QLabel* label = m_tickerLabels[symbol];
	label->setText(formatStockDisplay(symbol, data));
}

void StockTickerWidget::updateDisplay()
{
	// Refresh all ticker displays with latest data
	for (const QString& symbol : m_symbols) {
		MarketData* data = m_dataFeed->getMarketData(symbol);
		if (data && m_tickerLabels.contains(symbol)) {
			m_tickerLabels[symbol]->setText(formatStockDisplay(symbol, data));
		}
	}
}

void StockTickerWidget::scrollTicker()
{
	// Scrolling disabled for now - keep it simple
}

QString StockTickerWidget::formatStockDisplay(const QString& symbol, MarketData* data)
{
	if (!data) {
		return QString("<span style='color: #FFFFFF;'>%1: --</span>").arg(symbol);
	}

	double price = data->lastPrice();
	double change = data->changeAmount();
	double changePercent = data->changePercent();

	QString priceStr = QString("$%1").arg(price, 0, 'f', 2);
	QString changeStr;
	QString symbolColor;

	if (change > 0) {
		// Price went UP - make symbol GREEN
		symbolColor = "#00FF00";
		changeStr = QString("<span style='color: #00FF00;'>▲ +%1 (+%2%)</span>")
			.arg(change, 0, 'f', 2)
			.arg(changePercent, 0, 'f', 2);
	}
	else if (change < 0) {
		// Price went DOWN - make symbol RED
		symbolColor = "#FF0000";
		changeStr = QString("<span style='color: #FF0000;'>▼ %1 (%2%)</span>")
			.arg(change, 0, 'f', 2)
			.arg(changePercent, 0, 'f', 2);
	}
	else {
		// No change - keep white/gray
		symbolColor = "#CCCCCC";
		changeStr = "<span style='color: #888888;'>━ 0.00 (0.00%)</span>";
	}

	// Symbol colored based on price direction, price in white, change with arrow
	return QString("<span style='color: %1; font-weight: bold;'>%2</span> <span style='color: #FFFFFF;'>%3</span> %4")
		.arg(symbolColor, symbol, priceStr, changeStr);
}

bool StockTickerWidget::eventFilter(QObject* obj, QEvent* event)
{
	if (event->type() == QEvent::MouseButtonPress) {
		QLabel* label = qobject_cast<QLabel*>(obj);
		if (label) {
			QString symbol = label->property("symbol").toString();
			if (!symbol.isEmpty()) {
				qDebug() << "[Ticker] Symbol clicked:" << symbol;
				emit symbolClicked(symbol);
			}
		}
	}
	return QWidget::eventFilter(obj, event);
}

void StockTickerWidget::enterEvent(QEnterEvent* event)
{
	m_isPaused = true;
	QWidget::enterEvent(event);
}

void StockTickerWidget::leaveEvent(QEvent* event)
{
	m_isPaused = false;
	QWidget::leaveEvent(event);
}