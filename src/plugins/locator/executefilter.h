/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef EXECUTEFILTER_H
#define EXECUTEFILTER_H

#include "ilocatorfilter.h"

#include <utils/qtcprocess.h>

#include <QQueue>
#include <QStringList>
#include <QTimer>

namespace Locator {
namespace Internal {

class ExecuteFilter : public Locator::ILocatorFilter
{
    Q_OBJECT
    struct ExecuteData
    {
        QString executable;
        QString arguments;
        QString workingDirectory;
    };

public:
    ExecuteFilter();
    QList<Locator::FilterEntry> matchesFor(QFutureInterface<Locator::FilterEntry> &future,
                                           const QString &entry);
    void accept(Locator::FilterEntry selection) const;
    void refresh(QFutureInterface<void> &) {}

private slots:
    void finished(int exitCode, QProcess::ExitStatus status);
    void readStandardOutput();
    void readStandardError();
    void runHeadCommand();

private:
    QString headCommand() const;

private:
    QQueue<ExecuteData> m_taskQueue;
    QStringList m_commandHistory;
    Utils::QtcProcess *m_process;
    QTimer m_runTimer;
};

} // namespace Internal
} // namespace Locator

#endif // EXECUTEFILTER_H
