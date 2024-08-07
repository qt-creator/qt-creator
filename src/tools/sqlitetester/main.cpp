// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <sqlitedatabase.h>
#include <sqlitelibraryinitializer.h>

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextBrowser>
#include <QTemporaryFile>
#include <QDir>

#include <windows.h>

class DatabaseApp : public QWidget {
    Q_OBJECT

public:
    DatabaseApp(QWidget *parent = nullptr);

private slots:
    void onSetDirectoryClicked();
    void createDatabase(const QString &dirPath);

private:
    void deleteFileIfExist(const QString &filePath);
    void logMessage(const QString &message);
    void logDetailedWindowsError();

    QLineEdit *directoryLineEdit;
    QTextBrowser *logBrowser;
    QString databaseDirectory;
};

DatabaseApp::DatabaseApp(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);

    directoryLineEdit = new QLineEdit(this);
    directoryLineEdit->setPlaceholderText("Enter database directory path");
    layout->addWidget(directoryLineEdit);

    QPushButton *setDirectoryButton = new QPushButton("Set Directory and Create Database", this);
    layout->addWidget(setDirectoryButton);

    logBrowser = new QTextBrowser(this);
    layout->addWidget(logBrowser);

    connect(setDirectoryButton, &QPushButton::clicked, this, &DatabaseApp::onSetDirectoryClicked);
}

void DatabaseApp::onSetDirectoryClicked() {
    QString dirPath = directoryLineEdit->text();
    if (dirPath.isEmpty()) {
        logMessage("Directory path is empty.");
        return;
    }

    QDir dir(dirPath);
    if (!dir.exists()) {
        logMessage("Directory does not exist.");
        return;
    }

    QTemporaryFile tempFile(dirPath + "/tempfileXXXXXX");
    if (!tempFile.open()) {
        logMessage("Cannot create temporary file in the directory: " + tempFile.errorString());
        logDetailedWindowsError();
        return;
    }
    tempFile.close();

    createDatabase(dirPath);
}

void DatabaseApp::deleteFileIfExist(const QString &filePath) {
    if (QFile::exists(filePath)) {
        if (!QFile::remove(filePath)) {
            logMessage(QString("Failed to delete existing file %1 before creating database").arg(filePath));
            logDetailedWindowsError();
            return;
        }
    }
}

void DatabaseApp::createDatabase(const QString &dirPath) {
    databaseDirectory = dirPath;
    QString dbPath = dirPath + "/mysqlitetester.db";
    dbPath.replace("\\", "/");
    deleteFileIfExist(dbPath);

    try {
        Sqlite::Database database{Utils::PathString{dbPath}};
    } catch (const Sqlite::Exception &e) {
        logMessage(QString("Cannot create %1: %2").arg(dbPath, QString::fromUtf8(e.what())));
        logDetailedWindowsError();
    }
    deleteFileIfExist(dbPath);
    logMessage(QString("Test with %1 was successful.").arg(dbPath));
}

void DatabaseApp::logMessage(const QString &message) {
    logBrowser->append(message);
}

void DatabaseApp::logDetailedWindowsError() {
    DWORD errorCode = GetLastError();
    if (errorCode != 0) {
        LPVOID errorMsg;
        DWORD size = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&errorMsg, 0, NULL);

        if (size) {
            QString detailedMessage = QString::fromWCharArray((LPWSTR)errorMsg, size);
            logMessage("Windows error: " + detailedMessage);
            LocalFree(errorMsg);
        }
    }
}

int main(int argc, char *argv[]) {
    Sqlite::LibraryInitializer::initialize();


    QApplication app(argc, argv);
    DatabaseApp window;
    window.setWindowTitle("SQLite Write Database Tester");
    window.resize(400, 300);
    window.show();
    return app.exec();
}

#include "main.moc"
