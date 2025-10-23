#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QTabWidget>
#include "AuthManager.h"

class LoginDialog : public QDialog
{
	Q_OBJECT

public:
	explicit LoginDialog(AuthManager* authManager, QWidget* parent = nullptr);

	bool isAuthenticated() const { return m_authenticated; }

private slots:
	void onLoginClicked();
	void onRegisterClicked();
	void onCancelClicked();
	void onShowPasswordToggled(bool checked);

private:
	void setupUI();
	void setupLoginTab();
	void setupRegisterTab();

private:
	AuthManager* m_authManager;
	bool m_authenticated;

	QTabWidget* m_tabWidget;

	// Login tab
	QWidget* m_loginTab;
	QLineEdit* m_loginUsernameEdit;
	QLineEdit* m_loginPasswordEdit;
	QPushButton* m_loginButton;
	QPushButton* m_loginCancelButton;
	QCheckBox* m_loginShowPasswordCheck;
	QLabel* m_loginStatusLabel;

	// Register tab
	QWidget* m_registerTab;
	QLineEdit* m_registerUsernameEdit;
	QLineEdit* m_registerFullNameEdit;
	QLineEdit* m_registerEmailEdit;
	QLineEdit* m_registerPasswordEdit;
	QLineEdit* m_registerConfirmPasswordEdit;
	QPushButton* m_registerButton;
	QPushButton* m_registerCancelButton;
	QCheckBox* m_registerShowPasswordCheck;
	QLabel* m_registerStatusLabel;
	QLabel* m_passwordRequirementsLabel;
};