#include "OrderEntryWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>

OrderEntryWidget::OrderEntryWidget(QWidget* parent)
	: QWidget(parent)
{
	setupUI();
}

void OrderEntryWidget::setupUI()
{
	m_groupBox = new QGroupBox("Order Entry", this);

	QFormLayout* formLayout = new QFormLayout();

	// Symbol
	m_symbolEdit = new QLineEdit(this);
	m_symbolEdit->setPlaceholderText("e.g., AAPL");
	m_symbolEdit->setMaxLength(10);
	formLayout->addRow("Symbol:", m_symbolEdit);

	// Side
	m_sideCombo = new QComboBox(this);
	m_sideCombo->addItem("BUY", static_cast<int>(OrderSide::Buy));
	m_sideCombo->addItem("SELL", static_cast<int>(OrderSide::Sell));
	formLayout->addRow("Side:", m_sideCombo);

	// Order Type
	m_typeCombo = new QComboBox(this);
	m_typeCombo->addItem("MARKET", static_cast<int>(OrderType::Market));
	m_typeCombo->addItem("LIMIT", static_cast<int>(OrderType::Limit));
	m_typeCombo->addItem("STOP", static_cast<int>(OrderType::Stop));
	m_typeCombo->addItem("STOP LIMIT", static_cast<int>(OrderType::StopLimit));
	connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &OrderEntryWidget::onOrderTypeChanged);
	formLayout->addRow("Type:", m_typeCombo);

	// Quantity
	m_quantitySpinBox = new QDoubleSpinBox(this);
	m_quantitySpinBox->setRange(1, 1000000);
	m_quantitySpinBox->setDecimals(2);
	m_quantitySpinBox->setValue(100);
	m_quantitySpinBox->setSuffix(" shares");
	formLayout->addRow("Quantity:", m_quantitySpinBox);

	// Price
	m_priceSpinBox = new QDoubleSpinBox(this);
	m_priceSpinBox->setRange(0.01, 999999.99);
	m_priceSpinBox->setDecimals(2);
	m_priceSpinBox->setValue(100.00);
	m_priceSpinBox->setPrefix("$ ");
	m_priceSpinBox->setEnabled(false); // Disabled for market orders by default
	formLayout->addRow("Price:", m_priceSpinBox);

	// Time in Force
	m_tifCombo = new QComboBox(this);
	m_tifCombo->addItem("DAY", static_cast<int>(TimeInForce::Day));
	m_tifCombo->addItem("GTC", static_cast<int>(TimeInForce::GTC));
	m_tifCombo->addItem("IOC", static_cast<int>(TimeInForce::IOC));
	m_tifCombo->addItem("FOK", static_cast<int>(TimeInForce::FOK));
	formLayout->addRow("Time in Force:", m_tifCombo);

	// Buttons
	QHBoxLayout* buttonLayout = new QHBoxLayout();
	m_submitButton = new QPushButton("Submit Order", this);
	m_submitButton->setStyleSheet("QPushButton { background-color: #2a82da; font-weight: bold; }");
	m_clearButton = new QPushButton("Clear", this);

	buttonLayout->addWidget(m_submitButton);
	buttonLayout->addWidget(m_clearButton);

	connect(m_submitButton, &QPushButton::clicked, this, &OrderEntryWidget::onSubmitClicked);
	connect(m_clearButton, &QPushButton::clicked, this, &OrderEntryWidget::onClearClicked);

	// Status label
	m_statusLabel = new QLabel(this);
	m_statusLabel->setStyleSheet("QLabel { color: #2a82da; }");

	// Main layout
	QVBoxLayout* mainLayout = new QVBoxLayout(m_groupBox);
	mainLayout->addLayout(formLayout);
	mainLayout->addLayout(buttonLayout);
	mainLayout->addWidget(m_statusLabel);

	QVBoxLayout* widgetLayout = new QVBoxLayout(this);
	widgetLayout->addWidget(m_groupBox);
	widgetLayout->setContentsMargins(0, 0, 0, 0);
}

void OrderEntryWidget::onSubmitClicked()
{
	if (!validateInput()) {
		return;
	}

	QString symbol = m_symbolEdit->text().toUpper();
	OrderSide side = static_cast<OrderSide>(m_sideCombo->currentData().toInt());
	OrderType type = static_cast<OrderType>(m_typeCombo->currentData().toInt());
	double quantity = m_quantitySpinBox->value();
	double price = m_priceSpinBox->value();

	emit orderRequested(symbol, side, type, quantity, price);

	m_statusLabel->setText("Order submitted...");
	m_statusLabel->setStyleSheet("QLabel { color: #2a82da; }");
}

void OrderEntryWidget::onOrderTypeChanged(int index)
{
	OrderType type = static_cast<OrderType>(m_typeCombo->currentData().toInt());

	// Enable/disable price based on order type
	bool needsPrice = (type == OrderType::Limit || type == OrderType::StopLimit);
	m_priceSpinBox->setEnabled(needsPrice);
}

void OrderEntryWidget::onClearClicked()
{
	m_symbolEdit->clear();
	m_sideCombo->setCurrentIndex(0);
	m_typeCombo->setCurrentIndex(0);
	m_quantitySpinBox->setValue(100);
	m_priceSpinBox->setValue(100.00);
	m_tifCombo->setCurrentIndex(0);
	m_statusLabel->clear();
}

bool OrderEntryWidget::validateInput()
{
	if (m_symbolEdit->text().trimmed().isEmpty()) {
		QMessageBox::warning(this, "Validation Error", "Please enter a symbol.");
		m_symbolEdit->setFocus();
		return false;
	}

	if (m_quantitySpinBox->value() <= 0) {
		QMessageBox::warning(this, "Validation Error", "Quantity must be greater than 0.");
		m_quantitySpinBox->setFocus();
		return false;
	}

	OrderType type = static_cast<OrderType>(m_typeCombo->currentData().toInt());
	if ((type == OrderType::Limit || type == OrderType::StopLimit) && m_priceSpinBox->value() <= 0) {
		QMessageBox::warning(this, "Validation Error", "Price must be greater than 0 for limit orders.");
		m_priceSpinBox->setFocus();
		return false;
	}

	return true;
}