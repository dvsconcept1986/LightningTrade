#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QGroupBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "MarketData.h"
#include "MarketDataFeed.h"

class MarketDataWidget : public QWidget
{
	Q_OBJECT

public:
	explicit MarketDataWidget(MarketDataFeed* feed, QWidget* parent = nullptr);

	void addSymbol(const QString& symbol);
	void removeSymbol(const QString& symbol);
	void clearSymbols();

private slots:
	void onMarketDataUpdated(const QString& symbol, MarketData* data);
	void onAddSymbolClicked();
	void onRemoveSymbolClicked();
	void onConnectClicked();
	void onFeedStatusChanged(FeedStatus status);

private:
	void setupUI();
	void updateMarketDataRow(int row, MarketData* data);
	int findSymbolRow(const QString& symbol);
	QColor getPriceColor(double change);

private:
	MarketDataFeed* m_feed;

	QGroupBox* m_groupBox;
	QTableWidget* m_dataTable;
	QLineEdit* m_symbolEdit;
	QPushButton* m_addButton;
	QPushButton* m_removeButton;
	QPushButton* m_connectButton;
	QLabel* m_statusLabel;

	QMap<QString, double> m_previousPrices;  // For price change detection
};