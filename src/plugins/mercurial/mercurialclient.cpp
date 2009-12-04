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

#include "mercurialclient.h"
#include "mercurialjobrunner.h"
#include "constants.h"
#include "mercurialsettings.h"
#include "mercurialplugin.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/qtcassert.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseoutputwindow.h>

#include <QtCore/QStringList>
#include <QtCore/QSharedPointer>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QTextCodec>
#include <QtCore/QtDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QByteArray>
#include <QtCore/QMetaType>

Q_DECLARE_METATYPE(QVariant)

inline Core::IEditor* locateEditor(const Core::ICore *core, const char *property, const QString &entry)
{
    foreach (Core::IEditor *ed, core->editorManager()->openedEditors())
        if (ed->file()->property(property).toString() == entry)
            return ed;
    return 0;
}

namespace Mercurial {
namespace Internal  {

MercurialClient::MercurialClient() :
    jobManager(0),
    core(Core::ICore::instance())
{
    qRegisterMetaType<QVariant>();
}

MercurialClient::~MercurialClient()
{
    if (jobManager) {
        delete jobManager;
        jobManager = 0;
    }
}

bool MercurialClient::add(const QString &filename)
{
    QFileInfo file(filename);
    QStringList args;
    args << QLatin1String("add") << file.absoluteFilePath();

    return executeHgSynchronously(file, args);
}

bool MercurialClient::remove(const QString &filename)
{
    QFileInfo file(filename);
    QStringList args;
    args << QLatin1String("remove") << file.absoluteFilePath();

    return executeHgSynchronously(file, args);
}

bool MercurialClient::manifestSync(const QString &filename)
{
    QFileInfo file(filename);
    QStringList args(QLatin1String("manifest"));

    QByteArray output;
    executeHgSynchronously(file, args, &output);

    const QStringList files = QString::fromLocal8Bit(output).split(QLatin1Char('\n'));

    foreach (const QString &fileName, files) {
        const QFileInfo managedFile(fileName);
        if (file == managedFile)
            return true;
    }

    return false;
}

bool MercurialClient::executeHgSynchronously(const QFileInfo &file, const QStringList &args,
                                             QByteArray *output) const
{
    QProcess hgProcess;
    hgProcess.setWorkingDirectory(file.isDir() ? file.absoluteFilePath() : file.absolutePath());

    const MercurialSettings &settings = MercurialPlugin::instance()->settings();
    const QString binary = settings.binary();
    const QStringList arguments = settings.standardArguments() + args;

    VCSBase::VCSBaseOutputWindow *outputWindow = VCSBase::VCSBaseOutputWindow::instance();
    outputWindow->appendCommand(MercurialJobRunner::msgExecute(binary, arguments));

    hgProcess.start(binary, arguments);

    if (!hgProcess.waitForStarted()) {
        outputWindow->appendError(MercurialJobRunner::msgStartFailed(binary, hgProcess.errorString()));
        return false;
    }

    hgProcess.closeWriteChannel();

    if (!hgProcess.waitForFinished(settings.timeoutMilliSeconds())) {
        hgProcess.terminate();
        outputWindow->appendError(MercurialJobRunner::msgTimeout(settings.timeoutSeconds()));
        return false;
    }

    if ((hgProcess.exitStatus() == QProcess::NormalExit) && (hgProcess.exitCode() == 0)) {
        if (output)
            *output = hgProcess.readAllStandardOutput();
        return true;
    }

    return false;
}

QString MercurialClient::branchQuerySync(const QFileInfo &repositoryRoot)
{
    QByteArray output;
    if (executeHgSynchronously(repositoryRoot, QStringList(QLatin1String("branch")), &output))
        return QTextCodec::codecForLocale()->toUnicode(output).trimmed();

    return QLatin1String("Unknown Branch");
}

void MercurialClient::annotate(const QFileInfo &file)
{
    QStringList args;
    args << QLatin1String("annotate") << QLatin1String("-u") << QLatin1String("-c") << QLatin1String("-d") << file.absoluteFilePath();

    const QString kind = QLatin1String(Constants::ANNOTATELOG);
    const QString title = tr("Hg Annotate %1").arg(file.fileName());

    VCSBase::VCSBaseEditor *editor = createVCSEditor(kind, title, file.absolutePath(), true,
                                                     "annotate", file.absoluteFilePath());

    QSharedPointer<HgTask> job(new HgTask(file.absolutePath(), args, editor));
    enqueueJob(job);
}

void MercurialClient::diff(const QFileInfo &fileOrDir)
{
    QStringList args;
    QString id;
    QString workingPath;

    args << QLatin1String("diff") << QLatin1String("-g") << QLatin1String("-p")
         << QLatin1String("-U 8");

    if (!fileOrDir.isDir()) {
        args.append(fileOrDir.absoluteFilePath());
        id = fileOrDir.absoluteFilePath();
        workingPath = fileOrDir.absolutePath();
    } else {
        id = MercurialPlugin::instance()->currentProjectName();
        workingPath = fileOrDir.absoluteFilePath();
    }

    const QString kind = QLatin1String(Constants::DIFFLOG);
    const QString title = tr("Hg diff %1").arg(fileOrDir.isDir() ? id : fileOrDir.fileName());

    VCSBase::VCSBaseEditor *editor = createVCSEditor(kind, title, workingPath, true,
                                                     "diff", id);

    QSharedPointer<HgTask> job(new HgTask(workingPath, args, editor));
    enqueueJob(job);
}

void MercurialClient::log(const QFileInfo &fileOrDir)
{
    QStringList args(QLatin1String("log"));
    QString id;
    QString workingDir;

    if (!fileOrDir.isDir()) {
        args.append(fileOrDir.absoluteFilePath());
        id = fileOrDir.absoluteFilePath();
        workingDir = fileOrDir.absolutePath();
    } else {
        id = MercurialPlugin::instance()->currentProjectName();
        workingDir = fileOrDir.absoluteFilePath();
    }

    const QString kind = QLatin1String(Constants::FILELOG);
    const QString title = tr("Hg log %1").arg(fileOrDir.isDir() ? id : fileOrDir.fileName());

    VCSBase::VCSBaseEditor *editor = createVCSEditor(kind, title, workingDir, true,
                                                     "log", id);

    QSharedPointer<HgTask> job(new HgTask(workingDir, args, editor));
    enqueueJob(job);
}

void MercurialClient::revert(const QFileInfo &fileOrDir, const QString &revision)
{
    const QString filePath = fileOrDir.absoluteFilePath();
    const QString workingDir = fileOrDir.isDir() ? filePath : fileOrDir.absolutePath();

    QStringList args(QLatin1String("revert"));    
    if (!revision.isEmpty())
        args << QLatin1String("-r") << revision;
    args.append(fileOrDir.isDir() ? QString(QLatin1String("--all")) : filePath);

    // Indicate repository change or file list
    const QVariant cookie = fileOrDir.isDir() ? QVariant(filePath) : QVariant(QStringList(filePath));
    QSharedPointer<HgTask> job(new HgTask(workingDir, args, false, cookie));
    connect(job.data(), SIGNAL(succeeded(QVariant)), this, SIGNAL(changed(QVariant)), Qt::QueuedConnection);
    enqueueJob(job);
}

void MercurialClient::status(const QFileInfo &fileOrDir)
{
    QStringList args(QLatin1String("status"));
    if (!fileOrDir.isDir())
        args.append(fileOrDir.absoluteFilePath());

    QSharedPointer<HgTask> job(new HgTask(fileOrDir.isDir() ? fileOrDir.absoluteFilePath() :
                                          fileOrDir.absolutePath(), args, false));
    enqueueJob(job);
}

void MercurialClient::statusWithSignal(const QFileInfo &repositoryRoot)
{
    const QStringList args(QLatin1String("status"));

    QSharedPointer<HgTask> job(new HgTask(repositoryRoot.absoluteFilePath(), args, true));
    connect(job.data(), SIGNAL(rawData(QByteArray)),
            this, SLOT(statusParser(QByteArray)));

    enqueueJob(job);
}

void MercurialClient::statusParser(const QByteArray &data)
{
    QList<QPair<QString, QString> > statusList;

    QStringList rawStatusList = QTextCodec::codecForLocale()->toUnicode(data).split(QLatin1Char('\n'));

    foreach (const QString &string, rawStatusList) {
        QPair<QString, QString> status;

        if (string.startsWith(QLatin1Char('M')))
            status.first = QLatin1String("Modified");
        else if (string.startsWith(QLatin1Char('A')))
            status.first = QLatin1String("Added");
        else if (string.startsWith(QLatin1Char('R')))
            status.first = QLatin1String("Removed");
        else if (string.startsWith(QLatin1Char('!')))
            status.first = QLatin1String("Deleted");
        else if (string.startsWith(QLatin1Char('?')))
            status.first = QLatin1String("Untracked");
        else
            continue;

        //the status string should be similar to "M file_with_Changes"
        //so just should take the file name part and store it
        status.second = string.mid(2);
        statusList.append(status);
    }

    emit parsedStatus(statusList);
}

void MercurialClient::import(const QFileInfo &repositoryRoot, const QStringList &files)
{
    QStringList args;
    args << QLatin1String("import") << QLatin1String("--no-commit");
    args += files;

    QSharedPointer<HgTask> job(new HgTask(repositoryRoot.absoluteFilePath(), args, false));
    enqueueJob(job);
}

void MercurialClient::pull(const QFileInfo &repositoryRoot, const QString &repository)
{
    QStringList args(QLatin1String("pull"));
    if (!repository.isEmpty())
        args.append(repository);

    const QString path = repositoryRoot.absoluteFilePath();

    QSharedPointer<HgTask> job(new HgTask(path, args, false, QVariant(path)));
    connect(job.data(), SIGNAL(succeeded(QVariant)), this, SIGNAL(changed(QVariant)), Qt::QueuedConnection);
    enqueueJob(job);
}

void MercurialClient::push(const QFileInfo &repositoryRoot, const QString &repository)
{
    QStringList args(QLatin1String("push"));
    if (!repository.isEmpty())
        args.append(repository);

    QSharedPointer<HgTask> job(new HgTask(repositoryRoot.absoluteFilePath(), args, false));
    enqueueJob(job);
}

void MercurialClient::incoming(const QFileInfo &repositoryRoot, const QString &repository)
{
    QStringList args;
    args << QLatin1String("incoming") << QLatin1String("-g") << QLatin1String("-p");
    if (!repository.isEmpty())
        args.append(repository);

    QString id = MercurialPlugin::instance()->currentProjectName();

    const QString kind = QLatin1String(Constants::DIFFLOG);
    const QString title = tr("Hg incoming %1").arg(id);

    VCSBase::VCSBaseEditor *editor = createVCSEditor(kind, title, repositoryRoot.absoluteFilePath(),
                                                     true, "incoming", id);

    QSharedPointer<HgTask> job(new HgTask(repositoryRoot.absoluteFilePath(), args, editor));
    enqueueJob(job);
}

void MercurialClient::outgoing(const QFileInfo &repositoryRoot)
{
    QStringList args;
    args << QLatin1String("outgoing") << QLatin1String("-g") << QLatin1String("-p");

    QString id = MercurialPlugin::instance()->currentProjectName();

    const QString kind = QLatin1String(Constants::DIFFLOG);
    const QString title = tr("Hg outgoing %1").arg(id);

    VCSBase::VCSBaseEditor *editor = createVCSEditor(kind, title, repositoryRoot.absoluteFilePath(), true,
                                                     "outgoing", id);

    QSharedPointer<HgTask> job(new HgTask(repositoryRoot.absoluteFilePath(), args, editor));
    enqueueJob(job);
}

void MercurialClient::view(const QString &source, const QString &id)
{
    QStringList args;
    args << QLatin1String("log") << QLatin1String("-p") << QLatin1String("-g")
         << QLatin1String("-r") << id;

    const QString kind = QLatin1String(Constants::DIFFLOG);
    const QString title = tr("Hg log %1").arg(id);

    VCSBase::VCSBaseEditor *editor = createVCSEditor(kind, title, source,
                                                     true, "view", id);

    QSharedPointer<HgTask> job(new HgTask(source, args, editor));
    enqueueJob(job);
}

void MercurialClient::update(const QFileInfo &repositoryRoot, const QString &revision)
{
    QStringList args(QLatin1String("update"));
    if (!revision.isEmpty())
        args << revision;

    const QString path = repositoryRoot.absoluteFilePath();
    QSharedPointer<HgTask> job(new HgTask(path, args, false, QVariant(path)));
    connect(job.data(), SIGNAL(succeeded(QVariant)), this, SIGNAL(changed(QVariant)), Qt::QueuedConnection);
    enqueueJob(job);
}

void MercurialClient::commit(const QFileInfo &repositoryRoot, const QStringList &files,
                             const QString &committerInfo, const QString &commitMessageFile)
{
    QStringList args(QLatin1String("commit"));
    if (!committerInfo.isEmpty())
        args << QLatin1String("-u") << committerInfo;
    args << QLatin1String("-l") << commitMessageFile << files;
    QSharedPointer<HgTask> job(new HgTask(repositoryRoot.absoluteFilePath(), args, false));
    enqueueJob(job);
}

QString MercurialClient::findTopLevelForFile(const QFileInfo &file)
{
    const QString repositoryTopDir = QLatin1String(Constants::MECURIALREPO);
    QDir dir = file.isDir() ? QDir(file.absoluteFilePath()) : QDir(file.absolutePath());

    do {
        if (QFileInfo(dir, repositoryTopDir).exists())
            return dir.absolutePath();
    } while (dir.cdUp());

    return QString();
}

void MercurialClient::settingsChanged()
{
    if (jobManager)
        jobManager->restart();
}

VCSBase::VCSBaseEditor *MercurialClient::createVCSEditor(const QString &kind, QString title,
                                                         const QString &source, bool setSourceCodec,
                                                         const char *registerDynamicProperty,
                                                         const QString &dynamicPropertyValue) const
{
    VCSBase::VCSBaseEditor *baseEditor = 0;
    Core::IEditor* outputEditor = locateEditor(core, registerDynamicProperty, dynamicPropertyValue);
    const QString progressMsg = tr("Working...");
    if (outputEditor) {
        // Exists already
        outputEditor->createNew(progressMsg);
        baseEditor = VCSBase::VCSBaseEditor::getVcsBaseEditor(outputEditor);
        QTC_ASSERT(baseEditor, return 0);
    } else {
        outputEditor = core->editorManager()->openEditorWithContents(kind, &title, progressMsg);
        outputEditor->file()->setProperty(registerDynamicProperty, dynamicPropertyValue);
        baseEditor = VCSBase::VCSBaseEditor::getVcsBaseEditor(outputEditor);
        QTC_ASSERT(baseEditor, return 0);
        baseEditor->setSource(source);
        if (setSourceCodec)
            baseEditor->setCodec(VCSBase::VCSBaseEditor::getCodec(source));
    }

    core->editorManager()->activateEditor(outputEditor);
    return baseEditor;
}

void MercurialClient::enqueueJob(const QSharedPointer<HgTask> &job)
{
    if (!jobManager) {
        jobManager = new MercurialJobRunner();
        jobManager->start();
    }
    jobManager->enqueueJob(job);
}

} // namespace Internal
} // namespace Mercurial
