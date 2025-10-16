#include "MarketDataWidget.h"
#include <QHeaderView>
#include <QMessageBox>

MarketDataWidget::MarketDataWidget(MarketDataFeed* feed, QWidget* parent)
	: QWidget(parent)
	, m_feed(feed)
{
	setupUI();

	// Connect feed signals
	connect(m_feed, &MarketDataFeed::marketDataUpdated,
		this, &MarketDataWidget::onMarketDataUpdated);
	connect(m_feed, &MarketDataFeed::statusChanged,
		this, &MarketDataWidget::onFeedStatusChanged);
}

void MarketDataWidget::setupUI()
{
	m_groupBox = new QGroupBox("Real-Time Market Data", this);

	QVBoxLayout* mainLayout = new QVBoxLayout(m_groupBox);

	// Toolbar
	QHBoxLayout* toolbarLayout = new QHBoxLayout();

	m_symbolEdit = new QLineEdit(this);
	m_symbolEdit->setPlaceholderText("Enter symbol (e.g., AAPL)");
	m_symbolEdit->setMaxLength(10);

	m_addButton = new QPushButton("Add Symbol", this);
	m_removeButton = new QPushButton("Remove Symbol", this);
	m_connectButton = new QPushButton("Connect", this);
	m_connectButton->setStyleSheet("QPushButton { background-color: #2a82da; }");

	m_statusLabel = new QLabel("Disconnected", this);
	m_statusLabel->setStyleSheet("QLabel { color: #ff6464; }");

	connect(m_addButton, &QPushButton::clicked, this, &MarketDataWidget::onAddSymbolClicked);
	connect(m_removeButton, &QPushButton::clicked, this, &MarketDataWidget::onRemoveSymbolClicked);
	connect(m_connectButton, &QPushButton::clicked, this, &MarketDataWidget::onConnectClicked);

	toolbarLayout->addWidget(new QLabel("Symbol:", this));
	toolbarLayout->addWidget(m_symbolEdit);
	toolbarLayout->addWidget(m_addButton);
	toolbarLayout->addWidget(m_removeButton);
	toolbarLayout->addStretch();
	toolbarLayout->addWidget(m_statusLabel);
	toolbarLayout->addWidget(m_connectButton);

	// Market data table
	m_dataTable = new QTableWidget(0, 10, this);
	QStringList headers = {
		"Symbol", "Last", "Change", "Change %", "Bid", "Ask",
		"Bid Size", "Ask Size", "Volume", "Time"
	};
	m_dataTable->setHorizontalHeaderLabels(headers);
	m_dataTable->setAlternatingRowColors(true);
	m_dataTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_dataTable->setSelectionMode(QAbstractItemView::SingleSelection);
	m_dataTable->horizontalHeader()->setStretchLastSection(true);
	m_dataTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

	// Set column widths
	m_dataTable->setColumnWidth(0, 80);   // Symbol
	m_dataTable->setColumnWidth(1, 80);   // Last
	m_dataTable->setColumnWidth(2, 80);   // Change
	m_dataTable->setColumnWidth(3, 80);   // Change %
	m_dataTable->setColumnWidth(4, 80);   // Bid
	m_dataTable->setColumnWidth(5, 80);   // Ask
	m_dataTable->setColumnWidth(6, 80);   // Bid Size
	m_dataTable->setColumnWidth(7, 80);   // Ask Size
	m_dataTable->setColumnWidth(8, 100);  // Volume
	m_dataTable->setColumnWidth(9, 120);  // Time

	mainLayout->addLayout(toolbarLayout);
	mainLayout->addWidget(m_dataTable);

	QVBoxLayout* widgetLayout = new QVBoxLayout(this);
	widgetLayout->addWidget(m_groupBox);
	widgetLayout->setContentsMargins(0, 0, 0, 0);
}

void MarketDataWidget::addSymbol(const QString& symbol)
{
	QString upperSymbol = symbol.toUpper().trimmed();
	if (upperSymbol.isEmpty()) {
		return;
	}

	// Check if already exists
	if (findSymbolRow(upperSymbol) >= 0) {
		QMessageBox::information(this, "Symbol Exists",
			QString("Symbol %1 is already added").arg(upperSymbol));
		return;
	}

	// Add to table
	int row = m_dataTable->rowCount();
	m_dataTable->insertRow(row);
	m_dataTable->setItem(row, 0, new QTableWidgetItem(upperSymbol));

	// Initialize other columns
	for (int col = 1; col < m_dataTable->columnCount(); ++col) {
		m_dataTable->setItem(row, col, new QTableWidgetItem("-"));
	}

	// Subscribe to feed
	m_feed->subscribe(upperSymbol);
}

void MarketDataWidget::removeSymbol(const QString& symbol)
{
	int row = findSymbolRow(symbol);
	if (row >= 0) {
		m_dataTable->removeRow(row);
		m_feed->unsubscribe(symbol);
		m_previousPrices.remove(symbol);
	}
}

void MarketDataWidget::clearSymbols()
{
	m_dataTable->setRowCount(0);
	m_previousPrices.clear();
}

void MarketDataWidget::onMarketDataUpdated(const QString& symbol, MarketData* data)
{
	if (!data) return;

	int row = findSymbolRow(symbol);
	if (row >= 0) {
		updateMarketDataRow(row, data);
	}
}

void MarketDataWidget::onAddSymbolClicked()
{
	QString symbol = m_symbolEdit->text().trimmed().toUpper();
	if (!symbol.isEmpty()) {
		addSymbol(symbol);
		m_symbolEdit->clear();
	}
}

void MarketDataWidget::onRemoveSymbolClicked()
{
	int currentRow = m_dataTable->currentRow();
	if (currentRow >= 0) {
		QTableWidgetItem* item = m_dataTable->item(currentRow, 0);
		if (item) {
			QString symbol = item->text();
			removeSymbol(symbol);
		}
	}
}

void MarketDataWidget::onConnectClicked()
{
	if (m_feed->isConnected()) {
		m_feed->disconnectFromFeed();
		m_connectButton->setText("Connect");
	}
	else {
		m_feed->connectToFeed();
		m_connectButton->setText("Disconnect");
	}
}

void MarketDataWidget::onFeedStatusChanged(FeedStatus status)
{
	switch (status) {
	case FeedStatus::Disconnected:
		m_statusLabel->setText("Disconnected");
		m_statusLabel->setStyleSheet("QLabel { color: #ff6464; }");
		m_connectButton->setText("Connect");
		m_connectButton->setEnabled(true);
		break;
	case FeedStatus::Connecting:
		m_statusLabel->setText("Connecting...");
		m_statusLabel->setStyleSheet("QLabel { color: #ffa500; }");
		m_connectButton->setEnabled(false);
		break;
	case FeedStatus::Connected:
		m_statusLabel->setText("Connected");
		m_statusLabel->setStyleSheet("QLabel { color: #00c800; }");
		m_connectButton->setText("Disconnect");
		m_connectButton->setEnabled(true);
		break;
	case FeedStatus::Reconnecting:
		m_statusLabel->setText("Reconnecting...");
		m_statusLabel->setStyleSheet("QLabel { color: #ffa500; }");
		m_connectButton->setEnabled(false);
		break;
	case FeedStatus::Error:
		m_statusLabel->setText("Error");
		m_statusLabel->setStyleSheet("QLabel { color: #ff0000; }");
		m_connectButton->setText("Connect");
		m_connectButton->setEnabled(true);
		break;
	}
}

void MarketDataWidget::updateMarketDataRow(int row, MarketData* data)
{
	if (!data || row < 0 || row >= m_dataTable->rowCount()) {
		return;
	}

	QString symbol = data->symbol();
	double currentPrice = data->lastPrice();
	double previousPrice = m_previousPrices.value(symbol, currentPrice);

	// Update price with color
	QTableWidgetItem* priceItem = new QTableWidgetItem(QString::number(currentPrice, 'f', 2));
	priceItem->setForeground(getPriceColor(currentPrice - previousPrice));
	m_dataTable->setItem(row, 1, priceItem);

	// Change amount
	double changeAmount = data->changeAmount();
	QTableWidgetItem* changeItem = new QTableWidgetItem(QString::number(changeAmount, 'f', 2));
	changeItem->setForeground(getPriceColor(changeAmount));
	m_dataTable->setItem(row, 2, changeItem);

	// Change percent
	double changePercent = data->changePercent();
	QString percentStr = QString("%1%").arg(changePercent, 0, 'f', 2);
	QTableWidgetItem* percentItem = new QTableWidgetItem(percentStr);
	percentItem->setForeground(getPriceColor(changeAmount));
	m_dataTable->setItem(row, 3, percentItem);

	// Bid/Ask
	m_dataTable->setItem(row, 4, new QTableWidgetItem(QString::number(data->bidPrice(), 'f', 2)));
	m_dataTable->setItem(row, 5, new QTableWidgetItem(QString::number(data->askPrice(), 'f', 2)));
	m_dataTable->setItem(row, 6, new QTableWidgetItem(QString::number(data->bidVolume(), 'f', 0)));
	m_dataTable->setItem(row, 7, new QTableWidgetItem(QString::number(data->askVolume(), 'f', 0)));

	// Volume
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
	m_dataTable->setItem(row, 8, new QTableWidgetItem(volumeStr));

	// Time
	QString timeStr = data->timestamp().toString("hh:mm:ss");
	m_dataTable->setItem(row, 9, new QTableWidgetItem(timeStr));

	// Store current price for next update
	m_previousPrices[symbol] = currentPrice;
}

int MarketDataWidget::findSymbolRow(const QString& symbol)
{
	for (int row = 0; row < m_dataTable->rowCount(); ++row) {
		QTableWidgetItem* item = m_dataTable->item(row, 0);
		if (item && item->text() == symbol) {
			return row;
		}
	}
	return -1;
}

QColor MarketDataWidget::getPriceColor(double change)
{
	if (change > 0) {
		return QColor(0, 200, 0);  // Green for up
	}
	else if (change < 0) {
		return QColor(255, 100, 100);  // Red for down
	}
	else {
		return QColor(255, 255, 255);  // White for no change
	}
}