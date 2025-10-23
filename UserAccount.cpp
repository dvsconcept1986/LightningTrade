#include "UserAccount.h"
#include <QUuid>

// Transaction Implementation
Transaction::Transaction()
	: m_transactionId(QUuid::createUuid().toString(QUuid::WithoutBraces))
	, m_type(TransactionType::Deposit)
	, m_amount(0.0)
	, m_timestamp(QDateTime::currentDateTime())
	, m_balanceAfter(0.0)
{
}

Transaction::Transaction(TransactionType type, double amount, const QString& description)
	: m_transactionId(QUuid::createUuid().toString(QUuid::WithoutBraces))
	, m_type(type)
	, m_amount(amount)
	, m_description(description)
	, m_timestamp(QDateTime::currentDateTime())
	, m_balanceAfter(0.0)
{
}

QString Transaction::typeToString(TransactionType type)
{
	switch (type) {
	case TransactionType::Deposit: return "DEPOSIT";
	case TransactionType::Withdrawal: return "WITHDRAWAL";
	case TransactionType::Trade: return "TRADE";
	case TransactionType::Dividend: return "DIVIDEND";
	case TransactionType::Interest: return "INTEREST";
	case TransactionType::Fee: return "FEE";
	default: return "UNKNOWN";
	}
}

// Position Implementation
Position::Position()
	: m_quantity(0.0)
	, m_averagePrice(0.0)
	, m_currentPrice(0.0)
{
}

Position::Position(const QString& symbol, double quantity, double averagePrice)
	: m_symbol(symbol)
	, m_quantity(quantity)
	, m_averagePrice(averagePrice)
	, m_currentPrice(averagePrice)
{
}

double Position::unrealizedPnLPercent() const
{
	if (m_averagePrice <= 0.0) return 0.0;
	return ((m_currentPrice - m_averagePrice) / m_averagePrice) * 100.0;
}

void Position::addQuantity(double quantity, double price)
{
	if (quantity <= 0) return;

	// Update average price
	double totalCost = (m_quantity * m_averagePrice) + (quantity * price);
	m_quantity += quantity;
	m_averagePrice = totalCost / m_quantity;
}

void Position::reduceQuantity(double quantity)
{
	if (quantity <= 0) return;
	m_quantity -= quantity;
	if (m_quantity < 0) m_quantity = 0;
}

// UserAccount Implementation
UserAccount::UserAccount()
	: m_userId(QUuid::createUuid().toString(QUuid::WithoutBraces))
	, m_createdDate(QDateTime::currentDateTime())
	, m_cashBalance(0.0)
	, m_realizedPnL(0.0)
{
}

UserAccount::UserAccount(const QString& username, const QString& fullName, const QString& email)
	: m_userId(QUuid::createUuid().toString(QUuid::WithoutBraces))
	, m_username(username)
	, m_fullName(fullName)
	, m_email(email)
	, m_createdDate(QDateTime::currentDateTime())
	, m_cashBalance(0.0)
	, m_realizedPnL(0.0)
{
}

double UserAccount::portfolioValue() const
{
	double total = 0.0;
	for (auto it = m_positions.begin(); it != m_positions.end(); ++it) {
		total += it.value().marketValue();
	}
	return total;
}

double UserAccount::buyingPower() const
{
	// Simple implementation: cash balance
	// In real system, would include margin calculations
	return m_cashBalance;
}

bool UserAccount::deposit(double amount, const QString& description)
{
	if (amount <= 0) return false;

	m_cashBalance += amount;

	Transaction transaction(TransactionType::Deposit, amount, description);
	transaction.setBalanceAfter(m_cashBalance);
	m_transactions.append(transaction);

	return true;
}

bool UserAccount::withdraw(double amount, const QString& description)
{
	if (amount <= 0 || amount > m_cashBalance) return false;

	m_cashBalance -= amount;

	Transaction transaction(TransactionType::Withdrawal, -amount, description);
	transaction.setBalanceAfter(m_cashBalance);
	m_transactions.append(transaction);

	return true;
}

void UserAccount::addTransaction(const Transaction& transaction)
{
	m_transactions.append(transaction);
}

QList<Transaction> UserAccount::getRecentTransactions(int count) const
{
	QList<Transaction> recent;
	int start = qMax(0, m_transactions.size() - count);
	for (int i = start; i < m_transactions.size(); ++i) {
		recent.append(m_transactions[i]);
	}
	return recent;
}

void UserAccount::addPosition(const QString& symbol, double quantity, double price)
{
	if (m_positions.contains(symbol)) {
		m_positions[symbol].addQuantity(quantity, price);
	}
	else {
		m_positions[symbol] = Position(symbol, quantity, price);
	}

	// Deduct cash for purchase
	double cost = quantity * price;
	m_cashBalance -= cost;

	// Record trade transaction
	Transaction transaction(TransactionType::Trade, -cost,
		QString("Buy %1 shares of %2 @ $%3")
		.arg(quantity).arg(symbol).arg(price));
	transaction.setBalanceAfter(m_cashBalance);
	m_transactions.append(transaction);
}

void UserAccount::reducePosition(const QString& symbol, double quantity)
{
	if (!m_positions.contains(symbol)) return;

	Position& position = m_positions[symbol];
	double salePrice = position.currentPrice();
	double proceeds = quantity * salePrice;

	// Calculate realized P&L
	double costBasis = quantity * position.averagePrice();
	double realizedPnL = proceeds - costBasis;
	m_realizedPnL += realizedPnL;
	m_cashBalance += proceeds;

	position.reduceQuantity(quantity);

	// Remove position if quantity is zero
	if (position.quantity() <= 0) {
		m_positions.remove(symbol);
	}

	// Record trade transaction
	Transaction transaction(TransactionType::Trade, proceeds,
		QString("Sell %1 shares of %2 @ $%3 (P&L: %4%5)")
		.arg(quantity).arg(symbol).arg(salePrice)
		.arg(realizedPnL >= 0 ? "+" : "")
		.arg(realizedPnL, 0, 'f', 2));
	transaction.setBalanceAfter(m_cashBalance);
	m_transactions.append(transaction);
}

Position* UserAccount::getPosition(const QString& symbol)
{
	if (m_positions.contains(symbol)) {
		return &m_positions[symbol];
	}
	return nullptr;
}

QList<Position> UserAccount::getAllPositions() const
{
	QList<Position> positions;
	for (auto it = m_positions.begin(); it != m_positions.end(); ++it) {
		positions.append(it.value());
	}
	return positions;
}

bool UserAccount::hasPosition(const QString& symbol) const
{
	return m_positions.contains(symbol);
}

void UserAccount::updatePositionPrice(const QString& symbol, double currentPrice)
{
	if (m_positions.contains(symbol)) {
		m_positions[symbol].setCurrentPrice(currentPrice);
	}
}

double UserAccount::totalDeposits() const
{
	double total = 0.0;
	for (const Transaction& trans : m_transactions) {
		if (trans.type() == TransactionType::Deposit) {
			total += trans.amount();
		}
	}
	return total;
}

double UserAccount::totalWithdrawals() const
{
	double total = 0.0;
	for (const Transaction& trans : m_transactions) {
		if (trans.type() == TransactionType::Withdrawal) {
			total += qAbs(trans.amount());
		}
	}
	return total;
}

double UserAccount::unrealizedPnL() const
{
	double total = 0.0;
	for (auto it = m_positions.begin(); it != m_positions.end(); ++it) {
		total += it.value().unrealizedPnL();
	}
	return total;
}