#include <windows.h>
#include <sqlext.h>
#include <string>
#include <iostream>
#include <locale>
#include <codecvt>

std::wstring stringToWstring(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}

class ChatDatabase {
public:
    ChatDatabase(const std::string& dsn, const std::string& user, const std::string& password) {
        SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);

        SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

        SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);

        SQLSetConnectAttr(dbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

        std::wstring wDsn = stringToWstring(dsn);
        std::wstring wUser = stringToWstring(user);
        std::wstring wPassword = stringToWstring(password);

        SQLWCHAR retConString[1024];
        SQLDriverConnect(dbc, NULL, (SQLWCHAR*)(wDsn + L";UID=" + wUser + L";PWD=" + wPassword).c_str(), SQL_NTS, retConString, 1024, NULL, SQL_DRIVER_NOPROMPT);

        std::wcout << L"Connected to database successfully" << std::endl;

        createTable();
    }

    ~ChatDatabase() {
        SQLDisconnect(dbc);
        SQLFreeHandle(SQL_HANDLE_DBC, dbc);
        SQLFreeHandle(SQL_HANDLE_ENV, env);
    }

    void createTable() {
        const wchar_t* sql = L"CREATE TABLE IF NOT EXISTS MESSAGES ("
            L"ID INT PRIMARY KEY NOT NULL AUTO_INCREMENT,"
            L"SENDER VARCHAR(255) NOT NULL,"
            L"MESSAGE TEXT NOT NULL,"
            L"TIMESTAMP DATETIME DEFAULT CURRENT_TIMESTAMP);";

        executeSQL(sql);
    }

    void insertMessage(const std::string& sender, const std::string& message) {
        std::wstring wSender = stringToWstring(sender);
        std::wstring wMessage = stringToWstring(message);
        std::wstring sql = L"INSERT INTO MESSAGES (SENDER, MESSAGE) VALUES ('" + wSender + L"', '" + wMessage + L"');";
        executeSQL(sql.c_str());
    }

    void fetchMessages() {
        SQLHSTMT stmt;
        SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
        SQLExecDirect(stmt, (SQLWCHAR*)L"SELECT * FROM MESSAGES;", SQL_NTS);

        SQLWCHAR sender[256];
        SQLWCHAR message[1024];
        SQLLEN indicator;
        while (SQLFetch(stmt) == SQL_SUCCESS) {
            SQLGetData(stmt, 2, SQL_C_WCHAR, sender, sizeof(sender), &indicator);
            SQLGetData(stmt, 3, SQL_C_WCHAR, message, sizeof(message), &indicator);
            std::wcout << sender << L": " << message << std::endl;
        }
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    }

private:
    SQLHENV env;
    SQLHDBC dbc;

    void executeSQL(const wchar_t* sql) {
        SQLHSTMT stmt;
        SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
        SQLRETURN ret = SQLExecDirect(stmt, (SQLWCHAR*)sql, SQL_NTS);

        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            std::wcerr << L"Error executing SQL: " << sql << std::endl;
        }

        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    }
};
