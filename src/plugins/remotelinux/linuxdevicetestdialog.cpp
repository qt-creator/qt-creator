/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "linuxdevicetestdialog.h"
#include "ui_linuxdevicetestdialog.h"

#include <utils/qtcassert.h>
#include <utils/ssh/sshconnectionmanager.h>

#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QPushButton>
#include <QtGui/QTextCharFormat>

namespace RemoteLinux {
namespace Internal {

class LinuxDeviceTestDialogPrivate
{
public:
    LinuxDeviceTestDialogPrivate(LinuxDeviceTestDialog *qptr, QList<LinuxDeviceTester *> &tester)
        : deviceTests(tester), lastTest(0), currentTestPosition(-1), failCount(0), q(qptr)
    { }

    ~LinuxDeviceTestDialogPrivate()
    {
        qDeleteAll(deviceTests);
    }

    LinuxDeviceTester *currentTest()
    {
        return deviceTests.at(currentTestPosition);
    }

    void runTest()
    {
        if (lastTest)
            QObject::disconnect(lastTest, 0, q, 0);

        LinuxDeviceTester *curTest = currentTest();

        QObject::connect(curTest, SIGNAL(progressMessage(QString)),
                         q, SLOT(handleProgressMessage(QString)));
        QObject::connect(curTest, SIGNAL(errorMessage(QString)),
                         q, SLOT(handleErrorMessage(QString)));
        QObject::connect(curTest, SIGNAL(finished(int)),
                         q, SLOT(handleTestFinished(int)));

        lastTest = curTest;

        q->addText(curTest->headLine(), QLatin1String("black"), true);
        q->addText(curTest->commandLine(), QLatin1String("darkGray"), false);
        curTest->run();
    }

    bool runNextTest()
    {
        ++currentTestPosition;
        if (currentTestPosition < deviceTests.count()) {
            runTest();
        } else {
            currentTestPosition = -1;
            lastTest = 0;
        }

        return currentTestPosition != -1;
    }

    Ui::LinuxDeviceTestDialog ui;
    QList<LinuxDeviceTester *> deviceTests;
    LinuxDeviceTester *lastTest;
    int currentTestPosition;
    int failCount;
    LinuxDeviceTestDialog *const q;
};

} // namespace Internal

LinuxDeviceTestDialog::LinuxDeviceTestDialog(QList<LinuxDeviceTester *> tests, QWidget *parent) :
    QDialog(parent),
    d(new Internal::LinuxDeviceTestDialogPrivate(this, tests))
{
    QTC_ASSERT(!tests.isEmpty(), return);

    d->ui.setupUi(this);

    d->runNextTest();
}

LinuxDeviceTestDialog::~LinuxDeviceTestDialog()
{
    delete d;
}

void LinuxDeviceTestDialog::reject()
{
    if (d->currentTestPosition >= 0) {
        d->deviceTests.at(d->currentTestPosition)->cancel();
        disconnect(d->deviceTests.at(d->currentTestPosition), 0, this, 0);
    }

    QDialog::reject();
}

void LinuxDeviceTestDialog::handleProgressMessage(const QString &message)
{
    QString tmp = QLatin1String("    ") + message;
    if (tmp.endsWith('\n'))
        tmp = tmp.left(tmp.count() - 1);
    tmp.replace(QLatin1Char('\n'), QLatin1String("\n    "));

    addText(tmp, QLatin1String("black"), false);
}

void LinuxDeviceTestDialog::handleErrorMessage(const QString &message)
{
    addText(message, QLatin1String("red"), false);
}

void LinuxDeviceTestDialog::handleTestFinished(int result)
{
    bool abortRun = false;
    if (result == LinuxDeviceTester::TestSuccess) {
        addText(tr("Ok.\n"), QLatin1String("black"), true);
    } else if (result == LinuxDeviceTester::TestCriticalFailure) {
        addText(tr("Critical device test failure, aborting.\n"), QLatin1String("red"), true);
        ++d->failCount;
        abortRun = true;
    } else {
        addText(tr("Device test failed.\n"), QLatin1String("red"), true);
        ++d->failCount;
    }

    if (abortRun || !d->runNextTest()) {
        if (d->failCount == 0 && !abortRun) {
            addText(tr("All device tests finished successfully.\n"), QLatin1String("blue"), true);
        } else {
            if (!abortRun) {
                //: %1: number of failed tests, %2 total tests
                addText(tr("%1 device tests of %2 failed.\n").arg(d->failCount).arg(d->deviceTests.count()),
                        QLatin1String("red"), true);
            }
        }

        d->ui.buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Close"));
    }
}

void LinuxDeviceTestDialog::addText(const QString &text, const QString &color, bool bold)
{
    QTextCharFormat format = d->ui.textEdit->currentCharFormat();
    format.setForeground(QBrush(QColor(color)));
    QFont font = format.font();
    font.setBold(bold);
    format.setFont(font);
    d->ui.textEdit->setCurrentCharFormat(format);
    d->ui.textEdit->appendPlainText(text);
}

} // namespace RemoteLinux
