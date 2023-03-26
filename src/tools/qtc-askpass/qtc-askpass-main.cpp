// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QApplication>
#include <QInputDialog>
#include <QTimer>
#include <iostream>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QTimer::singleShot(0, [] {
        QInputDialog dlg;
        const QStringList appArgs = qApp->arguments();
        QString labelText = QCoreApplication::translate("qtc-askpass",
                                                        "Password required.");
        if (appArgs.count() > 1)
            labelText.append('\n').append(appArgs.at(1));
        dlg.setLabelText(labelText);
        dlg.setTextEchoMode(QLineEdit::Password);
        if (dlg.exec() == QDialog::Accepted)
            std::cout << qPrintable(dlg.textValue()) << std::endl;
    });
    return app.exec();
}
