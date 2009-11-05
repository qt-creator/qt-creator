/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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

#ifndef MERCURIALJOBRUNNER_H
#define MERCURIALJOBRUNNER_H

#include <QtCore/QThread>
#include <QtCore/QQueue>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QStringList>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>

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
    HgTask(const QString &workingDir, const QStringList &arguments, bool emitRaw=false);
    HgTask(const QString &workingDir, const QStringList &arguments,
           VCSBase::VCSBaseEditor *editor);

    bool shouldEmit() { return emitRaw; }
    VCSBase::VCSBaseEditor* displayEditor() { return editor; }
    QStringList args() { return arguments; }
    QString repositoryRoot() { return m_repositoryRoot; }

signals:
    void rawData(const QByteArray &data);

private:
    const QString m_repositoryRoot;
    const QStringList arguments;
    const bool emitRaw;
    VCSBase::VCSBaseEditor *editor;
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

    static QString msgExecute(const QString &binary, const QStringList &args);
    static QString msgStartFailed(const QString &binary, const QString &why);
    static QString msgTimeout(int timeoutMS);

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
    int timeout;
};

} //namespace Internal
} //namespace Mercurial
#endif // MERCURIALJOBRUNNER_H
