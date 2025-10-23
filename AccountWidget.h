#pragma once
#include <QWidget>
#include <QTabWidget>
#include <QGroupBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QDoubleSpinBox>
#include "UserAccount.h"

class AccountWidget : public QWidget
{
	Q_OBJECT

public:
	explicit AccountWidget(UserAccount* account, QWidget* parent = nullptr);

	void updateDisplay();
	void updatePositions();
	void updateTransactionHistory();

signals:
	void depositRequested(double amount);
	void withdrawalRequested(double amount);
	void profileUpdated();

private slots:
	void onDepositClicked();
	void onWithdrawClicked();
	void onSaveProfileClicked();
	void onRefreshClicked();

private:
	void setupUI();
	void setupProfileTab();
	void setupBalanceTab();
	void setupPositionsTab();
	void setupTransactionsTab();

private:
	UserAccount* m_account;

	QTabWidget* m_tabWidget;

	// Profile tab
	QWidget* m_profileTab;
	QLineEdit* m_fullNameEdit;
	QLineEdit* m_usernameEdit;
	QLineEdit* m_emailEdit;
	QLineEdit* m_phoneEdit;
	QTextEdit* m_addressEdit;
	QLabel* m_userIdLabel;
	QLabel* m_createdDateLabel;
	QPushButton* m_saveProfileButton;

	// Balance tab
	QWidget* m_balanceTab;
	QLabel* m_cashBalanceLabel;
	QLabel* m_portfolioValueLabel;
	QLabel* m_totalValueLabel;
	QLabel* m_buyingPowerLabel;
	QLabel* m_unrealizedPnLLabel;
	QLabel* m_realizedPnLLabel;
	QLabel* m_totalPnLLabel;
	QDoubleSpinBox* m_depositAmountSpinBox;
	QDoubleSpinBox* m_withdrawAmountSpinBox;
	QPushButton* m_depositButton;
	QPushButton* m_withdrawButton;

	// Positions tab
	QWidget* m_positionsTab;
	QTableWidget* m_positionsTable;
	QPushButton* m_refreshPositionsButton;

	// Transactions tab
	QWidget* m_transactionsTab;
	QTableWidget* m_transactionsTable;
	QPushButton* m_refreshTransactionsButton;
};