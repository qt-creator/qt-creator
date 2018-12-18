/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
                                                        "Password required for SSH login.");
        if (appArgs.count() > 1)
            labelText.append('\n').append(appArgs.at(1));
        dlg.setLabelText(labelText);
        dlg.setTextEchoMode(QLineEdit::Password);
        if (dlg.exec() == QDialog::Accepted)
            std::cout << qPrintable(dlg.textValue()) << std::endl;
    });
    return app.exec();
}
