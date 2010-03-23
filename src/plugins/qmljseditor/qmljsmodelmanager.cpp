/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmljseditorconstants.h"
#include "qmljsmodelmanager.h"
#include "qmljseditor.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/mimedatabase.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/parser/qmldirparser_p.h>
#include <texteditor/itexteditor.h>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QtConcurrentRun>
#include <qtconcurrent/runextensions.h>
#include <QTextStream>

#include <QDebug>

using namespace QmlJS;
using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;

static QStringList environmentImportPaths();

ModelManager::ModelManager(QObject *parent):
        ModelManagerInterface(parent),
        m_core(Core::ICore::instance())
{
    m_synchronizer.setCancelOnWait(true);

    qRegisterMetaType<QmlJS::Document::Ptr>("QmlJS::Document::Ptr");
    qRegisterMetaType<QmlJS::LibraryInfo>("QmlJS::LibraryInfo");

    connect(this, SIGNAL(documentUpdated(QmlJS::Document::Ptr)),
            this, SLOT(onDocumentUpdated(QmlJS::Document::Ptr)));
    connect(this, SIGNAL(libraryInfoUpdated(QString,QmlJS::LibraryInfo)),
            this, SLOT(onLibraryInfoUpdated(QString,QmlJS::LibraryInfo)));

    loadQmlTypeDescriptions();

    m_defaultImportPaths << environmentImportPaths();
    m_defaultImportPaths << QLibraryInfo::location(QLibraryInfo::ImportsPath);
    refreshSourceDirectories(m_defaultImportPaths);
}

void ModelManager::loadQmlTypeDescriptions()
{
    const QString resourcePath = Core::ICore::instance()->resourcePath();
    const QDir typeFileDir(resourcePath + QLatin1String("/qml-type-descriptions"));
    const QStringList xmlExtensions = QStringList() << QLatin1String("*.xml");
    const QFileInfoList xmlFiles = typeFileDir.entryInfoList(xmlExtensions,
                                                             QDir::Files,
                                                             QDir::Name);

    const QStringList errors = Interpreter::MetaTypeSystem::load(xmlFiles);
    foreach (const QString &error, errors)
        qWarning() << qPrintable(error);
}

Snapshot ModelManager::snapshot() const
{
    QMutexLocker locker(&m_mutex);

    return _snapshot;
}

void ModelManager::updateSourceFiles(const QStringList &files)
{
    refreshSourceFiles(files);
}

QFuture<void> ModelManager::refreshSourceFiles(const QStringList &sourceFiles)
{
    if (sourceFiles.isEmpty()) {
        return QFuture<void>();
    }

    const QMap<QString, WorkingCopy> workingCopy = buildWorkingCopyList();

    QFuture<void> result = QtConcurrent::run(&ModelManager::parse,
                                              workingCopy, sourceFiles,
                                              this);

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
                        QmlJSEditor::Constants::TASK_INDEX);
    }

    return result;
}

void ModelManager::updateSourceDirectories(const QStringList &directories)
{
    refreshSourceDirectories(directories);
}

QFuture<void> ModelManager::refreshSourceDirectories(const QStringList &sourceDirectories)
{
    if (sourceDirectories.isEmpty()) {
        return QFuture<void>();
    }

    const QMap<QString, WorkingCopy> workingCopy = buildWorkingCopyList();

    QFuture<void> result = QtConcurrent::run(&ModelManager::parseDirectories,
                                              workingCopy, sourceDirectories,
                                              this);

    if (m_synchronizer.futures().size() > 10) {
        QList<QFuture<void> > futures = m_synchronizer.futures();

        m_synchronizer.clearFutures();

        foreach (QFuture<void> future, futures) {
            if (! (future.isFinished() || future.isCanceled()))
                m_synchronizer.addFuture(future);
        }
    }

    m_synchronizer.addFuture(result);

    m_core->progressManager()->addTask(result, tr("Indexing"),
                                       QmlJSEditor::Constants::TASK_INDEX);

    return result;
}

QMap<QString, ModelManager::WorkingCopy> ModelManager::buildWorkingCopyList()
{
    QMap<QString, WorkingCopy> workingCopy;
    Core::EditorManager *editorManager = m_core->editorManager();

    foreach (Core::IEditor *editor, editorManager->openedEditors()) {
        const QString key = editor->file()->fileName();

        if (TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor*>(editor)) {
            if (QmlJSTextEditor *ed = qobject_cast<QmlJSTextEditor *>(textEditor->widget())) {
                workingCopy[key].contents = ed->toPlainText();
                workingCopy[key].documentRevision = ed->document()->revision();
            }
        }
    }

    return workingCopy;
}

void ModelManager::emitDocumentUpdated(Document::Ptr doc)
{ emit documentUpdated(doc); }

void ModelManager::onDocumentUpdated(Document::Ptr doc)
{
    QMutexLocker locker(&m_mutex);

    _snapshot.insert(doc);
}

void ModelManager::emitLibraryInfoUpdated(const QString &path, const LibraryInfo &info)
{ emit libraryInfoUpdated(path, info); }

void ModelManager::onLibraryInfoUpdated(const QString &path, const LibraryInfo &info)
{
    QMutexLocker locker(&m_mutex);

    _snapshot.insertLibraryInfo(path, info);
}

void ModelManager::parse(QFutureInterface<void> &future,
                            QMap<QString, WorkingCopy> workingCopy,
                            QStringList files,
                            ModelManager *modelManager)
{
    Core::MimeDatabase *db = Core::ICore::instance()->mimeDatabase();
    Core::MimeType jsSourceTy = db->findByType(QLatin1String("application/javascript"));
    Core::MimeType qmlSourceTy = db->findByType(QLatin1String("application/x-qml"));

    future.setProgressRange(0, files.size());

    for (int i = 0; i < files.size(); ++i) {
        future.setProgressValue(i);

        const QString fileName = files.at(i);

        const QFileInfo fileInfo(fileName);
        Core::MimeType fileMimeTy = db->findByFile(fileInfo);

        bool isQmlFile = true;

        if (matchesMimeType(fileMimeTy, jsSourceTy))
            isQmlFile = false;

        else if (! matchesMimeType(fileMimeTy, qmlSourceTy))
            continue; // skip it. it's not a QML or a JS file.

        QString contents;
        int documentRevision = 0;

        if (workingCopy.contains(fileName)) {
            WorkingCopy wc = workingCopy.value(fileName);
            contents = wc.contents;
            documentRevision = wc.documentRevision;
        } else {
            QFile inFile(fileName);

            if (inFile.open(QIODevice::ReadOnly)) {
                QTextStream ins(&inFile);
                contents = ins.readAll();
                inFile.close();
            }
        }

        Document::Ptr doc = Document::create(fileName);
        doc->setDocumentRevision(documentRevision);
        doc->setSource(contents);

        if (isQmlFile)
            doc->parseQml();
        else
            doc->parseJavaScript();

        modelManager->emitDocumentUpdated(doc);
    }

    future.setProgressValue(files.size());
}

void ModelManager::parseDirectories(QFutureInterface<void> &future,
                                    QMap<QString, WorkingCopy> workingCopy,
                                    QStringList directories,
                                    ModelManager *modelManager)
{
    Core::MimeDatabase *db = Core::ICore::instance()->mimeDatabase();
    Core::MimeType jsSourceTy = db->findByType(QLatin1String("application/javascript"));
    Core::MimeType qmlSourceTy = db->findByType(QLatin1String("application/x-qml"));

    QStringList pattern;
    foreach (const QRegExp &glob, jsSourceTy.globPatterns())
        pattern << glob.pattern();
    foreach (const QRegExp &glob, qmlSourceTy.globPatterns())
        pattern << glob.pattern();
    pattern << QLatin1String("qmldir");

    QStringList importedFiles;
    QStringList qmldirFiles;
    foreach (const QString &path, directories) {
        QDirIterator fileIterator(path, pattern, QDir::Files,
                                  QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
        while (fileIterator.hasNext()) {
            fileIterator.next();
            if (fileIterator.fileName() == QLatin1String("qmldir"))
                qmldirFiles << fileIterator.filePath();
            else
                importedFiles << fileIterator.filePath();
        }
    }

    foreach (const QString &qmldirFilePath, qmldirFiles) {
        QFile qmldirFile(qmldirFilePath);
        qmldirFile.open(QFile::ReadOnly);
        QString qmldirData = QString::fromUtf8(qmldirFile.readAll());

        QmlDirParser qmldirParser;
        qmldirParser.setSource(qmldirData);
        qmldirParser.parse();

        modelManager->emitLibraryInfoUpdated(QFileInfo(qmldirFilePath).path(),
                                             LibraryInfo(qmldirParser));
    }

    parse(future, workingCopy, importedFiles, modelManager);
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

void ModelManager::setProjectImportPaths(const QStringList &importPaths)
{
    m_projectImportPaths = importPaths;

    refreshSourceDirectories(importPaths);
}

QStringList ModelManager::importPaths() const
{
    QStringList paths;
    paths << m_projectImportPaths;
    paths << m_defaultImportPaths;
    return paths;
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
