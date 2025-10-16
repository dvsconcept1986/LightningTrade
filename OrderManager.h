#pragma once
#include <QObject>
#include <QMap>
#include <QList>
#include "Order.h"

class OrderManager : public QObject
{
	Q_OBJECT

public:
	explicit OrderManager(QObject* parent = nullptr);
	~OrderManager();

	// Order submission
	QString submitOrder(const QString& symbol, OrderSide side, OrderType type,
		double quantity, double price = 0.0);
	bool cancelOrder(const QString& orderId);
	bool modifyOrder(const QString& orderId, double newQuantity, double newPrice);

	// Order queries
	Order* getOrder(const QString& orderId);
	QList<Order*> getAllOrders() const;
	QList<Order*> getActiveOrders() const;
	QList<Order*> getOrdersBySymbol(const QString& symbol) const;
	QList<Order*> getOrdersByStatus(OrderStatus status) const;

	// Statistics
	int getTotalOrderCount() const { return m_orders.size(); }
	int getActiveOrderCount() const;
	double getTotalVolume() const;
	double getTotalValueTraded() const;

signals:
	// Order lifecycle events
	void orderSubmitted(const QString& orderId);
	void orderAccepted(const QString& orderId);
	void orderRejected(const QString& orderId, const QString& reason);
	void orderFilled(const QString& orderId, double quantity, double price);
	void orderPartiallyFilled(const QString& orderId, double quantity, double price);
	void orderCancelled(const QString& orderId);
	void orderModified(const QString& orderId);

	// Status updates
	void orderStatusChanged(const QString& orderId, OrderStatus newStatus);
	void logMessage(const QString& message);

public slots:
	// Simulated exchange responses (for demo/testing)
	void simulateOrderAcceptance(const QString& orderId);
	void simulateOrderFill(const QString& orderId, double quantity, double price);
	void simulateOrderRejection(const QString& orderId, const QString& reason);

private:
	void validateOrder(const Order& order);
	void processOrderSubmission(Order* order);
	void updateOrderStatus(const QString& orderId, OrderStatus status,
		const QString& message = QString());

private:
	QMap<QString, Order*> m_orders;
	int m_orderSequence;
};