/**************************************************************************
**
** Copyright (c) 2013 Brian McGillion and Hugues Delorme
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

#ifndef VCSBASECLIENT_H
#define VCSBASECLIENT_H

#include "vcsbase_global.h"

#include <QObject>
#include <QStringList>
#include <QSharedPointer>
#include <QVariant>
#include <QProcessEnvironment>

QT_BEGIN_NAMESPACE
class QFileInfo;
QT_END_NAMESPACE

namespace Utils {
struct SynchronousProcessResponse;
}

namespace VcsBase {

class Command;
class VcsBaseEditorWidget;
class VcsBaseClientSettings;
class VcsJob;
class VcsBaseClientPrivate;
class VcsBaseEditorParameterWidget;

class VCSBASE_EXPORT VcsBaseClient : public QObject
{
    Q_OBJECT

public:
    class VCSBASE_EXPORT StatusItem
    {
    public:
        StatusItem() {}
        StatusItem(const QString &s, const QString &f);
        QString flags;
        QString file;
    };

    explicit VcsBaseClient(VcsBaseClientSettings *settings);
    ~VcsBaseClient();
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

    virtual VcsBaseClientSettings *settings() const;

signals:
    void parsedStatus(const QList<VcsBase::VcsBaseClient::StatusItem> &statusList);
    // Passes on changed signals from VcsJob to Control
    void changed(const QVariant &v);

public slots:
    virtual void view(const QString &source, const QString &id,
                      const QStringList &extraOptions = QStringList());

protected:
    enum VcsCommand
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
    virtual QString vcsCommandString(VcsCommand cmd) const;
    virtual QString vcsEditorKind(VcsCommand cmd) const = 0;

    virtual QStringList revisionSpec(const QString &revision) const = 0;
    virtual VcsBaseEditorParameterWidget *createDiffEditor(const QString &workingDir,
                                                           const QStringList &files,
                                                           const QStringList &extraOptions);
    virtual VcsBaseEditorParameterWidget *createLogEditor(const QString &workingDir,
                                                          const QStringList &files,
                                                          const QStringList &extraOptions);
    virtual StatusItem parseStatusLine(const QString &line) const = 0;

    QString vcsEditorTitle(const QString &vcsCmd, const QString &sourceId) const;
    // Fully synchronous VCS execution (QProcess-based)
    bool vcsFullySynchronousExec(const QString &workingDir,
                                 const QStringList &args,
                                 QByteArray *output);
    // Synchronous VCS execution using Utils::SynchronousProcess, with
    // log windows updating (using VcsBasePlugin::runVcs with flags)
    Utils::SynchronousProcessResponse vcsSynchronousExec(const QString &workingDir,
                                                         const QStringList &args,
                                                         unsigned flags = 0,
                                                         QTextCodec *outputCodec = 0);
    VcsBase::VcsBaseEditorWidget *createVcsEditor(const QString &kind, QString title,
                                                  const QString &source, bool setSourceCodec,
                                                  const char *registerDynamicProperty,
                                                  const QString &dynamicPropertyValue) const;

    virtual QProcessEnvironment processEnvironment() const;

    enum JobOutputBindMode {
        NoOutputBind,
        VcsWindowOutputBind
    };
    Command *createCommand(const QString &workingDirectory,
                           VcsBase::VcsBaseEditorWidget *editor = 0,
                           JobOutputBindMode mode = NoOutputBind);
    void enqueueJob(Command *cmd, const QStringList &args);

    void resetCachedVcsInfo(const QString &workingDir);

private:
    friend class VcsBaseClientPrivate;
    VcsBaseClientPrivate *d;

    Q_PRIVATE_SLOT(d, void statusParser(QByteArray))
    Q_PRIVATE_SLOT(d, void annotateRevision(QString, QString, int))
    Q_PRIVATE_SLOT(d, void saveSettings())
    Q_PRIVATE_SLOT(d, void commandFinishedGotoLine(QObject *))
};

} //namespace VcsBase

#endif // VCSBASECLIENT_H
