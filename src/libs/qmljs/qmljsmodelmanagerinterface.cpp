/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmljsbind.h"
#include "qmljsconstants.h"
#include "qmljsfindexportedcpptypes.h"
#include "qmljsinterpreter.h"
#include "qmljsmodelmanagerinterface.h"
#include "qmljsplugindumper.h"
#include "qmljstypedescriptionreader.h"
#include "qmljsdialect.h"
#include "qmljsviewercontext.h"
#include "qmljsutils.h"

#include <cplusplus/cppmodelmanagerbase.h>
#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/runextensions.h>
#include <utils/stringutils.h>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QTextDocument>
#include <QTextStream>
#include <QTimer>
#include <QtAlgorithms>
#include <QLibraryInfo>

using namespace Utils;

namespace QmlJS {

QMLJS_EXPORT Q_LOGGING_CATEGORY(qmljsLog, "qtc.qmljs.common", QtWarningMsg)

/*!
    \class QmlJS::ModelManagerInterface
    \brief The ModelManagerInterface class acts as an interface to the
    global state of the QmlJS code model.
    \sa QmlJS::Document QmlJS::Snapshot QmlJSTools::Internal::ModelManager

    The ModelManagerInterface is an interface for global state and actions in
    the QmlJS code model. It is implemented by \l{QmlJSTools::Internal::ModelManager}
    and the instance can be accessed through ModelManagerInterface::instance().

    One of its primary concerns is to keep the Snapshots it
    maintains up to date by parsing documents and finding QML modules.

    It has a Snapshot that contains only valid Documents,
    accessible through ModelManagerInterface::snapshot() and a Snapshot with
    potentially more recent, but invalid documents that is exposed through
    ModelManagerInterface::newestSnapshot().
*/

static ModelManagerInterface *g_instance = nullptr;
static const char *qtQuickUISuffix = "ui.qml";

static void maybeAddPath(ViewerContext &context, const QString &path)
{
    if (!path.isEmpty() && !context.paths.contains(path))
        context.paths.append(path);
}

static QStringList environmentImportPaths()
{
    QStringList paths;

    const QStringList importPaths = QString::fromLocal8Bit(qgetenv("QML_IMPORT_PATH")).split(
        Utils::HostOsInfo::pathListSeparator(), Qt::SkipEmptyParts);

    for (const QString &path : importPaths) {
        const QString canonicalPath = QDir(path).canonicalPath();
        if (!canonicalPath.isEmpty() && !paths.contains(canonicalPath))
            paths.append(canonicalPath);
    }

    return paths;
}

ModelManagerInterface::ModelManagerInterface(QObject *parent)
    : QObject(parent),
      m_defaultImportPaths(environmentImportPaths()),
      m_pluginDumper(new PluginDumper(this))
{
    m_indexerDisabled = qEnvironmentVariableIsSet("QTC_NO_CODE_INDEXER");

    m_updateCppQmlTypesTimer = new QTimer(this);
    const int second = 1000;
    m_updateCppQmlTypesTimer->setInterval(second);
    m_updateCppQmlTypesTimer->setSingleShot(true);
    connect(m_updateCppQmlTypesTimer, &QTimer::timeout,
            this, &ModelManagerInterface::startCppQmlTypeUpdate);

    m_asyncResetTimer = new QTimer(this);
    const int fifteenSeconds = 15000;
    m_asyncResetTimer->setInterval(fifteenSeconds);
    m_asyncResetTimer->setSingleShot(true);
    connect(m_asyncResetTimer, &QTimer::timeout, this, &ModelManagerInterface::resetCodeModel);

    qRegisterMetaType<QmlJS::Document::Ptr>("QmlJS::Document::Ptr");
    qRegisterMetaType<QmlJS::LibraryInfo>("QmlJS::LibraryInfo");
    qRegisterMetaType<QmlJS::Dialect>("QmlJS::Dialect");
    qRegisterMetaType<QmlJS::PathAndLanguage>("QmlJS::PathAndLanguage");
    qRegisterMetaType<QmlJS::PathsAndLanguages>("QmlJS::PathsAndLanguages");

    m_defaultProjectInfo.qtQmlPath = QFileInfo(
                QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath)).canonicalFilePath();

    updateImportPaths();

    Q_ASSERT(! g_instance);
    g_instance = this;
}

ModelManagerInterface::~ModelManagerInterface()
{
    m_cppQmlTypesUpdater.cancel();
    m_cppQmlTypesUpdater.waitForFinished();
    Q_ASSERT(g_instance == this);
    g_instance = nullptr;
}

static QHash<QString, Dialect> defaultLanguageMapping()
{
    static QHash<QString, Dialect> res{
        {QLatin1String("js"), Dialect::JavaScript},
        {QLatin1String("qml"), Dialect::Qml},
        {QLatin1String("qmltypes"), Dialect::QmlTypeInfo},
        {QLatin1String("qmlproject"), Dialect::QmlProject},
        {QLatin1String("json"), Dialect::Json},
        {QLatin1String("qbs"), Dialect::QmlQbs},
        {QLatin1String(qtQuickUISuffix), Dialect::QmlQtQuick2Ui}
    };
    return res;
}

Dialect ModelManagerInterface::guessLanguageOfFile(const QString &fileName)
{
    QHash<QString, Dialect> lMapping;
    if (instance())
        lMapping = instance()->languageForSuffix();
    else
        lMapping = defaultLanguageMapping();
    const QFileInfo info(fileName);
    QString fileSuffix = info.suffix();

    /*
     * I was reluctant to use complete suffix in all cases, because it is a huge
     * change in behaivour. But in case of .qml this should be safe.
     */

    if (fileSuffix == QLatin1String("qml"))
        fileSuffix = info.completeSuffix();

    return lMapping.value(fileSuffix, Dialect::NoLanguage);
}

QStringList ModelManagerInterface::globPatternsForLanguages(const QList<Dialect> &languages)
{
    QStringList patterns;
    const QHash<QString, Dialect> lMapping =
            instance() ? instance()->languageForSuffix() : defaultLanguageMapping();
    for (auto i = lMapping.cbegin(), end = lMapping.cend(); i != end; ++i) {
        if (languages.contains(i.value()))
            patterns << QLatin1String("*.") + i.key();
    }
    return patterns;
}

ModelManagerInterface *ModelManagerInterface::instance()
{
    return g_instance;
}

void ModelManagerInterface::writeWarning(const QString &msg)
{
    if (ModelManagerInterface *i = instance())
        i->writeMessageInternal(msg);
    else
        qCWarning(qmljsLog) << msg;
}

ModelManagerInterface::WorkingCopy ModelManagerInterface::workingCopy()
{
    if (ModelManagerInterface *i = instance())
        return i->workingCopyInternal();
    return WorkingCopy();
}

void ModelManagerInterface::activateScan()
{
    if (!m_shouldScanImports) {
        m_shouldScanImports = true;
        updateImportPaths();
    }
}

QHash<QString, Dialect> ModelManagerInterface::languageForSuffix() const
{
    return defaultLanguageMapping();
}

void ModelManagerInterface::writeMessageInternal(const QString &msg) const
{
    qCDebug(qmljsLog) << msg;
}

ModelManagerInterface::WorkingCopy ModelManagerInterface::workingCopyInternal() const
{
    ModelManagerInterface::WorkingCopy res;
    return res;
}

void ModelManagerInterface::addTaskInternal(const QFuture<void> &result, const QString &msg,
                                            const char *taskId) const
{
    Q_UNUSED(result)
    qCDebug(qmljsLog) << "started " << taskId << " " << msg;
}

void ModelManagerInterface::loadQmlTypeDescriptionsInternal(const QString &resourcePath)
{
    const QDir typeFileDir(resourcePath + QLatin1String("/qml-type-descriptions"));
    const QStringList qmlTypesExtensions = QStringList("*.qmltypes");
    QFileInfoList qmlTypesFiles = typeFileDir.entryInfoList(
                qmlTypesExtensions,
                QDir::Files,
                QDir::Name);

    QStringList errors;
    QStringList warnings;

    // filter out the actual Qt builtins
    for (int i = 0; i < qmlTypesFiles.size(); ++i) {
        if (qmlTypesFiles.at(i).baseName() == QLatin1String("builtins")) {
            QFileInfoList list;
            list.append(qmlTypesFiles.at(i));
            CppQmlTypesLoader::defaultQtObjects =
                    CppQmlTypesLoader::loadQmlTypes(list, &errors, &warnings);
            qmlTypesFiles.removeAt(i);
            break;
        }
    }

    // load the fallbacks for libraries
    const CppQmlTypesLoader::BuiltinObjects objs =
            CppQmlTypesLoader::loadQmlTypes(qmlTypesFiles, &errors, &warnings);
    for (auto it = objs.cbegin(); it != objs.cend(); ++it)
        CppQmlTypesLoader::defaultLibraryObjects.insert(it.key(), it.value());

    for (const QString &error : qAsConst(errors))
        writeMessageInternal(error);
    for (const QString &warning : qAsConst(warnings))
        writeMessageInternal(warning);
}

void ModelManagerInterface::setDefaultProject(const ModelManagerInterface::ProjectInfo &pInfo,
                                              ProjectExplorer::Project *p)
{
    QMutexLocker l(mutex());
    m_defaultProject = p;
    m_defaultProjectInfo = pInfo;
}

Snapshot ModelManagerInterface::snapshot() const
{
    QMutexLocker locker(&m_mutex);
    return m_validSnapshot;
}

Snapshot ModelManagerInterface::newestSnapshot() const
{
    QMutexLocker locker(&m_mutex);
    return m_newestSnapshot;
}

void ModelManagerInterface::updateSourceFiles(const QStringList &files,
                                              bool emitDocumentOnDiskChanged)
{
    if (m_indexerDisabled)
        return;
    refreshSourceFiles(files, emitDocumentOnDiskChanged);
}

void ModelManagerInterface::cleanupFutures()
{
    QMutexLocker lock(&m_futuresMutex);
    const int maxFutures = 10;
    if (m_futures.size() > maxFutures) {
        const QList<QFuture<void>> futures = m_futures;
        m_futures.clear();
        for (const QFuture<void> &future : futures) {
            if (!(future.isFinished() || future.isCanceled()))
                m_futures.append(future);
        }
    }
}

QFuture<void> ModelManagerInterface::refreshSourceFiles(const QStringList &sourceFiles,
                                                        bool emitDocumentOnDiskChanged)
{
    if (sourceFiles.isEmpty())
        return QFuture<void>();

    QFuture<void> result = Utils::runAsync(&ModelManagerInterface::parse,
                                           workingCopyInternal(), sourceFiles,
                                           this, Dialect(Dialect::Qml),
                                           emitDocumentOnDiskChanged);
    addFuture(result);

    if (sourceFiles.count() > 1)
         addTaskInternal(result, tr("Parsing QML Files"), Constants::TASK_INDEX);

    if (sourceFiles.count() > 1 && !m_shouldScanImports) {
        bool scan = false;
        {
            QMutexLocker l(&m_mutex);
            if (!m_shouldScanImports) {
                m_shouldScanImports = true;
                scan = true;
            }
        }
        if (scan)
        updateImportPaths();
    }

    return result;
}


void ModelManagerInterface::fileChangedOnDisk(const QString &path)
{
    Utils::runAsync(&ModelManagerInterface::parse,
                    workingCopyInternal(), QStringList(path),
                    this, Dialect(Dialect::AnyLanguage), true);
}

void ModelManagerInterface::removeFiles(const QStringList &files)
{
    emit aboutToRemoveFiles(files);

    QMutexLocker locker(&m_mutex);

    for (const QString &file : files) {
        m_validSnapshot.remove(file);
        m_newestSnapshot.remove(file);
    }
}

namespace {
bool pInfoLessThanActive(const ModelManagerInterface::ProjectInfo &p1,
                         const ModelManagerInterface::ProjectInfo &p2)
{
    QStringList s1 = p1.activeResourceFiles;
    QStringList s2 = p2.activeResourceFiles;
    if (s1.size() < s2.size())
        return true;
    if (s1.size() > s2.size())
        return false;
    for (int i = 0; i < s1.size(); ++i) {
        if (s1.at(i) < s2.at(i))
            return true;
        if (s1.at(i) > s2.at(i))
            return false;
    }
    return false;
}

bool pInfoLessThanAll(const ModelManagerInterface::ProjectInfo &p1,
                      const ModelManagerInterface::ProjectInfo &p2)
{
    QStringList s1 = p1.allResourceFiles;
    QStringList s2 = p2.allResourceFiles;
    if (s1.size() < s2.size())
        return true;
    if (s1.size() > s2.size())
        return false;
    for (int i = 0; i < s1.size(); ++i) {
        if (s1.at(i) < s2.at(i))
            return true;
        if (s1.at(i) > s2.at(i))
            return false;
    }
    return false;
}

bool pInfoLessThanImports(const ModelManagerInterface::ProjectInfo &p1,
                          const ModelManagerInterface::ProjectInfo &p2)
{
    if (p1.qtQmlPath < p2.qtQmlPath)
        return true;
    if (p1.qtQmlPath > p2.qtQmlPath)
        return false;
    const PathsAndLanguages &s1 = p1.importPaths;
    const PathsAndLanguages &s2 = p2.importPaths;
    if (s1.size() < s2.size())
        return true;
    if (s1.size() > s2.size())
        return false;
    for (int i = 0; i < s1.size(); ++i) {
        if (s1.at(i) < s2.at(i))
            return true;
        if (s2.at(i) < s1.at(i))
            return false;
    }
    return false;
}

}

void ModelManagerInterface::iterateQrcFiles(
        ProjectExplorer::Project *project, QrcResourceSelector resources,
        const std::function<void(QrcParser::ConstPtr)> &callback)
{
    QList<ProjectInfo> pInfos;
    if (project) {
        pInfos.append(projectInfo(project));
    } else {
        pInfos = projectInfos();
        if (resources == ActiveQrcResources) // make the result predictable
            Utils::sort(pInfos, &pInfoLessThanActive);
        else
            Utils::sort(pInfos, &pInfoLessThanAll);
    }

    QSet<QString> pathsChecked;
    for (const ModelManagerInterface::ProjectInfo &pInfo : qAsConst(pInfos)) {
        QStringList qrcFilePaths;
        if (resources == ActiveQrcResources)
            qrcFilePaths = pInfo.activeResourceFiles;
        else
            qrcFilePaths = pInfo.allResourceFiles;
        for (const QString &qrcFilePath : qAsConst(qrcFilePaths)) {
            if (pathsChecked.contains(qrcFilePath))
                continue;
            pathsChecked.insert(qrcFilePath);
            QrcParser::ConstPtr qrcFile = m_qrcCache.parsedPath(qrcFilePath);
            if (qrcFile.isNull())
                continue;
            callback(qrcFile);
        }
    }
}

QStringList ModelManagerInterface::qrcPathsForFile(const QString &file, const QLocale *locale,
                                                   ProjectExplorer::Project *project,
                                                   QrcResourceSelector resources)
{
    QStringList res;
    iterateQrcFiles(project, resources, [&](const QrcParser::ConstPtr &qrcFile) {
        qrcFile->collectResourceFilesForSourceFile(file, &res, locale);
    });
    return res;
}

QStringList ModelManagerInterface::filesAtQrcPath(const QString &path, const QLocale *locale,
                                         ProjectExplorer::Project *project,
                                         QrcResourceSelector resources)
{
    QString normPath = QrcParser::normalizedQrcFilePath(path);
    QStringList res;
    iterateQrcFiles(project, resources, [&](const QrcParser::ConstPtr &qrcFile) {
        qrcFile->collectFilesAtPath(normPath, &res, locale);
    });
    return res;
}

QMap<QString, QStringList> ModelManagerInterface::filesInQrcPath(const QString &path,
                                                        const QLocale *locale,
                                                        ProjectExplorer::Project *project,
                                                        bool addDirs,
                                                        QrcResourceSelector resources)
{
    QString normPath = QrcParser::normalizedQrcDirectoryPath(path);
    QMap<QString, QStringList> res;
    iterateQrcFiles(project, resources, [&](const QrcParser::ConstPtr &qrcFile) {
        qrcFile->collectFilesInPath(normPath, &res, addDirs, locale);
    });
    return res;
}

QList<ModelManagerInterface::ProjectInfo> ModelManagerInterface::projectInfos() const
{
    QMutexLocker locker(&m_mutex);

    return m_projects.values();
}

bool ModelManagerInterface::containsProject(ProjectExplorer::Project *project) const
{
    QMutexLocker locker(&m_mutex);
    return m_projects.contains(project);
}

ModelManagerInterface::ProjectInfo ModelManagerInterface::projectInfo(
        ProjectExplorer::Project *project) const
{
    QMutexLocker locker(&m_mutex);
    return m_projects.value(project);
}

void ModelManagerInterface::updateProjectInfo(const ProjectInfo &pinfo, ProjectExplorer::Project *p)
{
    if (pinfo.project.isNull() || !p || m_indexerDisabled)
        return;

    Snapshot snapshot;
    ProjectInfo oldInfo;
    {
        QMutexLocker locker(&m_mutex);
        oldInfo = m_projects.value(p);
        m_projects.insert(p, pinfo);
        if (p == m_defaultProject)
            m_defaultProjectInfo = pinfo;
        snapshot = m_validSnapshot;
    }

    if (oldInfo.qmlDumpPath != pinfo.qmlDumpPath
            || oldInfo.qmlDumpEnvironment != pinfo.qmlDumpEnvironment) {
        m_pluginDumper->scheduleRedumpPlugins();
    }


    updateImportPaths();

    // remove files that are no longer in the project and have been deleted
    QStringList deletedFiles;
    for (const QString &oldFile : qAsConst(oldInfo.sourceFiles)) {
        if (snapshot.document(oldFile)
                && !pinfo.sourceFiles.contains(oldFile)
                && !QFile::exists(oldFile)) {
            deletedFiles += oldFile;
        }
    }
    removeFiles(deletedFiles);
    for (const QString &oldFile : qAsConst(deletedFiles))
        m_fileToProject.remove(oldFile, p);

    // parse any files not yet in the snapshot
    QStringList newFiles;
    for (const QString &file : qAsConst(pinfo.sourceFiles)) {
        if (!m_fileToProject.contains(file, p))
            m_fileToProject.insert(file, p);
        if (!snapshot.document(file))
            newFiles += file;
    }

    updateSourceFiles(newFiles, false);

    // update qrc cache
    m_qrcContents = pinfo.resourceFileContents;
    for (const QString &newQrc : qAsConst(pinfo.allResourceFiles))
        m_qrcCache.addPath(newQrc, m_qrcContents.value(newQrc));
    for (const QString &oldQrc : qAsConst(oldInfo.allResourceFiles))
        m_qrcCache.removePath(oldQrc);

    m_pluginDumper->loadBuiltinTypes(pinfo);
    emit projectInfoUpdated(pinfo);
}


void ModelManagerInterface::removeProjectInfo(ProjectExplorer::Project *project)
{
    ProjectInfo info;
    info.sourceFiles.clear();
    // update with an empty project info to clear data
    updateProjectInfo(info, project);

    {
        QMutexLocker locker(&m_mutex);
        m_projects.remove(project);
    }
}

/*!
    Returns project info with summarized info for \a path

    \note Project pointer will be empty
 */
ModelManagerInterface::ProjectInfo ModelManagerInterface::projectInfoForPath(
        const QString &path) const
{
    ProjectInfo res;
    const auto allProjectInfos = allProjectInfosForPath(path);
    for (const ProjectInfo &pInfo : allProjectInfos) {
        if (res.qtQmlPath.isEmpty())
            res.qtQmlPath = pInfo.qtQmlPath;
        res.applicationDirectories.append(pInfo.applicationDirectories);
        for (const auto &importPath : pInfo.importPaths)
            res.importPaths.maybeInsert(importPath);
    }
    res.applicationDirectories = Utils::filteredUnique(res.applicationDirectories);
    return res;
}

/*!
    Returns list of project infos for \a path
 */
QList<ModelManagerInterface::ProjectInfo> ModelManagerInterface::allProjectInfosForPath(
        const QString &path) const
{
    QList<ProjectExplorer::Project *> projects;
    {
        QMutexLocker locker(&m_mutex);
        projects = m_fileToProject.values(path);
        if (projects.isEmpty()) {
            QFileInfo fInfo(path);
            projects = m_fileToProject.values(fInfo.canonicalFilePath());
        }
    }
    QList<ProjectInfo> infos;
    for (ProjectExplorer::Project *project : qAsConst(projects)) {
        ProjectInfo info = projectInfo(project);
        if (!info.project.isNull())
            infos.append(info);
    }
    std::sort(infos.begin(), infos.end(), &pInfoLessThanImports);
    return infos;
}

bool ModelManagerInterface::isIdle() const
{
    return m_futures.isEmpty();
}

void ModelManagerInterface::emitDocumentChangedOnDisk(Document::Ptr doc)
{
    emit documentChangedOnDisk(std::move(doc));
}

void ModelManagerInterface::updateQrcFile(const QString &path)
{
    m_qrcCache.updatePath(path, m_qrcContents.value(path));
}

void ModelManagerInterface::updateDocument(const Document::Ptr &doc)
{
    {
        QMutexLocker locker(&m_mutex);
        m_validSnapshot.insert(doc);
        m_newestSnapshot.insert(doc, true);
    }
    emit documentUpdated(doc);
}

void ModelManagerInterface::updateLibraryInfo(const QString &path, const LibraryInfo &info)
{
    if (!info.pluginTypeInfoError().isEmpty())
        qCDebug(qmljsLog) << "Dumping errors for " << path << ":" << info.pluginTypeInfoError();

    {
        QMutexLocker locker(&m_mutex);
        m_validSnapshot.insertLibraryInfo(path, info);
        m_newestSnapshot.insertLibraryInfo(path, info);
    }
    // only emit if we got new useful information
    if (info.isValid())
        emit libraryInfoUpdated(path, info);
}

static QStringList filesInDirectoryForLanguages(const QString &path,
                                                const QList<Dialect> &languages)
{
    const QStringList pattern = ModelManagerInterface::globPatternsForLanguages(languages);
    QStringList files;

    const QDir dir(path);
    const auto entries = dir.entryInfoList(pattern, QDir::Files);
    for (const QFileInfo &fi : entries)
        files += fi.absoluteFilePath();

    return files;
}

static void findNewImplicitImports(const Document::Ptr &doc, const Snapshot &snapshot,
                                   QStringList *importedFiles, QSet<QString> *scannedPaths)
{
    // scan files that could be implicitly imported
    // it's important we also do this for JS files, otherwise the isEmpty check will fail
    if (snapshot.documentsInDirectory(doc->path()).isEmpty()) {
        if (!scannedPaths->contains(doc->path())) {
            *importedFiles += filesInDirectoryForLanguages(doc->path(),
                                                           doc->language().companionLanguages());
            scannedPaths->insert(doc->path());
        }
    }
}

static void findNewFileImports(const Document::Ptr &doc, const Snapshot &snapshot,
                        QStringList *importedFiles, QSet<QString> *scannedPaths)
{
    // scan files and directories that are explicitly imported
    const auto imports = doc->bind()->imports();
    for (const ImportInfo &import : imports) {
        const QString &importName = import.path();
        if (import.type() == ImportType::File) {
            if (! snapshot.document(importName))
                *importedFiles += importName;
        } else if (import.type() == ImportType::Directory) {
            if (snapshot.documentsInDirectory(importName).isEmpty()) {
                if (! scannedPaths->contains(importName)) {
                    *importedFiles += filesInDirectoryForLanguages(
                                importName, doc->language().companionLanguages());
                    scannedPaths->insert(importName);
                }
            }
        } else if (import.type() == ImportType::QrcFile) {
            const QStringList importPaths
                    = ModelManagerInterface::instance()->filesAtQrcPath(importName);
            for (const QString &importPath : importPaths) {
                if (!snapshot.document(importPath))
                    *importedFiles += importPath;
            }
        } else if (import.type() == ImportType::QrcDirectory) {
            const QMap<QString, QStringList> files
                    = ModelManagerInterface::instance()->filesInQrcPath(importName);
            for (auto qrc = files.cbegin(), end = files.cend(); qrc != end; ++qrc) {
                if (ModelManagerInterface::guessLanguageOfFile(qrc.key()).isQmlLikeOrJsLanguage()) {
                    for (const QString &sourceFile : qrc.value()) {
                        if (!snapshot.document(sourceFile))
                            *importedFiles += sourceFile;
                    }
                }
            }
        }
    }
}

enum class LibraryStatus {
    Accepted,
    Rejected,
    Unknown
};

static LibraryStatus libraryStatus(const QString &path, const Snapshot &snapshot,
                                   QSet<QString> *newLibraries)
{
    if (path.isEmpty())
        return LibraryStatus::Rejected;
    // if we know there is a library, done
    const LibraryInfo &existingInfo = snapshot.libraryInfo(path);
    if (existingInfo.isValid())
        return LibraryStatus::Accepted;
    if (newLibraries->contains(path))
        return LibraryStatus::Accepted;
    // if we looked at the path before, done
    return existingInfo.wasScanned()
            ? LibraryStatus::Rejected
            : LibraryStatus::Unknown;
}

static bool findNewQmlApplicationInPath(const QString &path,
                                        const Snapshot &snapshot,
                                        ModelManagerInterface *modelManager,
                                        QSet<QString> *newLibraries)
{
    switch (libraryStatus(path, snapshot, newLibraries)) {
    case LibraryStatus::Accepted: return true;
    case LibraryStatus::Rejected: return false;
    default: break;
    }

    QString qmltypesFile;

    QDir dir(path);
    QDirIterator it(path, QStringList { "*.qmltypes" }, QDir::Files);

    if (!it.hasNext())
        return false;

    qmltypesFile = it.next();

    LibraryInfo libraryInfo = LibraryInfo(QmlDirParser::TypeInfo(qmltypesFile));
    const QString libraryPath = dir.absolutePath();
    newLibraries->insert(libraryPath);
    modelManager->updateLibraryInfo(path, libraryInfo);
    modelManager->loadPluginTypes(QFileInfo(libraryPath).canonicalFilePath(), libraryPath,
                                  QString(), QString());
    return true;
}

static bool findNewQmlLibraryInPath(const QString &path,
                                    const Snapshot &snapshot,
                                    ModelManagerInterface *modelManager,
                                    QStringList *importedFiles,
                                    QSet<QString> *scannedPaths,
                                    QSet<QString> *newLibraries,
                                    bool ignoreMissing)
{
    switch (libraryStatus(path, snapshot, newLibraries)) {
    case LibraryStatus::Accepted: return true;
    case LibraryStatus::Rejected: return false;
    default: break;
    }

    const QDir dir(path);
    QFile qmldirFile(dir.filePath(QLatin1String("qmldir")));
    if (!qmldirFile.exists()) {
        if (!ignoreMissing) {
            LibraryInfo libraryInfo(LibraryInfo::NotFound);
            modelManager->updateLibraryInfo(path, libraryInfo);
        }
        return false;
    }

    if (Utils::HostOsInfo::isWindowsHost()) {
        // QTCREATORBUG-3402 - be case sensitive even here?
    }

    // found a new library!
    if (!qmldirFile.open(QFile::ReadOnly))
        return false;
    QString qmldirData = QString::fromUtf8(qmldirFile.readAll());

    QmlDirParser qmldirParser;
    qmldirParser.parse(qmldirData);

    const QString libraryPath = QFileInfo(qmldirFile).absolutePath();
    newLibraries->insert(libraryPath);
    modelManager->updateLibraryInfo(libraryPath, LibraryInfo(qmldirParser));
    modelManager->loadPluginTypes(QFileInfo(libraryPath).canonicalFilePath(), libraryPath,
                QString(), QString());

    // scan the qml files in the library
    const auto components = qmldirParser.components();
    for (const QmlDirParser::Component &component : components) {
        if (!component.fileName.isEmpty()) {
            const QFileInfo componentFileInfo(dir.filePath(component.fileName));
            const QString path = QDir::cleanPath(componentFileInfo.absolutePath());
            if (!scannedPaths->contains(path)) {
                *importedFiles += filesInDirectoryForLanguages(path, Dialect(Dialect::AnyLanguage)
                                                               .companionLanguages());
                scannedPaths->insert(path);
            }
        }
    }

    return true;
}

static QString modulePath(const ImportInfo &import, const QStringList &paths)
{
    if (!import.version().isValid())
        return QString();
    return modulePath(import.name(), import.version().toString(), paths);
}

static void findNewLibraryImports(const Document::Ptr &doc, const Snapshot &snapshot,
                                  ModelManagerInterface *modelManager, QStringList *importedFiles,
                                  QSet<QString> *scannedPaths, QSet<QString> *newLibraries)
{
    // scan current dir
    findNewQmlLibraryInPath(doc->path(), snapshot, modelManager,
                            importedFiles, scannedPaths, newLibraries, false);

    // scan dir and lib imports
    const QStringList importPaths = modelManager->importPathsNames();
    const auto imports = doc->bind()->imports();
    for (const ImportInfo &import : imports) {
        switch (import.type()) {
        case ImportType::Directory:
            findNewQmlLibraryInPath(import.path(), snapshot, modelManager,
                                    importedFiles, scannedPaths, newLibraries, false);
            break;
        case ImportType::Library:
            findNewQmlLibraryInPath(modulePath(import, importPaths), snapshot, modelManager,
                                    importedFiles, scannedPaths, newLibraries, false);
            break;
        default:
            break;
        }
    }
}

void ModelManagerInterface::parseLoop(QSet<QString> &scannedPaths,
                                      QSet<QString> &newLibraries,
                                      const WorkingCopy &workingCopy,
                                      QStringList files,
                                      ModelManagerInterface *modelManager,
                                      Dialect mainLanguage,
                                      bool emitDocChangedOnDisk,
                                      const std::function<bool(qreal)> &reportProgress)
{
    for (int i = 0; i < files.size(); ++i) {
        if (!reportProgress(qreal(i) / files.size()))
            return;

        const QString fileName = files.at(i);

        Dialect language = guessLanguageOfFile(fileName);
        if (language == Dialect::NoLanguage) {
            if (fileName.endsWith(QLatin1String(".qrc")))
                modelManager->updateQrcFile(fileName);
            continue;
        }
        if (language == Dialect::Qml
                && (mainLanguage == Dialect::QmlQtQuick2))
            language = mainLanguage;
        if (language == Dialect::Qml && mainLanguage == Dialect::QmlQtQuick2Ui)
            language = Dialect::QmlQtQuick2;
        if (language == Dialect::QmlTypeInfo || language == Dialect::QmlProject)
            continue;
        QString contents;
        int documentRevision = 0;

        if (workingCopy.contains(fileName)) {
            QPair<QString, int> entry = workingCopy.get(fileName);
            contents = entry.first;
            documentRevision = entry.second;
        } else {
            QFile inFile(fileName);

            if (inFile.open(QIODevice::ReadOnly)) {
                QTextStream ins(&inFile);
                contents = ins.readAll();
                inFile.close();
            } else {
                continue;
            }
        }

        Document::MutablePtr doc = Document::create(fileName, language);
        doc->setEditorRevision(documentRevision);
        doc->setSource(contents);
        doc->parse();

        // update snapshot. requires synchronization, but significantly reduces amount of file
        // system queries for library imports because queries are cached in libraryInfo
        const Snapshot snapshot = modelManager->snapshot();

        // get list of referenced files not yet in snapshot or in directories already scanned
        QStringList importedFiles;
        findNewImplicitImports(doc, snapshot, &importedFiles, &scannedPaths);
        findNewFileImports(doc, snapshot, &importedFiles, &scannedPaths);
        findNewLibraryImports(doc, snapshot, modelManager, &importedFiles, &scannedPaths,
                              &newLibraries);

        // add new files to parse list
        for (const QString &file : qAsConst(importedFiles)) {
            if (!files.contains(file))
                files.append(file);
        }

        modelManager->updateDocument(doc);
        if (emitDocChangedOnDisk)
            modelManager->emitDocumentChangedOnDisk(doc);
    }
}

class FutureReporter
{
public:
    FutureReporter(QFutureInterface<void> &future, int multiplier, int base)
        : future(future), multiplier(multiplier), base(base)
    {}

    bool operator()(qreal val)
    {
        if (future.isCanceled())
            return false;
        future.setProgressValue(int(base + multiplier * val));
        return true;
    }
private:
    QFutureInterface<void> &future;
    int multiplier;
    int base;
};

void ModelManagerInterface::parse(QFutureInterface<void> &future,
                                  const WorkingCopy &workingCopy,
                                  QStringList files,
                                  ModelManagerInterface *modelManager,
                                  Dialect mainLanguage,
                                  bool emitDocChangedOnDisk)
{
    const int progressMax = 100;
    FutureReporter reporter(future, progressMax, 0);
    future.setProgressRange(0, progressMax);

    // paths we have scanned for files and added to the files list
    QSet<QString> scannedPaths;
    // libraries we've found while scanning imports
    QSet<QString> newLibraries;
    parseLoop(scannedPaths, newLibraries, workingCopy, std::move(files), modelManager, mainLanguage,
              emitDocChangedOnDisk, reporter);
    future.setProgressValue(progressMax);
}

struct ScanItem {
    QString path;
    int depth = 0;
    Dialect language = Dialect::AnyLanguage;
};

void ModelManagerInterface::importScan(QFutureInterface<void> &future,
                                       const ModelManagerInterface::WorkingCopy &workingCopy,
                                       const PathsAndLanguages &paths,
                                       ModelManagerInterface *modelManager,
                                       bool emitDocChangedOnDisk, bool libOnly, bool forceRescan)
{
    // paths we have scanned for files and added to the files list
    QSet<QString> scannedPaths;
    {
        QMutexLocker l(&modelManager->m_mutex);
        scannedPaths = modelManager->m_scannedPaths;
    }
    // libraries we've found while scanning imports
    QSet<QString> newLibraries;

    QVector<ScanItem> pathsToScan;
    pathsToScan.reserve(paths.size());
    {
        QMutexLocker l(&modelManager->m_mutex);
        for (const auto &path : paths) {
            QString cPath = QDir::cleanPath(path.path().toString());
            if (!forceRescan && modelManager->m_scannedPaths.contains(cPath))
                continue;
            pathsToScan.append({cPath, 0, path.language()});
            modelManager->m_scannedPaths.insert(cPath);
        }
    }
    const int maxScanDepth = 5;
    int progressRange = pathsToScan.size() * (1 << (2 + maxScanDepth));
    int totalWork = progressRange;
    int workDone = 0;
    future.setProgressRange(0, progressRange); // update max length while iterating?
    const Snapshot snapshot = modelManager->snapshot();
    bool isCanceled = future.isCanceled();
    while (!pathsToScan.isEmpty() && !isCanceled) {
        ScanItem toScan = pathsToScan.last();
        pathsToScan.pop_back();
        int pathBudget = (1 << (maxScanDepth + 2 - toScan.depth));
        if (forceRescan || !scannedPaths.contains(toScan.path)) {
            QStringList importedFiles;
            if (forceRescan ||
                    (!findNewQmlLibraryInPath(toScan.path, snapshot, modelManager, &importedFiles,
                                              &scannedPaths, &newLibraries, true)
                    && !libOnly && snapshot.documentsInDirectory(toScan.path).isEmpty())) {
                importedFiles += filesInDirectoryForLanguages(toScan.path,
                                                              toScan.language.companionLanguages());
            }
            workDone += 1;
            future.setProgressValue(progressRange * workDone / totalWork);
            if (!importedFiles.isEmpty()) {
                FutureReporter reporter(future, progressRange * pathBudget / (4 * totalWork),
                                        progressRange * workDone / totalWork);
                parseLoop(scannedPaths, newLibraries, workingCopy, importedFiles, modelManager,
                          toScan.language, emitDocChangedOnDisk, reporter); // run in parallel??
                importedFiles.clear();
            }
            workDone += pathBudget / 4 - 1;
            future.setProgressValue(progressRange * workDone / totalWork);
        } else {
            workDone += pathBudget / 4;
        }
        // always descend tree, as we might have just scanned with a smaller depth
        if (toScan.depth < maxScanDepth) {
            QDir dir(toScan.path);
            QStringList subDirs(dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot));
            workDone += 1;
            totalWork += pathBudget / 2 * subDirs.size() - pathBudget * 3 / 4 + 1;
            for (const QString &path : qAsConst(subDirs))
                pathsToScan.append({dir.absoluteFilePath(path), toScan.depth + 1, toScan.language});
        } else {
            workDone += pathBudget * 3 / 4;
        }
        future.setProgressValue(progressRange * workDone / totalWork);
        isCanceled = future.isCanceled();
    }
    future.setProgressValue(progressRange);
    if (isCanceled) {
        // assume no work has been done
        QMutexLocker l(&modelManager->m_mutex);
        for (const auto &path : paths)
            modelManager->m_scannedPaths.remove(path.path().toString());
    }
}

QStringList ModelManagerInterface::importPathsNames() const
{
    QStringList names;
    QMutexLocker l(&m_mutex);
    names.reserve(m_allImportPaths.size());
    for (const PathAndLanguage &x: m_allImportPaths)
        names << x.path().toString();
    return names;
}

QmlLanguageBundles ModelManagerInterface::activeBundles() const
{
    QMutexLocker l(&m_mutex);
    return m_activeBundles;
}

QmlLanguageBundles ModelManagerInterface::extendedBundles() const
{
    QMutexLocker l(&m_mutex);
    return m_extendedBundles;
}

void ModelManagerInterface::maybeScan(const PathsAndLanguages &importPaths)
{
    if (m_indexerDisabled)
        return;
    PathsAndLanguages pathToScan;
    {
        QMutexLocker l(&m_mutex);
        for (const PathAndLanguage &importPath : importPaths)
            if (!m_scannedPaths.contains(importPath.path().toString()))
                pathToScan.maybeInsert(importPath);
    }

    if (pathToScan.length() >= 1) {
        QFuture<void> result = Utils::runAsync(&ModelManagerInterface::importScan,
                                               workingCopyInternal(), pathToScan,
                                               this, true, true, false);
        addFuture(result);
        addTaskInternal(result, tr("Scanning QML Imports"), Constants::TASK_IMPORT_SCAN);
    }
}

void ModelManagerInterface::updateImportPaths()
{
    if (m_indexerDisabled)
        return;
    PathsAndLanguages allImportPaths;
    QStringList allApplicationDirectories;
    QmlLanguageBundles activeBundles;
    QmlLanguageBundles extendedBundles;
    for (const ProjectInfo &pInfo : qAsConst(m_projects)) {
        for (const auto &importPath : pInfo.importPaths) {
            const QString canonicalPath = importPath.path().toFileInfo().canonicalFilePath();
            if (!canonicalPath.isEmpty()) {
                allImportPaths.maybeInsert(Utils::FilePath::fromString(canonicalPath),
                                           importPath.language());
            }
        }
        allApplicationDirectories.append(pInfo.applicationDirectories);
    }

    for (const ViewerContext &vContext : qAsConst(m_defaultVContexts)) {
        for (const QString &path : vContext.paths)
            allImportPaths.maybeInsert(Utils::FilePath::fromString(path), vContext.language);
        allApplicationDirectories.append(vContext.applicationDirectories);
    }

    for (const ProjectInfo &pInfo : qAsConst(m_projects)) {
        activeBundles.mergeLanguageBundles(pInfo.activeBundle);
        const auto languages = pInfo.activeBundle.languages();
        for (Dialect l : languages) {
            const auto paths = pInfo.activeBundle.bundleForLanguage(l).searchPaths().stringList();
            for (const QString &path : paths) {
                const QString canonicalPath = QFileInfo(path).canonicalFilePath();
                if (!canonicalPath.isEmpty())
                    allImportPaths.maybeInsert(Utils::FilePath::fromString(canonicalPath), l);
            }
        }
    }

    for (const ProjectInfo &pInfo : qAsConst(m_projects)) {
        if (!pInfo.qtQmlPath.isEmpty()) {
            allImportPaths.maybeInsert(Utils::FilePath::fromString(pInfo.qtQmlPath),
                                       Dialect::QmlQtQuick2);
        }
    }

    {
        const QString pathAtt = defaultProjectInfo().qtQmlPath;
        if (!pathAtt.isEmpty())
            allImportPaths.maybeInsert(Utils::FilePath::fromString(pathAtt), Dialect::QmlQtQuick2);
    }

    for (const QString &path : qAsConst(m_defaultImportPaths))
        allImportPaths.maybeInsert(Utils::FilePath::fromString(path), Dialect::Qml);
    allImportPaths.compact();
    allApplicationDirectories = Utils::filteredUnique(allApplicationDirectories);

    {
        QMutexLocker l(&m_mutex);
        m_allImportPaths = allImportPaths;
        m_activeBundles = activeBundles;
        m_extendedBundles = extendedBundles;
    }


    // check if any file in the snapshot imports something new in the new paths
    Snapshot snapshot = m_validSnapshot;
    QStringList importedFiles;
    QSet<QString> scannedPaths;
    QSet<QString> newLibraries;
    for (const Document::Ptr &doc : qAsConst(snapshot))
        findNewLibraryImports(doc, snapshot, this, &importedFiles, &scannedPaths, &newLibraries);
    for (const QString &path : allApplicationDirectories)
        findNewQmlApplicationInPath(path, snapshot, this, &newLibraries);

    updateSourceFiles(importedFiles, true);

    if (!m_shouldScanImports)
        return;
    maybeScan(allImportPaths);
}

void ModelManagerInterface::loadPluginTypes(const QString &libraryPath, const QString &importPath,
                                   const QString &importUri, const QString &importVersion)
{
    m_pluginDumper->loadPluginTypes(libraryPath, importPath, importUri, importVersion);
}

// is called *inside a c++ parsing thread*, to allow hanging on to source and ast
void ModelManagerInterface::maybeQueueCppQmlTypeUpdate(const CPlusPlus::Document::Ptr &doc)
{
    // avoid scanning documents without source code available
    doc->keepSourceAndAST();
    if (doc->utf8Source().isEmpty()) {
        doc->releaseSourceAndAST();
        return;
    }

    // keep source and AST alive if we want to scan for register calls
    const bool scan = FindExportedCppTypes::maybeExportsTypes(doc);
    if (!scan)
        doc->releaseSourceAndAST();

    // delegate actual queuing to the gui thread
    QMetaObject::invokeMethod(this, "queueCppQmlTypeUpdate",
                              Q_ARG(CPlusPlus::Document::Ptr, doc), Q_ARG(bool, scan));
}

void ModelManagerInterface::queueCppQmlTypeUpdate(const CPlusPlus::Document::Ptr &doc, bool scan)
{
    QPair<CPlusPlus::Document::Ptr, bool> prev = m_queuedCppDocuments.value(doc->fileName());
    if (prev.first && prev.second)
        prev.first->releaseSourceAndAST();
    m_queuedCppDocuments.insert(doc->fileName(), {doc, scan});
    m_updateCppQmlTypesTimer->start();
}

void ModelManagerInterface::startCppQmlTypeUpdate()
{
    // if a future is still running, delay
    if (m_cppQmlTypesUpdater.isRunning()) {
        m_updateCppQmlTypesTimer->start();
        return;
    }

    CPlusPlus::CppModelManagerBase *cppModelManager =
            CPlusPlus::CppModelManagerBase::instance();
    if (!cppModelManager)
        return;

    m_cppQmlTypesUpdater = Utils::runAsync(&ModelManagerInterface::updateCppQmlTypes,
                this, cppModelManager->snapshot(), m_queuedCppDocuments);
    m_queuedCppDocuments.clear();
}

QMutex *ModelManagerInterface::mutex() const
{
    return &m_mutex;
}

void ModelManagerInterface::asyncReset()
{
    m_asyncResetTimer->start();
}

bool rescanExports(const QString &fileName, FindExportedCppTypes &finder,
                   ModelManagerInterface::CppDataHash &newData)
{
    bool hasNewInfo = false;

    QList<LanguageUtils::FakeMetaObject::ConstPtr> exported = finder.exportedTypes();
    QHash<QString, QString> contextProperties = finder.contextProperties();
    if (exported.isEmpty() && contextProperties.isEmpty()) {
        hasNewInfo = hasNewInfo || newData.remove(fileName) > 0;
    } else {
        ModelManagerInterface::CppData &data = newData[fileName];
        if (!hasNewInfo && (data.exportedTypes.size() != exported.size()
                            || data.contextProperties != contextProperties)) {
            hasNewInfo = true;
        }
        if (!hasNewInfo) {
            QHash<QString, QByteArray> newFingerprints;
            for (const auto &newType : qAsConst(exported))
                newFingerprints[newType->className()]=newType->fingerprint();
            for (const auto &oldType : qAsConst(data.exportedTypes)) {
                if (newFingerprints.value(oldType->className()) != oldType->fingerprint()) {
                    hasNewInfo = true;
                    break;
                }
            }
        }
        data.exportedTypes = exported;
        data.contextProperties = contextProperties;
    }
    return hasNewInfo;
}

void ModelManagerInterface::updateCppQmlTypes(
        QFutureInterface<void> &futureInterface, ModelManagerInterface *qmlModelManager,
        const CPlusPlus::Snapshot &snapshot,
        const QHash<QString, QPair<CPlusPlus::Document::Ptr, bool>> &documents)
{
    futureInterface.setProgressRange(0, documents.size());
    futureInterface.setProgressValue(0);

    CppDataHash newData;
    QHash<QString, QList<CPlusPlus::Document::Ptr>> newDeclarations;
    {
        QMutexLocker locker(&qmlModelManager->m_cppDataMutex);
        newData = qmlModelManager->m_cppDataHash;
        newDeclarations = qmlModelManager->m_cppDeclarationFiles;
    }

    FindExportedCppTypes finder(snapshot);

    bool hasNewInfo = false;
    using DocScanPair = QPair<CPlusPlus::Document::Ptr, bool>;
    for (const DocScanPair &pair : documents) {
        if (futureInterface.isCanceled())
            return;
        futureInterface.setProgressValue(futureInterface.progressValue() + 1);

        CPlusPlus::Document::Ptr doc = pair.first;
        const bool scan = pair.second;
        const QString fileName = doc->fileName();
        if (!scan) {
            hasNewInfo = newData.remove(fileName) > 0 || hasNewInfo;
            const auto savedDocs = newDeclarations.value(fileName);
            for (const CPlusPlus::Document::Ptr &savedDoc : savedDocs) {
                finder(savedDoc);
                hasNewInfo = rescanExports(savedDoc->fileName(), finder, newData) || hasNewInfo;
            }
            continue;
        }

        for (auto it = newDeclarations.begin(), end = newDeclarations.end(); it != end;) {
            for (auto docIt = it->begin(), endDocIt = it->end(); docIt != endDocIt;) {
                const CPlusPlus::Document::Ptr &savedDoc = *docIt;
                if (savedDoc->fileName() == fileName) {
                    savedDoc->releaseSourceAndAST();
                    it->erase(docIt);
                    break;
                }
                ++docIt;
            }
            if (it->isEmpty())
                it = newDeclarations.erase(it);
            else
                ++it;
        }

        const auto found = finder(doc);
        for (const QString &declarationFile : found) {
            newDeclarations[declarationFile].append(doc);
            doc->keepSourceAndAST(); // keep for later reparsing when dependent doc changes
        }

        hasNewInfo = rescanExports(fileName, finder, newData) || hasNewInfo;
        doc->releaseSourceAndAST();
    }

    QMutexLocker locker(&qmlModelManager->m_cppDataMutex);
    qmlModelManager->m_cppDataHash = newData;
    qmlModelManager->m_cppDeclarationFiles = newDeclarations;
    if (hasNewInfo)
        // one could get away with re-linking the cpp types...
        QMetaObject::invokeMethod(qmlModelManager, "asyncReset");
}

ModelManagerInterface::CppDataHash ModelManagerInterface::cppData() const
{
    QMutexLocker locker(&m_cppDataMutex);
    return m_cppDataHash;
}

LibraryInfo ModelManagerInterface::builtins(const Document::Ptr &doc) const
{
    const ProjectInfo info = projectInfoForPath(doc->fileName());
    if (!info.qtQmlPath.isEmpty())
        return m_validSnapshot.libraryInfo(info.qtQmlPath);
    return LibraryInfo();
}

ViewerContext ModelManagerInterface::completeVContext(const ViewerContext &vCtx,
                                                      const Document::Ptr &doc) const
{
    return getVContext(vCtx, doc, false);
}

ViewerContext ModelManagerInterface::getVContext(const ViewerContext &vCtx,
                                                 const Document::Ptr &doc,
                                                 bool limitToProject) const
{
    ViewerContext res = vCtx;

    if (!doc.isNull()
            && ((vCtx.language == Dialect::AnyLanguage && doc->language() != Dialect::NoLanguage)
                || (vCtx.language == Dialect::Qml
                    && (doc->language() == Dialect::QmlQtQuick2
                        || doc->language() == Dialect::QmlQtQuick2Ui))))
        res.language = doc->language();
    ProjectInfo info;
    if (!doc.isNull())
        info = projectInfoForPath(doc->fileName());
    ViewerContext defaultVCtx = defaultVContext(res.language, Document::Ptr(nullptr), false);
    ProjectInfo defaultInfo = defaultProjectInfo();
    if (info.qtQmlPath.isEmpty())
        info.qtQmlPath = defaultInfo.qtQmlPath;
    info.applicationDirectories = Utils::filteredUnique(info.applicationDirectories
                                                        + defaultInfo.applicationDirectories);
    switch (res.flags) {
    case ViewerContext::Complete:
        break;
    case ViewerContext::AddAllPathsAndDefaultSelectors:
        res.selectors.append(defaultVCtx.selectors);
        Q_FALLTHROUGH();
    case ViewerContext::AddAllPaths:
    {
        for (const QString &path : qAsConst(defaultVCtx.paths))
            maybeAddPath(res, path);
        switch (res.language.dialect()) {
        case Dialect::AnyLanguage:
        case Dialect::Qml:
            maybeAddPath(res, info.qtQmlPath);
            Q_FALLTHROUGH();
        case Dialect::QmlQtQuick2:
        case Dialect::QmlQtQuick2Ui:
        {
            if (res.language == Dialect::QmlQtQuick2 || res.language == Dialect::QmlQtQuick2Ui)
                maybeAddPath(res, info.qtQmlPath);

            QList<Dialect> languages = res.language.companionLanguages();
            auto addPathsOnLanguageMatch = [&](const PathsAndLanguages &importPaths) {
                for (const auto &importPath : importPaths) {
                    if (languages.contains(importPath.language())
                            || importPath.language().companionLanguages().contains(res.language)) {
                        maybeAddPath(res, importPath.path().toString());
                    }
                }
            };
            if (limitToProject) {
                addPathsOnLanguageMatch(info.importPaths);
            } else {
                QList<ProjectInfo> allProjects;
                {
                    QMutexLocker locker(&m_mutex);
                    allProjects = m_projects.values();
                }
                std::sort(allProjects.begin(), allProjects.end(), &pInfoLessThanImports);
                for (const ProjectInfo &pInfo : qAsConst(allProjects))
                    addPathsOnLanguageMatch(pInfo.importPaths);
            }
            const auto environmentPaths = environmentImportPaths();
            for (const QString &path : environmentPaths)
                maybeAddPath(res, path);
            break;
        }
        case Dialect::NoLanguage:
        case Dialect::JavaScript:
        case Dialect::QmlTypeInfo:
        case Dialect::Json:
        case Dialect::QmlQbs:
        case Dialect::QmlProject:
            break;
        }
        break;
    }
    case ViewerContext::AddDefaultPathsAndSelectors:
        res.selectors.append(defaultVCtx.selectors);
        Q_FALLTHROUGH();
    case ViewerContext::AddDefaultPaths:
        for (const QString &path : qAsConst(defaultVCtx.paths))
            maybeAddPath(res, path);
        if (res.language == Dialect::AnyLanguage || res.language == Dialect::Qml)
            maybeAddPath(res, info.qtQmlPath);
        if (res.language == Dialect::AnyLanguage || res.language == Dialect::Qml
                || res.language == Dialect::QmlQtQuick2 || res.language == Dialect::QmlQtQuick2Ui) {
            const auto environemntPaths = environmentImportPaths();
            for (const QString &path : environemntPaths)
                maybeAddPath(res, path);
        }
        break;
    }
    res.flags = ViewerContext::Complete;
    res.applicationDirectories = info.applicationDirectories;
    return res;
}

ViewerContext ModelManagerInterface::defaultVContext(Dialect language,
                                                     const Document::Ptr &doc,
                                                     bool autoComplete) const
{
    if (!doc.isNull()) {
        if (language == Dialect::AnyLanguage && doc->language() != Dialect::NoLanguage)
            language = doc->language();
        else if (language == Dialect::Qml &&
                 (doc->language() == Dialect::QmlQtQuick2
                  || doc->language() == Dialect::QmlQtQuick2Ui))
            language = doc->language();
    }
    ViewerContext defaultCtx;
    {
        QMutexLocker locker(&m_mutex);
        defaultCtx = m_defaultVContexts.value(language);
    }
    defaultCtx.language = language;
    return autoComplete ? completeVContext(defaultCtx, doc) : defaultCtx;
}

ViewerContext ModelManagerInterface::projectVContext(Dialect language, const Document::Ptr &doc) const
{
    // Returns context limited to the project the file belongs to
    ViewerContext defaultCtx = defaultVContext(language, doc, false);
    return getVContext(defaultCtx, doc, true);
}

ModelManagerInterface::ProjectInfo ModelManagerInterface::defaultProjectInfo() const
{
    QMutexLocker l(mutex());
    return m_defaultProjectInfo;
}

ModelManagerInterface::ProjectInfo ModelManagerInterface::defaultProjectInfoForProject(
        ProjectExplorer::Project *) const
{
    return ModelManagerInterface::ProjectInfo();
}

void ModelManagerInterface::setDefaultVContext(const ViewerContext &vContext)
{
    QMutexLocker locker(&m_mutex);
    m_defaultVContexts[vContext.language] = vContext;
}

void ModelManagerInterface::test_joinAllThreads()
{
    // Loop since futures can spawn more futures as they finish.
    while (true) {
        QFuture<void> f;
        // get one future
        {
            QMutexLocker lock(&m_futuresMutex);
            for (QFuture<void> &future : m_futures) {
                if (!future.isFinished() && !future.isCanceled()) {
                    f = future;
                    break;
                }
            }
        }
        if (!f.isFinished() && !f.isCanceled()) {
            f.waitForFinished();

            // Some futures trigger more futures from connected signals
            // and in tests, we care about finishing all of these too.
            QEventLoop().processEvents();
        } else {
            break;
        }
    }
    m_futures.clear();
}

void ModelManagerInterface::addFuture(const QFuture<void> &future)
{
    {
        QMutexLocker lock(&m_futuresMutex);
        m_futures.append(future);
    }
    cleanupFutures();
}

Document::Ptr ModelManagerInterface::ensuredGetDocumentForPath(const QString &filePath)
{
    QmlJS::Document::Ptr document = newestSnapshot().document(filePath);
    if (!document) {
        document = QmlJS::Document::create(filePath, QmlJS::Dialect::Qml);
        QMutexLocker lock(&m_mutex);

        m_newestSnapshot.insert(document);
    }

    return document;
}

void ModelManagerInterface::resetCodeModel()
{
    QStringList documents;

    {
        QMutexLocker locker(&m_mutex);

        // find all documents currently in the code model
        for (const Document::Ptr &doc : qAsConst(m_validSnapshot))
            documents.append(doc->fileName());

        // reset the snapshot
        m_validSnapshot = Snapshot();
        m_newestSnapshot = Snapshot();
        m_scannedPaths.clear();
    }

    // start a reparse thread
    updateSourceFiles(documents, false);

    // rescan import directories
    m_shouldScanImports = true;
    updateImportPaths();
}

} // namespace QmlJS
