#pragma once
#include <QObject>
#include <QMap>
#include <QString>
#include <QDateTime>
#include <QCryptographicHash>
#include "UserAccount.h"

struct UserCredentials {
	QString username;
	QString passwordHash;
	QString salt;
	QString email;
	bool isActive;
	QDateTime lastLogin;
	int failedLoginAttempts;

	// Constructor for safe initialization
	UserCredentials()
		: isActive(true)
		, failedLoginAttempts(0)
	{
	}
};

class AuthManager : public QObject
{
	Q_OBJECT

public:
	explicit AuthManager(QObject* parent = nullptr);
	~AuthManager();

	// Authentication
	bool login(const QString& username, const QString& password);
	void logout();
	bool isLoggedIn() const { return m_currentUser != nullptr; }

	// Registration
	bool registerUser(const QString& username, const QString& password,
		const QString& fullName, const QString& email);
	bool usernameExists(const QString& username) const;
	bool emailExists(const QString& email) const;

	// Current user
	UserAccount* getCurrentUser() { return m_currentUser; }
	QString getCurrentUsername() const;

	// Password management
	bool changePassword(const QString& oldPassword, const QString& newPassword);
	bool resetPassword(const QString& username, const QString& email);

	// Account management
	bool deleteAccount(const QString& username, const QString& password);
	bool lockAccount(const QString& username);
	bool unlockAccount(const QString& username);

	// Validation
	bool validatePassword(const QString& password, QString& errorMessage);
	bool validateEmail(const QString& email);
	bool validateUsername(const QString& username, QString& errorMessage);

signals:
	void loginSuccessful(const QString& username);
	void loginFailed(const QString& reason);
	void loggedOut();
	void registrationSuccessful(const QString& username);
	void registrationFailed(const QString& reason);
	void passwordChanged();
	void accountLocked(const QString& username);

private:
	QString hashPassword(const QString& password, const QString& salt);
	QString generateSalt();
	bool verifyPassword(const QString& password, const QString& hash, const QString& salt);
	void loadUsers();
	void saveUsers();
	bool createUserAccount(const QString& username, const QString& fullName, const QString& email);

private:
	QMap<QString, UserCredentials*> m_credentials;
	QMap<QString, UserAccount*> m_userAccounts;
	UserAccount* m_currentUser;
	QString m_currentUsername;

	// Security settings
	int m_maxFailedAttempts;
	int m_passwordMinLength;
	bool m_requireSpecialChar;
	bool m_requireNumber;
	bool m_requireUppercase;
};