/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmljsmodelmanager.h"
#include "qmljstoolsconstants.h"
#include "qmljsplugindumper.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/mimedatabase.h>
#include <cplusplus/ModelManagerInterface.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/TypeOfExpression.h>
#include <cplusplus/Overview.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/parser/qmldirparser_p.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLibraryInfo>
#include <QtCore/QtConcurrentRun>
#include <qtconcurrent/runextensions.h>
#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>

#include <QtCore/QDebug>

using namespace QmlJS;
using namespace QmlJSTools;
using namespace QmlJSTools::Internal;

static QStringList environmentImportPaths();

ModelManager::ModelManager(QObject *parent):
        ModelManagerInterface(parent),
        m_core(Core::ICore::instance()),
        m_pluginDumper(new PluginDumper(this))
{
    m_synchronizer.setCancelOnWait(true);

    m_updateCppQmlTypesTimer = new QTimer(this);
    m_updateCppQmlTypesTimer->setInterval(1000);
    m_updateCppQmlTypesTimer->setSingleShot(true);
    connect(m_updateCppQmlTypesTimer, SIGNAL(timeout()), SLOT(startCppQmlTypeUpdate()));

    qRegisterMetaType<QmlJS::Document::Ptr>("QmlJS::Document::Ptr");
    qRegisterMetaType<QmlJS::LibraryInfo>("QmlJS::LibraryInfo");

    loadQmlTypeDescriptions();

    m_defaultImportPaths << environmentImportPaths();
    updateImportPaths();
}

void ModelManager::delayedInitialization()
{
    CPlusPlus::CppModelManagerInterface *cppModelManager =
            CPlusPlus::CppModelManagerInterface::instance();
    if (cppModelManager) {
        connect(cppModelManager, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
                this, SLOT(queueCppQmlTypeUpdate(CPlusPlus::Document::Ptr)));
    }
}

void ModelManager::loadQmlTypeDescriptions()
{
    if (Core::ICore::instance()) {
        loadQmlTypeDescriptions(Core::ICore::instance()->resourcePath());
        loadQmlTypeDescriptions(Core::ICore::instance()->userResourcePath());
    }
}

void ModelManager::loadQmlTypeDescriptions(const QString &resourcePath)
{
    const QDir typeFileDir(resourcePath + QLatin1String("/qml-type-descriptions"));
    const QStringList xmlExtensions = QStringList() << QLatin1String("*.xml");
    const QFileInfoList xmlFiles = typeFileDir.entryInfoList(xmlExtensions,
                                                             QDir::Files,
                                                             QDir::Name);

    const QStringList errors = Interpreter::CppQmlTypesLoader::load(xmlFiles);
    foreach (const QString &error, errors)
        qWarning() << qPrintable(error);

    // disabled for now: Prefer the xml file until the type dumping functionality
    // has been moved into Qt.
    // loads the builtin types
    //loadQmlPluginTypes(QString());
}

ModelManagerInterface::WorkingCopy ModelManager::workingCopy() const
{
    WorkingCopy workingCopy;
    if (!m_core)
        return workingCopy;

    Core::EditorManager *editorManager = m_core->editorManager();

    foreach (Core::IEditor *editor, editorManager->openedEditors()) {
        const QString key = editor->file()->fileName();

        if (TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor*>(editor)) {
            if (textEditor->context().contains(ProjectExplorer::Constants::LANG_QMLJS)) {
                if (TextEditor::BaseTextEditor *ed = qobject_cast<TextEditor::BaseTextEditor *>(textEditor->widget())) {
                    workingCopy.insert(key, ed->toPlainText(), ed->document()->revision());
                }
            }
        }
    }

    return workingCopy;
}

Snapshot ModelManager::snapshot() const
{
    QMutexLocker locker(&m_mutex);

    return _snapshot;
}

void ModelManager::updateSourceFiles(const QStringList &files,
                                     bool emitDocumentOnDiskChanged)
{
    refreshSourceFiles(files, emitDocumentOnDiskChanged);
}

QFuture<void> ModelManager::refreshSourceFiles(const QStringList &sourceFiles,
                                               bool emitDocumentOnDiskChanged)
{
    if (sourceFiles.isEmpty()) {
        return QFuture<void>();
    }

    QFuture<void> result = QtConcurrent::run(&ModelManager::parse,
                                              workingCopy(), sourceFiles,
                                              this,
                                              emitDocumentOnDiskChanged);

    if (m_synchronizer.futures().size() > 10) {
        QList<QFuture<void> > futures = m_synchronizer.futures();

        m_synchronizer.clearFutures();

        foreach (QFuture<void> future, futures) {
            if (! (future.isFinished() || future.isCanceled()))
                m_synchronizer.addFuture(future);
        }
    }

    m_synchronizer.addFuture(result);

    if (sourceFiles.count() > 1) {
        m_core->progressManager()->addTask(result, tr("Indexing"),
                        Constants::TASK_INDEX);
    }

    return result;
}

void ModelManager::fileChangedOnDisk(const QString &path)
{
    QtConcurrent::run(&ModelManager::parse,
                      workingCopy(), QStringList() << path,
                      this, true);
}

void ModelManager::removeFiles(const QStringList &files)
{
    emit aboutToRemoveFiles(files);

    QMutexLocker locker(&m_mutex);

    foreach (const QString &file, files)
        _snapshot.remove(file);
}

QList<ModelManager::ProjectInfo> ModelManager::projectInfos() const
{
    QMutexLocker locker(&m_mutex);

    return m_projects.values();
}

ModelManager::ProjectInfo ModelManager::projectInfo(ProjectExplorer::Project *project) const
{
    QMutexLocker locker(&m_mutex);

    return m_projects.value(project, ProjectInfo(project));
}

void ModelManager::updateProjectInfo(const ProjectInfo &pinfo)
{
    if (! pinfo.isValid())
        return;

    Snapshot snapshot;
    ProjectInfo oldInfo;
    {
        QMutexLocker locker(&m_mutex);
        oldInfo = m_projects.value(pinfo.project);
        m_projects.insert(pinfo.project, pinfo);
        snapshot = _snapshot;

        if (oldInfo.qmlDumpPath != pinfo.qmlDumpPath)
            m_pluginDumper->scheduleCompleteRedump();
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

    // parse any files not yet in the snapshot
    QStringList newFiles;
    foreach (const QString &file, pinfo.sourceFiles) {
        if (!snapshot.document(file))
            newFiles += file;
    }
    updateSourceFiles(newFiles, false);
}

void ModelManager::emitDocumentChangedOnDisk(Document::Ptr doc)
{ emit documentChangedOnDisk(doc); }

void ModelManager::updateDocument(Document::Ptr doc)
{
    {
        QMutexLocker locker(&m_mutex);
        _snapshot.insert(doc);
    }
    emit documentUpdated(doc);
}

void ModelManager::updateLibraryInfo(const QString &path, const LibraryInfo &info)
{
    {
        QMutexLocker locker(&m_mutex);
        _snapshot.insertLibraryInfo(path, info);
    }
    emit libraryInfoUpdated(path, info);
}

static QStringList qmlFilesInDirectory(const QString &path)
{
    QStringList pattern;
    if (Core::ICore::instance()) {
        // ### It would suffice to build pattern once. This function needs to be thread-safe.
        Core::MimeDatabase *db = Core::ICore::instance()->mimeDatabase();
        Core::MimeType jsSourceTy = db->findByType(Constants::JS_MIMETYPE);
        Core::MimeType qmlSourceTy = db->findByType(Constants::QML_MIMETYPE);

        QStringList pattern;
        foreach (const Core::MimeGlobPattern &glob, jsSourceTy.globPatterns())
            pattern << glob.regExp().pattern();
        foreach (const Core::MimeGlobPattern &glob, qmlSourceTy.globPatterns())
            pattern << glob.regExp().pattern();
    } else {
        pattern << "*.qml" << "*.js";
    }

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
            *importedFiles += qmlFilesInDirectory(doc->path());
            scannedPaths->insert(doc->path());
        }
    }
}

static void findNewFileImports(const Document::Ptr &doc, const Snapshot &snapshot,
                        QStringList *importedFiles, QSet<QString> *scannedPaths)
{
    // scan files and directories that are explicitly imported
    foreach (const Interpreter::ImportInfo &import, doc->bind()->imports()) {
        const QString &importName = import.name();
        if (import.type() == Interpreter::ImportInfo::FileImport) {
            if (! snapshot.document(importName))
                *importedFiles += importName;
        } else if (import.type() == Interpreter::ImportInfo::DirectoryImport) {
            if (snapshot.documentsInDirectory(importName).isEmpty()) {
                if (! scannedPaths->contains(importName)) {
                    *importedFiles += qmlFilesInDirectory(importName);
                    scannedPaths->insert(importName);
                }
            }
        }
    }
}

static void findNewLibraryImports(const Document::Ptr &doc, const Snapshot &snapshot,
                           ModelManager *modelManager,
                           QStringList *importedFiles, QSet<QString> *scannedPaths, QSet<QString> *newLibraries)
{
    // scan library imports
    const QStringList importPaths = modelManager->importPaths();
    foreach (const Interpreter::ImportInfo &import, doc->bind()->imports()) {
        if (import.type() != Interpreter::ImportInfo::LibraryImport)
            continue;
        foreach (const QString &importPath, importPaths) {
            const QString targetPath = QDir(importPath).filePath(import.name());

            // if we know there is a library, done
            if (snapshot.libraryInfo(targetPath).isValid())
                break;
            if (newLibraries->contains(targetPath))
                break;

            // check for a qmldir file
            const QDir targetDir(targetPath);
            QFile qmldirFile(targetDir.filePath(QLatin1String("qmldir")));
            if (!qmldirFile.exists())
                continue;

#ifdef Q_OS_WIN
            // QTCREATORBUG-3402 - be case sensitive even here?
#endif

            // found a new library!
            qmldirFile.open(QFile::ReadOnly);
            QString qmldirData = QString::fromUtf8(qmldirFile.readAll());

            QmlDirParser qmldirParser;
            qmldirParser.setSource(qmldirData);
            qmldirParser.parse();

            const QString libraryPath = QFileInfo(qmldirFile).absolutePath();
            newLibraries->insert(libraryPath);
            modelManager->updateLibraryInfo(libraryPath,
                                            LibraryInfo(qmldirParser));

            // scan the qml files in the library
            foreach (const QmlDirParser::Component &component, qmldirParser.components()) {
                if (! component.fileName.isEmpty()) {
                    const QFileInfo componentFileInfo(targetDir.filePath(component.fileName));
                    const QString path = QDir::cleanPath(componentFileInfo.absolutePath());
                    if (! scannedPaths->contains(path)) {
                        *importedFiles += qmlFilesInDirectory(path);
                        scannedPaths->insert(path);
                    }
                }
            }
        }
    }
}

static bool suffixMatches(const QString &fileName, const Core::MimeType &mimeType)
{
    foreach (const QString &suffix, mimeType.suffixes()) {
        if (fileName.endsWith(suffix, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

void ModelManager::parse(QFutureInterface<void> &future,
                            WorkingCopy workingCopy,
                            QStringList files,
                            ModelManager *modelManager,
                            bool emitDocChangedOnDisk)
{
    Core::MimeDatabase *db = 0;
    Core::MimeType jsSourceTy;
    Core::MimeType qmlSourceTy;
    if (Core::ICore::instance()) {
        db = Core::ICore::instance()->mimeDatabase();
        jsSourceTy = db->findByType(QLatin1String("application/javascript"));
        qmlSourceTy = db->findByType(QLatin1String("application/x-qml"));
    }

    int progressRange = files.size();
    future.setProgressRange(0, progressRange);

    Snapshot snapshot = modelManager->_snapshot;

    // paths we have scanned for files and added to the files list
    QSet<QString> scannedPaths;
    // libraries we've found while scanning imports
    QSet<QString> newLibraries;

    for (int i = 0; i < files.size(); ++i) {
        future.setProgressValue(qreal(i) / files.size() * progressRange);

        const QString fileName = files.at(i);

        bool isQmlFile = true;
        if (db) {
            if (suffixMatches(fileName, jsSourceTy)) {
                isQmlFile = false;
            } else if (! suffixMatches(fileName, qmlSourceTy)) {
                continue; // skip it. it's not a QML or a JS file.
            }
        } else {
            if (fileName.contains(QLatin1String(".js"), Qt::CaseInsensitive))
                isQmlFile = false;
            else if (!fileName.contains(QLatin1String(".qml"), Qt::CaseInsensitive))
                continue;
        }

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

        Document::Ptr doc = Document::create(fileName);
        doc->setEditorRevision(documentRevision);
        doc->setSource(contents);
        doc->parse();

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

    future.setProgressValue(progressRange);
}

// Check whether fileMimeType is the same or extends knownMimeType
bool ModelManager::matchesMimeType(const Core::MimeType &fileMimeType, const Core::MimeType &knownMimeType)
{
    Core::MimeDatabase *db = Core::ICore::instance()->mimeDatabase();

    const QStringList knownTypeNames = QStringList(knownMimeType.type()) + knownMimeType.aliases();

    foreach (const QString knownTypeName, knownTypeNames)
        if (fileMimeType.matchesType(knownTypeName))
            return true;

    // recursion to parent types of fileMimeType
    foreach (const QString &parentMimeType, fileMimeType.subClassesOf()) {
        if (matchesMimeType(db->findByType(parentMimeType), knownMimeType))
            return true;
    }

    return false;
}

QStringList ModelManager::importPaths() const
{
    return m_allImportPaths;
}

static QStringList environmentImportPaths()
{
    QStringList paths;

    QByteArray envImportPath = qgetenv("QML_IMPORT_PATH");

#if defined(Q_OS_WIN)
    QLatin1Char pathSep(';');
#else
    QLatin1Char pathSep(':');
#endif
    foreach (const QString &path, QString::fromLatin1(envImportPath).split(pathSep, QString::SkipEmptyParts)) {
        QString canonicalPath = QDir(path).canonicalPath();
        if (!canonicalPath.isEmpty() && !paths.contains(canonicalPath))
            paths.append(canonicalPath);
    }

    return paths;
}

void ModelManager::updateImportPaths()
{
    m_allImportPaths.clear();
    QMapIterator<ProjectExplorer::Project *, ProjectInfo> it(m_projects);
    while (it.hasNext()) {
        it.next();
        foreach (const QString &path, it.value().importPaths) {
            const QString canonicalPath = QFileInfo(path).canonicalFilePath();
            if (!canonicalPath.isEmpty())
                m_allImportPaths += canonicalPath;
        }
    }
    m_allImportPaths += m_defaultImportPaths;
    m_allImportPaths.removeDuplicates();

    // check if any file in the snapshot imports something new in the new paths
    Snapshot snapshot = _snapshot;
    QStringList importedFiles;
    QSet<QString> scannedPaths;
    QSet<QString> newLibraries;
    foreach (const Document::Ptr &doc, snapshot)
        findNewLibraryImports(doc, snapshot, this, &importedFiles, &scannedPaths, &newLibraries);

    updateSourceFiles(importedFiles, true);
}

void ModelManager::loadPluginTypes(const QString &libraryPath, const QString &importPath, const QString &importUri)
{
    m_pluginDumper->loadPluginTypes(libraryPath, importPath, importUri);
}

void ModelManager::queueCppQmlTypeUpdate(const CPlusPlus::Document::Ptr &doc)
{
    m_queuedCppDocuments.insert(doc->fileName());
    m_updateCppQmlTypesTimer->start();
}

void ModelManager::startCppQmlTypeUpdate()
{
    CPlusPlus::CppModelManagerInterface *cppModelManager =
            CPlusPlus::CppModelManagerInterface::instance();
    if (!cppModelManager)
        return;

    QtConcurrent::run(&ModelManager::updateCppQmlTypes,
                      this, cppModelManager, m_queuedCppDocuments);
    m_queuedCppDocuments.clear();
}

void ModelManager::updateCppQmlTypes(ModelManager *qmlModelManager, CPlusPlus::CppModelManagerInterface *cppModelManager, QSet<QString> files)
{
    CppQmlTypeHash newCppTypes = qmlModelManager->cppQmlTypes();
    CPlusPlus::Snapshot snapshot = cppModelManager->snapshot();

    foreach (const QString &fileName, files) {
        CPlusPlus::Document::Ptr doc = snapshot.document(fileName);
        QList<LanguageUtils::FakeMetaObject::ConstPtr> exported;
        if (doc)
            exported = cppModelManager->exportedQmlObjects(doc);
        if (!exported.isEmpty())
            newCppTypes[fileName] = exported;
        else
            newCppTypes.remove(fileName);
    }

    QMutexLocker locker(&qmlModelManager->m_cppTypesMutex);
    qmlModelManager->m_cppTypes = newCppTypes;
}

ModelManagerInterface::CppQmlTypeHash ModelManager::cppQmlTypes() const
{
    QMutexLocker locker(&m_cppTypesMutex);
    return m_cppTypes;
}
