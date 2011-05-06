/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Brian McGillion & Hugues Delorme
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef VCSJOBRUNNER_H
#define VCSJOBRUNNER_H

#include "vcsbase_global.h"

#include <QtCore/QThread>
#include <QtCore/QStringList>
#include <QtCore/QString>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QScopedPointer>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace VCSBase {
class VCSBaseEditorWidget;
class VCSJobRunnerPrivate;

class VCSBASE_EXPORT VCSJob : public QObject
{
    Q_OBJECT
public:
    enum DataEmitMode {
        NoDataEmitMode,
        RawDataEmitMode,
        EditorDataEmitMode
    };

    VCSJob(const QString &workingDir,
           const QStringList &args,
           DataEmitMode emitMode = NoDataEmitMode);
    VCSJob(const QString &workingDir,
           const QStringList &args,
           VCSBase::VCSBaseEditorWidget *editor);

    DataEmitMode dataEmitMode() const;
    VCSBase::VCSBaseEditorWidget *displayEditor() const;
    QStringList arguments() const;
    QString workingDirectory() const;
    const QVariant &cookie() const;
    bool unixTerminalDisabled() const;

    void setDisplayEditor(VCSBase::VCSBaseEditorWidget *editor);
    void setCookie(const QVariant &cookie);
    // Disable terminal to suppress SSH prompting
    void setUnixTerminalDisabled(bool v);

signals:
    void succeeded(const QVariant &cookie); // Use a queued connection
    void rawData(const QByteArray &data);

private:
    friend class VCSJobRunner;
    const QString m_workingDir;
    const QStringList m_arguments;
    bool m_emitRaw;
    QVariant m_cookie;
    QPointer<VCSBase::VCSBaseEditorWidget> m_editor; // User might close it
    bool m_unixTerminalDisabled;
};

class VCSBASE_EXPORT VCSJobRunner : public QThread
{
    Q_OBJECT
public:
    VCSJobRunner();
    ~VCSJobRunner();
    void enqueueJob(const QSharedPointer<VCSJob> &job);
    void restart();

    static QString msgStartFailed(const QString &binary, const QString &why);
    static QString msgTimeout(const QString &binary, int timeoutSeconds);

    // Set environment for a VCS process to run in locale "C"
    static void setProcessEnvironment(QProcess *p);

    void setSettings(const QString &bin, const QStringList &stdArgs, int timeoutMsec);

protected:
    void run();

signals:
    void commandStarted(const QString &notice);
    void error(const QString &error);
    void output(const QByteArray &output);

private:
    void task(const QSharedPointer<VCSJob> &job);
    void stop();

    QScopedPointer<VCSJobRunnerPrivate> d;
};

} //namespace VCSBase

#endif // VCSJOBRUNNER_H
