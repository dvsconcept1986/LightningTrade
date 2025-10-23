#include "AccountWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QGroupBox>

AccountWidget::AccountWidget(UserAccount* account, QWidget* parent)
	: QWidget(parent)
	, m_account(account)
{
	setupUI();
	updateDisplay();
}

void AccountWidget::setupUI()
{
	QVBoxLayout* mainLayout = new QVBoxLayout(this);

	m_tabWidget = new QTabWidget(this);

	setupProfileTab();
	setupBalanceTab();
	setupPositionsTab();
	setupTransactionsTab();

	m_tabWidget->addTab(m_profileTab, "Profile");
	m_tabWidget->addTab(m_balanceTab, "Balance");
	m_tabWidget->addTab(m_positionsTab, "Positions");
	m_tabWidget->addTab(m_transactionsTab, "Transactions");

	mainLayout->addWidget(m_tabWidget);
}

void AccountWidget::setupProfileTab()
{
	m_profileTab = new QWidget();
	QVBoxLayout* layout = new QVBoxLayout(m_profileTab);

	QGroupBox* profileGroup = new QGroupBox("Profile Information");
	QFormLayout* formLayout = new QFormLayout();

	// User ID (read-only)
	m_userIdLabel = new QLabel();
	m_userIdLabel->setStyleSheet("QLabel { color: #888; }");
	formLayout->addRow("User ID:", m_userIdLabel);

	// Username
	m_usernameEdit = new QLineEdit();
	formLayout->addRow("Username:", m_usernameEdit);

	// Full Name
	m_fullNameEdit = new QLineEdit();
	formLayout->addRow("Full Name:", m_fullNameEdit);

	// Email
	m_emailEdit = new QLineEdit();
	formLayout->addRow("Email:", m_emailEdit);

	// Phone
	m_phoneEdit = new QLineEdit();
	formLayout->addRow("Phone:", m_phoneEdit);

	// Address
	m_addressEdit = new QTextEdit();
	m_addressEdit->setMaximumHeight(80);
	formLayout->addRow("Address:", m_addressEdit);

	// Created Date (read-only)
	m_createdDateLabel = new QLabel();
	m_createdDateLabel->setStyleSheet("QLabel { color: #888; }");
	formLayout->addRow("Member Since:", m_createdDateLabel);

	profileGroup->setLayout(formLayout);

	// Save button
	m_saveProfileButton = new QPushButton("Save Profile");
	m_saveProfileButton->setStyleSheet("QPushButton { background-color: #2a82da; font-weight: bold; }");
	connect(m_saveProfileButton, &QPushButton::clicked, this, &AccountWidget::onSaveProfileClicked);

	layout->addWidget(profileGroup);
	layout->addWidget(m_saveProfileButton);
	layout->addStretch();
}

void AccountWidget::setupBalanceTab()
{
	m_balanceTab = new QWidget();
	QVBoxLayout* layout = new QVBoxLayout(m_balanceTab);

	// Account Summary
	QGroupBox* summaryGroup = new QGroupBox("Account Summary");
	QFormLayout* summaryLayout = new QFormLayout();

	m_cashBalanceLabel = new QLabel("$0.00");
	m_cashBalanceLabel->setStyleSheet("QLabel { font-size: 14pt; font-weight: bold; }");
	summaryLayout->addRow("Cash Balance:", m_cashBalanceLabel);

	m_portfolioValueLabel = new QLabel("$0.00");
	m_portfolioValueLabel->setStyleSheet("QLabel { font-size: 14pt; }");
	summaryLayout->addRow("Portfolio Value:", m_portfolioValueLabel);

	m_totalValueLabel = new QLabel("$0.00");
	m_totalValueLabel->setStyleSheet("QLabel { font-size: 16pt; font-weight: bold; color: #2a82da; }");
	summaryLayout->addRow("Total Account Value:", m_totalValueLabel);

	m_buyingPowerLabel = new QLabel("$0.00");
	summaryLayout->addRow("Buying Power:", m_buyingPowerLabel);

	summaryGroup->setLayout(summaryLayout);

	// P&L Summary
	QGroupBox* pnlGroup = new QGroupBox("Profit & Loss");
	QFormLayout* pnlLayout = new QFormLayout();

	m_unrealizedPnLLabel = new QLabel("$0.00");
	pnlLayout->addRow("Unrealized P&L:", m_unrealizedPnLLabel);

	m_realizedPnLLabel = new QLabel("$0.00");
	pnlLayout->addRow("Realized P&L:", m_realizedPnLLabel);

	m_totalPnLLabel = new QLabel("$0.00");
	m_totalPnLLabel->setStyleSheet("QLabel { font-weight: bold; }");
	pnlLayout->addRow("Total P&L:", m_totalPnLLabel);

	pnlGroup->setLayout(pnlLayout);

	// Deposit/Withdraw
	QGroupBox* transactionGroup = new QGroupBox("Deposit / Withdraw");
	QVBoxLayout* transLayout = new QVBoxLayout();

	// Deposit
	QHBoxLayout* depositLayout = new QHBoxLayout();
	depositLayout->addWidget(new QLabel("Deposit:"));
	m_depositAmountSpinBox = new QDoubleSpinBox();
	m_depositAmountSpinBox->setRange(0, 1000000);
	m_depositAmountSpinBox->setDecimals(2);
	m_depositAmountSpinBox->setValue(1000.00);
	m_depositAmountSpinBox->setPrefix("$ ");
	depositLayout->addWidget(m_depositAmountSpinBox);
	m_depositButton = new QPushButton("Deposit");
	m_depositButton->setStyleSheet("QPushButton { background-color: #00c800; }");
	connect(m_depositButton, &QPushButton::clicked, this, &AccountWidget::onDepositClicked);
	depositLayout->addWidget(m_depositButton);

	// Withdraw
	QHBoxLayout* withdrawLayout = new QHBoxLayout();
	withdrawLayout->addWidget(new QLabel("Withdraw:"));
	m_withdrawAmountSpinBox = new QDoubleSpinBox();
	m_withdrawAmountSpinBox->setRange(0, 1000000);
	m_withdrawAmountSpinBox->setDecimals(2);
	m_withdrawAmountSpinBox->setValue(500.00);
	m_withdrawAmountSpinBox->setPrefix("$ ");
	withdrawLayout->addWidget(m_withdrawAmountSpinBox);
	m_withdrawButton = new QPushButton("Withdraw");
	m_withdrawButton->setStyleSheet("QPushButton { background-color: #ff6464; }");
	connect(m_withdrawButton, &QPushButton::clicked, this, &AccountWidget::onWithdrawClicked);
	withdrawLayout->addWidget(m_withdrawButton);

	transLayout->addLayout(depositLayout);
	transLayout->addLayout(withdrawLayout);
	transactionGroup->setLayout(transLayout);

	layout->addWidget(summaryGroup);
	layout->addWidget(pnlGroup);
	layout->addWidget(transactionGroup);
	layout->addStretch();
}

void AccountWidget::setupPositionsTab()
{
	m_positionsTab = new QWidget();
	QVBoxLayout* layout = new QVBoxLayout(m_positionsTab);

	QHBoxLayout* toolbarLayout = new QHBoxLayout();
	toolbarLayout->addStretch();
	m_refreshPositionsButton = new QPushButton("Refresh");
	connect(m_refreshPositionsButton, &QPushButton::clicked, this, &AccountWidget::onRefreshClicked);
	toolbarLayout->addWidget(m_refreshPositionsButton);

	m_positionsTable = new QTableWidget(0, 8);
	QStringList headers = { "Symbol", "Quantity", "Avg Price", "Current Price",
						  "Market Value", "Cost Basis", "Unrealized P&L", "P&L %" };
	m_positionsTable->setHorizontalHeaderLabels(headers);
	m_positionsTable->setAlternatingRowColors(true);
	m_positionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_positionsTable->horizontalHeader()->setStretchLastSection(true);
	m_positionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

	layout->addLayout(toolbarLayout);
	layout->addWidget(m_positionsTable);
}

void AccountWidget::setupTransactionsTab()
{
	m_transactionsTab = new QWidget();
	QVBoxLayout* layout = new QVBoxLayout(m_transactionsTab);

	QHBoxLayout* toolbarLayout = new QHBoxLayout();
	toolbarLayout->addStretch();
	m_refreshTransactionsButton = new QPushButton("Refresh");
	connect(m_refreshTransactionsButton, &QPushButton::clicked, this, &AccountWidget::onRefreshClicked);
	toolbarLayout->addWidget(m_refreshTransactionsButton);

	m_transactionsTable = new QTableWidget(0, 5);
	QStringList headers = { "Date/Time", "Type", "Description", "Amount", "Balance" };
	m_transactionsTable->setHorizontalHeaderLabels(headers);
	m_transactionsTable->setAlternatingRowColors(true);
	m_transactionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_transactionsTable->horizontalHeader()->setStretchLastSection(true);
	m_transactionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

	layout->addLayout(toolbarLayout);
	layout->addWidget(m_transactionsTable);
}

void AccountWidget::updateDisplay()
{
	if (!m_account) return;

	// Update profile
	m_userIdLabel->setText(m_account->userId().left(8) + "...");
	m_usernameEdit->setText(m_account->username());
	m_fullNameEdit->setText(m_account->fullName());
	m_emailEdit->setText(m_account->email());
	m_phoneEdit->setText(m_account->phoneNumber());
	m_addressEdit->setPlainText(m_account->address());
	m_createdDateLabel->setText(m_account->createdDate().toString("MMM dd, yyyy"));

	// Update balance
	m_cashBalanceLabel->setText(QString("$%1").arg(m_account->cashBalance(), 0, 'f', 2));
	m_portfolioValueLabel->setText(QString("$%1").arg(m_account->portfolioValue(), 0, 'f', 2));
	m_totalValueLabel->setText(QString("$%1").arg(m_account->totalAccountValue(), 0, 'f', 2));
	m_buyingPowerLabel->setText(QString("$%1").arg(m_account->buyingPower(), 0, 'f', 2));

	// Update P&L with colors
	double unrealizedPnL = m_account->unrealizedPnL();
	double realizedPnL = m_account->realizedPnL();
	double totalPnL = m_account->totalPnL();

	m_unrealizedPnLLabel->setText(QString("%1$%2")
		.arg(unrealizedPnL >= 0 ? "+" : "")
		.arg(qAbs(unrealizedPnL), 0, 'f', 2));
	m_unrealizedPnLLabel->setStyleSheet(QString("QLabel { color: %1; }")
		.arg(unrealizedPnL >= 0 ? "#00c800" : "#ff6464"));

	m_realizedPnLLabel->setText(QString("%1$%2")
		.arg(realizedPnL >= 0 ? "+" : "")
		.arg(qAbs(realizedPnL), 0, 'f', 2));
	m_realizedPnLLabel->setStyleSheet(QString("QLabel { color: %1; }")
		.arg(realizedPnL >= 0 ? "#00c800" : "#ff6464"));

	m_totalPnLLabel->setText(QString("%1$%2")
		.arg(totalPnL >= 0 ? "+" : "")
		.arg(qAbs(totalPnL), 0, 'f', 2));
	m_totalPnLLabel->setStyleSheet(QString("QLabel { color: %1; font-weight: bold; }")
		.arg(totalPnL >= 0 ? "#00c800" : "#ff6464"));

	updatePositions();
	updateTransactionHistory();
}

void AccountWidget::updatePositions()
{
	if (!m_account) return;

	m_positionsTable->setRowCount(0);

	QList<Position> positions = m_account->getAllPositions();
	for (const Position& pos : positions) {
		int row = m_positionsTable->rowCount();
		m_positionsTable->insertRow(row);

		m_positionsTable->setItem(row, 0, new QTableWidgetItem(pos.symbol()));
		m_positionsTable->setItem(row, 1, new QTableWidgetItem(QString::number(pos.quantity(), 'f', 2)));
		m_positionsTable->setItem(row, 2, new QTableWidgetItem(QString("$%1").arg(pos.averagePrice(), 0, 'f', 2)));
		m_positionsTable->setItem(row, 3, new QTableWidgetItem(QString("$%1").arg(pos.currentPrice(), 0, 'f', 2)));
		m_positionsTable->setItem(row, 4, new QTableWidgetItem(QString("$%1").arg(pos.marketValue(), 0, 'f', 2)));
		m_positionsTable->setItem(row, 5, new QTableWidgetItem(QString("$%1").arg(pos.costBasis(), 0, 'f', 2)));

		// Unrealized P&L with color
		double pnl = pos.unrealizedPnL();
		QTableWidgetItem* pnlItem = new QTableWidgetItem(QString("%1$%2")
			.arg(pnl >= 0 ? "+" : "")
			.arg(qAbs(pnl), 0, 'f', 2));
		pnlItem->setForeground(pnl >= 0 ? QColor(0, 200, 0) : QColor(255, 100, 100));
		m_positionsTable->setItem(row, 6, pnlItem);

		// P&L % with color
		double pnlPercent = pos.unrealizedPnLPercent();
		QTableWidgetItem* percentItem = new QTableWidgetItem(QString("%1%2%")
			.arg(pnlPercent >= 0 ? "+" : "")
			.arg(pnlPercent, 0, 'f', 2));
		percentItem->setForeground(pnlPercent >= 0 ? QColor(0, 200, 0) : QColor(255, 100, 100));
		m_positionsTable->setItem(row, 7, percentItem);
	}
}

void AccountWidget::updateTransactionHistory()
{
	if (!m_account) return;

	m_transactionsTable->setRowCount(0);

	QList<Transaction> transactions = m_account->getTransactions();

	// Show most recent first
	for (int i = transactions.size() - 1; i >= 0; --i) {
		const Transaction& trans = transactions[i];
		int row = m_transactionsTable->rowCount();
		m_transactionsTable->insertRow(row);

		m_transactionsTable->setItem(row, 0, new QTableWidgetItem(trans.timestamp().toString("MM/dd/yyyy hh:mm:ss")));
		m_transactionsTable->setItem(row, 1, new QTableWidgetItem(Transaction::typeToString(trans.type())));
		m_transactionsTable->setItem(row, 2, new QTableWidgetItem(trans.description()));

		// Amount with color
		double amount = trans.amount();
		QTableWidgetItem* amountItem = new QTableWidgetItem(QString("%1$%2")
			.arg(amount >= 0 ? "+" : "")
			.arg(qAbs(amount), 0, 'f', 2));
		amountItem->setForeground(amount >= 0 ? QColor(0, 200, 0) : QColor(255, 100, 100));
		m_transactionsTable->setItem(row, 3, amountItem);

		m_transactionsTable->setItem(row, 4, new QTableWidgetItem(QString("$%1").arg(trans.balanceAfter(), 0, 'f', 2)));
	}
}

void AccountWidget::onDepositClicked()
{
	double amount = m_depositAmountSpinBox->value();

	if (amount <= 0) {
		QMessageBox::warning(this, "Invalid Amount", "Deposit amount must be greater than zero.");
		return;
	}

	QMessageBox::StandardButton reply = QMessageBox::question(
		this, "Confirm Deposit",
		QString("Deposit $%1 into your account?").arg(amount, 0, 'f', 2),
		QMessageBox::Yes | QMessageBox::No
	);

	if (reply == QMessageBox::Yes) {
		if (m_account->deposit(amount, "Account Deposit")) {
			QMessageBox::information(this, "Deposit Successful",
				QString("$%1 has been deposited to your account.").arg(amount, 0, 'f', 2));
			updateDisplay();
			emit depositRequested(amount);
		}
	}
}

void AccountWidget::onWithdrawClicked()
{
	double amount = m_withdrawAmountSpinBox->value();

	if (amount <= 0) {
		QMessageBox::warning(this, "Invalid Amount", "Withdrawal amount must be greater than zero.");
		return;
	}

	if (amount > m_account->cashBalance()) {
		QMessageBox::warning(this, "Insufficient Funds",
			QString("Insufficient cash balance. Available: $%1").arg(m_account->cashBalance(), 0, 'f', 2));
		return;
	}

	QMessageBox::StandardButton reply = QMessageBox::question(
		this, "Confirm Withdrawal",
		QString("Withdraw $%1 from your account?").arg(amount, 0, 'f', 2),
		QMessageBox::Yes | QMessageBox::No
	);

	if (reply == QMessageBox::Yes) {
		if (m_account->withdraw(amount, "Account Withdrawal")) {
			QMessageBox::information(this, "Withdrawal Successful",
				QString("$%1 has been withdrawn from your account.").arg(amount, 0, 'f', 2));
			updateDisplay();
			emit withdrawalRequested(amount);
		}
	}
}

void AccountWidget::onSaveProfileClicked()
{
	m_account->setFullName(m_fullNameEdit->text());
	m_account->setUsername(m_usernameEdit->text());
	m_account->setEmail(m_emailEdit->text());
	m_account->setPhoneNumber(m_phoneEdit->text());
	m_account->setAddress(m_addressEdit->toPlainText());

	QMessageBox::information(this, "Profile Saved", "Your profile has been updated successfully.");
	emit profileUpdated();
}

void AccountWidget::onRefreshClicked()
{
	updateDisplay();
}