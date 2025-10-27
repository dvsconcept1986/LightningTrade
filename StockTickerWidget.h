#pragma once
#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPropertyAnimation>
#include <QEvent>
#include <QEnterEvent>
#include "MarketDataFeed.h"

class StockTickerWidget : public QWidget
{
	Q_OBJECT

public:
	explicit StockTickerWidget(MarketDataFeed* dataFeed, QWidget* parent = nullptr);
	~StockTickerWidget();

	void addSymbol(const QString& symbol);
	void removeSymbol(const QString& symbol);
	void clearSymbols();
	void setScrollSpeed(int pixelsPerSecond);
	void setUpdateInterval(int milliseconds);

signals:
	void symbolClicked(const QString& symbol);

protected:
	bool eventFilter(QObject* obj, QEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;

private slots:
	void onMarketDataUpdated(const QString& symbol, MarketData* data);
	void updateDisplay();
	void scrollTicker();

private:
	void setupUI();
	void createTickerLabels();
	QString formatStockDisplay(const QString& symbol, MarketData* data);

private:
	MarketDataFeed* m_dataFeed;
	QWidget* m_tickerContainer;
	QHBoxLayout* m_tickerLayout;
	QTimer* m_scrollTimer;
	QTimer* m_updateTimer;

	QMap<QString, QLabel*> m_tickerLabels;
	QMap<QString, QLabel*> m_separatorLabels;
	QStringList m_symbols;

	int m_scrollSpeed;
	int m_scrollPosition;
	int m_containerWidth;
	bool m_isPaused;
};