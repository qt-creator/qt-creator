// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QApplication>
#include <QInputDialog>
#include <QLocalSocket>
#include <QTimer>
#include <iostream>

enum { Timeout = 5000 };

// Qt Creator sets these when the device has a stored password. Only the server
// name and a token are passed on; the password is read back over the socket.
static QString storedPassword()
{
    const QString server = qEnvironmentVariable("QTC_ASKPASS_SERVER");
    const QByteArray token = qgetenv("QTC_ASKPASS_TOKEN");
    if (server.isEmpty() || token.isEmpty())
        return {};

    QLocalSocket socket;
    socket.connectToServer(server);
    if (!socket.waitForConnected(Timeout))
        return {};
    socket.write(token + '\n');
    if (!socket.waitForBytesWritten(Timeout))
        return {};

    while (!socket.canReadLine()) {
        if (!socket.waitForReadyRead(Timeout))
            return {};
    }
    // Drop the newline the server appends, and nothing else: everything before
    // it belongs to the password.
    QByteArray password = socket.readLine();
    if (password.endsWith('\n'))
        password.chop(1);
    return QString::fromUtf8(password);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QTimer::singleShot(0, &app, [] {
        const QString stored = storedPassword();
        if (!stored.isEmpty()) {
            std::cout << qPrintable(stored) << std::endl;
            qApp->exit(0);
            return;
        }

        QInputDialog dlg;
        const QStringList appArgs = qApp->arguments();
        QString labelText = QCoreApplication::translate("qtc-askpass",
                                                        "Password required.");
        if (appArgs.count() > 1)
            labelText.append('\n').append(appArgs.at(1));
        dlg.setLabelText(labelText);
        dlg.setTextEchoMode(QLineEdit::Password);
        const bool accepted = dlg.exec() == QDialog::Accepted;
        if (accepted)
            std::cout << qPrintable(dlg.textValue()) << std::endl;
        qApp->exit(accepted ? 0 : 1);
    });
    return app.exec();
}
