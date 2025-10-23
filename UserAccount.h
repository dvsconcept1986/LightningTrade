#pragma once
#include <QString>
#include <QDateTime>
#include <QMap>

enum class TransactionType {
	Deposit,
	Withdrawal,
	Trade,
	Dividend,
	Interest,
	Fee
};

class Transaction {
public:
	Transaction();
	Transaction(TransactionType type, double amount, const QString& description);

	QString transactionId() const { return m_transactionId; }
	TransactionType type() const { return m_type; }
	double amount() const { return m_amount; }
	QString description() const { return m_description; }
	QDateTime timestamp() const { return m_timestamp; }
	double balanceAfter() const { return m_balanceAfter; }

	void setBalanceAfter(double balance) { m_balanceAfter = balance; }

	static QString typeToString(TransactionType type);

private:
	QString m_transactionId;
	TransactionType m_type;
	double m_amount;
	QString m_description;
	QDateTime m_timestamp;
	double m_balanceAfter;
};

class Position {
public:
	Position();
	Position(const QString& symbol, double quantity, double averagePrice);

	QString symbol() const { return m_symbol; }
	double quantity() const { return m_quantity; }
	double averagePrice() const { return m_averagePrice; }
	double currentPrice() const { return m_currentPrice; }
	double marketValue() const { return m_quantity * m_currentPrice; }
	double costBasis() const { return m_quantity * m_averagePrice; }
	double unrealizedPnL() const { return marketValue() - costBasis(); }
	double unrealizedPnLPercent() const;

	void setCurrentPrice(double price) { m_currentPrice = price; }
	void addQuantity(double quantity, double price);
	void reduceQuantity(double quantity);

private:
	QString m_symbol;
	double m_quantity;
	double m_averagePrice;
	double m_currentPrice;
};

class UserAccount {
public:
	UserAccount();
	UserAccount(const QString& username, const QString& fullName, const QString& email);

	// Profile getters
	QString userId() const { return m_userId; }
	QString username() const { return m_username; }
	QString fullName() const { return m_fullName; }
	QString email() const { return m_email; }
	QString phoneNumber() const { return m_phoneNumber; }
	QString address() const { return m_address; }
	QDateTime createdDate() const { return m_createdDate; }

	// Profile setters
	void setUsername(const QString& username) { m_username = username; }
	void setFullName(const QString& fullName) { m_fullName = fullName; }
	void setEmail(const QString& email) { m_email = email; }
	void setPhoneNumber(const QString& phone) { m_phoneNumber = phone; }
	void setAddress(const QString& address) { m_address = address; }

	// Balance management
	double cashBalance() const { return m_cashBalance; }
	double portfolioValue() const;
	double totalAccountValue() const { return m_cashBalance + portfolioValue(); }
	double buyingPower() const;

	// Transaction operations
	bool deposit(double amount, const QString& description = "Deposit");
	bool withdraw(double amount, const QString& description = "Withdrawal");
	void addTransaction(const Transaction& transaction);
	QList<Transaction> getTransactions() const { return m_transactions; }
	QList<Transaction> getRecentTransactions(int count) const;

	// Position management
	void addPosition(const QString& symbol, double quantity, double price);
	void reducePosition(const QString& symbol, double quantity);
	Position* getPosition(const QString& symbol);
	QList<Position> getAllPositions() const;
	bool hasPosition(const QString& symbol) const;

	// Update positions with current market prices
	void updatePositionPrice(const QString& symbol, double currentPrice);

	// Statistics
	double totalDeposits() const;
	double totalWithdrawals() const;
	double realizedPnL() const { return m_realizedPnL; }
	double unrealizedPnL() const;
	double totalPnL() const { return m_realizedPnL + unrealizedPnL(); }

private:
	// Profile information
	QString m_userId;
	QString m_username;
	QString m_fullName;
	QString m_email;
	QString m_phoneNumber;
	QString m_address;
	QDateTime m_createdDate;

	// Balance and trading
	double m_cashBalance;
	double m_realizedPnL;

	// Positions and transactions
	QMap<QString, Position> m_positions;
	QList<Transaction> m_transactions;
};