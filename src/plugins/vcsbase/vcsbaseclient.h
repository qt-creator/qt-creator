/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Brian McGillion & Hugues Delorme
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

#ifndef VCSBASECLIENT_H
#define VCSBASECLIENT_H

#include "vcsbase_global.h"

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QPair>
#include <QtCore/QSharedPointer>
#include <QtCore/QScopedPointer>
#include <QtCore/QHash>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE
class QFileInfo;
class QVariant;
QT_END_NAMESPACE

namespace Core {
class ICore;
}

namespace Utils {
struct SynchronousProcessResponse;
}

namespace VCSBase {

class VCSBaseEditorWidget;
class VCSBaseClientSettings;
class VCSJob;
class VCSBaseClientPrivate;

class VCSBASE_EXPORT VCSBaseClient : public QObject
{
    Q_OBJECT
public:
    typedef QHash<int, QVariant> ExtraCommandOptions;

    explicit VCSBaseClient(const VCSBaseClientSettings &settings);
    ~VCSBaseClient();
    virtual bool synchronousCreateRepository(const QString &workingDir);
    virtual bool synchronousClone(const QString &workingDir,
                                  const QString &srcLocation,
                                  const QString &dstLocation,
                                  const ExtraCommandOptions &extraOptions = ExtraCommandOptions());
    virtual bool synchronousAdd(const QString &workingDir, const QString &fileName);
    virtual bool synchronousRemove(const QString &workingDir, const QString &fileName);
    virtual bool synchronousMove(const QString &workingDir,
                                 const QString &from, const QString &to);
    virtual bool synchronousPull(const QString &workingDir,
                                 const QString &srcLocation,
                                 const ExtraCommandOptions &extraOptions = ExtraCommandOptions());
    virtual bool synchronousPush(const QString &workingDir,
                                 const QString &dstLocation,
                                 const ExtraCommandOptions &extraOptions = ExtraCommandOptions());
    void annotate(const QString &workingDir, const QString &file,
                  const QString revision = QString(), int lineNumber = -1);
    void diff(const QString &workingDir, const QStringList &files = QStringList());
    void log(const QString &workingDir, const QStringList &files = QStringList(),
             bool enableAnnotationContextMenu = false);
    void status(const QString &workingDir, const QString &file = QString());
    void statusWithSignal(const QString &repository);
    void revertFile(const QString &workingDir, const QString &file, const QString &revision = QString());
    void revertAll(const QString &workingDir, const QString &revision = QString());
    void import(const QString &repositoryRoot, const QStringList &files);
    void update(const QString &repositoryRoot, const QString &revision = QString());
    void commit(const QString &repositoryRoot,
                const QStringList &files,
                const QString &commitMessageFile,
                const ExtraCommandOptions &extraOptions = ExtraCommandOptions());

    virtual QString findTopLevelForFile(const QFileInfo &file) const = 0;

signals:
    void parsedStatus(const QList<QPair<QString, QString> > &statusList);
    // Passes on changed signals from VCSJob to Control
    void changed(const QVariant &v);

public slots:
    void view(const QString &source, const QString &id);
    void settingsChanged();

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

    virtual QStringList cloneArguments(const QString &srcLocation,
                                       const QString &dstLocation,
                                       const ExtraCommandOptions &extraOptions) const = 0;
    virtual QStringList pullArguments(const QString &srcLocation,
                                      const ExtraCommandOptions &extraOptions) const = 0;
    virtual QStringList pushArguments(const QString &dstLocation,
                                      const ExtraCommandOptions &extraOptions) const = 0;
    virtual QStringList commitArguments(const QStringList &files,
                                        const QString &commitMessageFile,
                                        const ExtraCommandOptions &extraOptions) const = 0;
    virtual QStringList importArguments(const QStringList &files) const = 0;
    virtual QStringList updateArguments(const QString &revision) const = 0;
    virtual QStringList revertArguments(const QString &file, const QString &revision) const = 0;
    virtual QStringList revertAllArguments(const QString &revision) const = 0;
    virtual QStringList annotateArguments(const QString &file,
                                          const QString &revision, int lineNumber) const = 0;
    virtual QStringList diffArguments(const QStringList &files) const = 0;
    virtual QStringList logArguments(const QStringList &files) const = 0;
    virtual QStringList statusArguments(const QString &file) const = 0;
    virtual QStringList viewArguments(const QString &revision) const = 0;

    virtual QPair<QString, QString> parseStatusLine(const QString &line) const = 0;

    QString vcsEditorTitle(const QString &vcsCmd, const QString &sourceId) const;
    void enqueueJob(const QSharedPointer<VCSJob> &);
    // Fully synchronous VCS execution (QProcess-based)
    bool vcsFullySynchronousExec(const QString  &workingDir,
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

private slots:
    void statusParser(const QByteArray &data);
    void slotAnnotateRevisionRequested(const QString &source, QString change, int lineNumber);

private:
    QScopedPointer<VCSBaseClientPrivate> d;
};

} //namespace VCSBase

#endif // VCSBASECLIENT_H
