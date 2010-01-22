/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CDBAPPLICATION_H
#define CDBAPPLICATION_H

#include <QtCore/QCoreApplication>
#include <QtCore/QSharedPointer>

namespace CdbCore {
    class CoreEngine;
}

class CdbPromptThread;

class CdbApplication : public QCoreApplication
{
    Q_OBJECT
    Q_DISABLE_COPY(CdbApplication)
public:
    enum InitResult { InitFailed, InitUsageShown, InitOk };

    CdbApplication(int argc, char *argv[]);
    ~CdbApplication();

    InitResult init();

private slots:
    void promptThreadTerminated();
    void asyncCommand(int command, const QString &arg);
    void syncCommand(int command, const QString &arg);
    void executionCommand(int command, const QString &arg);
    void debugEvent();
    void processAttached(void *handle);

private:
    bool parseOptions();

    QString m_engineDll;
    QSharedPointer<CdbCore::CoreEngine> m_engine;
    CdbPromptThread *m_promptThread;
    void *m_processHandle;
};

#endif // CDBAPPLICATION_H
