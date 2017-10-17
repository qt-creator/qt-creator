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

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QRegExp>
#include <QTextDocument>
#include <QTextStream>
#include <QTimer>
#include <QtAlgorithms>
#include <QLibraryInfo>

#include <stdio.h>

namespace QmlJS {

QMLJS_EXPORT Q_LOGGING_CATEGORY(qmljsLog, "qtc.qmljs.common")

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

static ModelManagerInterface *g_instance = 0;

const char qtQuickUISuffix[] = "ui.qml";

static QStringList environmentImportPaths()
{
    QStringList paths;

    QByteArray envImportPath = qgetenv("QML_IMPORT_PATH");

    foreach (const QString &path, QString::fromLatin1(envImportPath)
             .split(Utils::HostOsInfo::pathListSeparator(), QString::SkipEmptyParts)) {
        QString canonicalPath = QDir(path).canonicalPath();
        if (!canonicalPath.isEmpty() && !paths.contains(canonicalPath))
            paths.append(canonicalPath);
    }

    return paths;
}

ModelManagerInterface::ModelManagerInterface(QObject *parent)
    : QObject(parent),
      m_shouldScanImports(false),
      m_defaultProject(0),
      m_pluginDumper(new PluginDumper(this))
{
    m_indexerEnabled = qgetenv("QTC_NO_CODE_INDEXER") != "1";

    m_updateCppQmlTypesTimer = new QTimer(this);
    m_updateCppQmlTypesTimer->setInterval(1000);
    m_updateCppQmlTypesTimer->setSingleShot(true);
    connect(m_updateCppQmlTypesTimer, &QTimer::timeout,
            this, &ModelManagerInterface::startCppQmlTypeUpdate);

    m_asyncResetTimer = new QTimer(this);
    m_asyncResetTimer->setInterval(15000);
    m_asyncResetTimer->setSingleShot(true);
    connect(m_asyncResetTimer, &QTimer::timeout, this, &ModelManagerInterface::resetCodeModel);

    qRegisterMetaType<QmlJS::Document::Ptr>("QmlJS::Document::Ptr");
    qRegisterMetaType<QmlJS::LibraryInfo>("QmlJS::LibraryInfo");
    qRegisterMetaType<QmlJS::Dialect>("QmlJS::Dialect");
    qRegisterMetaType<QmlJS::PathAndLanguage>("QmlJS::PathAndLanguage");
    qRegisterMetaType<QmlJS::PathsAndLanguages>("QmlJS::PathsAndLanguages");

    m_defaultProjectInfo.qtImportsPath = QFileInfo(
                QLibraryInfo::location(QLibraryInfo::ImportsPath)).canonicalFilePath();
    m_defaultProjectInfo.qtQmlPath = QFileInfo(
                QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath)).canonicalFilePath();

    m_defaultImportPaths << environmentImportPaths();
    updateImportPaths();

    Q_ASSERT(! g_instance);
    g_instance = this;
}

ModelManagerInterface::~ModelManagerInterface()
{
    m_cppQmlTypesUpdater.cancel();
    m_cppQmlTypesUpdater.waitForFinished();
    Q_ASSERT(g_instance == this);
    g_instance = 0;
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

QStringList ModelManagerInterface::globPatternsForLanguages(const QList<Dialect> languages)
{
    QHash<QString, Dialect> lMapping;
    if (instance())
        lMapping = instance()->languageForSuffix();
    else
        lMapping = defaultLanguageMapping();
    QStringList patterns;
    QHashIterator<QString,Dialect> i(lMapping);
    while (i.hasNext()) {
        i.next();
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

void ModelManagerInterface::addTaskInternal(QFuture<void> result, const QString &msg,
                                            const char *taskId) const
{
    Q_UNUSED(result);
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
    CppQmlTypesLoader::defaultLibraryObjects.unite(
                CppQmlTypesLoader::loadQmlTypes(qmlTypesFiles, &errors, &warnings));

    foreach (const QString &error, errors)
        writeMessageInternal(error);
    foreach (const QString &warning, warnings)
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
    if (!m_indexerEnabled)
        return;
    refreshSourceFiles(files, emitDocumentOnDiskChanged);
}

void ModelManagerInterface::cleanupFutures()
{
    if (m_futures.size() > 10) {
        QList<QFuture<void> > futures = m_futures;
        m_futures.clear();
        foreach (const QFuture<void> &future, futures) {
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
    cleanupFutures();
    m_futures.append(result);

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

    foreach (const QString &file, files) {
        m_validSnapshot.remove(file);
        m_newestSnapshot.remove(file);
    }
}

namespace {
bool pInfoLessThanActive(const ModelManagerInterface::ProjectInfo &p1, const ModelManagerInterface::ProjectInfo &p2)
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
        else if (s1.at(i) > s2.at(i))
            return false;
    }
    return false;
}

bool pInfoLessThanAll(const ModelManagerInterface::ProjectInfo &p1, const ModelManagerInterface::ProjectInfo &p2)
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
        else if (s1.at(i) > s2.at(i))
            return false;
    }
    return false;
}

bool pInfoLessThanImports(const ModelManagerInterface::ProjectInfo &p1, const ModelManagerInterface::ProjectInfo &p2)
{
    if (p1.qtQmlPath < p2.qtQmlPath)
        return true;
    if (p1.qtQmlPath > p2.qtQmlPath)
        return false;
    if (p1.qtImportsPath < p2.qtImportsPath)
        return true;
    if (p1.qtImportsPath > p2.qtImportsPath)
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
        else if (s2.at(i) < s1.at(i))
            return false;
    }
    return false;
}

}

void ModelManagerInterface::iterateQrcFiles(ProjectExplorer::Project *project,
                                            QrcResourceSelector resources,
                                            std::function<void(QrcParser::ConstPtr)> callback)
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
    foreach (const ModelManagerInterface::ProjectInfo &pInfo, pInfos) {
        QStringList qrcFilePaths;
        if (resources == ActiveQrcResources)
            qrcFilePaths = pInfo.activeResourceFiles;
        else
            qrcFilePaths = pInfo.allResourceFiles;
        foreach (const QString &qrcFilePath, qrcFilePaths) {
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
    iterateQrcFiles(project, resources, [&](QrcParser::ConstPtr qrcFile) {
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
    iterateQrcFiles(project, resources, [&](QrcParser::ConstPtr qrcFile) {
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
    iterateQrcFiles(project, resources, [&](QrcParser::ConstPtr qrcFile) {
        qrcFile->collectFilesInPath(normPath, &res, addDirs, locale);
    });
    return res;
}

QList<ModelManagerInterface::ProjectInfo> ModelManagerInterface::projectInfos() const
{
    QMutexLocker locker(&m_mutex);

    return m_projects.values();
}

ModelManagerInterface::ProjectInfo ModelManagerInterface::projectInfo(
        ProjectExplorer::Project *project,
        const ModelManagerInterface::ProjectInfo &defaultValue) const
{
    QMutexLocker locker(&m_mutex);

    return m_projects.value(project, defaultValue);
}

void ModelManagerInterface::updateProjectInfo(const ProjectInfo &pinfo, ProjectExplorer::Project *p)
{
    if (! pinfo.isValid() || !p || !m_indexerEnabled)
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
        m_pluginDumper->scheduleMaybeRedumpBuiltins(pinfo);
    }


    updateImportPaths();

    // remove files that are no longer in the project and have been deleted
    QStringList deletedFiles;
    foreach (const QString &oldFile, oldInfo.sourceFiles) {
        if (snapshot.document(oldFile)
                && !pinfo.sourceFiles.contains(oldFile)
                && !QFile::exists(oldFile)) {
            deletedFiles += oldFile;
        }
    }
    removeFiles(deletedFiles);
    foreach (const QString &oldFile, deletedFiles)
        m_fileToProject.remove(oldFile, p);

    // parse any files not yet in the snapshot
    QStringList newFiles;
    foreach (const QString &file, pinfo.sourceFiles) {
        if (!snapshot.document(file))
            newFiles += file;
    }
    foreach (const QString &newFile, newFiles)
        m_fileToProject.insert(newFile, p);
    updateSourceFiles(newFiles, false);

    // update qrc cache
    m_qrcContents = pinfo.resourceFileContents;
    foreach (const QString &newQrc, pinfo.allResourceFiles)
        m_qrcCache.addPath(newQrc, m_qrcContents.value(newQrc));
    foreach (const QString &oldQrc, oldInfo.allResourceFiles)
        m_qrcCache.removePath(oldQrc);

    int majorVersion, minorVersion, patchVersion;
    // dump builtin types if the shipped definitions are probably outdated and the
    // Qt version ships qmlplugindump
    if (::sscanf(pinfo.qtVersionString.toLatin1().constData(), "%d.%d.%d",
           &majorVersion, &minorVersion, &patchVersion) != 3)
        majorVersion = minorVersion = patchVersion = -1;

    if (majorVersion > 4 || (majorVersion == 4 && (minorVersion > 8 || (minorVersion == 8
                                                                       && patchVersion >= 5)))) {
        m_pluginDumper->loadBuiltinTypes(pinfo);
    }

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
ModelManagerInterface::ProjectInfo ModelManagerInterface::projectInfoForPath(const QString &path) const
{
    QList<ProjectInfo> infos = allProjectInfosForPath(path);

    ProjectInfo res;
    foreach (const ProjectInfo &pInfo, infos) {
        if (res.qtImportsPath.isEmpty())
            res.qtImportsPath = pInfo.qtImportsPath;
        if (res.qtQmlPath.isEmpty())
            res.qtQmlPath = pInfo.qtQmlPath;
        for (int i = 0; i < pInfo.importPaths.size(); ++i)
            res.importPaths.maybeInsert(pInfo.importPaths.at(i));
    }
    return res;
}

/*!
    Returns list of project infos for \a path
 */
QList<ModelManagerInterface::ProjectInfo> ModelManagerInterface::allProjectInfosForPath(const QString &path) const
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
    foreach (ProjectExplorer::Project *project, projects) {
        ProjectInfo info = projectInfo(project);
        if (info.isValid())
            infos.append(info);
    }
    std::sort(infos.begin(), infos.end(), &pInfoLessThanImports);
    infos.append(m_defaultProjectInfo);
    return infos;
}

bool ModelManagerInterface::isIdle() const
{
    return m_futures.isEmpty();
}

void ModelManagerInterface::emitDocumentChangedOnDisk(Document::Ptr doc)
{ emit documentChangedOnDisk(doc); }

void ModelManagerInterface::updateQrcFile(const QString &path)
{
    m_qrcCache.updatePath(path, m_qrcContents.value(path));
}

void ModelManagerInterface::updateDocument(Document::Ptr doc)
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

static QStringList filesInDirectoryForLanguages(const QString &path, QList<Dialect> languages)
{
    const QStringList pattern = ModelManagerInterface::globPatternsForLanguages(languages);
    QStringList files;

    const QDir dir(path);
    foreach (const QFileInfo &fi, dir.entryInfoList(pattern, QDir::Files))
        files += fi.absoluteFilePath();

    return files;
}

static void findNewImplicitImports(const Document::Ptr &doc, const Snapshot &snapshot,
                            QStringList *importedFiles, QSet<QString> *scannedPaths)
{
    // scan files that could be implicitly imported
    // it's important we also do this for JS files, otherwise the isEmpty check will fail
    if (snapshot.documentsInDirectory(doc->path()).isEmpty()) {
        if (! scannedPaths->contains(doc->path())) {
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
    foreach (const ImportInfo &import, doc->bind()->imports()) {
        const QString &importName = import.path();
        if (import.type() == ImportType::File) {
            if (! snapshot.document(importName))
                *importedFiles += importName;
        } else if (import.type() == ImportType::Directory) {
            if (snapshot.documentsInDirectory(importName).isEmpty()) {
                if (! scannedPaths->contains(importName)) {
                    *importedFiles += filesInDirectoryForLanguages(importName,
                                                          doc->language().companionLanguages());
                    scannedPaths->insert(importName);
                }
            }
        } else if (import.type() == ImportType::QrcFile) {
            QStringList importPaths = ModelManagerInterface::instance()->filesAtQrcPath(importName);
            foreach (const QString &importPath, importPaths) {
                if (! snapshot.document(importPath))
                    *importedFiles += importPath;
            }
        } else if (import.type() == ImportType::QrcDirectory) {
            QMapIterator<QString,QStringList> dirContents(ModelManagerInterface::instance()->filesInQrcPath(importName));
            while (dirContents.hasNext()) {
                dirContents.next();
                if (ModelManagerInterface::guessLanguageOfFile(dirContents.key()).isQmlLikeOrJsLanguage()) {
                    foreach (const QString &filePath, dirContents.value()) {
                        if (! snapshot.document(filePath))
                            *importedFiles += filePath;
                    }
                }
            }
        }
    }
}

static bool findNewQmlLibraryInPath(const QString &path,
                                    const Snapshot &snapshot,
                                    ModelManagerInterface *modelManager,
                                    QStringList *importedFiles,
                                    QSet<QString> *scannedPaths,
                                    QSet<QString> *newLibraries,
                                    bool ignoreMissing)
{
    // if we know there is a library, done
    const LibraryInfo &existingInfo = snapshot.libraryInfo(path);
    if (existingInfo.isValid())
        return true;
    if (newLibraries->contains(path))
        return true;
    // if we looked at the path before, done
    if (existingInfo.wasScanned())
        return false;

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
    foreach (const QmlDirParser::Component &component, qmldirParser.components()) {
        if (! component.fileName.isEmpty()) {
            const QFileInfo componentFileInfo(dir.filePath(component.fileName));
            const QString path = QDir::cleanPath(componentFileInfo.absolutePath());
            if (! scannedPaths->contains(path)) {
                *importedFiles += filesInDirectoryForLanguages(path,
                     Dialect(Dialect::AnyLanguage).companionLanguages());
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
                           ModelManagerInterface *modelManager,
                           QStringList *importedFiles, QSet<QString> *scannedPaths, QSet<QString> *newLibraries)
{
    // scan current dir
    findNewQmlLibraryInPath(doc->path(), snapshot, modelManager,
                            importedFiles, scannedPaths, newLibraries, false);

    // scan dir and lib imports
    const QStringList importPaths = modelManager->importPathsNames();
    foreach (const ImportInfo &import, doc->bind()->imports()) {
        if (import.type() == ImportType::Directory) {
            const QString targetPath = import.path();
            findNewQmlLibraryInPath(targetPath, snapshot, modelManager,
                                    importedFiles, scannedPaths, newLibraries, false);
        }

        if (import.type() == ImportType::Library) {
            const QString libraryPath = modulePath(import, importPaths);
            if (libraryPath.isEmpty())
                continue;
            findNewQmlLibraryInPath(libraryPath, snapshot, modelManager, importedFiles,
                                    scannedPaths, newLibraries, false);
        }
    }
}

void ModelManagerInterface::parseLoop(QSet<QString> &scannedPaths,
                             QSet<QString> &newLibraries,
                             WorkingCopy workingCopy,
                             QStringList files,
                             ModelManagerInterface *modelManager,
                             Dialect mainLanguage,
                             bool emitDocChangedOnDisk,
                             std::function<bool(qreal)> reportProgress)
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
                && (mainLanguage == Dialect::QmlQtQuick1 || mainLanguage == Dialect::QmlQtQuick2))
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
        findNewLibraryImports(doc, snapshot, modelManager, &importedFiles, &scannedPaths, &newLibraries);

        // add new files to parse list
        foreach (const QString &file, importedFiles) {
            if (! files.contains(file))
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
    FutureReporter(QFutureInterface<void> &future, int multiplier = 100, int base = 0)
        :future(future), multiplier(multiplier), base(base)
    { }
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
                         WorkingCopy workingCopy,
                         QStringList files,
                         ModelManagerInterface *modelManager,
                         Dialect mainLanguage,
                         bool emitDocChangedOnDisk)
{
    FutureReporter reporter(future);
    future.setProgressRange(0, 100);

    // paths we have scanned for files and added to the files list
    QSet<QString> scannedPaths;
    // libraries we've found while scanning imports
    QSet<QString> newLibraries;
    parseLoop(scannedPaths, newLibraries, workingCopy, files, modelManager, mainLanguage,
              emitDocChangedOnDisk, reporter);
    future.setProgressValue(100);
}

struct ScanItem {
    QString path;
    int depth;
    Dialect language;
    ScanItem(QString path = QString(), int depth = 0, Dialect language = Dialect::AnyLanguage)
        : path(path), depth(depth), language(language)
    { }
};

void ModelManagerInterface::importScan(QFutureInterface<void> &future,
                              ModelManagerInterface::WorkingCopy workingCopy,
                              PathsAndLanguages paths, ModelManagerInterface *modelManager,
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
        for (int i = 0; i < paths.size(); ++i) {
            PathAndLanguage pAndL = paths.at(i);
            QString cPath = QDir::cleanPath(pAndL.path().toString());
            if (!forceRescan && modelManager->m_scannedPaths.contains(cPath))
                continue;
            pathsToScan.append(ScanItem(cPath, 0, pAndL.language()));
            modelManager->m_scannedPaths.insert(cPath);
        }
    }
    const int maxScanDepth = 5;
    int progressRange = pathsToScan.size() * (1 << (2 + maxScanDepth));
    int totalWork(progressRange), workDone(0);
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
            foreach (const QString path, subDirs)
                pathsToScan.append(ScanItem(dir.absoluteFilePath(path), toScan.depth + 1, toScan.language));
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
        for (int i = 0; i < paths.size(); ++i)
            modelManager->m_scannedPaths.remove(paths.at(i).path().toString());
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
    if (!m_indexerEnabled)
        return;
    PathsAndLanguages pathToScan;
    {
        QMutexLocker l(&m_mutex);
        foreach (const PathAndLanguage &importPath, importPaths)
            if (!m_scannedPaths.contains(importPath.path().toString()))
                pathToScan.maybeInsert(importPath);
    }

    if (pathToScan.length() > 1) {
        QFuture<void> result = Utils::runAsync(&ModelManagerInterface::importScan,
                                               workingCopyInternal(), pathToScan,
                                               this, true, true, false);
        cleanupFutures();
        m_futures.append(result);

        addTaskInternal(result, tr("Scanning QML Imports"), Constants::TASK_IMPORT_SCAN);
    }
}

void ModelManagerInterface::updateImportPaths()
{
    if (!m_indexerEnabled)
        return;
    PathsAndLanguages allImportPaths;
    QmlLanguageBundles activeBundles;
    QmlLanguageBundles extendedBundles;
    QMapIterator<ProjectExplorer::Project *, ProjectInfo> pInfoIter(m_projects);
    QHashIterator<Dialect, QmlJS::ViewerContext> vCtxsIter = m_defaultVContexts;
    while (pInfoIter.hasNext()) {
        pInfoIter.next();
        const PathsAndLanguages &iPaths = pInfoIter.value().importPaths;
        for (int i = 0; i < iPaths.size(); ++i) {
            PathAndLanguage pAndL = iPaths.at(i);
            const QString canonicalPath = pAndL.path().toFileInfo().canonicalFilePath();
            if (!canonicalPath.isEmpty())
                allImportPaths.maybeInsert(Utils::FileName::fromString(canonicalPath),
                                           pAndL.language());
        }
    }
    while (vCtxsIter.hasNext()) {
        vCtxsIter.next();
        foreach (const QString &path, vCtxsIter.value().paths)
            allImportPaths.maybeInsert(Utils::FileName::fromString(path), vCtxsIter.value().language);
    }
    pInfoIter.toFront();
    while (pInfoIter.hasNext()) {
        pInfoIter.next();
        activeBundles.mergeLanguageBundles(pInfoIter.value().activeBundle);
        foreach (Dialect l, pInfoIter.value().activeBundle.languages()) {
            foreach (const QString &path, pInfoIter.value().activeBundle.bundleForLanguage(l)
                 .searchPaths().stringList()) {
                const QString canonicalPath = QFileInfo(path).canonicalFilePath();
                if (!canonicalPath.isEmpty())
                    allImportPaths.maybeInsert(Utils::FileName::fromString(canonicalPath), l);
            }
        }
    }
    pInfoIter.toFront();
    while (pInfoIter.hasNext()) {
        pInfoIter.next();
        QString pathAtt = pInfoIter.value().qtQmlPath;
        if (!pathAtt.isEmpty())
            allImportPaths.maybeInsert(Utils::FileName::fromString(pathAtt), Dialect::QmlQtQuick2);
    }
    {
        QString pathAtt = defaultProjectInfo().qtQmlPath;
        if (!pathAtt.isEmpty())
            allImportPaths.maybeInsert(Utils::FileName::fromString(pathAtt), Dialect::QmlQtQuick2);
    }
    pInfoIter.toFront();
    while (pInfoIter.hasNext()) {
        pInfoIter.next();
        QString pathAtt = pInfoIter.value().qtImportsPath;
        if (!pathAtt.isEmpty())
            allImportPaths.maybeInsert(Utils::FileName::fromString(pathAtt), Dialect::QmlQtQuick1);
    }
    {
        QString pathAtt = defaultProjectInfo().qtImportsPath;
        if (!pathAtt.isEmpty())
            allImportPaths.maybeInsert(Utils::FileName::fromString(pathAtt), Dialect::QmlQtQuick1);
    }
    foreach (const QString &path, m_defaultImportPaths)
        allImportPaths.maybeInsert(Utils::FileName::fromString(path), Dialect::Qml);
    allImportPaths.compact();

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
    foreach (const Document::Ptr &doc, snapshot)
        findNewLibraryImports(doc, snapshot, this, &importedFiles, &scannedPaths, &newLibraries);

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
                            || data.contextProperties != contextProperties))
            hasNewInfo = true;
        if (!hasNewInfo) {
            QHash<QString, QByteArray> newFingerprints;
            foreach (LanguageUtils::FakeMetaObject::ConstPtr newType, exported)
                newFingerprints[newType->className()]=newType->fingerprint();
            foreach (LanguageUtils::FakeMetaObject::ConstPtr oldType, data.exportedTypes) {
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

void ModelManagerInterface::updateCppQmlTypes(QFutureInterface<void> &futureInterface,
                                     ModelManagerInterface *qmlModelManager,
                                     CPlusPlus::Snapshot snapshot,
                                     QHash<QString, QPair<CPlusPlus::Document::Ptr, bool> > documents)
{
    futureInterface.setProgressRange(0, documents.size());
    futureInterface.setProgressValue(0);

    CppDataHash newData;
    QHash<QString, QList<CPlusPlus::Document::Ptr> > newDeclarations;
    {
        QMutexLocker locker(&qmlModelManager->m_cppDataMutex);
        newData = qmlModelManager->m_cppDataHash;
        newDeclarations = qmlModelManager->m_cppDeclarationFiles;
    }

    FindExportedCppTypes finder(snapshot);

    bool hasNewInfo = false;
    typedef QPair<CPlusPlus::Document::Ptr, bool> DocScanPair;
    foreach (const DocScanPair &pair, documents) {
        if (futureInterface.isCanceled())
            return;
        futureInterface.setProgressValue(futureInterface.progressValue() + 1);

        CPlusPlus::Document::Ptr doc = pair.first;
        const bool scan = pair.second;
        const QString fileName = doc->fileName();
        if (!scan) {
            hasNewInfo = newData.remove(fileName) > 0 || hasNewInfo;
            foreach (const CPlusPlus::Document::Ptr &savedDoc, newDeclarations.value(fileName)) {
                finder(savedDoc);
                hasNewInfo = rescanExports(savedDoc->fileName(), finder, newData) || hasNewInfo;
            }
            continue;
        }

        for (auto it = newDeclarations.begin(), end = newDeclarations.end(); it != end;) {
            for (auto docIt = it->begin(), endDocIt = it->end(); docIt != endDocIt;) {
                CPlusPlus::Document::Ptr &savedDoc = *docIt;
                if (savedDoc->fileName() == fileName) {
                    savedDoc->releaseSourceAndAST();
                    it->erase(docIt);
                    break;
                } else {
                    ++docIt;
                }
            }
            if (it->isEmpty())
                it = newDeclarations.erase(it);
            else
                ++it;
        }

        foreach (const QString &declarationFile, finder(doc)) {
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
    ProjectInfo info = projectInfoForPath(doc->fileName());
    if (!info.isValid())
        return LibraryInfo();
    if (!info.qtQmlPath.isEmpty())
        return m_validSnapshot.libraryInfo(info.qtQmlPath);
    return m_validSnapshot.libraryInfo(info.qtImportsPath);
}

ViewerContext ModelManagerInterface::completeVContext(const ViewerContext &vCtx,
                                                      const Document::Ptr &doc) const
{
    ViewerContext res = vCtx;

    if (!doc.isNull()
            && ((vCtx.language == Dialect::AnyLanguage && doc->language() != Dialect::NoLanguage)
                || (vCtx.language == Dialect::Qml
                    && (doc->language() == Dialect::QmlQtQuick1
                        || doc->language() == Dialect::QmlQtQuick2
                        || doc->language() == Dialect::QmlQtQuick2Ui))))
        res.language = doc->language();
    ProjectInfo info;
    if (!doc.isNull())
        info = projectInfoForPath(doc->fileName());
    ViewerContext defaultVCtx = defaultVContext(res.language, Document::Ptr(0), false);
    ProjectInfo defaultInfo = defaultProjectInfo();
    if (info.qtImportsPath.isEmpty())
        info.qtImportsPath = defaultInfo.qtImportsPath;
    if (info.qtQmlPath.isEmpty())
        info.qtQmlPath = defaultInfo.qtQmlPath;
    switch (res.flags) {
    case ViewerContext::Complete:
        break;
    case ViewerContext::AddAllPathsAndDefaultSelectors:
        res.selectors.append(defaultVCtx.selectors);
        // fallthrough
    case ViewerContext::AddAllPaths:
    {
        foreach (const QString &path, defaultVCtx.paths)
            res.maybeAddPath(path);
        switch (res.language.dialect()) {
        case Dialect::AnyLanguage:
        case Dialect::Qml:
            res.maybeAddPath(info.qtQmlPath);
            // fallthrough
        case Dialect::QmlQtQuick1:
            res.maybeAddPath(info.qtImportsPath);
            // fallthrough
        case Dialect::QmlQtQuick2:
        case Dialect::QmlQtQuick2Ui:
        {
            if (res.language == Dialect::QmlQtQuick2 || res.language == Dialect::QmlQtQuick2Ui)
                res.maybeAddPath(info.qtQmlPath);
            QList<ProjectInfo> allProjects;
            {
                QMutexLocker locker(&m_mutex);
                allProjects = m_projects.values();
            }
            std::sort(allProjects.begin(), allProjects.end(), &pInfoLessThanImports);
            QList<Dialect> languages = res.language.companionLanguages();
            foreach (const ProjectInfo &pInfo, allProjects) {
                for (int i = 0; i< pInfo.importPaths.size(); ++i) {
                    PathAndLanguage pAndL = pInfo.importPaths.at(i);
                    if (languages.contains(pAndL.language()) || pAndL.language().companionLanguages().contains(res.language))
                        res.maybeAddPath(pAndL.path().toString());
                }
            }
            foreach (const QString &path, environmentImportPaths())
                res.maybeAddPath(path);
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
        // fallthrough
    case ViewerContext::AddDefaultPaths:
        foreach (const QString &path, defaultVCtx.paths)
            res.maybeAddPath(path);
        if (res.language == Dialect::AnyLanguage || res.language == Dialect::Qml
                || res.language == Dialect::QmlQtQuick2 || res.language == Dialect::QmlQtQuick2Ui)
            res.maybeAddPath(info.qtImportsPath);
        if (res.language == Dialect::AnyLanguage || res.language == Dialect::Qml
                || res.language == Dialect::QmlQtQuick1)
            res.maybeAddPath(info.qtQmlPath);
        if (res.language == Dialect::AnyLanguage || res.language == Dialect::Qml
                || res.language == Dialect::QmlQtQuick1 || res.language == Dialect::QmlQtQuick2
                || res.language == Dialect::QmlQtQuick2Ui) {
            foreach (const QString &path, environmentImportPaths())
                res.maybeAddPath(path);
        }
        break;
    }
    res.flags = ViewerContext::Complete;
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
                 (doc->language() == Dialect::QmlQtQuick1 || doc->language() == Dialect::QmlQtQuick2
                  || doc->language() == Dialect::QmlQtQuick2Ui))
            language = doc->language();
    }
    ViewerContext defaultCtx;
    {
        QMutexLocker locker(&m_mutex);
        defaultCtx = m_defaultVContexts.value(language);
    }
    defaultCtx.language = language;
    if (autoComplete)
        return completeVContext(defaultCtx, doc);
    else
        return defaultCtx;
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

void ModelManagerInterface::joinAllThreads()
{
    foreach (QFuture<void> future, m_futures)
        future.waitForFinished();
    m_futures.clear();
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
        foreach (Document::Ptr doc, m_validSnapshot)
            documents.append(doc->fileName());

        // reset the snapshot
        m_validSnapshot = Snapshot();
        m_newestSnapshot = Snapshot();
    }

    // start a reparse thread
    updateSourceFiles(documents, false);
}

} // namespace QmlJS
