#include "OrderBlotterWidget.h"
#include <QHeaderView>
#include <QMessageBox>

OrderBlotterWidget::OrderBlotterWidget(QWidget* parent)
	: QWidget(parent)
{
	setupUI();
}

void OrderBlotterWidget::setupUI()
{
	m_groupBox = new QGroupBox("Order Blotter", this);

	QVBoxLayout* mainLayout = new QVBoxLayout(m_groupBox);

	// Toolbar
	QHBoxLayout* toolbarLayout = new QHBoxLayout();

	m_filterCombo = new QComboBox(this);
	m_filterCombo->addItem("All Orders", -1);
	m_filterCombo->addItem("Active Only", static_cast<int>(OrderStatus::New));
	m_filterCombo->addItem("Filled", static_cast<int>(OrderStatus::Filled));
	m_filterCombo->addItem("Cancelled", static_cast<int>(OrderStatus::Cancelled));
	connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &OrderBlotterWidget::onFilterChanged);

	m_refreshButton = new QPushButton("Refresh", this);
	m_cancelButton = new QPushButton("Cancel Order", this);
	m_modifyButton = new QPushButton("Modify Order", this);

	m_cancelButton->setEnabled(false);
	m_modifyButton->setEnabled(false);

	connect(m_refreshButton, &QPushButton::clicked, this, &OrderBlotterWidget::onRefreshClicked);
	connect(m_cancelButton, &QPushButton::clicked, this, &OrderBlotterWidget::onCancelClicked);
	connect(m_modifyButton, &QPushButton::clicked, this, &OrderBlotterWidget::onModifyClicked);

	toolbarLayout->addWidget(new QLabel("Filter:", this));
	toolbarLayout->addWidget(m_filterCombo);
	toolbarLayout->addStretch();
	toolbarLayout->addWidget(m_refreshButton);
	toolbarLayout->addWidget(m_modifyButton);
	toolbarLayout->addWidget(m_cancelButton);

	// Order table
	m_orderTable = new QTableWidget(0, 11, this);
	QStringList headers = {
		"Order ID", "Symbol", "Side", "Type", "Status",
		"Quantity", "Filled", "Price", "Avg Fill Price",
		"Time", "Last Update"
	};
	m_orderTable->setHorizontalHeaderLabels(headers);
	m_orderTable->setAlternatingRowColors(true);
	m_orderTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_orderTable->setSelectionMode(QAbstractItemView::SingleSelection);
	m_orderTable->horizontalHeader()->setStretchLastSection(true);
	m_orderTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

	// Set column widths
	m_orderTable->setColumnWidth(0, 80);  // Order ID (shortened)
	m_orderTable->setColumnWidth(1, 80);  // Symbol
	m_orderTable->setColumnWidth(2, 60);  // Side
	m_orderTable->setColumnWidth(3, 80);  // Type
	m_orderTable->setColumnWidth(4, 120); // Status
	m_orderTable->setColumnWidth(5, 80);  // Quantity
	m_orderTable->setColumnWidth(6, 80);  // Filled
	m_orderTable->setColumnWidth(7, 80);  // Price
	m_orderTable->setColumnWidth(8, 100); // Avg Fill
	m_orderTable->setColumnWidth(9, 150); // Time

	connect(m_orderTable, &QTableWidget::itemSelectionChanged, this, [this]() {
		bool hasSelection = !m_orderTable->selectedItems().isEmpty();
		m_cancelButton->setEnabled(hasSelection);
		m_modifyButton->setEnabled(hasSelection);
		});

	mainLayout->addLayout(toolbarLayout);
	mainLayout->addWidget(m_orderTable);

	QVBoxLayout* widgetLayout = new QVBoxLayout(this);
	widgetLayout->addWidget(m_groupBox);
	widgetLayout->setContentsMargins(0, 0, 0, 0);
}

void OrderBlotterWidget::addOrder(Order* order)
{
	if (!order) return;

	int row = m_orderTable->rowCount();
	m_orderTable->insertRow(row);
	updateOrderRow(row, order);
}

void OrderBlotterWidget::updateOrder(Order* order)
{
	if (!order) return;

	int row = findOrderRow(order->orderId());
	if (row >= 0) {
		updateOrderRow(row, order);
	}
	else {
		addOrder(order);
	}
}

void OrderBlotterWidget::clearOrders()
{
	m_orderTable->setRowCount(0);
}

void OrderBlotterWidget::updateOrderRow(int row, Order* order)
{
	if (!order || row < 0 || row >= m_orderTable->rowCount()) return;

	// Shorten order ID for display
	QString shortId = order->orderId().left(8);

	m_orderTable->setItem(row, 0, new QTableWidgetItem(shortId));
	m_orderTable->setItem(row, 1, new QTableWidgetItem(order->symbol()));
	m_orderTable->setItem(row, 2, new QTableWidgetItem(Order::sideToString(order->side())));
	m_orderTable->setItem(row, 3, new QTableWidgetItem(Order::typeToString(order->type())));

	// Color-code status
	QTableWidgetItem* statusItem = new QTableWidgetItem(Order::statusToString(order->status()));
	switch (order->status()) {
	case OrderStatus::Filled:
		statusItem->setForeground(QColor(0, 200, 0));
		break;
	case OrderStatus::Cancelled:
	case OrderStatus::Rejected:
		statusItem->setForeground(QColor(255, 100, 100));
		break;
	case OrderStatus::PartiallyFilled:
		statusItem->setForeground(QColor(255, 200, 0));
		break;
	default:
		statusItem->setForeground(QColor(100, 150, 255));
		break;
	}
	m_orderTable->setItem(row, 4, statusItem);

	m_orderTable->setItem(row, 5, new QTableWidgetItem(QString::number(order->quantity(), 'f', 2)));
	m_orderTable->setItem(row, 6, new QTableWidgetItem(QString::number(order->filledQuantity(), 'f', 2)));
	m_orderTable->setItem(row, 7, new QTableWidgetItem(QString::number(order->price(), 'f', 2)));

	QString avgFill = order->filledQuantity() > 0 ?
		QString::number(order->averageFillPrice(), 'f', 2) : "-";
	m_orderTable->setItem(row, 8, new QTableWidgetItem(avgFill));

	m_orderTable->setItem(row, 9, new QTableWidgetItem(formatDateTime(order->createdTime())));
	m_orderTable->setItem(row, 10, new QTableWidgetItem(formatDateTime(order->lastUpdateTime())));

	// Store full order ID in hidden column
	m_orderTable->item(row, 0)->setData(Qt::UserRole, order->orderId());
}

int OrderBlotterWidget::findOrderRow(const QString& orderId)
{
	for (int row = 0; row < m_orderTable->rowCount(); ++row) {
		QTableWidgetItem* item = m_orderTable->item(row, 0);
		if (item && item->data(Qt::UserRole).toString() == orderId) {
			return row;
		}
	}
	return -1;
}

QString OrderBlotterWidget::formatDateTime(const QDateTime& dt)
{
	return dt.toString("MM/dd hh:mm:ss");
}

void OrderBlotterWidget::onCancelClicked()
{
	int currentRow = m_orderTable->currentRow();
	if (currentRow < 0) return;

	QTableWidgetItem* item = m_orderTable->item(currentRow, 0);
	if (!item) return;

	QString orderId = item->data(Qt::UserRole).toString();

	QMessageBox::StandardButton reply = QMessageBox::question(
		this, "Cancel Order",
		QString("Are you sure you want to cancel order %1?").arg(orderId.left(8)),
		QMessageBox::Yes | QMessageBox::No
	);

	if (reply == QMessageBox::Yes) {
		emit cancelOrderRequested(orderId);
	}
}

void OrderBlotterWidget::onModifyClicked()
{
	int currentRow = m_orderTable->currentRow();
	if (currentRow < 0) return;

	QTableWidgetItem* item = m_orderTable->item(currentRow, 0);
	if (!item) return;

	QString orderId = item->data(Qt::UserRole).toString();
	emit modifyOrderRequested(orderId);
}

void OrderBlotterWidget::onFilterChanged(int index)
{
	emit onRefreshClicked();
}

void OrderBlotterWidget::onRefreshClicked()
{
	// This will be handled by MainWindow to refresh data from OrderManager
}