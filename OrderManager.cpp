#include "OrderManager.h"
#include <QTimer>
#include <QDebug>

OrderManager::OrderManager(QObject* parent)
	: QObject(parent)
	, m_orderSequence(1)
{
}

OrderManager::~OrderManager()
{
	// Clean up all orders
	qDeleteAll(m_orders);
	m_orders.clear();
}

QString OrderManager::submitOrder(const QString& symbol, OrderSide side,
	OrderType type, double quantity, double price)
{
	// Create new order
	Order* order = new Order(symbol, side, type, quantity, price);
	QString orderId = order->orderId();

	try {
		validateOrder(*order);
	}
	catch (const std::exception& e) {
		emit logMessage(QString("[ERROR] Order validation failed: %1").arg(e.what()));
		emit orderRejected(orderId, e.what());
		delete order;
		return QString();
	}

	// Store order
	m_orders[orderId] = order;

	emit logMessage(QString("[ORDER] Submitted %1 %2 %3 @ %4 - ID: %5")
		.arg(Order::sideToString(side))
		.arg(quantity)
		.arg(symbol)
		.arg(price)
		.arg(orderId.left(8)));

	emit orderSubmitted(orderId);

	// Process submission (send to exchange in real implementation)
	processOrderSubmission(order);

	return orderId;
}

bool OrderManager::cancelOrder(const QString& orderId)
{
	if (!m_orders.contains(orderId)) {
		emit logMessage(QString("[ERROR] Order not found: %1").arg(orderId.left(8)));
		return false;
	}

	Order* order = m_orders[orderId];

	if (!order->isActive()) {
		emit logMessage(QString("[ERROR] Cannot cancel order in status: %1")
			.arg(Order::statusToString(order->status())));
		return false;
	}

	// Update to pending cancel
	updateOrderStatus(orderId, OrderStatus::PendingCancel, "Cancel requested");

	// Simulate exchange processing (in real system, send cancel request)
	QTimer::singleShot(100, this, [this, orderId]() {
		updateOrderStatus(orderId, OrderStatus::Cancelled, "Cancelled by user");
		emit orderCancelled(orderId);
		});

	return true;
}

bool OrderManager::modifyOrder(const QString& orderId, double newQuantity, double newPrice)
{
	if (!m_orders.contains(orderId)) {
		return false;
	}

	Order* order = m_orders[orderId];

	if (!order->isActive()) {
		emit logMessage(QString("[ERROR] Cannot modify order in status: %1")
			.arg(Order::statusToString(order->status())));
		return false;
	}

	// In real implementation, send modify request to exchange
	// For now, we'll do a simple update
	if (newPrice > 0) {
		order->setPrice(newPrice);
	}

	emit logMessage(QString("[ORDER] Modified %1 - New price: %2")
		.arg(orderId.left(8)).arg(newPrice));
	emit orderModified(orderId);

	return true;
}

Order* OrderManager::getOrder(const QString& orderId)
{
	if (m_orders.contains(orderId)) {
		return m_orders[orderId];
	}
	return nullptr;
}

QList<Order*> OrderManager::getAllOrders() const
{
	QList<Order*> orders;
	for (auto it = m_orders.begin(); it != m_orders.end(); ++it) {
		orders.append(it.value());
	}
	return orders;
}

QList<Order*> OrderManager::getActiveOrders() const
{
	QList<Order*> orders;
	for (auto it = m_orders.begin(); it != m_orders.end(); ++it) {
		if (it.value()->isActive()) {
			orders.append(it.value());
		}
	}
	return orders;
}

QList<Order*> OrderManager::getOrdersBySymbol(const QString& symbol) const
{
	QList<Order*> orders;
	for (auto it = m_orders.begin(); it != m_orders.end(); ++it) {
		if (it.value()->symbol() == symbol) {
			orders.append(it.value());
		}
	}
	return orders;
}

QList<Order*> OrderManager::getOrdersByStatus(OrderStatus status) const
{
	QList<Order*> orders;
	for (auto it = m_orders.begin(); it != m_orders.end(); ++it) {
		if (it.value()->status() == status) {
			orders.append(it.value());
		}
	}
	return orders;
}

int OrderManager::getActiveOrderCount() const
{
	int count = 0;
	for (auto it = m_orders.begin(); it != m_orders.end(); ++it) {
		if (it.value()->isActive()) {
			count++;
		}
	}
	return count;
}

double OrderManager::getTotalVolume() const
{
	double volume = 0.0;
	for (auto it = m_orders.begin(); it != m_orders.end(); ++it) {
		volume += it.value()->filledQuantity();
	}
	return volume;
}

double OrderManager::getTotalValueTraded() const
{
	double value = 0.0;
	for (auto it = m_orders.begin(); it != m_orders.end(); ++it) {
		value += it.value()->filledQuantity() * it.value()->averageFillPrice();
	}
	return value;
}

void OrderManager::simulateOrderAcceptance(const QString& orderId)
{
	updateOrderStatus(orderId, OrderStatus::New, "Order accepted by exchange");
	emit orderAccepted(orderId);
}

void OrderManager::simulateOrderFill(const QString& orderId, double quantity, double price)
{
	if (!m_orders.contains(orderId)) return;

	Order* order = m_orders[orderId];
	order->addFill(quantity, price);

	if (order->isFilled()) {
		emit logMessage(QString("[FILL] Order %1 fully filled: %2 @ %3")
			.arg(orderId.left(8)).arg(quantity).arg(price));
		emit orderFilled(orderId, quantity, price);
	}
	else {
		emit logMessage(QString("[FILL] Order %1 partially filled: %2 @ %3 (%4/%5)")
			.arg(orderId.left(8)).arg(quantity).arg(price)
			.arg(order->filledQuantity()).arg(order->quantity()));
		emit orderPartiallyFilled(orderId, quantity, price);
	}

	emit orderStatusChanged(orderId, order->status());
}

void OrderManager::simulateOrderRejection(const QString& orderId, const QString& reason)
{
	updateOrderStatus(orderId, OrderStatus::Rejected, reason);
	emit logMessage(QString("[REJECT] Order %1 rejected: %2").arg(orderId.left(8)).arg(reason));
	emit orderRejected(orderId, reason);
}

void OrderManager::validateOrder(const Order& order)
{
	if (order.symbol().isEmpty()) {
		throw std::invalid_argument("Symbol cannot be empty");
	}

	if (order.quantity() <= 0) {
		throw std::invalid_argument("Quantity must be positive");
	}

	if ((order.type() == OrderType::Limit || order.type() == OrderType::StopLimit)
		&& order.price() <= 0) {
		throw std::invalid_argument("Price must be positive for limit orders");
	}
}

void OrderManager::processOrderSubmission(Order* order)
{
	if (!order) return;

	// Simulate exchange processing delay
	QString orderId = order->orderId();

	QTimer::singleShot(50, this, [this, orderId]() {
		simulateOrderAcceptance(orderId);

		// Simulate a fill after acceptance (for demo purposes)
		QTimer::singleShot(200, this, [this, orderId]() {
			if (m_orders.contains(orderId)) {
				Order* order = m_orders[orderId];
				if (order->isActive()) {
					// Simulate full fill at order price (or near market for market orders)
					double fillPrice = order->price() > 0 ? order->price() : 100.0;
					simulateOrderFill(orderId, order->quantity(), fillPrice);
				}
			}
			});
		});
}

void OrderManager::updateOrderStatus(const QString& orderId, OrderStatus status,
	const QString& message)
{
	if (!m_orders.contains(orderId)) return;

	Order* order = m_orders[orderId];
	order->setStatus(status);

	if (!message.isEmpty()) {
		order->setStatusMessage(message);
	}

	emit orderStatusChanged(orderId, status);
}