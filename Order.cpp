#include "Order.h"

Order::Order()
	: m_orderId(QUuid::createUuid().toString(QUuid::WithoutBraces))
	, m_side(OrderSide::Buy)
	, m_type(OrderType::Market)
	, m_status(OrderStatus::PendingNew)
	, m_timeInForce(TimeInForce::Day)
	, m_quantity(0.0)
	, m_price(0.0)
	, m_filledQuantity(0.0)
	, m_avgFillPrice(0.0)
	, m_createdTime(QDateTime::currentDateTime())
	, m_lastUpdateTime(QDateTime::currentDateTime())
{
}

Order::Order(const QString& symbol, OrderSide side, OrderType type,
	double quantity, double price)
	: m_orderId(QUuid::createUuid().toString(QUuid::WithoutBraces))
	, m_symbol(symbol)
	, m_side(side)
	, m_type(type)
	, m_status(OrderStatus::PendingNew)
	, m_timeInForce(TimeInForce::Day)
	, m_quantity(quantity)
	, m_price(price)
	, m_filledQuantity(0.0)
	, m_avgFillPrice(0.0)
	, m_createdTime(QDateTime::currentDateTime())
	, m_lastUpdateTime(QDateTime::currentDateTime())
{
}

void Order::setStatus(OrderStatus status)
{
	m_status = status;
	m_lastUpdateTime = QDateTime::currentDateTime();
}

void Order::setStatusMessage(const QString& message)
{
	m_statusMessage = message;
	m_lastUpdateTime = QDateTime::currentDateTime();
}

void Order::addFill(double quantity, double price)
{
	if (quantity <= 0) return;

	// Update average fill price
	double totalFilled = m_filledQuantity + quantity;
	m_avgFillPrice = ((m_avgFillPrice * m_filledQuantity) + (price * quantity)) / totalFilled;

	m_filledQuantity += quantity;
	m_lastUpdateTime = QDateTime::currentDateTime();

	// Update status based on fill
	if (m_filledQuantity >= m_quantity) {
		m_status = OrderStatus::Filled;
	}
	else if (m_filledQuantity > 0) {
		m_status = OrderStatus::PartiallyFilled;
	}
}

bool Order::isActive() const
{
	return m_status == OrderStatus::PendingNew ||
		m_status == OrderStatus::New ||
		m_status == OrderStatus::PartiallyFilled ||
		m_status == OrderStatus::PendingCancel;
}

bool Order::isFinal() const
{
	return m_status == OrderStatus::Filled ||
		m_status == OrderStatus::Cancelled ||
		m_status == OrderStatus::Rejected ||
		m_status == OrderStatus::Expired;
}

QString Order::sideToString(OrderSide side)
{
	switch (side) {
	case OrderSide::Buy: return "BUY";
	case OrderSide::Sell: return "SELL";
	default: return "UNKNOWN";
	}
}

QString Order::typeToString(OrderType type)
{
	switch (type) {
	case OrderType::Market: return "MARKET";
	case OrderType::Limit: return "LIMIT";
	case OrderType::Stop: return "STOP";
	case OrderType::StopLimit: return "STOP_LIMIT";
	default: return "UNKNOWN";
	}
}

QString Order::statusToString(OrderStatus status)
{
	switch (status) {
	case OrderStatus::PendingNew: return "PENDING_NEW";
	case OrderStatus::New: return "NEW";
	case OrderStatus::PartiallyFilled: return "PARTIALLY_FILLED";
	case OrderStatus::Filled: return "FILLED";
	case OrderStatus::PendingCancel: return "PENDING_CANCEL";
	case OrderStatus::Cancelled: return "CANCELLED";
	case OrderStatus::Rejected: return "REJECTED";
	case OrderStatus::Expired: return "EXPIRED";
	default: return "UNKNOWN";
	}
}

QString Order::tifToString(TimeInForce tif)
{
	switch (tif) {
	case TimeInForce::Day: return "DAY";
	case TimeInForce::GTC: return "GTC";
	case TimeInForce::IOC: return "IOC";
	case TimeInForce::FOK: return "FOK";
	default: return "UNKNOWN";
	}
}