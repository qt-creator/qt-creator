/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Brian McGillion & Hugues Delorme
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

#ifndef VCSBASECLIENT_H
#define VCSBASECLIENT_H

#include "vcsbase_global.h"

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QSharedPointer>
#include <QtCore/QVariant>
#include <QtCore/QProcessEnvironment>

QT_BEGIN_NAMESPACE
class QFileInfo;
QT_END_NAMESPACE

namespace Utils {
struct SynchronousProcessResponse;
}

namespace VCSBase {

class Command;
class VCSBaseEditorWidget;
class VCSBaseClientSettings;
class VCSJob;
class VCSBaseClientPrivate;
class VCSBaseEditorParameterWidget;

class VCSBASE_EXPORT VCSBaseClient : public QObject
{
    Q_OBJECT

public:
    struct VCSBASE_EXPORT StatusItem
    {
        StatusItem();
        StatusItem(const QString &s, const QString &f);
        QString flags;
        QString file;
    };

    explicit VCSBaseClient(VCSBaseClientSettings *settings);
    ~VCSBaseClient();
    virtual bool synchronousCreateRepository(const QString &workingDir,
                                             const QStringList &extraOptions = QStringList());
    virtual bool synchronousClone(const QString &workingDir,
                                  const QString &srcLocation,
                                  const QString &dstLocation,
                                  const QStringList &extraOptions = QStringList());
    virtual bool synchronousAdd(const QString &workingDir, const QString &fileName,
                                const QStringList &extraOptions = QStringList());
    virtual bool synchronousRemove(const QString &workingDir, const QString &fileName,
                                   const QStringList &extraOptions = QStringList());
    virtual bool synchronousMove(const QString &workingDir,
                                 const QString &from, const QString &to,
                                 const QStringList &extraOptions = QStringList());
    virtual bool synchronousPull(const QString &workingDir,
                                 const QString &srcLocation,
                                 const QStringList &extraOptions = QStringList());
    virtual bool synchronousPush(const QString &workingDir,
                                 const QString &dstLocation,
                                 const QStringList &extraOptions = QStringList());
    virtual void annotate(const QString &workingDir, const QString &file,
                          const QString revision = QString(), int lineNumber = -1,
                          const QStringList &extraOptions = QStringList());
    virtual void diff(const QString &workingDir, const QStringList &files = QStringList(),
                      const QStringList &extraOptions = QStringList());
    virtual void log(const QString &workingDir, const QStringList &files = QStringList(),
                     const QStringList &extraOptions = QStringList(),
                     bool enableAnnotationContextMenu = false);
    virtual void status(const QString &workingDir, const QString &file = QString(),
                        const QStringList &extraOptions = QStringList());
    virtual void emitParsedStatus(const QString &repository,
                                  const QStringList &extraOptions = QStringList());
    virtual void revertFile(const QString &workingDir, const QString &file,
                            const QString &revision = QString(),
                            const QStringList &extraOptions = QStringList());
    virtual void revertAll(const QString &workingDir, const QString &revision = QString(),
                           const QStringList &extraOptions = QStringList());
    virtual void import(const QString &repositoryRoot, const QStringList &files,
                        const QStringList &extraOptions = QStringList());
    virtual void update(const QString &repositoryRoot, const QString &revision = QString(),
                        const QStringList &extraOptions = QStringList());
    virtual void commit(const QString &repositoryRoot, const QStringList &files,
                        const QString &commitMessageFile,
                        const QStringList &extraOptions = QStringList());

    virtual QString findTopLevelForFile(const QFileInfo &file) const = 0;

    virtual VCSBaseClientSettings *settings() const;

signals:
    void parsedStatus(const QList<VCSBase::VCSBaseClient::StatusItem> &statusList);
    // Passes on changed signals from VCSJob to Control
    void changed(const QVariant &v);

public slots:
    virtual void view(const QString &source, const QString &id,
                      const QStringList &extraOptions = QStringList());

protected:
    enum VCSCommand
    {
        CreateRepositoryCommand,
        CloneCommand,
        AddCommand,
        RemoveCommand,
        MoveCommand,
        PullCommand,
        PushCommand,
        CommitCommand,
        ImportCommand,
        UpdateCommand,
        RevertCommand,
        AnnotateCommand,
        DiffCommand,
        LogCommand,
        StatusCommand
    };
    virtual QString vcsCommandString(VCSCommand cmd) const;
    virtual QString vcsEditorKind(VCSCommand cmd) const = 0;

    virtual QStringList revisionSpec(const QString &revision) const = 0;
    virtual VCSBaseEditorParameterWidget *createDiffEditor(const QString &workingDir,
                                                           const QStringList &files,
                                                           const QStringList &extraOptions);
    virtual VCSBaseEditorParameterWidget *createLogEditor(const QString &workingDir,
                                                          const QStringList &files,
                                                          const QStringList &extraOptions);
    virtual StatusItem parseStatusLine(const QString &line) const = 0;

    QString vcsEditorTitle(const QString &vcsCmd, const QString &sourceId) const;
    // Fully synchronous VCS execution (QProcess-based)
    bool vcsFullySynchronousExec(const QString &workingDir,
                                 const QStringList &args,
                                 QByteArray *output);
    // Synchronous VCS execution using Utils::SynchronousProcess, with
    // log windows updating (using VCSBasePlugin::runVCS with flags)
    Utils::SynchronousProcessResponse vcsSynchronousExec(const QString &workingDir,
                                                         const QStringList &args,
                                                         unsigned flags = 0,
                                                         QTextCodec *outputCodec = 0);
    VCSBase::VCSBaseEditorWidget *createVCSEditor(const QString &kind, QString title,
                                                  const QString &source, bool setSourceCodec,
                                                  const char *registerDynamicProperty,
                                                  const QString &dynamicPropertyValue) const;

    virtual QProcessEnvironment processEnvironment() const;

    enum JobOutputBindMode {
        NoOutputBind,
        VcsWindowOutputBind
    };
    Command *createCommand(const QString &workingDirectory,
                           VCSBase::VCSBaseEditorWidget *editor = 0,
                           JobOutputBindMode mode = NoOutputBind);
    void enqueueJob(Command *cmd, const QStringList &args);

    void resetCachedVcsInfo(const QString &workingDir);

private:
    friend class VCSBaseClientPrivate;
    VCSBaseClientPrivate *d;

    Q_PRIVATE_SLOT(d, void statusParser(QByteArray))
    Q_PRIVATE_SLOT(d, void annotateRevision(QString, QString, int))
    Q_PRIVATE_SLOT(d, void saveSettings())
    Q_PRIVATE_SLOT(d, void commandFinishedGotoLine(QObject *))
};

} //namespace VCSBase

#endif // VCSBASECLIENT_H
