/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef MERCURIALJOBRUNNER_H
#define MERCURIALJOBRUNNER_H

#include <QtCore/QThread>
#include <QtCore/QQueue>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QStringList>
#include <QtCore/QSharedPointer>
#include <QtCore/QVariant>
#include <QtCore/QString>
#include <QtCore/QPointer>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace VCSBase {
class VCSBaseEditor;
}

namespace Mercurial {
namespace Internal {

class MercurialPlugin;

class HgTask : public QObject
{
    Q_OBJECT
public:
    explicit HgTask(const QString &workingDir,
                    const QStringList &arguments,
                    bool emitRaw=false,
                    const QVariant &cookie = QVariant());
    explicit HgTask(const QString &workingDir, const QStringList &arguments,
                    VCSBase::VCSBaseEditor *editor,
                    const QVariant &cookie = QVariant());

    bool shouldEmit() const { return emitRaw; }
    VCSBase::VCSBaseEditor* displayEditor() const;
    QStringList args() const { return arguments; }
    QString repositoryRoot() const { return m_repositoryRoot; }

    // Disable terminal to suppress SSH prompting.
    bool unixTerminalDisabled() const     { return m_unixTerminalDisabled; }
    void setUnixTerminalDisabled(bool v)  { m_unixTerminalDisabled = v; }

signals:
    void succeeded(const QVariant &cookie); // Use a queued connection
    void rawData(const QByteArray &data);

public slots:
    void emitSucceeded();

private:
    const QString m_repositoryRoot;
    const QStringList arguments;
    const bool emitRaw;
    const QVariant m_cookie;
    QPointer<VCSBase::VCSBaseEditor> editor; // User might close it.
    bool m_unixTerminalDisabled;
};

/* A job queue running in a separate thread, executing commands
 * and emitting status/log  signals. */
class MercurialJobRunner : public QThread
{
    Q_OBJECT
public:
    MercurialJobRunner();
    ~MercurialJobRunner();
    void enqueueJob(const QSharedPointer<HgTask> &job);
    void restart();

    static QString msgStartFailed(const QString &binary, const QString &why);
    static QString msgTimeout(int timeoutSeconds);

    // Set environment for a hg process to run in locale "C"
    static void setProcessEnvironment(QProcess &p);

protected:
    void run();

signals:
    void commandStarted(const QString &notice);
    void error(const QString &error);
    void output(const QByteArray &output);

private:
    void task(const QSharedPointer<HgTask> &job);
    void stop();
    void getSettings();

    QQueue<QSharedPointer<HgTask> > jobs;
    QMutex mutex;
    QWaitCondition waiter;
    MercurialPlugin *plugin;
    bool keepRunning;
    QString binary;
    QStringList standardArguments;
    int m_timeoutMS;
};

} //namespace Internal
} //namespace Mercurial
#endif // MERCURIALJOBRUNNER_H
