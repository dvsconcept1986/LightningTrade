#include "AuthManager.h"
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QDebug>

AuthManager::AuthManager(QObject* parent)
	: QObject(parent)
	, m_credentials()  // Explicitly initialize QMap
	, m_userAccounts()  // Explicitly initialize QMap
	, m_currentUser(nullptr)
	, m_currentUsername()
	, m_maxFailedAttempts(3)
	, m_passwordMinLength(8)
	, m_requireSpecialChar(true)
	, m_requireNumber(true)
	, m_requireUppercase(true)
{
	qDebug() << "AuthManager constructor called";
	qDebug() << "Maps initialized";
	loadUsers();
	qDebug() << "Users loaded, credential count:" << m_credentials.size();
}

AuthManager::~AuthManager()
{
	saveUsers();
	qDeleteAll(m_credentials);
	qDeleteAll(m_userAccounts);
}

bool AuthManager::login(const QString& username, const QString& password)
{
	qDebug() << "Login attempt for:" << username;

	if (username.isEmpty() || password.isEmpty()) {
		emit loginFailed("Username and password are required");
		return false;
	}

	if (!m_credentials.contains(username)) {
		qDebug() << "Username not found in credentials";
		emit loginFailed("Invalid username or password");
		return false;
	}

	UserCredentials* creds = m_credentials[username];

	// Add null pointer check
	if (!creds) {
		qDebug() << "ERROR: Null credentials pointer for username:" << username;
		emit loginFailed("Invalid username or password");
		return false;
	}

	qDebug() << "Credentials found, checking account status";

	// Check if account is locked
	if (!creds->isActive) {
		qDebug() << "Account is locked";
		emit loginFailed("Account is locked. Please contact support.");
		return false;
	}

	// Check if too many failed attempts
	if (creds->failedLoginAttempts >= m_maxFailedAttempts) {
		qDebug() << "Too many failed attempts";
		creds->isActive = false;
		emit accountLocked(username);
		emit loginFailed("Account locked due to too many failed login attempts");
		return false;
	}

	// Verify password
	qDebug() << "Verifying password";
	if (!verifyPassword(password, creds->passwordHash, creds->salt)) {
		qDebug() << "Password verification failed";
		creds->failedLoginAttempts++;
		emit loginFailed("Invalid username or password");
		return false;
	}

	// Successful login
	qDebug() << "Password verified, logging in";
	creds->failedLoginAttempts = 0;
	creds->lastLogin = QDateTime::currentDateTime();

	// Check if user account exists
	if (!m_userAccounts.contains(username)) {
		qDebug() << "ERROR: User account not found for username:" << username;
		emit loginFailed("User account error");
		return false;
	}

	m_currentUser = m_userAccounts[username];
	if (!m_currentUser) {
		qDebug() << "ERROR: Null user account pointer for username:" << username;
		emit loginFailed("User account error");
		return false;
	}

	m_currentUsername = username;

	qDebug() << "Login successful for:" << username;
	emit loginSuccessful(username);
	return true;
}

void AuthManager::logout()
{
	if (m_currentUser) {
		qDebug() << "Logging out user:" << m_currentUsername;
		m_currentUser = nullptr;
		m_currentUsername.clear();
		emit loggedOut();
	}
}

bool AuthManager::registerUser(const QString& username, const QString& password,
	const QString& fullName, const QString& email)
{
	qDebug() << "Registration attempt for username:" << username;

	// Validate username
	QString errorMsg;
	if (!validateUsername(username, errorMsg)) {
		qDebug() << "Username validation failed:" << errorMsg;
		emit registrationFailed(errorMsg);
		return false;
	}

	// Check if username already exists
	if (usernameExists(username)) {
		qDebug() << "Username already exists:" << username;
		emit registrationFailed("Username already exists");
		return false;
	}

	// Check if email already exists
	if (emailExists(email)) {
		qDebug() << "Email already registered:" << email;
		emit registrationFailed("Email already registered");
		return false;
	}

	// Validate password
	if (!validatePassword(password, errorMsg)) {
		qDebug() << "Password validation failed:" << errorMsg;
		emit registrationFailed(errorMsg);
		return false;
	}

	// Validate email
	if (!validateEmail(email)) {
		qDebug() << "Email validation failed:" << email;
		emit registrationFailed("Invalid email address");
		return false;
	}

	// Create credentials
	UserCredentials* creds = new UserCredentials();
	creds->username = username;
	creds->email = email;
	creds->salt = generateSalt();
	creds->passwordHash = hashPassword(password, creds->salt);
	creds->isActive = true;
	creds->lastLogin = QDateTime::currentDateTime();
	creds->failedLoginAttempts = 0;

	m_credentials[username] = creds;
	qDebug() << "Credentials created for:" << username;

	// Create user account
	if (!createUserAccount(username, fullName, email)) {
		qDebug() << "Failed to create user account for:" << username;
		delete creds;
		m_credentials.remove(username);
		emit registrationFailed("Failed to create user account");
		return false;
	}

	qDebug() << "Registration successful for:" << username;
	emit registrationSuccessful(username);
	return true;
}

bool AuthManager::usernameExists(const QString& username) const
{
	return m_credentials.contains(username);
}

bool AuthManager::emailExists(const QString& email) const
{
	for (auto it = m_credentials.begin(); it != m_credentials.end(); ++it) {
		UserCredentials* creds = it.value();
		if (creds && creds->email.toLower() == email.toLower()) {
			return true;
		}
	}
	return false;
}

QString AuthManager::getCurrentUsername() const
{
	return m_currentUsername;
}

bool AuthManager::changePassword(const QString& oldPassword, const QString& newPassword)
{
	if (!m_currentUser || m_currentUsername.isEmpty()) {
		qDebug() << "No current user logged in";
		return false;
	}

	if (!m_credentials.contains(m_currentUsername)) {
		qDebug() << "Current user credentials not found";
		return false;
	}

	UserCredentials* creds = m_credentials[m_currentUsername];
	if (!creds) {
		qDebug() << "Null credentials pointer for current user";
		return false;
	}

	// Verify old password
	if (!verifyPassword(oldPassword, creds->passwordHash, creds->salt)) {
		qDebug() << "Old password verification failed";
		return false;
	}

	// Validate new password
	QString errorMsg;
	if (!validatePassword(newPassword, errorMsg)) {
		qDebug() << "New password validation failed:" << errorMsg;
		return false;
	}

	// Update password
	creds->salt = generateSalt();
	creds->passwordHash = hashPassword(newPassword, creds->salt);

	qDebug() << "Password changed successfully for:" << m_currentUsername;
	emit passwordChanged();
	return true;
}

bool AuthManager::resetPassword(const QString& username, const QString& email)
{
	if (!m_credentials.contains(username)) {
		qDebug() << "Username not found for password reset:" << username;
		return false;
	}

	UserCredentials* creds = m_credentials[username];
	if (!creds) {
		qDebug() << "Null credentials pointer for password reset";
		return false;
	}

	if (creds->email.toLower() != email.toLower()) {
		qDebug() << "Email does not match for password reset";
		return false;
	}

	// In a real system, you would send a reset email
	// For now, we'll just unlock the account
	creds->failedLoginAttempts = 0;
	creds->isActive = true;

	qDebug() << "Password reset successful for:" << username;
	return true;
}

bool AuthManager::deleteAccount(const QString& username, const QString& password)
{
	if (!m_credentials.contains(username)) {
		qDebug() << "Username not found for deletion:" << username;
		return false;
	}

	// Verify password
	UserCredentials* creds = m_credentials[username];
	if (!creds) {
		qDebug() << "Null credentials pointer for account deletion";
		return false;
	}

	if (!verifyPassword(password, creds->passwordHash, creds->salt)) {
		qDebug() << "Password verification failed for account deletion";
		return false;
	}

	// Delete user account
	if (m_userAccounts.contains(username)) {
		delete m_userAccounts[username];
		m_userAccounts.remove(username);
	}

	// Delete credentials
	delete creds;
	m_credentials.remove(username);

	// Logout if deleting current user
	if (m_currentUsername == username) {
		logout();
	}

	qDebug() << "Account deleted successfully:" << username;
	return true;
}

bool AuthManager::lockAccount(const QString& username)
{
	if (!m_credentials.contains(username)) {
		qDebug() << "Username not found for locking:" << username;
		return false;
	}

	UserCredentials* creds = m_credentials[username];
	if (!creds) {
		qDebug() << "Null credentials pointer for account locking";
		return false;
	}

	creds->isActive = false;
	qDebug() << "Account locked:" << username;
	emit accountLocked(username);
	return true;
}

bool AuthManager::unlockAccount(const QString& username)
{
	if (!m_credentials.contains(username)) {
		qDebug() << "Username not found for unlocking:" << username;
		return false;
	}

	UserCredentials* creds = m_credentials[username];
	if (!creds) {
		qDebug() << "Null credentials pointer for account unlocking";
		return false;
	}

	creds->isActive = true;
	creds->failedLoginAttempts = 0;

	qDebug() << "Account unlocked:" << username;
	return true;
}

bool AuthManager::validatePassword(const QString& password, QString& errorMessage)
{
	if (password.length() < m_passwordMinLength) {
		errorMessage = QString("Password must be at least %1 characters long").arg(m_passwordMinLength);
		return false;
	}

	if (m_requireUppercase && !password.contains(QRegularExpression("[A-Z]"))) {
		errorMessage = "Password must contain at least one uppercase letter";
		return false;
	}

	if (m_requireNumber && !password.contains(QRegularExpression("[0-9]"))) {
		errorMessage = "Password must contain at least one number";
		return false;
	}

	if (m_requireSpecialChar && !password.contains(QRegularExpression("[!@#$%^&*()_+\\-=\\[\\]{};':\"\\\\|,.<>\\/?]"))) {
		errorMessage = "Password must contain at least one special character";
		return false;
	}

	return true;
}

bool AuthManager::validateEmail(const QString& email)
{
	QRegularExpression regex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
	return regex.match(email).hasMatch();
}

bool AuthManager::validateUsername(const QString& username, QString& errorMessage)
{
	if (username.length() < 3) {
		errorMessage = "Username must be at least 3 characters long";
		return false;
	}

	if (username.length() > 20) {
		errorMessage = "Username must be no more than 20 characters long";
		return false;
	}

	QRegularExpression regex("^[a-zA-Z0-9_]+$");
	if (!regex.match(username).hasMatch()) {
		errorMessage = "Username can only contain letters, numbers, and underscores";
		return false;
	}

	return true;
}

QString AuthManager::hashPassword(const QString& password, const QString& salt)
{
	QString saltedPassword = password + salt;
	QByteArray hash = QCryptographicHash::hash(saltedPassword.toUtf8(), QCryptographicHash::Sha256);
	return hash.toHex();
}

QString AuthManager::generateSalt()
{
	QString salt;
	for (int i = 0; i < 16; ++i) {
		salt += QString::number(QRandomGenerator::global()->bounded(256), 16);
	}
	return salt;
}

bool AuthManager::verifyPassword(const QString& password, const QString& hash, const QString& salt)
{
	QString computedHash = hashPassword(password, salt);
	return computedHash == hash;
}

void AuthManager::loadUsers()
{
	qDebug() << "Loading users...";

	// In a real application, load from database or encrypted file
	// For now, create some demo users

	// Demo admin user
	bool success = registerUser("admin", "Admin123!", "Administrator", "admin@lightning.com");
	qDebug() << "Admin user registration:" << (success ? "SUCCESS" : "FAILED");

	if (success && m_userAccounts.contains("admin")) {
		UserAccount* adminAccount = m_userAccounts["admin"];
		if (adminAccount) {
			qDebug() << "Setting admin balance";
			adminAccount->deposit(1000000.00, "Initial Admin Balance");
		}
		else {
			qDebug() << "ERROR: Admin account pointer is null";
		}
	}

	qDebug() << "Credentials map size:" << m_credentials.size();
	qDebug() << "User accounts map size:" << m_userAccounts.size();
}

void AuthManager::saveUsers()
{
	// In a real application, save to database or encrypted file
	// For now, this is a placeholder
	qDebug() << "Saving users...";
}

bool AuthManager::createUserAccount(const QString& username, const QString& fullName, const QString& email)
{
	qDebug() << "Creating user account for:" << username;

	UserAccount* account = new UserAccount(username, fullName, email);
	if (!account) {
		qDebug() << "ERROR: Failed to allocate UserAccount";
		return false;
	}

	// Give new users starting balance
	account->deposit(10000.00, "Welcome Bonus");

	m_userAccounts[username] = account;
	qDebug() << "User account created successfully";
	return true;
}