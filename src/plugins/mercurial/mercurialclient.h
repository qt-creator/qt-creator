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

#ifndef MERCURIALCLIENT_H
#define MERCURIALCLIENT_H

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QPair>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QFileInfo;
class QVariant;
QT_END_NAMESPACE

namespace Core {
class ICore;
}

namespace VCSBase{
class VCSBaseEditor;
}

namespace Utils {
    struct SynchronousProcessResponse;
}

namespace Mercurial {
namespace Internal {

class MercurialJobRunner;
class HgTask;

class MercurialClient : public QObject
{
    Q_OBJECT
public:
    MercurialClient();
    ~MercurialClient();
    bool add(const QString &workingDir, const QString &fileName);
    bool remove(const QString &workingDir, const QString &fileName);
    bool move(const QString &workingDir, const QString &from, const QString &to);
    bool manifestSync(const QString &repository, const QString &filename);
    QString branchQuerySync(const QString &repositoryRoot);
    bool parentRevisionsSync(const QString &workingDirectory,
                             const QString &file /* = QString() */,
                             const QString &revision,
                             QStringList *parents);
    bool shortDescriptionSync(const QString &workingDirectory, const QString &revision,
                              const QString &format /* = QString() */, QString *description);
    bool shortDescriptionSync(const QString &workingDirectory, const QString &revision,
                              QString *description);
    bool shortDescriptionsSync(const QString &workingDirectory, const QStringList &revisions,
                              QStringList *descriptions);
    void annotate(const QString &workingDir, const QString &files,
                  const QString revision = QString(), int lineNumber = -1);
    void diff(const QString &workingDir, const QStringList &files = QStringList());
    void log(const QString &workingDir, const QStringList &files = QStringList(),
             bool enableAnnotationContextMenu = false);
    void import(const QString &repositoryRoot, const QStringList &files);
    bool pullSync(const QString &repositoryRoot, const QString &repository = QString());
    bool pushSync(const QString &repositoryRoot, const QString &repository = QString());
    void incoming(const QString &repositoryRoot, const QString &repository = QString());
    void outgoing(const QString &repositoryRoot);
    void status(const QString &workingDir, const QString &file = QString());
    void statusWithSignal(const QString &repository);
    void revertFile(const QString &workingDir, const QString &file, const QString &revision = QString());
    void revertRepository(const QString &workingDir, const QString &revision = QString());
    bool createRepositorySync(const QString &workingDir);
    void update(const QString &repositoryRoot, const QString &revision = QString());
    void commit(const QString &repositoryRoot,
                const QStringList &files,
                const QString &commiterInfo,
                const QString &commitMessageFile,
                bool autoAddRemove = false);
    bool clone(const QString &directory, const QString &url);
    QString vcsGetRepositoryURL(const QString &directory);

    static QString findTopLevelForFile(const QFileInfo &file);

signals:
    void parsedStatus(const QList<QPair<QString, QString> > &statusList);
    // Passes on changed signals from HgTask to Control.
    void changed(const QVariant &v);

public slots:
    void view(const QString &source, const QString &id);
    void settingsChanged();

private slots:
    void statusParser(const QByteArray &data);
    void slotAnnotateRevisionRequested(const QString &source, QString change, int lineNumber);

private:
    // Fully synchronous git execution (QProcess-based).
    bool executeHgFullySynchronously(const QString  &workingDir,
                                     const QStringList &args,
                                    QByteArray *output) const;
    // Synchronous hg execution using Utils::SynchronousProcess, with
    // log windows updating (using VCSBasePlugin::runVCS with flags).
    inline Utils::SynchronousProcessResponse
        executeHgSynchronously(const QString  &workingDir, const QStringList &args,
                               unsigned flags = 0, QTextCodec *outputCodec = 0);

    void enqueueJob(const QSharedPointer<HgTask> &);
    void revert(const QString &workingDir, const QString &argument,
                const QString &revision, const QVariant &cookie);

    MercurialJobRunner *jobManager;
    Core::ICore *core;

    VCSBase::VCSBaseEditor *createVCSEditor(const QString &kind, QString title,
                                            const QString &source, bool setSourceCodec,
                                            const char *registerDynamicProperty,
                                            const QString &dynamicPropertyValue) const;
};

} //namespace Internal
} //namespace Mercurial

#endif // MERCURIALCLIENT_H
