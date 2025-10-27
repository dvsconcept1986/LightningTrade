#pragma once
#include <QString>
#include <QDateTime>
#include <QUuid>

enum class OrderSide {
	Buy,
	Sell
};

enum class OrderType {
	Market,
	Limit,
	Stop,
	StopLimit
};

enum class OrderStatus {
	PendingNew,      // Order created but not sent
	New,             // Order accepted by exchange
	PartiallyFilled, // Order partially executed
	Filled,          // Order fully executed
	PendingCancel,   // Cancel request sent
	Cancelled,       // Order cancelled
	Rejected,        // Order rejected by exchange
	Expired          // Order expired
};

enum class TimeInForce {
	Day,             // Good for day
	GTC,             // Good till cancelled
	IOC,             // Immediate or cancel
	FOK              // Fill or kill
};

class Order {
public:
	Order();
	Order(const QString& symbol, OrderSide side, OrderType type,
		double quantity, double price = 0.0);

	// Getters
	QString orderId() const { return m_orderId; }
	QString symbol() const { return m_symbol; }
	OrderSide side() const { return m_side; }
	OrderType type() const { return m_type; }
	OrderStatus status() const { return m_status; }
	TimeInForce timeInForce() const { return m_timeInForce; }

	double quantity() const { return m_quantity; }
	double price() const { return m_price; }
	double filledQuantity() const { return m_filledQuantity; }
	double averageFillPrice() const { return m_avgFillPrice; }
	double remainingQuantity() const { return m_quantity - m_filledQuantity; }

	QDateTime createdTime() const { return m_createdTime; }
	QDateTime lastUpdateTime() const { return m_lastUpdateTime; }
	QString statusMessage() const { return m_statusMessage; }

	// Setters
	void setStatus(OrderStatus status);
	void setStatusMessage(const QString& message);
	void setTimeInForce(TimeInForce tif) { m_timeInForce = tif; }
	void setPrice(double price) { m_price = price; }

	// Order execution
	void addFill(double quantity, double price);
	bool isFilled() const { return m_status == OrderStatus::Filled; }
	bool isActive() const;
	bool isFinal() const;

	// String conversions
	static QString sideToString(OrderSide side);
	static QString typeToString(OrderType type);
	static QString statusToString(OrderStatus status);
	static QString tifToString(TimeInForce tif);

private:
	QString m_orderId;
	QString m_symbol;
	OrderSide m_side;
	OrderType m_type;
	OrderStatus m_status;
	TimeInForce m_timeInForce;

	double m_quantity;
	double m_price;
	double m_filledQuantity;
	double m_avgFillPrice;

	QDateTime m_createdTime;
	QDateTime m_lastUpdateTime;
	QString m_statusMessage;
};