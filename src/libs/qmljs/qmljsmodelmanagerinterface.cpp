// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljsmodelmanagerinterface.h"

#include "qmljsbind.h"
#include "qmljsconstants.h"
#include "qmljsdialect.h"
#include "qmljsfindexportedcpptypes.h"
#include "qmljsinterpreter.h"
#include "qmljsplugindumper.h"
#include "qmljstr.h"
#include "qmljsutils.h"
#include "qmljsviewercontext.h"

#include <cplusplus/cppmodelmanagerbase.h>
#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/hostosinfo.h>
#include <utils/stringutils.h>

#ifdef WITH_TESTS
#include <extensionsystem/pluginmanager.h>
#endif

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
static QMutex g_instanceMutex;
static const char *qtQuickUISuffix = "ui.qml";

static void maybeAddPath(ViewerContext &context, const Utils::FilePath &path)
{
    if (!path.isEmpty() && (context.paths.count(path) <= 0))
        context.paths.insert(path);
}

static QList<Utils::FilePath> environmentImportPaths()
{
    QList<Utils::FilePath> paths;

    const QStringList importPaths = QString::fromLocal8Bit(qgetenv("QML_IMPORT_PATH")).split(
        Utils::HostOsInfo::pathListSeparator(), Qt::SkipEmptyParts);

    for (const QString &path : importPaths) {
        const Utils::FilePath canonicalPath = Utils::FilePath::fromString(path).canonicalPath();
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
    m_threadPool.setMaxThreadCount(4);
    m_futureSynchronizer.setCancelOnWait(false);
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

    m_defaultProjectInfo.qtQmlPath =
            FilePath::fromUserInput(QLibraryInfo::path(QLibraryInfo::Qml2ImportsPath));
    m_defaultProjectInfo.qmllsPath = ModelManagerInterface::qmllsForBinPath(
        FilePath::fromUserInput(QLibraryInfo::path(QLibraryInfo::BinariesPath)),
                QLibraryInfo::version());
    m_defaultProjectInfo.qtVersionString = QLibraryInfo::version().toString();

    updateImportPaths();

    QMutexLocker locker(&g_instanceMutex);
    Q_ASSERT(! g_instance);
    g_instance = this;
}

ModelManagerInterface::~ModelManagerInterface()
{
    Q_ASSERT(g_instance == this);
    m_cppQmlTypesUpdater.cancel();
    m_cppQmlTypesUpdater.waitForFinished();

    while (true) {
        joinAllThreads(true);
        // Keep these 2 mutexes in the same order as inside instanceForFuture()
        QMutexLocker instanceLocker(&g_instanceMutex);
        QMutexLocker futureLocker(&m_futuresMutex);
        if (m_futureSynchronizer.isEmpty()) {
            g_instance = nullptr;
            return;
        }
    }
}

static QHash<QString, Dialect> defaultLanguageMapping()
{
    static QHash<QString, Dialect> res{
        {QLatin1String("mjs"), Dialect::JavaScript},
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

Dialect ModelManagerInterface::guessLanguageOfFile(const Utils::FilePath &fileName)
{
    QHash<QString, Dialect> lMapping;
    if (instance())
        lMapping = instance()->languageForSuffix();
    else
        lMapping = defaultLanguageMapping();
    QString fileSuffix = fileName.suffix();

    /*
     * I was reluctant to use complete suffix in all cases, because it is a huge
     * change in behaivour. But in case of .qml this should be safe.
     */

    if (fileSuffix == QLatin1String("qml"))
        fileSuffix = fileName.completeSuffix();

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

// If the returned instance is not null, it's guaranteed that it will be valid at least as long
// as the passed QFuture object isn't finished.
ModelManagerInterface *ModelManagerInterface::instanceForFuture(const QFuture<void> &future)
{
    QMutexLocker locker(&g_instanceMutex);
    if (g_instance)
        g_instance->addFuture(future);
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

FilePath ModelManagerInterface::qmllsForBinPath(const Utils::FilePath &binPath, const QVersionNumber &version)
{
    if (version < QVersionNumber(6,4,0))
        return {};
    QString qmllsExe = "qmlls";
    if (HostOsInfo::isWindowsHost())
        qmllsExe = "qmlls.exe";
    return binPath.resolvePath(qmllsExe);
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
            CppQmlTypesLoader::defaultQtObjects() = CppQmlTypesLoader::loadQmlTypes(list,
                                                                                    &errors,
                                                                                    &warnings);
            qmlTypesFiles.removeAt(i);
            break;
        }
    }

    // load the fallbacks for libraries
    const CppQmlTypesLoader::BuiltinObjects objs =
            CppQmlTypesLoader::loadQmlTypes(qmlTypesFiles, &errors, &warnings);
    for (auto it = objs.cbegin(); it != objs.cend(); ++it)
        CppQmlTypesLoader::defaultLibraryObjects().insert(it.key(), it.value());

    for (const QString &error : std::as_const(errors))
        writeMessageInternal(error);
    for (const QString &warning : std::as_const(warnings))
        writeMessageInternal(warning);
}

void ModelManagerInterface::setDefaultProject(const ModelManagerInterface::ProjectInfo &pInfo,
                                              ProjectExplorer::Project *p)
{
    QMutexLocker locker(&m_mutex);
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

QThreadPool *ModelManagerInterface::threadPool()
{
    return &m_threadPool;
}

void ModelManagerInterface::updateSourceFiles(const QList<Utils::FilePath> &files,
                                              bool emitDocumentOnDiskChanged)
{
    if (m_indexerDisabled)
        return;
    refreshSourceFiles(files, emitDocumentOnDiskChanged);
}

QFuture<void> ModelManagerInterface::refreshSourceFiles(const QList<Utils::FilePath> &sourceFiles,
                                                        bool emitDocumentOnDiskChanged)
{
    if (sourceFiles.isEmpty())
        return QFuture<void>();

    QFuture<void> result = Utils::asyncRun(&m_threadPool, &ModelManagerInterface::parse,
                                           workingCopyInternal(), sourceFiles, this,
                                           Dialect(Dialect::Qml), emitDocumentOnDiskChanged);
    addFuture(result);

    if (sourceFiles.count() > 1)
         addTaskInternal(result, Tr::tr("Parsing QML Files"), Constants::TASK_INDEX);

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

void ModelManagerInterface::fileChangedOnDisk(const Utils::FilePath &path)
{
    addFuture(Utils::asyncRun(&m_threadPool, &ModelManagerInterface::parse, workingCopyInternal(),
                              FilePaths({path}), this, Dialect(Dialect::AnyLanguage), true));
}

void ModelManagerInterface::removeFiles(const QList<Utils::FilePath> &files)
{
    emit aboutToRemoveFiles(files);

    QMutexLocker locker(&m_mutex);

    for (const Utils::FilePath &file : files) {
        m_validSnapshot.remove(file);
        m_newestSnapshot.remove(file);
    }
}

namespace {
bool pInfoLessThanActive(const ModelManagerInterface::ProjectInfo &p1,
                         const ModelManagerInterface::ProjectInfo &p2)
{
    QList<Utils::FilePath> s1 = p1.activeResourceFiles;
    QList<Utils::FilePath> s2 = p2.activeResourceFiles;
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
    QList<Utils::FilePath> s1 = p1.allResourceFiles;
    QList<Utils::FilePath> s2 = p2.allResourceFiles;
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
    if (p1.qmllsPath < p2.qmllsPath)
        return true;
    if (p1.qmllsPath > p2.qmllsPath)
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

inline void combine(QSet<FilePath> &set, const QList<FilePath> &list)
{
    for (const FilePath &path : list)
        set.insert(path);
}

static QSet<Utils::FilePath> generatedQrc(
    const QList<ModelManagerInterface::ProjectInfo> &projectInfos)
{
    QSet<Utils::FilePath> res;
    for (const auto &pInfo : projectInfos) {
        combine(res, pInfo.generatedQrcFiles);
    }
    return res;
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

    QSet<Utils::FilePath> allQrcs = generatedQrc(pInfos);

    for (const ModelManagerInterface::ProjectInfo &pInfo : std::as_const(pInfos)) {
        if (resources == ActiveQrcResources)
            combine(allQrcs, pInfo.activeResourceFiles);
        else
            combine(allQrcs, pInfo.allResourceFiles);
    }

    for (const Utils::FilePath &qrcFilePath : std::as_const(allQrcs)) {
        QrcParser::ConstPtr qrcFile = m_qrcCache.parsedPath(qrcFilePath.toFSPathString());
        if (qrcFile.isNull())
            continue;
        callback(qrcFile);
    }
}

QStringList ModelManagerInterface::qrcPathsForFile(const Utils::FilePath &file,
                                                   const QLocale *locale,
                                                   ProjectExplorer::Project *project,
                                                   QrcResourceSelector resources)
{
    QStringList res;
    iterateQrcFiles(project, resources, [&](const QrcParser::ConstPtr &qrcFile) {
        qrcFile->collectResourceFilesForSourceFile(file.toString(), &res, locale);
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
    QList<Utils::FilePath> deletedFiles;
    for (const Utils::FilePath &oldFile : std::as_const(oldInfo.sourceFiles)) {
        if (snapshot.document(oldFile) && !pinfo.sourceFiles.contains(oldFile)
            && !oldFile.exists()) {
            deletedFiles += oldFile;
        }
    }
    removeFiles(deletedFiles);
    for (const Utils::FilePath &oldFile : std::as_const(deletedFiles))
        m_fileToProject.remove(oldFile, p);

    // parse any files not yet in the snapshot
    QList<Utils::FilePath> newFiles;
    for (const Utils::FilePath &file : std::as_const(pinfo.sourceFiles)) {
        if (!m_fileToProject.contains(file, p))
            m_fileToProject.insert(file, p);
        if (!snapshot.document(file))
            newFiles += file;
    }

    updateSourceFiles(newFiles, false);

    // update qrc cache
    m_qrcContents = pinfo.resourceFileContents;
    for (const Utils::FilePath &newQrc : std::as_const(pinfo.allResourceFiles))
        m_qrcCache.addPath(newQrc.toString(), m_qrcContents.value(newQrc));
    for (const Utils::FilePath &newQrc : pinfo.generatedQrcFiles)
        m_qrcCache.addPath(newQrc.toString(), m_qrcContents.value(newQrc));
    for (const Utils::FilePath &oldQrc : std::as_const(oldInfo.allResourceFiles))
        m_qrcCache.removePath(oldQrc.toString());

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
    const Utils::FilePath &path) const
{
    ProjectInfo res;
    const auto allProjectInfos = allProjectInfosForPath(path);
    for (const ProjectInfo &pInfo : allProjectInfos) {
        if (res.qtQmlPath.isEmpty()) {
            res.qtQmlPath = pInfo.qtQmlPath;
            res.qtVersionString = pInfo.qtVersionString;
        }
        if (res.qmllsPath.isEmpty())
            res.qmllsPath = pInfo.qmllsPath;
        res.applicationDirectories.append(pInfo.applicationDirectories);
        for (const auto &importPath : pInfo.importPaths)
            res.importPaths.maybeInsert(importPath);
        auto end = pInfo.moduleMappings.cend();
        for (auto it = pInfo.moduleMappings.cbegin(); it != end; ++it)
            res.moduleMappings.insert(it.key(), it.value());
    }
    res.applicationDirectories = Utils::filteredUnique(res.applicationDirectories);
    return res;
}

/*!
    Returns list of project infos for \a path
 */
QList<ModelManagerInterface::ProjectInfo> ModelManagerInterface::allProjectInfosForPath(
    const Utils::FilePath &path) const
{
    QList<ProjectExplorer::Project *> projects;
    {
        QMutexLocker locker(&m_mutex);
        projects = m_fileToProject.values(path);
        if (projects.isEmpty())
            projects = m_fileToProject.values(path.canonicalPath());
    }
    QList<ProjectInfo> infos;
    for (ProjectExplorer::Project *project : std::as_const(projects)) {
        ProjectInfo info = projectInfo(project);
        if (!info.project.isNull())
            infos.append(info);
    }
    if (infos.isEmpty()) {
        QMutexLocker locker(&m_mutex);
        return { m_defaultProjectInfo };
    }
    std::sort(infos.begin(), infos.end(), &pInfoLessThanImports);
    return infos;
}

void ModelManagerInterface::emitDocumentChangedOnDisk(Document::Ptr doc)
{
    emit documentChangedOnDisk(std::move(doc));
}

void ModelManagerInterface::updateQrcFile(const Utils::FilePath &path)
{
    m_qrcCache.updatePath(path.toString(), m_qrcContents.value(path));
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

void ModelManagerInterface::updateLibraryInfo(const FilePath &path, const LibraryInfo &info)
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

static QList<Utils::FilePath> filesInDirectoryForLanguages(const Utils::FilePath &path,
                                                           const QList<Dialect> &languages)
{
    const QStringList pattern = ModelManagerInterface::globPatternsForLanguages(languages);
    QList<Utils::FilePath> files;

    for (const Utils::FilePath &p : path.dirEntries(FileFilter(pattern, QDir::Files)))
        files.append(p.absoluteFilePath());

    return files;
}

static void findNewImplicitImports(const Document::Ptr &doc,
                                   const Snapshot &snapshot,
                                   QList<Utils::FilePath> *importedFiles,
                                   QSet<Utils::FilePath> *scannedPaths)
{
    // scan files that could be implicitly imported
    // it's important we also do this for JS files, otherwise the isEmpty check will fail
    if (snapshot.documentsInDirectory(doc->path()).isEmpty()) {
        if (Utils::insert(*scannedPaths, doc->path())) {
            *importedFiles += filesInDirectoryForLanguages(doc->path(),
                                                           doc->language().companionLanguages());
        }
    }
}

static void findNewFileImports(const Document::Ptr &doc,
                               const Snapshot &snapshot,
                               QList<Utils::FilePath> *importedFiles,
                               QSet<Utils::FilePath> *scannedPaths)
{
    // scan files and directories that are explicitly imported
    const auto imports = doc->bind()->imports();
    for (const ImportInfo &import : imports) {
        const QString &importName = import.path();
        Utils::FilePath importPath = Utils::FilePath::fromString(importName);
        if (import.type() == ImportType::File) {
            if (!snapshot.document(importPath))
                *importedFiles += importPath;
        } else if (import.type() == ImportType::Directory) {
            if (snapshot.documentsInDirectory(importPath).isEmpty()) {
                if (Utils::insert(*scannedPaths, importPath)) {
                    *importedFiles
                        += filesInDirectoryForLanguages(importPath,
                                                        doc->language().companionLanguages());
                }
            }
        } else if (import.type() == ImportType::QrcFile) {
            const QStringList importPaths
                    = ModelManagerInterface::instance()->filesAtQrcPath(importName);
            for (const QString &importStr : importPaths) {
                Utils::FilePath importPath = Utils::FilePath::fromString(importStr);
                if (!snapshot.document(importPath))
                    *importedFiles += importPath;
            }
        } else if (import.type() == ImportType::QrcDirectory) {
            const QMap<QString, QStringList> files
                    = ModelManagerInterface::instance()->filesInQrcPath(importName);
            for (auto qrc = files.cbegin(), end = files.cend(); qrc != end; ++qrc) {
                if (ModelManagerInterface::guessLanguageOfFile(
                        Utils::FilePath::fromString(qrc.key()))
                        .isQmlLikeOrJsLanguage()) {
                    for (const QString &sourceFile : qrc.value()) {
                        auto sourceFilePath = Utils::FilePath::fromString(sourceFile);
                        if (!snapshot.document(sourceFilePath))
                            *importedFiles += sourceFilePath;
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

static LibraryStatus libraryStatus(const FilePath &path,
                                   const Snapshot &snapshot,
                                   QSet<Utils::FilePath> *newLibraries)
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

static bool findNewQmlApplicationInPath(const FilePath &path,
                                        const Snapshot &snapshot,
                                        ModelManagerInterface *modelManager,
                                        QSet<FilePath> *newLibraries)
{
    switch (libraryStatus(path, snapshot, newLibraries)) {
    case LibraryStatus::Accepted: return true;
    case LibraryStatus::Rejected: return false;
    default: break;
    }

    FilePath qmltypesFile;

    QList<Utils::FilePath> qmlTypes = path.dirEntries(
        FileFilter(QStringList{"*.qmltypes"}, QDir::Files));

    if (qmlTypes.isEmpty())
        return false;

    qmltypesFile = qmlTypes.first();

    LibraryInfo libraryInfo = LibraryInfo(qmltypesFile.toString());
    const Utils::FilePath libraryPath = path.absolutePath();
    newLibraries->insert(libraryPath);
    modelManager->updateLibraryInfo(path, libraryInfo);
    modelManager->loadPluginTypes(libraryPath.canonicalPath(), libraryPath, QString(), QString());
    return true;
}

static bool findNewQmlLibraryInPath(const Utils::FilePath &path,
                                    const Snapshot &snapshot,
                                    ModelManagerInterface *modelManager,
                                    QList<Utils::FilePath> *importedFiles,
                                    QSet<Utils::FilePath> *scannedPaths,
                                    QSet<Utils::FilePath> *newLibraries,
                                    bool ignoreMissing)
{
    switch (libraryStatus(path, snapshot, newLibraries)) {
    case LibraryStatus::Accepted: return true;
    case LibraryStatus::Rejected: return false;
    default: break;
    }

    Utils::FilePath qmldirFile = path.pathAppended(QLatin1String("qmldir"));
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
    const expected_str<QByteArray> contents = qmldirFile.fileContents();
    if (!contents)
        return false;
    QString qmldirData = QString::fromUtf8(*contents);

    QmlDirParser qmldirParser;
    qmldirParser.parse(qmldirData);

    const Utils::FilePath libraryPath = qmldirFile.absolutePath();
    newLibraries->insert(libraryPath);
    modelManager->updateLibraryInfo(libraryPath, LibraryInfo(qmldirParser));
    modelManager->loadPluginTypes(libraryPath.canonicalPath(), libraryPath, QString(), QString());

    // scan the qml files in the library
    const auto components = qmldirParser.components();
    for (const QmlDirParser::Component &component : components) {
        if (!component.fileName.isEmpty()) {
            const FilePath componentFile = path.pathAppended(component.fileName);
            const FilePath path = componentFile.absolutePath().cleanPath();
            if (Utils::insert(*scannedPaths, path)) {
                *importedFiles += filesInDirectoryForLanguages(path, Dialect(Dialect::AnyLanguage)
                                                               .companionLanguages());
            }
        }
    }

    return true;
}

static FilePath modulePath(const ImportInfo &import, const FilePaths &paths)
{
    if (!import.version().isValid())
        return {};

    const FilePaths modPaths = modulePaths(import.name(), import.version().toString(), paths);
    return modPaths.value(0); // first is best match
}

static void findNewLibraryImports(const Document::Ptr &doc,
                                  const Snapshot &snapshot,
                                  ModelManagerInterface *modelManager,
                                  FilePaths *importedFiles,
                                  QSet<Utils::FilePath> *scannedPaths,
                                  QSet<Utils::FilePath> *newLibraries)
{
    // scan current dir
    findNewQmlLibraryInPath(doc->path(), snapshot, modelManager,
                            importedFiles, scannedPaths, newLibraries, false);

    // scan dir and lib imports
    const FilePaths importPaths = modelManager->importPathsNames();
    const auto imports = doc->bind()->imports();
    for (const ImportInfo &import : imports) {
        switch (import.type()) {
        case ImportType::Directory:
            findNewQmlLibraryInPath(Utils::FilePath::fromString(import.path()),
                                    snapshot,
                                    modelManager,
                                    importedFiles,
                                    scannedPaths,
                                    newLibraries,
                                    false);
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

void ModelManagerInterface::parseLoop(QSet<Utils::FilePath> &scannedPaths,
                                      QSet<Utils::FilePath> &newLibraries,
                                      const WorkingCopy &workingCopy,
                                      QList<Utils::FilePath> files,
                                      ModelManagerInterface *modelManager,
                                      Dialect mainLanguage,
                                      bool emitDocChangedOnDisk,
                                      const std::function<bool(qreal)> &reportProgress)
{
    for (int i = 0; i < files.size(); ++i) {
        if (!reportProgress(qreal(i) / files.size()))
            return;

        const Utils::FilePath fileName = files.at(i);

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
            const expected_str<QByteArray> fileContents = fileName.fileContents();
            if (fileContents) {
                QTextStream ins(*fileContents);
                contents = ins.readAll();
            } else {
                continue;
            }
        }

        Document::MutablePtr doc = Document::create(fileName, language);
        doc->setEditorRevision(documentRevision);
        doc->setSource(contents);
        doc->parse();

#ifdef WITH_TESTS
        if (ExtensionSystem::PluginManager::instance() // we might run as an auto-test
            && ExtensionSystem::PluginManager::isScenarioRunning("TestModelManagerInterface")) {
            ExtensionSystem::PluginManager::waitForScenarioFullyInitialized();
            if (ExtensionSystem::PluginManager::finishScenario()) {
                qDebug() << "Point 1: Shutdown triggered";
                QThread::sleep(2);
                qDebug() << "Point 3: If Point 2 was already reached, expect a crash now";
            }
        }
#endif
        // get list of referenced files not yet in snapshot or in directories already scanned
        QList<Utils::FilePath> importedFiles;

        // update snapshot. requires synchronization, but significantly reduces amount of file
        // system queries for library imports because queries are cached in libraryInfo
        {
            // Make sure the snapshot is destroyed before updateDocument, so that we don't trigger
            // the copy-on-write mechanism on its internals.
            const Snapshot snapshot = modelManager->snapshot();

            findNewImplicitImports(doc, snapshot, &importedFiles, &scannedPaths);
            findNewFileImports(doc, snapshot, &importedFiles, &scannedPaths);
            findNewLibraryImports(doc, snapshot, modelManager, &importedFiles, &scannedPaths,
                                  &newLibraries);
        }

        // add new files to parse list
        for (const Utils::FilePath &file : std::as_const(importedFiles)) {
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
    FutureReporter(QPromise<void> &promise, int multiplier, int base)
        : m_promise(promise), m_multiplier(multiplier), m_base(base)
    {}

    bool operator()(qreal val)
    {
        if (m_promise.isCanceled())
            return false;
        m_promise.setProgressValue(int(m_base + m_multiplier * val));
        return true;
    }
private:
    QPromise<void> &m_promise;
    int m_multiplier;
    int m_base;
};

void ModelManagerInterface::parse(QPromise<void> &promise,
                                  const WorkingCopy &workingCopy,
                                  QList<Utils::FilePath> files,
                                  ModelManagerInterface *modelManager,
                                  Dialect mainLanguage,
                                  bool emitDocChangedOnDisk)
{
    const int progressMax = 100;
    FutureReporter reporter(promise, progressMax, 0);
    promise.setProgressRange(0, progressMax);

    // paths we have scanned for files and added to the files list
    QSet<Utils::FilePath> scannedPaths;
    // libraries we've found while scanning imports
    QSet<Utils::FilePath> newLibraries;
    parseLoop(scannedPaths, newLibraries, workingCopy, std::move(files), modelManager, mainLanguage,
              emitDocChangedOnDisk, reporter);
    promise.setProgressValue(progressMax);
}

struct ScanItem {
    Utils::FilePath path;
    int depth = 0;
    Dialect language = Dialect::AnyLanguage;
};

void ModelManagerInterface::importScan(const WorkingCopy &workingCopy,
                                       const PathsAndLanguages &paths,
                                       ModelManagerInterface *modelManager,
                                       bool emitDocChanged, bool libOnly, bool forceRescan)
{
    QPromise<void> promise;
    promise.start();
    importScanAsync(promise, workingCopy, paths, modelManager, emitDocChanged, libOnly, forceRescan);
}

void ModelManagerInterface::importScanAsync(QPromise<void> &promise, const WorkingCopy &workingCopy,
                                            const PathsAndLanguages &paths,
                                            ModelManagerInterface *modelManager,
                                            bool emitDocChanged, bool libOnly, bool forceRescan)
{
    // paths we have scanned for files and added to the files list
    QSet<Utils::FilePath> scannedPaths;
    {
        QMutexLocker l(&modelManager->m_mutex);
        scannedPaths = modelManager->m_scannedPaths;
    }
    // libraries we've found while scanning imports
    QSet<Utils::FilePath> newLibraries;

    QVector<ScanItem> pathsToScan;
    pathsToScan.reserve(paths.size());
    {
        QMutexLocker l(&modelManager->m_mutex);
        for (const auto &path : paths) {
            Utils::FilePath cPath = path.path().cleanPath();
            if (!forceRescan && !Utils::insert(modelManager->m_scannedPaths, cPath))
                continue;
            pathsToScan.append({cPath, 0, path.language()});
        }
    }
    const int maxScanDepth = 5;
    int progressRange = pathsToScan.size() * (1 << (2 + maxScanDepth));
    int totalWork = progressRange;
    int workDone = 0;
    promise.setProgressRange(0, progressRange); // update max length while iterating?
    const Snapshot snapshot = modelManager->snapshot();
    bool isCanceled = promise.isCanceled();
    while (!pathsToScan.isEmpty() && !isCanceled) {
        ScanItem toScan = pathsToScan.last();
        pathsToScan.pop_back();
        int pathBudget = (1 << (maxScanDepth + 2 - toScan.depth));
        if (forceRescan || !scannedPaths.contains(toScan.path)) {
            QList<Utils::FilePath> importedFiles;
            if (forceRescan ||
                    (!findNewQmlLibraryInPath(toScan.path, snapshot, modelManager, &importedFiles,
                                              &scannedPaths, &newLibraries, true)
                    && !libOnly && snapshot.documentsInDirectory(toScan.path).isEmpty())) {
                importedFiles += filesInDirectoryForLanguages(toScan.path,
                                                              toScan.language.companionLanguages());
            }
            workDone += 1;
            promise.setProgressValue(progressRange * workDone / totalWork);
            if (!importedFiles.isEmpty()) {
                FutureReporter reporter(promise, progressRange * pathBudget / (4 * totalWork),
                                        progressRange * workDone / totalWork);
                parseLoop(scannedPaths, newLibraries, workingCopy, importedFiles, modelManager,
                          toScan.language, emitDocChanged, reporter); // run in parallel??
                importedFiles.clear();
            }
            workDone += pathBudget / 4 - 1;
            promise.setProgressValue(progressRange * workDone / totalWork);
        } else {
            workDone += pathBudget / 4;
        }
        // always descend tree, as we might have just scanned with a smaller depth
        if (toScan.depth < maxScanDepth) {
            Utils::FilePath dir = toScan.path;
            const QList<Utils::FilePath> subDirs = dir.dirEntries(QDir::Dirs | QDir::NoDotAndDotDot);
            workDone += 1;
            totalWork += pathBudget / 2 * subDirs.size() - pathBudget * 3 / 4 + 1;
            for (const Utils::FilePath &path : subDirs)
                pathsToScan.append({path.absoluteFilePath(), toScan.depth + 1, toScan.language});
        } else {
            workDone += pathBudget * 3 / 4;
        }
        promise.setProgressValue(progressRange * workDone / totalWork);
        isCanceled = promise.isCanceled();
    }
    promise.setProgressValue(progressRange);
    if (isCanceled) {
        // assume no work has been done
        QMutexLocker l(&modelManager->m_mutex);
        for (const auto &path : paths)
            modelManager->m_scannedPaths.remove(path.path());
    }
}

QList<Utils::FilePath> ModelManagerInterface::importPathsNames() const
{
    QList<Utils::FilePath> names;
    QMutexLocker l(&m_mutex);
    names.reserve(m_allImportPaths.size());
    for (const PathAndLanguage &x: m_allImportPaths)
        names << x.path();
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
            if (!m_scannedPaths.contains(importPath.path()))
                pathToScan.maybeInsert(importPath);
    }

    if (pathToScan.length() >= 1) {
        QFuture<void> result = Utils::asyncRun(&m_threadPool,
                                               &ModelManagerInterface::importScanAsync,
                                               workingCopyInternal(), pathToScan,
                                               this, true, true, false);
        addFuture(result);
        addTaskInternal(result, Tr::tr("Scanning QML Imports"), Constants::TASK_IMPORT_SCAN);
    }
}

static QList<Utils::FilePath> minimalPrefixPaths(const QList<Utils::FilePath> &paths)
{
    QList<Utils::FilePath> sortedPaths;
    // find minimal prefix, ensure '/' at end
    for (Utils::FilePath path : std::as_const(paths)) {
        if (!path.endsWith("/"))
            path = path.withNewPath(path.path() + "/");
        if (path.path().length() > 1)
            sortedPaths.append(path);
    }
    std::sort(sortedPaths.begin(), sortedPaths.end());
    QList<Utils::FilePath> res;
    QString lastPrefix;
    for (auto it = sortedPaths.begin(); it != sortedPaths.end(); ++it) {
        if (lastPrefix.isEmpty() || !it->startsWith(lastPrefix)) {
            lastPrefix = it->path();
            res.append(*it);
        }
    }
    return res;
}

void ModelManagerInterface::updateImportPaths()
{
    if (m_indexerDisabled)
        return;
    PathsAndLanguages allImportPaths;
    QList<Utils::FilePath> allApplicationDirectories;
    QmlLanguageBundles activeBundles;
    QmlLanguageBundles extendedBundles;
    for (const ProjectInfo &pInfo : std::as_const(m_projects)) {
        for (const auto &importPath : pInfo.importPaths) {
            const FilePath canonicalPath = importPath.path().canonicalPath();
            if (!canonicalPath.isEmpty())
                allImportPaths.maybeInsert(canonicalPath, importPath.language());
        }
        allApplicationDirectories.append(pInfo.applicationDirectories);
    }

    for (const ViewerContext &vContext : std::as_const(m_defaultVContexts)) {
        for (const Utils::FilePath &path : vContext.paths)
            allImportPaths.maybeInsert(path, vContext.language);
        allApplicationDirectories.append(vContext.applicationDirectories);
    }

    for (const ProjectInfo &pInfo : std::as_const(m_projects)) {
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

    for (const ProjectInfo &pInfo : std::as_const(m_projects)) {
        if (!pInfo.qtQmlPath.isEmpty())
            allImportPaths.maybeInsert(pInfo.qtQmlPath, Dialect::QmlQtQuick2);
    }

    {
        const FilePath pathAtt = defaultProjectInfo().qtQmlPath;
        if (!pathAtt.isEmpty())
            allImportPaths.maybeInsert(pathAtt, Dialect::QmlQtQuick2);
    }

    for (const auto &importPath : defaultProjectInfo().importPaths) {
        allImportPaths.maybeInsert(importPath);
    }

    for (const Utils::FilePath &path : std::as_const(m_defaultImportPaths))
        allImportPaths.maybeInsert(path, Dialect::Qml);
    allImportPaths.compact();
    allApplicationDirectories = Utils::filteredUnique(allApplicationDirectories);

    {
        QMutexLocker l(&m_mutex);
        m_allImportPaths = allImportPaths;
        m_activeBundles = activeBundles;
        m_extendedBundles = extendedBundles;
        m_applicationPaths = minimalPrefixPaths(allApplicationDirectories);
    }


    // check if any file in the snapshot imports something new in the new paths
    Snapshot snapshot = m_validSnapshot;
    QList<Utils::FilePath> importedFiles;
    QSet<Utils::FilePath> scannedPaths;
    QSet<Utils::FilePath> newLibraries;
    for (const Document::Ptr &doc : std::as_const(snapshot))
        findNewLibraryImports(doc, snapshot, this, &importedFiles, &scannedPaths, &newLibraries);
    for (const Utils::FilePath &path : std::as_const(allApplicationDirectories)) {
        allImportPaths.maybeInsert(path, Dialect::Qml);
        findNewQmlApplicationInPath(path, snapshot, this, &newLibraries);
    }
    for (const Utils::FilePath &qrcPath : generatedQrc(m_projects.values()))
        updateQrcFile(qrcPath);

    updateSourceFiles(importedFiles, true);

    if (!m_shouldScanImports)
        return;
    maybeScan(allImportPaths);
}

void ModelManagerInterface::loadPluginTypes(const Utils::FilePath &libraryPath,
                                            const Utils::FilePath &importPath,
                                            const QString &importUri,
                                            const QString &importVersion)
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

    QMutexLocker locker(&g_instanceMutex);
    if (g_instance) // delegate actual queuing to the gui thread
        QMetaObject::invokeMethod(g_instance, [=] { queueCppQmlTypeUpdate(doc, scan); });
}

void ModelManagerInterface::queueCppQmlTypeUpdate(const CPlusPlus::Document::Ptr &doc, bool scan)
{
    QPair<CPlusPlus::Document::Ptr, bool> prev = m_queuedCppDocuments.value(doc->filePath().path());
    if (prev.first && prev.second)
        prev.first->releaseSourceAndAST();
    m_queuedCppDocuments.insert(doc->filePath().path(), {doc, scan});
    m_updateCppQmlTypesTimer->start();
}

void ModelManagerInterface::startCppQmlTypeUpdate()
{
    // if a future is still running, delay
    if (m_cppQmlTypesUpdater.isRunning()) {
        m_updateCppQmlTypesTimer->start();
        return;
    }

    if (!CPlusPlus::CppModelManagerBase::hasSnapshots())
        return;

    m_cppQmlTypesUpdater = Utils::asyncRun(&ModelManagerInterface::updateCppQmlTypes, this,
                                           CPlusPlus::CppModelManagerBase::snapshot(),
                                           m_queuedCppDocuments);
    m_queuedCppDocuments.clear();
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
        hasNewInfo = hasNewInfo || newData.remove(fileName);
    } else {
        ModelManagerInterface::CppData &data = newData[fileName];
        if (!hasNewInfo && (data.exportedTypes.size() != exported.size()
                            || data.contextProperties != contextProperties)) {
            hasNewInfo = true;
        }
        if (!hasNewInfo) {
            QHash<QString, QByteArray> newFingerprints;
            for (const auto &newType : std::as_const(exported))
                newFingerprints[newType->className()]=newType->fingerprint();
            for (const auto &oldType : std::as_const(data.exportedTypes)) {
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

void ModelManagerInterface::updateCppQmlTypes(QPromise<void> &promise,
        ModelManagerInterface *qmlModelManager, const CPlusPlus::Snapshot &snapshot,
        const QHash<QString, QPair<CPlusPlus::Document::Ptr, bool>> &documents)
{
    promise.setProgressRange(0, documents.size());
    promise.setProgressValue(0);

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
        if (promise.isCanceled())
            return;
        promise.setProgressValue(promise.future().progressValue() + 1);

        CPlusPlus::Document::Ptr doc = pair.first;
        const bool scan = pair.second;
        const FilePath filePath = doc->filePath();
        if (!scan) {
            hasNewInfo = newData.remove(filePath.path()) || hasNewInfo;
            const auto savedDocs = newDeclarations.value(filePath.path());
            for (const CPlusPlus::Document::Ptr &savedDoc : savedDocs) {
                finder(savedDoc);
                hasNewInfo = rescanExports(savedDoc->filePath().path(), finder, newData) || hasNewInfo;
            }
            continue;
        }

        for (auto it = newDeclarations.begin(), end = newDeclarations.end(); it != end;) {
            for (auto docIt = it->begin(), endDocIt = it->end(); docIt != endDocIt;) {
                const CPlusPlus::Document::Ptr &savedDoc = *docIt;
                if (savedDoc->filePath() == filePath) {
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

        hasNewInfo = rescanExports(filePath.path(), finder, newData) || hasNewInfo;
        doc->releaseSourceAndAST();
    }

    QMutexLocker locker(&qmlModelManager->m_cppDataMutex);
    qmlModelManager->m_cppDataHash = newData;
    qmlModelManager->m_cppDeclarationFiles = newDeclarations;
    if (hasNewInfo)
        // one could get away with re-linking the cpp types...
        QMetaObject::invokeMethod(qmlModelManager, &ModelManagerInterface::asyncReset);
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
    if (info.qtQmlPath.isEmpty()) {
        info.qtQmlPath = defaultInfo.qtQmlPath;
        info.qtVersionString = defaultInfo.qtVersionString;
    }
    if (info.qtQmlPath.isEmpty() && info.importPaths.size() == 0)
        info.importPaths = defaultInfo.importPaths;
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
        for (const Utils::FilePath &path : std::as_const(defaultVCtx.paths))
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
                        maybeAddPath(res, importPath.path());
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
                for (const ProjectInfo &pInfo : std::as_const(allProjects))
                    addPathsOnLanguageMatch(pInfo.importPaths);
            }
            const auto environmentPaths = environmentImportPaths();
            for (const Utils::FilePath &path : environmentPaths)
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
        for (const Utils::FilePath &path : std::as_const(defaultVCtx.paths))
            maybeAddPath(res, path);
        if (res.language == Dialect::AnyLanguage || res.language == Dialect::Qml)
            maybeAddPath(res, info.qtQmlPath);
        if (res.language == Dialect::AnyLanguage || res.language == Dialect::Qml
                || res.language == Dialect::QmlQtQuick2 || res.language == Dialect::QmlQtQuick2Ui) {
            const auto environemntPaths = environmentImportPaths();
            for (const Utils::FilePath &path : environemntPaths)
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
    QMutexLocker locker(&m_mutex);
    return m_defaultProjectInfo;
}

ModelManagerInterface::ProjectInfo ModelManagerInterface::defaultProjectInfoForProject(
    ProjectExplorer::Project *project, const FilePaths &hiddenRccFolders) const
{
    Q_UNUSED(project);
    Q_UNUSED(hiddenRccFolders);
    return ModelManagerInterface::ProjectInfo();
}

void ModelManagerInterface::setDefaultVContext(const ViewerContext &vContext)
{
    QMutexLocker locker(&m_mutex);
    m_defaultVContexts[vContext.language] = vContext;
}

void ModelManagerInterface::joinAllThreads(bool cancelOnWait)
{
    while (true) {
        FutureSynchronizer futureSynchronizer;
        {
            QMutexLocker locker(&m_futuresMutex);
            futureSynchronizer = m_futureSynchronizer;
            m_futureSynchronizer.clearFutures();
        }
        futureSynchronizer.setCancelOnWait(cancelOnWait);
        if (futureSynchronizer.isEmpty())
            return;
    }
}

void ModelManagerInterface::test_joinAllThreads()
{
    while (true) {
        joinAllThreads();
        // In order to process all onFinished handlers of finished futures
        QCoreApplication::processEvents();
        QMutexLocker lock(&m_futuresMutex);
        // If handlers created new futures, repeat the loop
        if (m_futureSynchronizer.isEmpty())
            return;
    }
}

void ModelManagerInterface::addFuture(const QFuture<void> &future)
{
    QMutexLocker lock(&m_futuresMutex);
    m_futureSynchronizer.addFuture(future);
}

Document::Ptr ModelManagerInterface::ensuredGetDocumentForPath(const Utils::FilePath &filePath)
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
    QList<Utils::FilePath> documents;

    {
        QMutexLocker locker(&m_mutex);

        // find all documents currently in the code model
        for (const Document::Ptr &doc : std::as_const(m_validSnapshot))
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

Utils::FilePath ModelManagerInterface::fileToSource(const Utils::FilePath &path)
{
    if (!path.scheme().isEmpty())
        return path;
    for (const Utils::FilePath &p : m_applicationPaths) {
        if (!p.isEmpty() && path.startsWith(p.path())) {
            // if it is an applicationPath (i.e. in the build directory)
            // try to use the path from the build dir as resource path
            // and recover the path of the corresponding source file
            QString reducedPath = path.path().mid(p.path().size());
            QString reversePath(reducedPath);
            std::reverse(reversePath.begin(), reversePath.end());
            if (!reversePath.endsWith('/'))
                reversePath.append('/');
            QrcParser::MatchResult res;
            iterateQrcFiles(nullptr,
                            QrcResourceSelector::AllQrcResources,
                            [&](const QrcParser::ConstPtr &qrcFile) {
                                if (!qrcFile)
                                    return;
                                QrcParser::MatchResult matchNow = qrcFile->longestReverseMatches(
                                    reversePath);

                                if (matchNow.matchDepth < res.matchDepth)
                                    return;
                                if (matchNow.matchDepth == res.matchDepth) {
                                    res.reversedPaths += matchNow.reversedPaths;
                                    res.sourceFiles += matchNow.sourceFiles;
                                } else {
                                    res = matchNow;
                                }
                            });
            std::sort(res.sourceFiles.begin(), res.sourceFiles.end());
            if (!res.sourceFiles.isEmpty()) {
                return res.sourceFiles.first();
            }
            qCWarning(qmljsLog) << "Could not find source file for file" << path
                                << "in application path" << p;
        }
    }
    return path;
}

} // namespace QmlJS
