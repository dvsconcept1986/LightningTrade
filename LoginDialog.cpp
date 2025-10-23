#include "LoginDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QGroupBox>

LoginDialog::LoginDialog(AuthManager* authManager, QWidget* parent)
	: QDialog(parent)
	, m_authManager(authManager)
	, m_authenticated(false)
{
	setupUI();

	// Connect auth manager signals
	connect(m_authManager, &AuthManager::loginSuccessful, this, [this](const QString& username) {
		m_authenticated = true;
		accept();
		});

	connect(m_authManager, &AuthManager::loginFailed, this, [this](const QString& reason) {
		m_loginStatusLabel->setText(reason);
		m_loginStatusLabel->setStyleSheet("QLabel { color: #ff6464; }");
		});

	connect(m_authManager, &AuthManager::registrationSuccessful, this, [this](const QString& username) {
		QMessageBox::information(this, "Registration Successful",
			QString("Account '%1' created successfully!\nYou can now login with your credentials.")
			.arg(username));
		m_tabWidget->setCurrentIndex(0); // Switch to login tab
		m_loginUsernameEdit->setText(username);
		m_loginPasswordEdit->clear();
		m_loginPasswordEdit->setFocus();
		});

	connect(m_authManager, &AuthManager::registrationFailed, this, [this](const QString& reason) {
		m_registerStatusLabel->setText(reason);
		m_registerStatusLabel->setStyleSheet("QLabel { color: #ff6464; }");
		});
}

void LoginDialog::setupUI()
{
	setWindowTitle("Lightning Trade - Login");
	setModal(true);
	setMinimumWidth(400);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);

	// Logo/Title
	QLabel* titleLabel = new QLabel("Lightning Trade", this);
	titleLabel->setStyleSheet("QLabel { font-size: 24pt; font-weight: bold; color: #2a82da; }");
	titleLabel->setAlignment(Qt::AlignCenter);
	mainLayout->addWidget(titleLabel);

	QLabel* subtitleLabel = new QLabel("Professional Trading Platform", this);
	subtitleLabel->setStyleSheet("QLabel { font-size: 10pt; color: #888; }");
	subtitleLabel->setAlignment(Qt::AlignCenter);
	mainLayout->addWidget(subtitleLabel);

	mainLayout->addSpacing(20);

	// Tab widget
	m_tabWidget = new QTabWidget(this);
	setupLoginTab();
	setupRegisterTab();

	m_tabWidget->addTab(m_loginTab, "Login");
	m_tabWidget->addTab(m_registerTab, "Register");

	mainLayout->addWidget(m_tabWidget);
}

void LoginDialog::setupLoginTab()
{
	m_loginTab = new QWidget();
	QVBoxLayout* layout = new QVBoxLayout(m_loginTab);

	QGroupBox* loginGroup = new QGroupBox("Login to Your Account");
	QFormLayout* formLayout = new QFormLayout();

	// Username
	m_loginUsernameEdit = new QLineEdit();
	m_loginUsernameEdit->setPlaceholderText("Enter username");
	formLayout->addRow("Username:", m_loginUsernameEdit);

	// Password
	m_loginPasswordEdit = new QLineEdit();
	m_loginPasswordEdit->setEchoMode(QLineEdit::Password);
	m_loginPasswordEdit->setPlaceholderText("Enter password");
	formLayout->addRow("Password:", m_loginPasswordEdit);

	// Show password checkbox
	m_loginShowPasswordCheck = new QCheckBox("Show password");
	connect(m_loginShowPasswordCheck, &QCheckBox::toggled, this, &LoginDialog::onShowPasswordToggled);
	formLayout->addRow("", m_loginShowPasswordCheck);

	loginGroup->setLayout(formLayout);
	layout->addWidget(loginGroup);

	// Status label
	m_loginStatusLabel = new QLabel();
	m_loginStatusLabel->setWordWrap(true);
	layout->addWidget(m_loginStatusLabel);

	// Buttons
	QHBoxLayout* buttonLayout = new QHBoxLayout();
	m_loginButton = new QPushButton("Login");
	m_loginButton->setStyleSheet("QPushButton { background-color: #2a82da; font-weight: bold; padding: 10px; }");
	m_loginButton->setDefault(true);
	connect(m_loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);

	m_loginCancelButton = new QPushButton("Exit");
	m_loginCancelButton->setStyleSheet("QPushButton { background-color: #2a82da; font-weight: bold; padding: 10px; }");
	m_loginCancelButton->setDefault(false);
	connect(m_loginCancelButton, &QPushButton::clicked, this, &LoginDialog::onCancelClicked);

	buttonLayout->addWidget(m_loginButton);
	buttonLayout->addWidget(m_loginCancelButton);
	layout->addLayout(buttonLayout);

	// Demo credentials info
	layout->addSpacing(10);
	QLabel* demoLabel = new QLabel("Demo Account:\nUsername: admin\nPassword: Admin123!");
	demoLabel->setStyleSheet("QLabel { color: #888; font-size: 9pt; padding: 10px; background-color: #333; border-radius: 5px; }");
	layout->addWidget(demoLabel);

	layout->addStretch();

	// Connect enter key
	connect(m_loginPasswordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
}

void LoginDialog::setupRegisterTab()
{
	m_registerTab = new QWidget();
	QVBoxLayout* layout = new QVBoxLayout(m_registerTab);

	QGroupBox* registerGroup = new QGroupBox("Create New Account");
	QFormLayout* formLayout = new QFormLayout();

	// Username
	m_registerUsernameEdit = new QLineEdit();
	m_registerUsernameEdit->setPlaceholderText("3-20 characters, letters/numbers only");
	formLayout->addRow("Username:", m_registerUsernameEdit);

	// Full Name
	m_registerFullNameEdit = new QLineEdit();
	m_registerFullNameEdit->setPlaceholderText("Enter your full name");
	formLayout->addRow("Full Name:", m_registerFullNameEdit);

	// Email
	m_registerEmailEdit = new QLineEdit();
	m_registerEmailEdit->setPlaceholderText("your.email@example.com");
	formLayout->addRow("Email:", m_registerEmailEdit);

	// Password
	m_registerPasswordEdit = new QLineEdit();
	m_registerPasswordEdit->setEchoMode(QLineEdit::Password);
	m_registerPasswordEdit->setPlaceholderText("Enter strong password");
	formLayout->addRow("Password:", m_registerPasswordEdit);

	// Confirm Password
	m_registerConfirmPasswordEdit = new QLineEdit();
	m_registerConfirmPasswordEdit->setEchoMode(QLineEdit::Password);
	m_registerConfirmPasswordEdit->setPlaceholderText("Re-enter password");
	formLayout->addRow("Confirm:", m_registerConfirmPasswordEdit);

	// Show password checkbox
	m_registerShowPasswordCheck = new QCheckBox("Show passwords");
	connect(m_registerShowPasswordCheck, &QCheckBox::toggled, this, [this](bool checked) {
		QLineEdit::EchoMode mode = checked ? QLineEdit::Normal : QLineEdit::Password;
		m_registerPasswordEdit->setEchoMode(mode);
		m_registerConfirmPasswordEdit->setEchoMode(mode);
		});
	formLayout->addRow("", m_registerShowPasswordCheck);

	registerGroup->setLayout(formLayout);
	layout->addWidget(registerGroup);

	// Password requirements
	m_passwordRequirementsLabel = new QLabel(
		"Password Requirements:\n"
		"• At least 8 characters\n"
		"• At least one uppercase letter\n"
		"• At least one number\n"
		"• At least one special character (!@#$%^&*)"
	);
	m_passwordRequirementsLabel->setStyleSheet("QLabel { color: #888; font-size: 9pt; padding: 10px; background-color: #333; border-radius: 5px; }");
	layout->addWidget(m_passwordRequirementsLabel);

	// Status label
	m_registerStatusLabel = new QLabel();
	m_registerStatusLabel->setWordWrap(true);
	layout->addWidget(m_registerStatusLabel);

	// Buttons
	QHBoxLayout* buttonLayout = new QHBoxLayout();
	m_registerButton = new QPushButton("Create Account");
	m_registerButton->setStyleSheet("QPushButton { background-color: #00c800; font-weight: bold; padding: 10px; }");
	connect(m_registerButton, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);

	m_registerCancelButton = new QPushButton("Cancel");
	connect(m_registerCancelButton, &QPushButton::clicked, this, &LoginDialog::onCancelClicked);

	buttonLayout->addWidget(m_registerButton);
	buttonLayout->addWidget(m_registerCancelButton);
	layout->addLayout(buttonLayout);

	layout->addStretch();
}

void LoginDialog::onLoginClicked()
{
	QString username = m_loginUsernameEdit->text().trimmed();
	QString password = m_loginPasswordEdit->text();

	if (username.isEmpty() || password.isEmpty()) {
		m_loginStatusLabel->setText("Please enter both username and password");
		m_loginStatusLabel->setStyleSheet("QLabel { color: #ff6464; }");
		return;
	}

	m_loginStatusLabel->setText("Authenticating...");
	m_loginStatusLabel->setStyleSheet("QLabel { color: #2a82da; }");

	m_authManager->login(username, password);
}

void LoginDialog::onRegisterClicked()
{
	QString username = m_registerUsernameEdit->text().trimmed();
	QString fullName = m_registerFullNameEdit->text().trimmed();
	QString email = m_registerEmailEdit->text().trimmed();
	QString password = m_registerPasswordEdit->text();
	QString confirmPassword = m_registerConfirmPasswordEdit->text();

	// Validate fields
	if (username.isEmpty() || fullName.isEmpty() || email.isEmpty() ||
		password.isEmpty() || confirmPassword.isEmpty()) {
		m_registerStatusLabel->setText("All fields are required");
		m_registerStatusLabel->setStyleSheet("QLabel { color: #ff6464; }");
		return;
	}

	// Check password match
	if (password != confirmPassword) {
		m_registerStatusLabel->setText("Passwords do not match");
		m_registerStatusLabel->setStyleSheet("QLabel { color: #ff6464; }");
		return;
	}

	m_registerStatusLabel->setText("Creating account...");
	m_registerStatusLabel->setStyleSheet("QLabel { color: #2a82da; }");

	m_authManager->registerUser(username, password, fullName, email);
}

void LoginDialog::onCancelClicked()
{
	reject();
}

void LoginDialog::onShowPasswordToggled(bool checked)
{
	m_loginPasswordEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
}