#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include "Order.h"

class OrderBlotterWidget : public QWidget
{
	Q_OBJECT

public:
	explicit OrderBlotterWidget(QWidget* parent = nullptr);

	void addOrder(Order* order);
	void updateOrder(Order* order);
	void clearOrders();

signals:
	void cancelOrderRequested(const QString& orderId);
	void modifyOrderRequested(const QString& orderId);

private slots:
	void onCancelClicked();
	void onModifyClicked();
	void onFilterChanged(int index);
	void onRefreshClicked();

private:
	void setupUI();
	void updateOrderRow(int row, Order* order);
	int findOrderRow(const QString& orderId);
	QString formatDateTime(const QDateTime& dt);

private:
	QGroupBox* m_groupBox;
	QTableWidget* m_orderTable;
	QComboBox* m_filterCombo;
	QPushButton* m_cancelButton;
	QPushButton* m_modifyButton;
	QPushButton* m_refreshButton;
};