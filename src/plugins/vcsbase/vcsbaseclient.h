/**************************************************************************
**
** Copyright (C) 2015 Brian McGillion and Hugues Delorme
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef VCSBASECLIENT_H
#define VCSBASECLIENT_H

#include "vcsbase_global.h"

#include <utils/fileutils.h>

#include <QObject>
#include <QStringList>

#include <functional>

QT_BEGIN_NAMESPACE
class QFileInfo;
class QVariant;
class QProcessEnvironment;
QT_END_NAMESPACE

namespace Core { class Id; }

namespace Utils {
struct SynchronousProcessResponse;
class ExitCodeInterpreter;
}

namespace VcsBase {

class VcsCommand;
class VcsBaseEditorWidget;
class VcsBaseClientSettings;
class VcsJob;
class VcsBaseClientPrivate;
class VcsBaseEditorParameterWidget;

class VCSBASE_EXPORT VcsBaseClientImpl : public QObject
{
    Q_OBJECT

public:
};

class VCSBASE_EXPORT VcsBaseClient : public VcsBaseClientImpl
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
                          const QString &revision = QString(), int lineNumber = -1,
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
    virtual QProcessEnvironment processEnvironment() const;

    Utils::FileName vcsBinary() const;
    int vcsTimeout() const;

signals:
    void parsedStatus(const QList<VcsBase::VcsBaseClient::StatusItem> &statusList);
    // Passes on changed signals from VcsJob to Control
    void changed(const QVariant &v);

public slots:
    virtual void view(const QString &source, const QString &id,
                      const QStringList &extraOptions = QStringList());

protected:
    enum VcsCommandTag
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
    virtual QString vcsCommandString(VcsCommandTag cmd) const;
    virtual Core::Id vcsEditorKind(VcsCommandTag cmd) const = 0;
    virtual Utils::ExitCodeInterpreter *exitCodeInterpreter(VcsCommandTag cmd, QObject *parent) const;

    virtual QStringList revisionSpec(const QString &revision) const = 0;

    typedef std::function<VcsBaseEditorParameterWidget *()> ParameterWidgetCreator;
    void setDiffParameterWidgetCreator(ParameterWidgetCreator creator);
    void setLogParameterWidgetCreator(ParameterWidgetCreator creator);

    virtual StatusItem parseStatusLine(const QString &line) const = 0;

    QString vcsEditorTitle(const QString &vcsCmd, const QString &sourceId) const;
    // Fully synchronous VCS execution (QProcess-based)
    bool vcsFullySynchronousExec(const QString &workingDir, const QStringList &args,
                                 QByteArray *output) const;
    // Synchronous VCS execution using Utils::SynchronousProcess, with
    // log windows updating (using VcsBasePlugin::runVcs with flags)
    Utils::SynchronousProcessResponse vcsSynchronousExec(const QString &workingDir,
                                                         const QStringList &args,
                                                         unsigned flags = 0,
                                                         QTextCodec *outputCodec = 0) const;
    VcsBaseEditorWidget *createVcsEditor(Core::Id kind, QString title,
                                         const QString &source, bool setSourceCodec,
                                         const char *registerDynamicProperty,
                                         const QString &dynamicPropertyValue) const;

    enum JobOutputBindMode {
        NoOutputBind,
        VcsWindowOutputBind
    };

    VcsCommand *createCommand(const QString &workingDirectory,
                              VcsBaseEditorWidget *editor = 0,
                              JobOutputBindMode mode = NoOutputBind) const;
    void enqueueJob(VcsCommand *cmd, const QStringList &args, Utils::ExitCodeInterpreter *interpreter = 0);

    void resetCachedVcsInfo(const QString &workingDir);

private:
    void statusParser(const QString&);
    void annotateRevision(const QString&, const QString&, const QString&, int);
    void saveSettings();
    void commandFinishedGotoLine(QWidget*);

    friend class VcsBaseClientPrivate;
    VcsBaseClientPrivate *d;
};

} //namespace VcsBase

#endif // VCSBASECLIENT_H
