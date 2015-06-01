/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <QString>
#include <QtTest>

#include <connectionclient.h>
#include <clientdummy.h>
#include <cmbcommands.h>
#include <filecontainer.h>

#include <cmbcompletecodecommand.h>
#include <cmbcodecompletedcommand.h>

static const char fooHeaderFile[] = TEST_BASE_DIRECTORY "/foo.h";

class CodeModelBackendProcessTest : public QObject
{
    Q_OBJECT

public:
    CodeModelBackendProcessTest();

private Q_SLOTS:
    void init();
    void cleanup();
    void completeCode();

private:
    void registerFiles();

private:
    ClientDummy clientDummy;
    CodeModelBackEnd::ConnectionClient client;
};

CodeModelBackendProcessTest::CodeModelBackendProcessTest()
    : client(&clientDummy)
{
    CodeModelBackEnd::Commands::registerCommands();
    client.setProcessPath(QLatin1String(CODEMODELBACKENDPROCESSPATH));
}

void CodeModelBackendProcessTest::init()
{
    client.startProcess();
    client.connectToServer();

}

void CodeModelBackendProcessTest::cleanup()
{
    client.disconnectFromServer();
    client.sendEndCommand();
}

void CodeModelBackendProcessTest::completeCode()
{
    QSignalSpy clientSpy(&clientDummy, &ClientDummy::newCodeCompleted);
    registerFiles();

    client.sendCompleteCodeCommand(fooHeaderFile, 36, 0, "");

    QVERIFY(clientSpy.wait(3000));
}

void CodeModelBackendProcessTest::registerFiles()
{
    CodeModelBackEnd::FileContainer fileContainer(fooHeaderFile);
    QVector<CodeModelBackEnd::FileContainer> fileContainers({fileContainer});

    client.sendRegisterFilesForCodeCompletionCommand(fileContainers);
}

QTEST_MAIN(CodeModelBackendProcessTest)

#include "tst_codemodelbackendprocess.moc"
