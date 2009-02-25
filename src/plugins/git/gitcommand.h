/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef GITCOMMAND_H
#define GITCOMMAND_H

#include <projectexplorer/environment.h>

#include <QtCore/QObject>

namespace Git {
namespace Internal {

class GitCommand : public QObject
{
    Q_OBJECT
public:
    explicit GitCommand(const QString &workingDirectory,
                        ProjectExplorer::Environment &environment);


    void addJob(const QStringList &arguments, int timeout);
    void execute();

private:
    void run();

Q_SIGNALS:
    void outputData(const QByteArray&);
    void errorText(const QString&);

private:
    struct Job {
        explicit Job(const QStringList &a, int t);

        QStringList arguments;
        int timeout;
    };

    QStringList environment() const;

    const QString m_workingDirectory;
    const QStringList m_environment;

    QList<Job> m_jobs;
};

} // namespace Internal
} // namespace Git
#endif // GITCOMMAND_H
