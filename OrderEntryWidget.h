#pragma once
#include <QWidget>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QFormLayout>
#include <QLabel>
#include "Order.h"

class OrderEntryWidget : public QWidget
{
	Q_OBJECT

public:
	explicit OrderEntryWidget(QWidget* parent = nullptr);

signals:
	void orderRequested(const QString& symbol, OrderSide side, OrderType type,
		double quantity, double price);

private slots:
	void onSubmitClicked();
	void onOrderTypeChanged(int index);
	void onClearClicked();

private:
	void setupUI();
	bool validateInput();

private:
	QGroupBox* m_groupBox;
	QLineEdit* m_symbolEdit;
	QComboBox* m_sideCombo;
	QComboBox* m_typeCombo;
	QDoubleSpinBox* m_quantitySpinBox;
	QDoubleSpinBox* m_priceSpinBox;
	QComboBox* m_tifCombo;
	QPushButton* m_submitButton;
	QPushButton* m_clearButton;
	QLabel* m_statusLabel;
};