/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef QINSTALLER_H
#define QINSTALLER_H

#include <QtCore/QObject>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QIODevice;
class QInstallerTask;
class QInstallerComponent;

class QInstaller : public QObject
{
    Q_OBJECT

public:
    QInstaller();
    ~QInstaller();

    bool run();

    // parameter handling
    void setValue(const QString &key, const QString &value);
    QString value(const QString &key,
        const QString &defaultValue = QString()) const;
    bool containsValue(const QString &key) const;
    QString replaceVariables(const QString &str) const;
    QByteArray replaceVariables(const QByteArray &str) const;
    QString installerBinaryPath() const;
    QString uninstallerName() const;

    // installer-specific task creation
    virtual void createTasks() {}

    // component handling
    void appendComponent(QInstallerComponent *components);
    int componentCount() const;
    QInstallerComponent *component(int i) const;
    typedef QInstallerTask *(*TaskCreator)(QInstaller *);
    void registerTaskType(TaskCreator);
    int indexOfTaskType(TaskCreator) const;

    // progress handling
    //void setInstallationProgress(int);
    int installationProgress() const;
    void setInstallationProgressText(const QString &);
    QString installationProgressText() const;
    
    // convenience
    bool isCreator() const; 
    bool isInstaller() const;
    bool isUninstaller() const;
    bool isTempUninstaller() const;

    bool isVerbose() const;
    void setVerbose(bool on); 
    void connectGui(QObject *gui);
    
    QString libraryName(const QString &baseName, const QString &version);
    
    bool restartTempUninstaller(const QStringList &args);

    // status
    enum InstallerStatus {
        InstallerUnfinished,
        InstallerCanceledByUser,
        InstallerFailed,
        InstallerSucceeded,
    };
    InstallerStatus status() const;

    // I/O helper for authors of classes deriving from QInstallerStep
    static void appendInt(QIODevice *out, qint64 n);
    static void appendString(QIODevice *out, const QString &str);
    static void appendByteArray(QIODevice *out, const QByteArray &str);
    static qint64 retrieveInt(QIODevice *in);
    static QString retrieveString(QIODevice *in);
    static QByteArray retrieveByteArray(QIODevice *in);

    void dump() const;
    class Private;

public slots:
    bool runInstaller();
    bool runUninstaller();
    void interrupt();
    void showWarning(const QString &);

signals:
    void installationStarted();    
    void installationFinished();
    void uninstallationStarted();    
    void uninstallationFinished();
    void warning(QString);

private:
    Private *d;
};


class QInstallerComponent 
{
public:
    explicit QInstallerComponent(QInstaller *installer);
    ~QInstallerComponent();

    void setValue(const QString &key, const QString &value);
    QString value(const QString &key,
        const QString &defaultValue = QString()) const;

    void appendTask(QInstallerTask *step);
    void appendDirectoryTasks(const QString &sourcePath,
        const QString &targetPath);
    void appendSettingsTask(const QString &key, const QString &value);
    void appendUninstallerRegistrationTask();
    int taskCount() const;
    QInstallerTask *task(int) const;

    friend class QInstaller;
    friend class QInstaller::Private;
private:
    Q_DISABLE_COPY(QInstallerComponent);
    class Private;
    Private *d;
};


class QInstallerTask 
{
public:
    QInstallerTask(QInstaller *parent);
    virtual ~QInstallerTask() {}

    QInstaller *installer() const;

    virtual void writeToInstaller(QIODevice *out) const = 0;
    virtual void readAndExecuteFromInstaller(QIODevice *in) = 0;

    virtual void writeToUninstaller(QIODevice *out) const = 0;
    virtual void readAndExecuteFromUninstaller(QIODevice *in) = 0;

    virtual void undo() = 0;
    virtual void dump(QDebug &) const {}

    virtual QInstaller::TaskCreator creator() const = 0;

private:
    QInstaller *m_installer;
};


class QInstallerError
{
public:
    QInstallerError(const QString &m) : m_message(m) {}
    virtual ~QInstallerError() {}
    virtual QString message() const { return m_message; }
private:
    QString m_message;
};

/////////////////////////////////////////////////////////////////////
//
//  Some useful examples
//
/////////////////////////////////////////////////////////////////////


class QInstallerCopyFileTask : public QInstallerTask
{
public:
    QInstallerCopyFileTask(QInstaller *parent) : QInstallerTask(parent) {}
    QInstaller::TaskCreator creator() const;

    void setSourcePath(const QString &sourcePath) { m_sourcePath = sourcePath; }
    QString sourcePath() const { return m_sourcePath; }

    void setTargetPath(const QString &targetPath) { m_targetPath = targetPath; }
    QString targetPath() const { return m_targetPath; }

    void setPermissions(qint64 permissions) { m_permissions = permissions; }
    qint64 permissions() const { return m_permissions; }

    void writeToInstaller(QIODevice *out) const;
    void readAndExecuteFromInstaller(QIODevice *in);

    void writeToUninstaller(QIODevice *out) const;
    void readAndExecuteFromUninstaller(QIODevice *in);

    void undo();
    void dump(QDebug &) const;

private:
    QString m_sourcePath;
    QString m_targetPath;
    qint64 m_permissions;
    int m_parentDirCount;
};


class QInstallerLinkFileTask : public QInstallerTask
{
public:
    QInstallerLinkFileTask(QInstaller *parent) : QInstallerTask(parent) {}
    QInstaller::TaskCreator creator() const;

    void setTargetPath(const QString &path) { m_targetPath = path; }
    QString targetPath() const { return m_targetPath; }

    void setLinkTargetPath(const QString &path) { m_linkTargetPath = path; }
    QString linkTargetPath() const { return m_linkTargetPath; }

    void setPermissions(qint64 permissions) { m_permissions = permissions; }
    qint64 permissions() const { return m_permissions; }

    void writeToInstaller(QIODevice *out) const;
    void readAndExecuteFromInstaller(QIODevice *in);

    void writeToUninstaller(QIODevice *out) const;
    void readAndExecuteFromUninstaller(QIODevice *in);

    void undo();
    void dump(QDebug &) const;

public:
    QString m_targetPath; // location of the link in the target system
    QString m_linkTargetPath; // linkee
    qint64 m_permissions;
};


class QInstallerWriteSettingsTask : public QInstallerTask
{
public:
    QInstallerWriteSettingsTask(QInstaller *parent)
        : QInstallerTask(parent) {}
    QInstaller::TaskCreator creator() const;

    void setKey(const QString &key) { m_key = key; }
    QString key() const { return m_key; }

    void setValue(const QString &value) { m_value = value; }
    QString value() const { return m_value; }

    void writeToInstaller(QIODevice *out) const;
    void readAndExecuteFromInstaller(QIODevice *in);

    void writeToUninstaller(QIODevice *out) const;
    void readAndExecuteFromUninstaller(QIODevice *in);

    void undo();
    void dump(QDebug &) const;

public:
    QString m_key;
    QString m_value;
};


class QInstallerPatchFileTask : public QInstallerTask
{
public:
    QInstallerPatchFileTask(QInstaller *parent) : QInstallerTask(parent) {}
    QInstaller::TaskCreator creator() const;

    void setTargetPath(const QString &path) { m_targetPath = path; }
    QString targetPath() const { return m_targetPath; }

    void setNeedle(const QByteArray &needle) { m_needle = needle; }
    QByteArray needle() const { return m_needle; }

    void setReplacement(const QByteArray &repl) { m_replacement = repl; }
    QByteArray replacement() const { return m_replacement; }

    void writeToInstaller(QIODevice *out) const;
    void readAndExecuteFromInstaller(QIODevice *in);

    void writeToUninstaller(QIODevice *) const {}
    void readAndExecuteFromUninstaller(QIODevice *) {}

    void undo() {}
    void dump(QDebug &) const;

private:
    QByteArray m_needle;
    QByteArray m_replacement;
    QString m_targetPath;
};


class QInstallerMenuShortcutTask : public QInstallerTask
{
public:
    QInstallerMenuShortcutTask(QInstaller *parent) : QInstallerTask(parent) {}
    QInstaller::TaskCreator creator() const;

    void setTargetPath(const QString &path) { m_targetPath = path; }
    QString targetPath() const { return m_targetPath; }

    void setLinkTargetPath(const QString &path) { m_linkTargetPath = path; }
    QString linkTargetPath() const { return m_linkTargetPath; }
    
    void writeToInstaller(QIODevice *out) const;
    void readAndExecuteFromInstaller(QIODevice *in);

    void writeToUninstaller(QIODevice *out) const;
    void readAndExecuteFromUninstaller(QIODevice *in);

    void undo();
    void dump(QDebug &) const;

public:
    QString m_targetPath;
    QString m_linkTargetPath;
    QString m_startMenuPath;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QINSTALLER_H
