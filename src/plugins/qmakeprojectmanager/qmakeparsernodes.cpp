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

#include "qmakeparsernodes.h"

#include "qmakeproject.h"
#include "qmakeprojectmanagerconstants.h"
#include "qmakebuildconfiguration.h"
#include "qmakerunconfigurationfactory.h"

#include <coreplugin/dialogs/readonlyfilesdialog.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <cpptools/cpptoolsconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qtsupport/profilereader.h>

#include <utils/algorithm.h>
#include <utils/qtcprocess.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/stringutils.h>
#include <utils/QtConcurrentTools>

#include <QLoggingCategory>
#include <QMessageBox>
#include <QTextCodec>

using namespace Core;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace QmakeProjectManager::Internal;
using namespace QMakeInternal;
using namespace Utils;

namespace {

class QmakePriFileDocument : public Core::IDocument
{
public:
    QmakePriFileDocument(QmakePriFile *qmakePriFile, const Utils::FileName &filePath) :
        IDocument(nullptr), m_priFile(qmakePriFile)
    {
        setId("Qmake.PriFile");
        setMimeType(QLatin1String(QmakeProjectManager::Constants::PROFILE_MIMETYPE));
        setFilePath(filePath);
    }

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const override
    {
        Q_UNUSED(state)
        Q_UNUSED(type)
        return BehaviorSilent;
    }
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override
    {
        Q_UNUSED(errorString)
        Q_UNUSED(flag)
        if (type == TypePermissions)
            return true;
        m_priFile->scheduleUpdate();
        return true;
    }

private:
    QmakePriFile *m_priFile;
};

} // namespace

namespace QmakeProjectManager {

Q_LOGGING_CATEGORY(qmakeParse, "qtc.qmake.parsing");

uint qHash(Variable key, uint seed) { return ::qHash(static_cast<int>(key), seed); }

namespace Internal {

class QmakeEvalInput
{
public:
    QString projectDir;
    FileName projectFilePath;
    FileName buildDirectory;
    FileName sysroot;
    QtSupport::ProFileReader *readerExact;
    QtSupport::ProFileReader *readerCumulative;
    QMakeGlobals *qmakeGlobals;
    QMakeVfs *qmakeVfs;
};

class QmakePriFileEvalResult
{
public:
    QSet<FileName> folders;
    QSet<FileName> recursiveEnumerateFiles;
    QMap<FileType, QSet<FileName>> foundFiles;
};

class QmakeIncludedPriFile
{
public:
    ProFile *proFile;
    Utils::FileName name;
    QmakePriFileEvalResult result;
    QMap<Utils::FileName, QmakeIncludedPriFile *> children;

    ~QmakeIncludedPriFile()
    {
        qDeleteAll(children);
    }
};

class QmakeEvalResult
{
public:
    enum EvalResultState { EvalAbort, EvalFail, EvalPartial, EvalOk };
    EvalResultState state;
    ProjectType projectType;

    QStringList subProjectsNotToDeploy;
    QSet<FileName> exactSubdirs;
    QmakeIncludedPriFile includedFiles;
    TargetInformation targetInformation;
    InstallsList installsList;
    QHash<Variable, QStringList> newVarValues;
    QStringList errors;
};

} // namespace Internal

QmakePriFile::QmakePriFile(QmakeProject *project, QmakeProFile *qmakeProFile,
                           const FileName &filePath) :
    m_project(project),
    m_qmakeProFile(qmakeProFile)
{
    Q_ASSERT(project);
    m_priFileDocument = std::make_unique<QmakePriFileDocument>(this, filePath);
    Core::DocumentManager::addDocument(m_priFileDocument.get());
}

FileName QmakePriFile::filePath() const
{
    return m_priFileDocument->filePath();
}

FileName QmakePriFile::directoryPath() const
{
    return filePath().parentDir();
}

QString QmakePriFile::displayName() const
{
    return filePath().toFileInfo().completeBaseName();
}

QmakePriFile *QmakePriFile::parent() const
{
    return m_parent;
}

QmakeProject *QmakePriFile::project() const
{
    return m_project;
}

QVector<QmakePriFile *> QmakePriFile::children() const
{
    return m_children;
}

QmakePriFile *QmakePriFile::findPriFile(const FileName &fileName)
{
    if (fileName == filePath())
        return this;
    for (QmakePriFile *n : children()) {
        if (QmakePriFile *result = n->findPriFile(fileName))
            return result;
    }
    return nullptr;

}

void QmakePriFile::makeEmpty()
{
    qDeleteAll(m_children);
    m_children.clear();
}

QSet<FileName> QmakePriFile::files(const FileType &type) const
{
    return m_files.value(type);
}

QmakePriFile::~QmakePriFile()
{
    watchFolders( {} );
    qDeleteAll(m_children);
}

void QmakePriFile::scheduleUpdate()
{
    QtSupport::ProFileCacheManager::instance()->discardFile(filePath().toString());
    m_qmakeProFile->scheduleUpdate(QmakeProFile::ParseLater);
}

QStringList QmakePriFile::baseVPaths(QtSupport::ProFileReader *reader, const QString &projectDir, const QString &buildDir)
{
    QStringList result;
    if (!reader)
        return result;
    result += reader->absolutePathValues(QLatin1String("VPATH"), projectDir);
    result << projectDir; // QMAKE_ABSOLUTE_SOURCE_PATH
    result << buildDir;
    result.removeDuplicates();
    return result;
}

QStringList QmakePriFile::fullVPaths(const QStringList &baseVPaths, QtSupport::ProFileReader *reader,
                                               const QString &qmakeVariable, const QString &projectDir)
{
    QStringList vPaths;
    if (!reader)
        return vPaths;
    vPaths = reader->absolutePathValues(QLatin1String("VPATH_") + qmakeVariable, projectDir);
    vPaths += baseVPaths;
    vPaths.removeDuplicates();
    return vPaths;
}

QSet<FileName> QmakePriFile::recursiveEnumerate(const QString &folder)
{
    QSet<FileName> result;
    QDir dir(folder);
    dir.setFilter(dir.filter() | QDir::NoDotAndDotDot);
    foreach (const QFileInfo &file, dir.entryInfoList()) {
        if (file.isDir() && !file.isSymLink())
            result += recursiveEnumerate(file.absoluteFilePath());
        else if (!Core::EditorManager::isAutoSaveFile(file.fileName()))
            result += FileName(file);
    }
    return result;
}

static QStringList fileListForVar(
        const QHash<QString, QVector<ProFileEvaluator::SourceFile>> &sourceFiles,
        const QString &varName)
{
    const QVector<ProFileEvaluator::SourceFile> &sources = sourceFiles[varName];
    QStringList result;
    result.reserve(sources.size());
    foreach (const ProFileEvaluator::SourceFile &sf, sources)
        result << sf.fileName;
    return result;
}

void QmakePriFile::extractSources(
        QHash<const ProFile *, QmakePriFileEvalResult *> proToResult, QmakePriFileEvalResult *fallback,
        QVector<ProFileEvaluator::SourceFile> sourceFiles, FileType type)
{
    foreach (const ProFileEvaluator::SourceFile &source, sourceFiles) {
        QmakePriFileEvalResult *result = proToResult.value(source.proFile);
        if (!result)
            result = fallback;
        result->foundFiles[type].insert(FileName::fromString(source.fileName));
    }
}

void QmakePriFile::extractInstalls(
        QHash<const ProFile *, QmakePriFileEvalResult *> proToResult, QmakePriFileEvalResult *fallback,
        const InstallsList &installList)
{
    for (const InstallsItem &item : installList.items) {
        for (const ProFileEvaluator::SourceFile &source : item.files) {
            auto *result = proToResult.value(source.proFile);
            if (!result)
                result = fallback;
            result->folders.insert(FileName::fromString(source.fileName));
        }
    }
}

void QmakePriFile::processValues(QmakePriFileEvalResult &result)
{
    // Remove non existing items and non folders
    auto it = result.folders.begin();
    while (it != result.folders.end()) {
        QFileInfo fi((*it).toFileInfo());
        if (fi.exists()) {
            if (fi.isDir()) {
                result.recursiveEnumerateFiles += recursiveEnumerate((*it).toString());
                // keep directories
                ++it;
            } else {
                // move files directly to recursiveEnumerateFiles
                result.recursiveEnumerateFiles += (*it);
                it = result.folders.erase(it);
            }
        } else {
            // do remove non exsting stuff
            it = result.folders.erase(it);
        }
    }

    for (int i = 0; i < static_cast<int>(FileType::FileTypeSize); ++i) {
        FileType type = static_cast<FileType>(i);
        QSet<FileName> &foundFiles = result.foundFiles[type];
        result.recursiveEnumerateFiles.subtract(foundFiles);
        QSet<FileName> newFilePaths = filterFilesProVariables(type, foundFiles);
        newFilePaths += filterFilesRecursiveEnumerata(type, result.recursiveEnumerateFiles);
        foundFiles = newFilePaths;
    }
}

void QmakePriFile::update(const Internal::QmakePriFileEvalResult &result)
{
    m_recursiveEnumerateFiles = result.recursiveEnumerateFiles;
    watchFolders(result.folders);

    for (int i = 0; i < static_cast<int>(FileType::FileTypeSize); ++i) {
        const FileType type = static_cast<FileType>(i);
        m_files[type] = result.foundFiles.value(type);
    }
}

void QmakePriFile::watchFolders(const QSet<FileName> &folders)
{
    const QSet<QString> folderStrings =
            Utils::transform(folders, [] (const FileName &f) { return f.toString(); });
    QSet<QString> toUnwatch = m_watchedFolders;
    toUnwatch.subtract(folderStrings);

    QSet<QString> toWatch = folderStrings;
    toWatch.subtract(m_watchedFolders);

    m_project->unwatchFolders(toUnwatch.toList(), this);
    m_project->watchFolders(toWatch.toList(), this);

    m_watchedFolders = folderStrings;
}

bool QmakePriFile::knowsFile(const FileName &filePath) const
{
    return m_recursiveEnumerateFiles.contains(filePath);
}

bool QmakePriFile::folderChanged(const QString &changedFolder, const QSet<FileName> &newFiles)
{
    qCDebug(qmakeParse()) << "QmakePriFile::folderChanged";

    QSet<FileName> addedFiles = newFiles;
    addedFiles.subtract(m_recursiveEnumerateFiles);

    QSet<FileName> removedFiles = m_recursiveEnumerateFiles;
    removedFiles.subtract(newFiles);

    foreach (const FileName &file, removedFiles) {
        if (!file.isChildOf(FileName::fromString(changedFolder)))
            removedFiles.remove(file);
    }

    if (addedFiles.isEmpty() && removedFiles.isEmpty())
        return false;

    m_recursiveEnumerateFiles = newFiles;

    // Apply the differences per file type
    for (int i = 0; i < static_cast<int>(FileType::FileTypeSize); ++i) {
        FileType type = static_cast<FileType>(i);
        QSet<FileName> add = filterFilesRecursiveEnumerata(type, addedFiles);
        QSet<FileName> remove = filterFilesRecursiveEnumerata(type, removedFiles);

        if (!add.isEmpty() || !remove.isEmpty()) {
            qCDebug(qmakeParse()) << "For type" << static_cast<int>(type) <<"\n"
                                  << "added files"  <<  add << "\n"
                                  << "removed files" << remove;

            m_files[type].unite(add);
            m_files[type].subtract(remove);
        }
    }
    return true;
}

bool QmakePriFile::deploysFolder(const QString &folder) const
{
    QString f = folder;
    const QChar slash = QLatin1Char('/');
    if (!f.endsWith(slash))
        f.append(slash);

    foreach (const QString &wf, m_watchedFolders) {
        if (f.startsWith(wf)
            && (wf.endsWith(slash)
                || (wf.length() < f.length() && f.at(wf.length()) == slash)))
            return true;
    }
    return false;
}

QVector<QmakePriFile *> QmakePriFile::subPriFilesExact() const
{
    return Utils::filtered(m_children, &QmakePriFile::includedInExactParse);
}

QmakeProFile *QmakePriFile::proFile() const
{
    return m_qmakeProFile;
}

bool QmakePriFile::includedInExactParse() const
{
    return m_includedInExactParse;
}

void QmakePriFile::setIncludedInExactParse(bool b)
{
    m_includedInExactParse = b;
}

bool QmakePriFile::canAddSubProject(const QString &proFilePath) const
{
    QFileInfo fi(proFilePath);
    if (fi.suffix() == QLatin1String("pro")
        || fi.suffix() == QLatin1String("pri"))
        return true;
    return false;
}

static QString simplifyProFilePath(const QString &proFilePath)
{
    // if proFilePath is like: _path_/projectName/projectName.pro
    // we simplify it to: _path_/projectName
    QFileInfo fi(proFilePath);
    const QString parentPath = fi.absolutePath();
    QFileInfo parentFi(parentPath);
    if (parentFi.fileName() == fi.completeBaseName())
        return parentPath;
    return proFilePath;
}

bool QmakePriFile::addSubProject(const QString &proFile)
{
    QStringList uniqueProFilePaths;
    if (!m_recursiveEnumerateFiles.contains(FileName::fromString(proFile)))
        uniqueProFilePaths.append(simplifyProFilePath(proFile));

    QStringList failedFiles;
    changeFiles(QLatin1String(Constants::PROFILE_MIMETYPE), uniqueProFilePaths, &failedFiles, AddToProFile);

    return failedFiles.isEmpty();
}

bool QmakePriFile::removeSubProjects(const QString &proFilePath)
{
    QStringList failedOriginalFiles;
    changeFiles(QLatin1String(Constants::PROFILE_MIMETYPE), QStringList(proFilePath), &failedOriginalFiles, RemoveFromProFile);

    QStringList simplifiedProFiles = Utils::transform(failedOriginalFiles, &simplifyProFilePath);

    QStringList failedSimplifiedFiles;
    changeFiles(QLatin1String(Constants::PROFILE_MIMETYPE), simplifiedProFiles, &failedSimplifiedFiles, RemoveFromProFile);

    return failedSimplifiedFiles.isEmpty();
}

bool QmakePriFile::addFiles(const QStringList &filePaths, QStringList *notAdded)
{
    // If a file is already referenced in the .pro file then we don't add them.
    // That ignores scopes and which variable was used to reference the file
    // So it's obviously a bit limited, but in those cases you need to edit the
    // project files manually anyway.

    typedef QMap<QString, QStringList> TypeFileMap;
    // Split into lists by file type and bulk-add them.
    TypeFileMap typeFileMap;
    foreach (const QString &file, filePaths) {
        const Utils::MimeType mt = Utils::mimeTypeForFile(file);
        typeFileMap[mt.name()] << file;
    }

    QStringList failedFiles;
    foreach (const QString &type, typeFileMap.keys()) {
        const QStringList typeFiles = typeFileMap.value(type);
        QStringList qrcFiles; // the list of qrc files referenced from ui files
        if (type == QLatin1String(ProjectExplorer::Constants::RESOURCE_MIMETYPE)) {
            foreach (const QString &formFile, typeFiles) {
                QStringList resourceFiles = formResources(formFile);
                foreach (const QString &resourceFile, resourceFiles)
                    if (!qrcFiles.contains(resourceFile))
                        qrcFiles.append(resourceFile);
            }
        }

        QStringList uniqueQrcFiles;
        foreach (const QString &file, qrcFiles) {
            if (!m_recursiveEnumerateFiles.contains(FileName::fromString(file)))
                uniqueQrcFiles.append(file);
        }

        QStringList uniqueFilePaths;
        foreach (const QString &file, typeFiles) {
            if (!m_recursiveEnumerateFiles.contains(FileName::fromString(file)))
                uniqueFilePaths.append(file);
        }

        changeFiles(type, uniqueFilePaths, &failedFiles, AddToProFile);
        if (notAdded)
            *notAdded += failedFiles;
        changeFiles(QLatin1String(ProjectExplorer::Constants::RESOURCE_MIMETYPE), uniqueQrcFiles, &failedFiles, AddToProFile);
        if (notAdded)
            *notAdded += failedFiles;
    }
    return failedFiles.isEmpty();
}

bool QmakePriFile::removeFiles(const QStringList &filePaths,
                              QStringList *notRemoved)
{
    QStringList failedFiles;
    typedef QMap<QString, QStringList> TypeFileMap;
    // Split into lists by file type and bulk-add them.
    TypeFileMap typeFileMap;
    foreach (const QString &file, filePaths) {
        const Utils::MimeType mt = Utils::mimeTypeForFile(file);
        typeFileMap[mt.name()] << file;
    }
    foreach (const QString &type, typeFileMap.keys()) {
        const QStringList typeFiles = typeFileMap.value(type);
        changeFiles(type, typeFiles, &failedFiles, RemoveFromProFile);
        if (notRemoved)
            *notRemoved = failedFiles;
    }
    return failedFiles.isEmpty();
}

bool QmakePriFile::deleteFiles(const QStringList &filePaths)
{
    removeFiles(filePaths);
    return true;
}

bool QmakePriFile::canRenameFile(const QString &filePath, const QString &newFilePath)
{
    if (newFilePath.isEmpty())
        return false;

    bool changeProFileOptional = deploysFolder(QFileInfo(filePath).absolutePath());
    if (changeProFileOptional)
        return true;

    const Utils::MimeType mt = Utils::mimeTypeForFile(newFilePath);
    return renameFile(filePath, newFilePath, mt.name(), Change::TestOnly);
}

bool QmakePriFile::renameFile(const QString &filePath, const QString &newFilePath)
{
    if (newFilePath.isEmpty())
        return false;

    bool changeProFileOptional = deploysFolder(QFileInfo(filePath).absolutePath());
    const Utils::MimeType mt = Utils::mimeTypeForFile(newFilePath);

    if (renameFile(filePath, newFilePath, mt.name()))
        return true;
    return changeProFileOptional;
}

bool QmakePriFile::priFileWritable(const QString &path)
{
    ReadOnlyFilesDialog roDialog(path, ICore::mainWindow());
    roDialog.setShowFailWarning(true);
    return roDialog.exec() != ReadOnlyFilesDialog::RO_Cancel;
}

bool QmakePriFile::saveModifiedEditors()
{
    Core::IDocument *document
            = Core::DocumentModel::documentForFilePath(filePath().toString());
    if (!document || !document->isModified())
        return true;

    if (!Core::DocumentManager::saveDocument(document))
        return false;

    // force instant reload of ourselves
    QtSupport::ProFileCacheManager::instance()->discardFile(filePath().toString());
    QmakeProject::notifyChanged(filePath());
    return true;
}

QStringList QmakePriFile::formResources(const QString &formFile) const
{
    QStringList resourceFiles;
    QFile file(formFile);
    if (!file.open(QIODevice::ReadOnly))
        return resourceFiles;

    QXmlStreamReader reader(&file);

    QFileInfo fi(formFile);
    QDir formDir = fi.absoluteDir();
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            if (reader.name() == QLatin1String("iconset")) {
                const QXmlStreamAttributes attributes = reader.attributes();
                if (attributes.hasAttribute(QLatin1String("resource")))
                    resourceFiles.append(QDir::cleanPath(formDir.absoluteFilePath(
                                  attributes.value(QLatin1String("resource")).toString())));
            } else if (reader.name() == QLatin1String("include")) {
                const QXmlStreamAttributes attributes = reader.attributes();
                if (attributes.hasAttribute(QLatin1String("location")))
                    resourceFiles.append(QDir::cleanPath(formDir.absoluteFilePath(
                                  attributes.value(QLatin1String("location")).toString())));

            }
        }
    }

    if (reader.hasError())
        qWarning() << "Could not read form file:" << formFile;

    return resourceFiles;
}

bool QmakePriFile::ensureWriteableProFile(const QString &file)
{
    // Ensure that the file is not read only
    QFileInfo fi(file);
    if (!fi.isWritable()) {
        // Try via vcs manager
        Core::IVersionControl *versionControl = Core::VcsManager::findVersionControlForDirectory(fi.absolutePath());
        if (!versionControl || !versionControl->vcsOpen(file)) {
            bool makeWritable = QFile::setPermissions(file, fi.permissions() | QFile::WriteUser);
            if (!makeWritable) {
                QMessageBox::warning(Core::ICore::mainWindow(),
                                     QCoreApplication::translate("QmakePriFile", "Failed"),
                                     QCoreApplication::translate("QmakePriFile", "Could not write project file %1.").arg(file));
                return false;
            }
        }
    }
    return true;
}

QPair<ProFile *, QStringList> QmakePriFile::readProFile(const QString &file)
{
    QStringList lines;
    ProFile *includeFile = nullptr;
    {
        QString contents;
        {
            FileReader reader;
            if (!reader.fetch(file, QIODevice::Text)) {
                QmakeProject::proFileParseError(reader.errorString());
                return qMakePair(includeFile, lines);
            }
            const QTextCodec *codec = Core::EditorManager::defaultTextCodec();
            contents = codec->toUnicode(reader.data());
            lines = contents.split(QLatin1Char('\n'));
        }

        QMakeVfs vfs;
        QtSupport::ProMessageHandler handler;
        QMakeParser parser(nullptr, &vfs, &handler);
        includeFile = parser.parsedProBlock(QStringRef(&contents), file, 1);
    }
    return qMakePair(includeFile, lines);
}

bool QmakePriFile::prepareForChange()
{
    return saveModifiedEditors() && ensureWriteableProFile(filePath().toString());
}

bool QmakePriFile::renameFile(const QString &oldName,
                                  const QString &newName,
                                  const QString &mimeType,
                                  Change mode)
{
    if (!prepareForChange())
        return false;

    QPair<ProFile *, QStringList> pair = readProFile(filePath().toString());
    ProFile *includeFile = pair.first;
    QStringList lines = pair.second;

    if (!includeFile)
        return false;

    QDir priFileDir = QDir(m_qmakeProFile->directoryPath().toString());
    QStringList notChanged = ProWriter::removeFiles(includeFile, &lines, priFileDir,
                                                    QStringList(oldName), varNamesForRemoving());

    includeFile->deref();
    if (!notChanged.isEmpty())
        return false;

    // We need to re-parse here: The file has changed.
    QMakeParser parser(nullptr, nullptr, nullptr);
    QString contents = lines.join(QLatin1Char('\n'));
    includeFile = parser.parsedProBlock(QStringRef(&contents),
                                        filePath().toString(), 1, QMakeParser::FullGrammar);
    QTC_ASSERT(includeFile, return false); // The file should still be valid after what we did.

    ProWriter::addFiles(includeFile, &lines,
                        QStringList(newName),
                        varNameForAdding(mimeType));
    if (mode == Change::Save)
        save(lines);
    includeFile->deref();
    return true;
}

void QmakePriFile::changeFiles(const QString &mimeType,
                                 const QStringList &filePaths,
                                 QStringList *notChanged,
                                 ChangeType change, Change mode)
{
    if (filePaths.isEmpty())
        return;

    *notChanged = filePaths;

    // Check for modified editors
    if (!prepareForChange())
        return;

    QPair<ProFile *, QStringList> pair = readProFile(filePath().toString());
    ProFile *includeFile = pair.first;
    QStringList lines = pair.second;

    if (!includeFile)
        return;

    if (change == AddToProFile) {
        // Use the first variable for adding.
        ProWriter::addFiles(includeFile, &lines, filePaths, varNameForAdding(mimeType));
        notChanged->clear();
    } else { // RemoveFromProFile
        QDir priFileDir = QDir(m_qmakeProFile->directoryPath().toString());
        *notChanged = ProWriter::removeFiles(includeFile, &lines, priFileDir, filePaths, varNamesForRemoving());
    }

    // save file
    if (mode == Change::Save)
        save(lines);
    includeFile->deref();
}

void QmakePriFile::addChild(QmakePriFile *pf)
{
    QTC_ASSERT(!m_children.contains(pf), return);
    QTC_ASSERT(!pf->parent(), return);
    m_children.append(pf);
    pf->setParent(this);
}

void QmakePriFile::setParent(QmakePriFile *p)
{
    QTC_ASSERT(!m_parent, return);
    m_parent = p;
}

bool QmakePriFile::setProVariable(const QString &var, const QStringList &values, const QString &scope, int flags)
{
    if (!prepareForChange())
        return false;

    QPair<ProFile *, QStringList> pair = readProFile(filePath().toString());
    ProFile *includeFile = pair.first;
    QStringList lines = pair.second;

    if (!includeFile)
        return false;

    ProWriter::putVarValues(includeFile, &lines, values, var,
                            ProWriter::PutFlags(flags),
                            scope);

    save(lines);
    includeFile->deref();
    return true;
}

void QmakePriFile::save(const QStringList &lines)
{
    {
        FileChangeBlocker changeGuard(filePath().toString());
        FileSaver saver(filePath().toString(), QIODevice::Text);
        const QTextCodec *codec = Core::EditorManager::defaultTextCodec();
        saver.write(codec->fromUnicode(lines.join(QLatin1Char('\n'))));
        saver.finalize(Core::ICore::mainWindow());
    }

    // This is a hack.
    // We are saving twice in a very short timeframe, once the editor and once the ProFile.
    // So the modification time might not change between those two saves.
    // We manually tell each editor to reload it's file.
    // (The .pro files are notified by the file system watcher.)
    QStringList errorStrings;
    Core::IDocument *document = Core::DocumentModel::documentForFilePath(filePath().toString());
    if (document) {
        QString errorString;
        if (!document->reload(&errorString, Core::IDocument::FlagReload, Core::IDocument::TypeContents))
            errorStrings << errorString;
    }
    if (!errorStrings.isEmpty())
        QMessageBox::warning(Core::ICore::mainWindow(), QCoreApplication::translate("QmakePriFile", "File Error"),
                             errorStrings.join(QLatin1Char('\n')));
}

QStringList QmakePriFile::varNames(FileType type, QtSupport::ProFileReader *readerExact)
{
    QStringList vars;
    switch (type) {
    case FileType::Header:
        vars << QLatin1String("HEADERS");
        vars << QLatin1String("PRECOMPILED_HEADER");
        break;
    case FileType::Source: {
        vars << QLatin1String("SOURCES");
        QStringList listOfExtraCompilers = readerExact->values(QLatin1String("QMAKE_EXTRA_COMPILERS"));
        foreach (const QString &var, listOfExtraCompilers) {
            QStringList inputs = readerExact->values(var + QLatin1String(".input"));
            foreach (const QString &input, inputs)
                // FORMS, RESOURCES, and STATECHARTS are handled below, HEADERS and SOURCES above
                if (input != QLatin1String("FORMS")
                        && input != QLatin1String("STATECHARTS")
                        && input != QLatin1String("RESOURCES")
                        && input != QLatin1String("SOURCES")
                        && input != QLatin1String("HEADERS"))
                    vars << input;
        }
        break;
    }
    case FileType::Resource:
        vars << QLatin1String("RESOURCES");
        break;
    case FileType::Form:
        vars << QLatin1String("FORMS");
        break;
    case FileType::StateChart:
        vars << QLatin1String("STATECHARTS");
        break;
    case FileType::Project:
        vars << QLatin1String("SUBDIRS");
        break;
    case FileType::QML:
        vars << QLatin1String("OTHER_FILES");
        vars << QLatin1String("DISTFILES");
        break;
    default:
        vars << QLatin1String("OTHER_FILES");
        vars << QLatin1String("DISTFILES");
        vars << QLatin1String("ICON");
        vars << QLatin1String("QMAKE_INFO_PLIST");
        break;
    }
    return vars;
}

//!
//! \brief QmakePriFile::varNames
//! \param mimeType
//! \return the qmake variable name for the mime type
//! Note: Only used for adding.
//!
QString QmakePriFile::varNameForAdding(const QString &mimeType)
{
    if (mimeType == QLatin1String(ProjectExplorer::Constants::CPP_HEADER_MIMETYPE)
            || mimeType == QLatin1String(ProjectExplorer::Constants::C_HEADER_MIMETYPE)) {
        return QLatin1String("HEADERS");
    }

    if (mimeType == QLatin1String(ProjectExplorer::Constants::CPP_SOURCE_MIMETYPE)
               || mimeType == QLatin1String(CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE)
               || mimeType == QLatin1String(ProjectExplorer::Constants::C_SOURCE_MIMETYPE)) {
        return QLatin1String("SOURCES");
    }

    if (mimeType == QLatin1String(ProjectExplorer::Constants::RESOURCE_MIMETYPE))
        return QLatin1String("RESOURCES");

    if (mimeType == QLatin1String(ProjectExplorer::Constants::FORM_MIMETYPE))
        return QLatin1String("FORMS");

    if (mimeType == QLatin1String(ProjectExplorer::Constants::QML_MIMETYPE))
        return QLatin1String("DISTFILES");

    if (mimeType == QLatin1String(ProjectExplorer::Constants::SCXML_MIMETYPE))
        return QLatin1String("STATECHARTS");

    if (mimeType == QLatin1String(Constants::PROFILE_MIMETYPE))
        return QLatin1String("SUBDIRS");

    return QLatin1String("DISTFILES");
}

//!
//! \brief QmakePriFile::varNamesForRemoving
//! \return all qmake variables which are displayed in the project tree
//! Note: Only used for removing.
//!
QStringList QmakePriFile::varNamesForRemoving()
{
    QStringList vars;
    vars << QLatin1String("HEADERS");
    vars << QLatin1String("OBJECTIVE_HEADERS");
    vars << QLatin1String("PRECOMPILED_HEADER");
    vars << QLatin1String("SOURCES");
    vars << QLatin1String("OBJECTIVE_SOURCES");
    vars << QLatin1String("RESOURCES");
    vars << QLatin1String("FORMS");
    vars << QLatin1String("OTHER_FILES");
    vars << QLatin1String("SUBDIRS");
    vars << QLatin1String("DISTFILES");
    vars << QLatin1String("ICON");
    vars << QLatin1String("QMAKE_INFO_PLIST");
    vars << QLatin1String("STATECHARTS");
    return vars;
}

QSet<FileName> QmakePriFile::filterFilesProVariables(FileType fileType, const QSet<FileName> &files)
{
    if (fileType != FileType::QML && fileType != FileType::Unknown)
        return files;
    QSet<FileName> result;
    if (fileType == FileType::QML) {
        foreach (const FileName &file, files)
            if (file.toString().endsWith(QLatin1String(".qml")))
                result << file;
    } else {
        foreach (const FileName &file, files)
            if (!file.toString().endsWith(QLatin1String(".qml")))
                result << file;
    }
    return result;
}

QSet<FileName> QmakePriFile::filterFilesRecursiveEnumerata(FileType fileType, const QSet<FileName> &files)
{
    QSet<FileName> result;
    if (fileType != FileType::QML && fileType != FileType::Unknown)
        return result;
    if (fileType == FileType::QML) {
        foreach (const FileName &file, files)
            if (file.toString().endsWith(QLatin1String(".qml")))
                result << file;
    } else {
        foreach (const FileName &file, files)
            if (!file.toString().endsWith(QLatin1String(".qml")))
                result << file;
    }
    return result;
}

} // namespace QmakeProjectManager

static ProjectType proFileTemplateTypeToProjectType(ProFileEvaluator::TemplateType type)
{
    switch (type) {
    case ProFileEvaluator::TT_Unknown:
    case ProFileEvaluator::TT_Application:
        return ProjectType::ApplicationTemplate;
    case ProFileEvaluator::TT_StaticLibrary:
        return ProjectType::StaticLibraryTemplate;
    case ProFileEvaluator::TT_SharedLibrary:
        return ProjectType::SharedLibraryTemplate;
    case ProFileEvaluator::TT_Script:
        return ProjectType::ScriptTemplate;
    case ProFileEvaluator::TT_Aux:
        return ProjectType::AuxTemplate;
    case ProFileEvaluator::TT_Subdirs:
        return ProjectType::SubDirsTemplate;
    default:
        return ProjectType::Invalid;
    }
}

QmakeProFile *QmakeProFile::findProFile(const FileName &fileName)
{
    return dynamic_cast<QmakeProFile *>(findPriFile(fileName));
}

QString QmakeProFile::makefile() const
{
    return singleVariableValue(Variable::Makefile);
}

QString QmakeProFile::objectExtension() const
{
    if (m_varValues[Variable::ObjectExt].isEmpty())
        return HostOsInfo::isWindowsHost() ? QLatin1String(".obj") : QLatin1String(".o");
    return m_varValues[Variable::ObjectExt].first();
}

QString QmakeProFile::objectsDirectory() const
{
    return singleVariableValue(Variable::ObjectsDir);
}

QByteArray QmakeProFile::cxxDefines() const
{
    QByteArray result;
    foreach (const QString &def, variableValue(Variable::Defines)) {
        // 'def' is shell input, so interpret it.
        QtcProcess::SplitError error = QtcProcess::SplitOk;
        const QStringList args = QtcProcess::splitArgs(def, HostOsInfo::hostOs(), false, &error);
        if (error != QtcProcess::SplitOk || args.size() == 0)
            continue;

        result += "#define ";
        const QString defInterpreted = args.first();
        const int index = defInterpreted.indexOf(QLatin1Char('='));
        if (index == -1) {
            result += defInterpreted.toLatin1();
            result += " 1\n";
        } else {
            const QString name = defInterpreted.left(index);
            const QString value = defInterpreted.mid(index + 1);
            result += name.toLatin1();
            result += ' ';
            result += value.toLocal8Bit();
            result += '\n';
        }
    }
    return result;
}

/*!
  \class QmakeProFile
  Implements abstract ProjectNode class
  */
QmakeProFile::QmakeProFile(QmakeProject *project, const FileName &filePath) :
    QmakePriFile(project, this, filePath)
{
    // The lifetime of the m_parserFutureWatcher is shorter
    // than of this, so this is all safe
    QObject::connect(&m_parseFutureWatcher, &QFutureWatcherBase::finished,
                     [this](){ applyAsyncEvaluate(); });
}

QmakeProFile::~QmakeProFile()
{
    qDeleteAll(m_extraCompilers);
    m_parseFutureWatcher.waitForFinished();
    if (m_readerExact)
        applyAsyncEvaluate();
}

bool QmakeProFile::isParent(QmakeProFile *node)
{
    while ((node = dynamic_cast<QmakeProFile *>(node->parent()))) {
        if (node == this)
            return true;
    }
    return false;
}

QString QmakeProFile::displayName() const
{
    if (!m_displayName.isEmpty())
        return m_displayName;
    return QmakePriFile::displayName();
}

QList<QmakeProFile *> QmakeProFile::allProFiles()
{
    QList<QmakeProFile *> result = { this };
    for (QmakePriFile *c : m_children) {
        auto proC = dynamic_cast<QmakeProFile *>(c);
        if (proC)
            result.append(proC->allProFiles());
    }
    return result;
}

bool QmakeProFile::isDebugAndRelease() const
{
    const QStringList configValues = m_varValues.value(Variable::Config);
    return configValues.contains(QLatin1String("debug_and_release"));
}

bool QmakeProFile::isQtcRunnable() const
{
    const QStringList configValues = m_varValues.value(Variable::Config);
    return configValues.contains(QLatin1String("qtc_runnable"));
}

ProjectType QmakeProFile::projectType() const
{
    return m_projectType;
}

QStringList QmakeProFile::variableValue(const Variable var) const
{
    return m_varValues.value(var);
}

QString QmakeProFile::singleVariableValue(const Variable var) const
{
    const QStringList &values = variableValue(var);
    return values.isEmpty() ? QString() : values.first();
}

void QmakeProFile::setParseInProgressRecursive(bool b)
{
    setParseInProgress(b);
    foreach (QmakePriFile *c, children()) {
        if (auto node = dynamic_cast<QmakeProFile *>(c))
            node->setParseInProgressRecursive(b);
    }
}

void QmakeProFile::setParseInProgress(bool b)
{
    if (m_parseInProgress == b)
        return;
    m_parseInProgress = b;
    emit m_project->proFileUpdated(this, m_validParse, m_parseInProgress);
}

// Do note the absence of signal emission, always set validParse
// before setParseInProgress, as that will emit the signals
void QmakeProFile::setValidParseRecursive(bool b)
{
    m_validParse = b;
    foreach (QmakePriFile *c, children()) {
        if (QmakeProFile *node = dynamic_cast<QmakeProFile *>(c))
            node->setValidParseRecursive(b);
    }
}

bool QmakeProFile::validParse() const
{
    return m_validParse;
}

bool QmakeProFile::parseInProgress() const
{
    return m_parseInProgress;
}

void QmakeProFile::scheduleUpdate(QmakeProFile::AsyncUpdateDelay delay)
{
    setParseInProgressRecursive(true);
    m_project->scheduleAsyncUpdate(this, delay);
}

void QmakeProFile::asyncUpdate()
{
    m_project->incrementPendingEvaluateFutures();
    setupReader();
    if (!includedInExactParse())
        m_readerExact->setExact(false);
    m_parseFutureWatcher.waitForFinished();
    QmakeEvalInput input = evalInput();
    QFuture<QmakeEvalResult *> future = Utils::runAsync(ProjectExplorerPlugin::sharedThreadPool(),
                                                   QThread::LowestPriority,
                                                   &QmakeProFile::asyncEvaluate,
                                                   this, input);
    m_parseFutureWatcher.setFuture(future);
}

QmakeEvalInput QmakeProFile::evalInput() const
{
    QmakeEvalInput input;
    input.projectDir = directoryPath().toString();
    input.projectFilePath = filePath();
    input.buildDirectory = buildDir();
    input.sysroot = FileName::fromString(m_project->qmakeSysroot());
    input.readerExact = m_readerExact;
    input.readerCumulative = m_readerCumulative;
    input.qmakeGlobals = m_project->qmakeGlobals();
    input.qmakeVfs = m_project->qmakeVfs();
    return input;
}

void QmakeProFile::setupReader()
{
    Q_ASSERT(!m_readerExact);
    Q_ASSERT(!m_readerCumulative);

    m_readerExact = m_project->createProFileReader(this);

    m_readerCumulative = m_project->createProFileReader(this);
    m_readerCumulative->setCumulative(true);
}

static bool evaluateOne(const QmakeEvalInput &input, ProFile *pro,
                        QtSupport::ProFileReader *reader, bool cumulative,
                        QtSupport::ProFileReader **buildPassReader)
{
    if (!reader->accept(pro, QMakeEvaluator::LoadAll))
        return false;

    QStringList builds = reader->values(QLatin1String("BUILDS"));
    if (builds.isEmpty()) {
        *buildPassReader = reader;
    } else {
        QString build = builds.first();
        QHash<QString, QStringList> basevars;
        QStringList basecfgs = reader->values(build + QLatin1String(".CONFIG"));
        basecfgs += build;
        basecfgs += QLatin1String("build_pass");
        basevars[QLatin1String("BUILD_PASS")] = QStringList(build);
        QStringList buildname = reader->values(build + QLatin1String(".name"));
        basevars[QLatin1String("BUILD_NAME")] = (buildname.isEmpty() ? QStringList(build) : buildname);

        // We don't increase/decrease m_qmakeGlobalsRefCnt here, because the outer profilereaders keep m_qmakeGlobals alive anyway
        auto bpReader = new QtSupport::ProFileReader(input.qmakeGlobals, input.qmakeVfs); // needs to access m_qmakeGlobals, m_qmakeVfs
        bpReader->setOutputDir(input.buildDirectory.toString());
        bpReader->setCumulative(cumulative);
        bpReader->setExtraVars(basevars);
        bpReader->setExtraConfigs(basecfgs);

        if (bpReader->accept(pro, QMakeEvaluator::LoadAll))
            *buildPassReader = bpReader;
        else
            delete bpReader;
    }

    return true;
}

QmakeEvalResult *QmakeProFile::evaluate(const QmakeEvalInput &input)
{
    QmakeEvalResult *result = new QmakeEvalResult;
    QtSupport::ProFileReader *exactBuildPassReader = nullptr;
    QtSupport::ProFileReader *cumulativeBuildPassReader = nullptr;
    ProFile *pro;
    if ((pro = input.readerExact->parsedProFile(input.projectFilePath.toString()))) {
        bool exactOk = evaluateOne(input, pro, input.readerExact, false, &exactBuildPassReader);
        bool cumulOk = evaluateOne(input, pro, input.readerCumulative, true, &cumulativeBuildPassReader);
        pro->deref();
        result->state = exactOk ? QmakeEvalResult::EvalOk
                                : cumulOk ? QmakeEvalResult::EvalPartial : QmakeEvalResult::EvalFail;
    } else {
        result->state = QmakeEvalResult::EvalFail;
    }

    if (result->state == QmakeEvalResult::EvalFail)
        return result;

    result->includedFiles.proFile = pro;
    result->includedFiles.name = input.projectFilePath;

    QHash<const ProFile *, QmakePriFileEvalResult *> proToResult;

    result->projectType
            = proFileTemplateTypeToProjectType(
                (result->state == QmakeEvalResult::EvalOk ? input.readerExact
                                                          : input.readerCumulative)->templateType());
    if (result->state == QmakeEvalResult::EvalOk) {
        if (result->projectType == ProjectType::SubDirsTemplate) {
            QStringList errors;
            FileNameList subDirs = subDirsPaths(input.readerExact, input.projectDir, &result->subProjectsNotToDeploy, &errors);
            result->errors.append(errors);

            foreach (const Utils::FileName &subDirName, subDirs) {
                auto subDir = new QmakeIncludedPriFile;
                subDir->proFile = nullptr;
                subDir->name = subDirName;
                result->includedFiles.children.insert(subDirName, subDir);
            }

            result->exactSubdirs = subDirs.toSet();
        }

        // Convert ProFileReader::includeFiles to IncludedPriFile structure
        QHash<ProFile *, QVector<ProFile *>> includeFiles = input.readerExact->includeFiles();
        QList<QmakeIncludedPriFile *> toBuild = {&result->includedFiles};
        while (!toBuild.isEmpty()) {
            QmakeIncludedPriFile *current = toBuild.takeFirst();
            if (!current->proFile)
                continue;  // Don't attempt to map subdirs here
            QVector<ProFile *> children = includeFiles.value(current->proFile);
            foreach (ProFile *child, children) {
                const Utils::FileName childName = Utils::FileName::fromString(child->fileName());
                auto it = current->children.find(childName);
                if (it == current->children.end()) {
                    auto childTree = new QmakeIncludedPriFile;
                    childTree->proFile = child;
                    childTree->name = childName;
                    current->children.insert(childName, childTree);
                    proToResult[child] = &childTree->result;
                }
            }
            toBuild.append(current->children.values());
        }
    }

    if (result->projectType == ProjectType::SubDirsTemplate) {
        FileNameList subDirs = subDirsPaths(input.readerCumulative, input.projectDir, nullptr, nullptr);
        foreach (const Utils::FileName &subDirName, subDirs) {
            auto it = result->includedFiles.children.find(subDirName);
            if (it == result->includedFiles.children.end()) {
                auto subDir = new QmakeIncludedPriFile;
                subDir->proFile = nullptr;
                subDir->name = subDirName;
                result->includedFiles.children.insert(subDirName, subDir);
            }
        }
    }

    // Add ProFileReader::includeFiles information from cumulative parse to IncludedPriFile structure
    QHash<ProFile *, QVector<ProFile *>> includeFiles = input.readerCumulative->includeFiles();
    QList<QmakeIncludedPriFile *> toBuild = {&result->includedFiles};
    while (!toBuild.isEmpty()) {
        QmakeIncludedPriFile *current = toBuild.takeFirst();
        if (!current->proFile)
            continue;  // Don't attempt to map subdirs here
        QVector<ProFile *> children = includeFiles.value(current->proFile);
        foreach (ProFile *child, children) {
            const Utils::FileName childName = Utils::FileName::fromString(child->fileName());
            auto it = current->children.find(childName);
            if (it == current->children.end()) {
                auto childTree = new QmakeIncludedPriFile;
                childTree->proFile = child;
                childTree->name = childName;
                current->children.insert(childName, childTree);
                proToResult[child] = &childTree->result;
            }
        }
        toBuild.append(current->children.values());
    }

    auto exactReader = exactBuildPassReader ? exactBuildPassReader : input.readerExact;
    auto cumulativeReader = cumulativeBuildPassReader ? cumulativeBuildPassReader : input.readerCumulative;

    QHash<QString, QVector<ProFileEvaluator::SourceFile>> exactSourceFiles;
    QHash<QString, QVector<ProFileEvaluator::SourceFile>> cumulativeSourceFiles;

    const QStringList baseVPathsExact
            = baseVPaths(exactReader, input.projectDir, input.buildDirectory.toString());
    const QStringList baseVPathsCumulative
            = baseVPaths(cumulativeReader, input.projectDir, input.buildDirectory.toString());

    for (int i = 0; i < static_cast<int>(FileType::FileTypeSize); ++i) {
        const FileType type = static_cast<FileType>(i);
        const QStringList qmakeVariables = varNames(type, exactReader);
        foreach (const QString &qmakeVariable, qmakeVariables) {
            QHash<ProString, bool> handled;
            if (result->state == QmakeEvalResult::EvalOk) {
                const QStringList vPathsExact = fullVPaths(
                            baseVPathsExact, exactReader, qmakeVariable, input.projectDir);
                auto sourceFiles = exactReader->absoluteFileValues(
                            qmakeVariable, input.projectDir, vPathsExact, &handled);
                exactSourceFiles[qmakeVariable] = sourceFiles;
                extractSources(proToResult, &result->includedFiles.result, sourceFiles, type);
            }
            const QStringList vPathsCumulative = fullVPaths(
                        baseVPathsCumulative, cumulativeReader, qmakeVariable, input.projectDir);
            auto sourceFiles = cumulativeReader->absoluteFileValues(
                        qmakeVariable, input.projectDir, vPathsCumulative, &handled);
            cumulativeSourceFiles[qmakeVariable] = sourceFiles;
            extractSources(proToResult, &result->includedFiles.result, sourceFiles, type);
        }
    }

    // This is used for two things:
    // - Actual deployment, in which case we need exact values.
    // - The project tree, in which case we also want exact values to avoid recursively
    //   watching bogus paths. However, we accept the values even if the evaluation
    //   failed, to at least have a best-effort result.
    result->installsList = installsList(exactBuildPassReader, input.projectFilePath.toString(),
                                        input.projectDir, input.buildDirectory.toString());
    extractInstalls(proToResult, &result->includedFiles.result, result->installsList);

    if (result->state == QmakeEvalResult::EvalOk) {
        result->targetInformation = targetInformation(input.readerExact, exactBuildPassReader,
                                                      input.buildDirectory, input.projectFilePath);

        // update other variables
        result->newVarValues[Variable::Defines] = exactReader->values(QLatin1String("DEFINES"));
        result->newVarValues[Variable::IncludePath] = includePaths(exactReader, input.sysroot,
                                                            input.buildDirectory, input.projectDir);
        result->newVarValues[Variable::CppFlags] = exactReader->values(QLatin1String("QMAKE_CXXFLAGS"));
        result->newVarValues[Variable::Source] =
                fileListForVar(exactSourceFiles, QLatin1String("SOURCES")) +
                fileListForVar(cumulativeSourceFiles, QLatin1String("SOURCES")) +
                fileListForVar(exactSourceFiles, QLatin1String("HEADERS")) +
                fileListForVar(cumulativeSourceFiles, QLatin1String("HEADERS")) +
                fileListForVar(exactSourceFiles, QLatin1String("OBJECTIVE_HEADERS")) +
                fileListForVar(cumulativeSourceFiles, QLatin1String("OBJECTIVE_HEADERS"));
        result->newVarValues[Variable::UiDir] = QStringList() << uiDirPath(exactReader, input.buildDirectory);
        result->newVarValues[Variable::HeaderExtension] = QStringList() << exactReader->value(QLatin1String("QMAKE_EXT_H"));
        result->newVarValues[Variable::CppExtension] = QStringList() << exactReader->value(QLatin1String("QMAKE_EXT_CPP"));
        result->newVarValues[Variable::MocDir] = QStringList() << mocDirPath(exactReader, input.buildDirectory);
        result->newVarValues[Variable::ExactResource] = fileListForVar(exactSourceFiles, QLatin1String("RESOURCES"));
        result->newVarValues[Variable::CumulativeResource] = fileListForVar(cumulativeSourceFiles, QLatin1String("RESOURCES"));
        result->newVarValues[Variable::PkgConfig] = exactReader->values(QLatin1String("PKGCONFIG"));
        result->newVarValues[Variable::PrecompiledHeader] = ProFileEvaluator::sourcesToFiles(exactReader->fixifiedValues(
                    QLatin1String("PRECOMPILED_HEADER"), input.projectDir, input.buildDirectory.toString()));
        result->newVarValues[Variable::LibDirectories] = libDirectories(exactReader);
        result->newVarValues[Variable::Config] = exactReader->values(QLatin1String("CONFIG"));
        result->newVarValues[Variable::QmlImportPath] = exactReader->absolutePathValues(
                    QLatin1String("QML_IMPORT_PATH"), input.projectDir);
        result->newVarValues[Variable::QmlDesignerImportPath] = exactReader->absolutePathValues(
                    QLatin1String("QML_DESIGNER_IMPORT_PATH"), input.projectDir);
        result->newVarValues[Variable::Makefile] = exactReader->values(QLatin1String("MAKEFILE"));
        result->newVarValues[Variable::Qt] = exactReader->values(QLatin1String("QT"));
        result->newVarValues[Variable::ObjectExt] = exactReader->values(QLatin1String("QMAKE_EXT_OBJ"));
        result->newVarValues[Variable::ObjectsDir] = exactReader->values(QLatin1String("OBJECTS_DIR"));
        result->newVarValues[Variable::Version] = exactReader->values(QLatin1String("VERSION"));
        result->newVarValues[Variable::TargetExt] = exactReader->values(QLatin1String("TARGET_EXT"));
        result->newVarValues[Variable::TargetVersionExt]
                = exactReader->values(QLatin1String("TARGET_VERSION_EXT"));
        result->newVarValues[Variable::StaticLibExtension] = exactReader->values(QLatin1String("QMAKE_EXTENSION_STATICLIB"));
        result->newVarValues[Variable::ShLibExtension] = exactReader->values(QLatin1String("QMAKE_EXTENSION_SHLIB"));
        result->newVarValues[Variable::AndroidArch] = exactReader->values(QLatin1String("ANDROID_TARGET_ARCH"));
        result->newVarValues[Variable::AndroidDeploySettingsFile] = exactReader->values(QLatin1String("ANDROID_DEPLOYMENT_SETTINGS_FILE"));
        result->newVarValues[Variable::AndroidPackageSourceDir] = exactReader->values(QLatin1String("ANDROID_PACKAGE_SOURCE_DIR"));
        result->newVarValues[Variable::AndroidExtraLibs] = exactReader->values(QLatin1String("ANDROID_EXTRA_LIBS"));
        result->newVarValues[Variable::IsoIcons] = exactReader->values(QLatin1String("ISO_ICONS"));
        result->newVarValues[Variable::QmakeProjectName] = exactReader->values(QLatin1String("QMAKE_PROJECT_NAME"));
        result->newVarValues[Variable::QmakeCc] = exactReader->values("QMAKE_CC");
        result->newVarValues[Variable::QmakeCxx] = exactReader->values("QMAKE_CXX");
    }

    if (result->state == QmakeEvalResult::EvalOk || result->state == QmakeEvalResult::EvalPartial) {

        QList<QmakeIncludedPriFile *> toExtract = {&result->includedFiles};
        while (!toExtract.isEmpty()) {
            QmakeIncludedPriFile *current = toExtract.takeFirst();
            processValues(current->result);
            toExtract.append(current->children.values());
        }
    }

    if (exactBuildPassReader && exactBuildPassReader != input.readerExact)
        delete exactBuildPassReader;
    if (cumulativeBuildPassReader && cumulativeBuildPassReader != input.readerCumulative)
        delete cumulativeBuildPassReader;

    return result;
}

void QmakeProFile::asyncEvaluate(QFutureInterface<QmakeEvalResult *> &fi, QmakeEvalInput input)
{
    QmakeEvalResult *evalResult = evaluate(input);
    fi.reportResult(evalResult);
}

void QmakeProFile::applyAsyncEvaluate()
{
    applyEvaluate(m_parseFutureWatcher.result());
    m_project->decrementPendingEvaluateFutures();
}

bool sortByParserNodes(Node *a, Node *b)
{
    return a->filePath() < b->filePath();
}

void QmakeProFile::applyEvaluate(QmakeEvalResult *evalResult)
{
    QScopedPointer<QmakeEvalResult> result(evalResult);
    if (!m_readerExact)
        return;

    if (m_project->asyncUpdateState() == QmakeProject::ShuttingDown) {
        cleanupProFileReaders();
        return;
    }

    foreach (const QString &error, evalResult->errors)
        QmakeProject::proFileParseError(error);

    // we are changing what is executed in that case
    if (result->state == QmakeEvalResult::EvalFail || m_project->wasEvaluateCanceled()) {
        m_validParse = false;
        cleanupProFileReaders();
        setValidParseRecursive(false);
        setParseInProgressRecursive(false);

        if (result->state == QmakeEvalResult::EvalFail) {
            QmakeProject::proFileParseError(QCoreApplication::translate("QmakeProFile", "Error while parsing file %1. Giving up.")
                                            .arg(filePath().toUserOutput()));
            if (m_projectType == ProjectType::Invalid)
                return;

            makeEmpty();

            m_projectType = ProjectType::Invalid;
        }
        return;
    }

    qCDebug(qmakeParse()) << "QmakeProFile - updating files for file " << filePath();

    if (result->projectType != m_projectType) {
        // probably all subfiles/projects have changed anyway
        // delete files && folders && projects
        foreach (QmakePriFile *c, children()) {
            if (auto qmakeProFile = dynamic_cast<QmakeProFile *>(c)) {
                qmakeProFile->setValidParseRecursive(false);
                qmakeProFile->setParseInProgressRecursive(false);
            }
        }

        makeEmpty();
        m_projectType = result->projectType;
    }

    //
    // Add/Remove pri files, sub projects
    //

    FileName buildDirectory = buildDir();

    QList<QPair<QmakePriFile *, QmakeIncludedPriFile *>> toCompare;

    toCompare.append(qMakePair(this, &result->includedFiles));

    makeEmpty();

    while (!toCompare.isEmpty()) {
        QmakePriFile *pn = toCompare.first().first;
        QmakeIncludedPriFile *tree = toCompare.first().second;
        toCompare.pop_front();

        for (QmakeIncludedPriFile *priFile : tree->children) {
            // Loop preventation, make sure that exact same node is not in our parent chain
            bool loop = false;
            QmakePriFile *n = pn;
            while ((n = n->parent())) {
                if (n->filePath() == priFile->name) {
                    loop = true;
                    break;
                }
            }

            if (loop)
                continue; // Do nothing

            if (priFile->proFile) {
                QmakePriFile *qmakePriFileNode = new QmakePriFile(m_project, this, priFile->name);
                pn->addChild(qmakePriFileNode);
                qmakePriFileNode->setIncludedInExactParse(
                            (result->state == QmakeEvalResult::EvalOk) && pn->includedInExactParse());
                qmakePriFileNode->update(priFile->result);
                toCompare.append(qMakePair(qmakePriFileNode, priFile));
            } else {
                QmakeProFile *qmakeProFileNode = new QmakeProFile(m_project, priFile->name);
                pn->addChild(qmakeProFileNode);
                qmakeProFileNode->setIncludedInExactParse(
                            result->exactSubdirs.contains(qmakeProFileNode->filePath())
                            && pn->includedInExactParse());
                qmakeProFileNode->setParseInProgress(true);
                qmakeProFileNode->asyncUpdate();
            }
        }
    }

    QmakePriFile::update(result->includedFiles.result);

    m_validParse = (result->state == QmakeEvalResult::EvalOk);
    if (m_validParse) {
        // update TargetInformation
        m_qmakeTargetInformation = result->targetInformation;

        m_subProjectsNotToDeploy
                = Utils::transform(result->subProjectsNotToDeploy,
                                   [](const QString &s) { return FileName::fromString(s); });
        m_installsList = result->installsList;

        if (m_varValues != result->newVarValues)
            m_varValues = result->newVarValues;

        m_displayName = singleVariableValue(Variable::QmakeProjectName);
    } // result == EvalOk

    setParseInProgress(false);

    updateGeneratedFiles(buildDirectory);

    cleanupProFileReaders();
}

void QmakeProFile::cleanupProFileReaders()
{
    m_project->destroyProFileReader(m_readerExact);
    m_project->destroyProFileReader(m_readerCumulative);

    m_readerExact = nullptr;
    m_readerCumulative = nullptr;
}

QString QmakeProFile::uiDirPath(QtSupport::ProFileReader *reader, const FileName &buildDir)
{
    QString path = reader->value(QLatin1String("UI_DIR"));
    if (QFileInfo(path).isRelative())
        path = QDir::cleanPath(buildDir.toString() + QLatin1Char('/') + path);
    return path;
}

QString QmakeProFile::mocDirPath(QtSupport::ProFileReader *reader, const FileName &buildDir)
{
    QString path = reader->value(QLatin1String("MOC_DIR"));
    if (QFileInfo(path).isRelative())
        path = QDir::cleanPath(buildDir.toString() + QLatin1Char('/') + path);
    return path;
}

QString QmakeProFile::sysrootify(const QString &path, const QString &sysroot,
                                     const QString &baseDir, const QString &outputDir)
{
#ifdef Q_OS_WIN
    Qt::CaseSensitivity cs = Qt::CaseInsensitive;
#else
    Qt::CaseSensitivity cs = Qt::CaseSensitive;
#endif
    if (sysroot.isEmpty() || path.startsWith(sysroot, cs)
        || path.startsWith(baseDir, cs) || path.startsWith(outputDir, cs)) {
        return path;
    }
    QString sysrooted = QDir::cleanPath(sysroot + path);
    return !IoUtils::exists(sysrooted) ? path : sysrooted;
}

QStringList QmakeProFile::includePaths(QtSupport::ProFileReader *reader, const FileName &sysroot,
                                           const FileName &buildDir, const QString &projectDir)
{
    QStringList paths;
    foreach (const QString &cxxflags, reader->values(QLatin1String("QMAKE_CXXFLAGS"))) {
        if (cxxflags.startsWith(QLatin1String("-I")))
            paths.append(cxxflags.mid(2));
    }

    foreach (const ProFileEvaluator::SourceFile &el,
             reader->fixifiedValues(QLatin1String("INCLUDEPATH"), projectDir, buildDir.toString())) {
        paths << sysrootify(el.fileName, sysroot.toString(), projectDir, buildDir.toString());
    }
    // paths already contains moc dir and ui dir, due to corrrectly parsing uic.prf and moc.prf
    // except if those directories don't exist at the time of parsing
    // thus we add those directories manually (without checking for existence)
    paths << mocDirPath(reader, buildDir) << uiDirPath(reader, buildDir);
    paths.removeDuplicates();
    return paths;
}

QStringList QmakeProFile::libDirectories(QtSupport::ProFileReader *reader)
{
    QStringList result;
    foreach (const QString &str, reader->values(QLatin1String("LIBS"))) {
        if (str.startsWith(QLatin1String("-L")))
            result.append(str.mid(2));
    }
    return result;
}

FileNameList QmakeProFile::subDirsPaths(QtSupport::ProFileReader *reader,
                                            const QString &projectDir,
                                            QStringList *subProjectsNotToDeploy,
                                            QStringList *errors)
{
    FileNameList subProjectPaths;

    const QStringList subDirVars = reader->values(QLatin1String("SUBDIRS"));

    foreach (const QString &subDirVar, subDirVars) {
        // Special case were subdir is just an identifier:
        //   "SUBDIR = subid
        //    subid.subdir = realdir"
        // or
        //   "SUBDIR = subid
        //    subid.file = realdir/realfile.pro"

        QString realDir;
        const QString subDirKey = subDirVar + QLatin1String(".subdir");
        const QString subDirFileKey = subDirVar + QLatin1String(".file");
        if (reader->contains(subDirKey))
            realDir = reader->value(subDirKey);
        else if (reader->contains(subDirFileKey))
            realDir = reader->value(subDirFileKey);
        else
            realDir = subDirVar;
        QFileInfo info(realDir);
        if (!info.isAbsolute())
            info.setFile(projectDir + QLatin1Char('/') + realDir);
        realDir = info.filePath();

        QString realFile;
        if (info.isDir())
            realFile = QString::fromLatin1("%1/%2.pro").arg(realDir, info.fileName());
        else
            realFile = realDir;

        if (QFile::exists(realFile)) {
            realFile = QDir::cleanPath(realFile);
            subProjectPaths << FileName::fromString(realFile);
            if (subProjectsNotToDeploy && !subProjectsNotToDeploy->contains(realFile)
                    && reader->values(subDirVar + QLatin1String(".CONFIG"))
                        .contains(QLatin1String("no_default_target"))) {
                subProjectsNotToDeploy->append(realFile);
            }
        } else {
            if (errors)
                errors->append(QCoreApplication::translate("QmakeProFile", "Could not find .pro file for subdirectory \"%1\" in \"%2\".")
                               .arg(subDirVar).arg(realDir));
        }
    }

    return Utils::filteredUnique(subProjectPaths);
}

TargetInformation QmakeProFile::targetInformation(QtSupport::ProFileReader *reader,
                                                  QtSupport::ProFileReader *readerBuildPass,
                                                  const FileName &buildDir,
                                                  const FileName &projectFilePath)
{
    TargetInformation result;
    if (!reader || !readerBuildPass)
        return result;

    QStringList builds = reader->values(QLatin1String("BUILDS"));
    if (!builds.isEmpty()) {
        QString build = builds.first();
        result.buildTarget = reader->value(build + QLatin1String(".target"));
    }

    // BUILD DIR
    result.buildDir = buildDir;

    if (readerBuildPass->contains(QLatin1String("DESTDIR")))
        result.destDir = FileName::fromString(readerBuildPass->value(QLatin1String("DESTDIR")));

    // Target
    result.target = readerBuildPass->value(QLatin1String("TARGET"));
    if (result.target.isEmpty())
        result.target = projectFilePath.toFileInfo().baseName();

    result.valid = true;

    return result;
}

TargetInformation QmakeProFile::targetInformation() const
{
    return m_qmakeTargetInformation;
}

InstallsList QmakeProFile::installsList(const QtSupport::ProFileReader *reader, const QString &projectFilePath,
                                        const QString &projectDir, const QString &buildDir)
{
    InstallsList result;
    if (!reader)
        return result;
    const QStringList &itemList = reader->values(QLatin1String("INSTALLS"));
    if (itemList.isEmpty())
        return result;

    const QString installPrefix
            = reader->propertyValue(QLatin1String("QT_INSTALL_PREFIX"));
    const QString devInstallPrefix
            = reader->propertyValue(QLatin1String("QT_INSTALL_PREFIX/dev"));
    bool fixInstallPrefix = (installPrefix != devInstallPrefix);

    foreach (const QString &item, itemList) {
        bool active = !reader->values(item + QLatin1String(".CONFIG"))
                        .contains(QLatin1String("no_default_install"));
        const QString pathVar = item + QLatin1String(".path");
        const QStringList &itemPaths = reader->values(pathVar);
        if (itemPaths.count() != 1) {
            qDebug("Invalid RHS: Variable '%s' has %d values.",
                qPrintable(pathVar), itemPaths.count());
            if (itemPaths.isEmpty()) {
                qDebug("%s: Ignoring INSTALLS item '%s', because it has no path.",
                    qPrintable(projectFilePath), qPrintable(item));
                continue;
            }
        }

        QString itemPath = itemPaths.last();
        if (fixInstallPrefix && itemPath.startsWith(installPrefix)) {
            // This is a hack for projects which install into $$[QT_INSTALL_*],
            // in particular Qt itself, examples being most relevant.
            // Projects which implement their own install path policy must
            // parametrize their INSTALLS themselves depending on the intended
            // installation/deployment mode.
            itemPath.replace(0, installPrefix.length(), devInstallPrefix);
        }
        if (item == QLatin1String("target")) {
            if (active)
                result.targetPath = itemPath;
        } else {
            const auto &itemFiles = reader->fixifiedValues(
                        item + QLatin1String(".files"), projectDir, buildDir);
            result.items << InstallsItem(itemPath, itemFiles, active);
        }
    }
    return result;
}

InstallsList QmakeProFile::installsList() const
{
    return m_installsList;
}

FileName QmakeProFile::sourceDir() const
{
    return directoryPath();
}

FileName QmakeProFile::buildDir(QmakeBuildConfiguration *bc) const
{
    const QDir srcDirRoot = QDir(m_project->projectDirectory().toString());
    const QString relativeDir = srcDirRoot.relativeFilePath(directoryPath().toString());
    if (!bc && m_project->activeTarget())
        bc = static_cast<QmakeBuildConfiguration *>(m_project->activeTarget()->activeBuildConfiguration());
    if (!bc)
        return { };
    return FileName::fromString(QDir::cleanPath(QDir(bc->buildDirectory().toString()).absoluteFilePath(relativeDir)));
}

FileNameList QmakeProFile::generatedFiles(const FileName &buildDir,
                                          const FileName &sourceFile,
                                          const FileType &sourceFileType) const
{
    // The mechanism for finding the file names is rather crude, but as we
    // cannot parse QMAKE_EXTRA_COMPILERS and qmake has facilities to put
    // ui_*.h files into a special directory, or even change the .h suffix, we
    // cannot help doing this here.

    if (sourceFileType == FileType::Form) {
        FileName location;
        auto it = m_varValues.constFind(Variable::UiDir);
        if (it != m_varValues.constEnd() && !it.value().isEmpty())
            location = FileName::fromString(it.value().front());
        else
            location = buildDir;
        if (location.isEmpty())
            return { };
        location.appendPath(QLatin1String("ui_")
                            + sourceFile.toFileInfo().completeBaseName()
                            + singleVariableValue(Variable::HeaderExtension));
        return { Utils::FileName::fromString(QDir::cleanPath(location.toString())) };
    } else if (sourceFileType == FileType::StateChart) {
        if (buildDir.isEmpty())
            return { };
        FileName location = buildDir;
        location.appendPath(sourceFile.toFileInfo().completeBaseName());
        FileName header = location;
        header.appendString(singleVariableValue(Variable::HeaderExtension));
        FileName cpp = location;
        cpp.appendString(singleVariableValue(Variable::CppExtension));

        return { header, cpp };
    }
    return { };
}

QList<ExtraCompiler *> QmakeProFile::extraCompilers() const
{
    return m_extraCompilers;
}

void QmakeProFile::setupExtraCompiler(const FileName &buildDir,
                                       const FileType &fileType, ExtraCompilerFactory *factory)
{
    foreach (const FileName &fn, files(fileType)) {
        const FileNameList generated = generatedFiles(buildDir, fn, fileType);
        if (!generated.isEmpty()) {
            m_extraCompilers.append(factory->create(m_project, fn, generated));
        }
    }
}

void QmakeProFile::updateGeneratedFiles(const FileName &buildDir)
{
    // We can do this because other plugins are not supposed to keep the compilers around.
    qDeleteAll(m_extraCompilers);
    m_extraCompilers.clear();

    // Only those project types can have generated files for us
    if (m_projectType != ProjectType::ApplicationTemplate
            && m_projectType != ProjectType::SharedLibraryTemplate
            && m_projectType != ProjectType::StaticLibraryTemplate) {
        return;
    }

    const QList<ExtraCompilerFactory *> factories =
            ProjectExplorer::ExtraCompilerFactory::extraCompilerFactories();

    ExtraCompilerFactory *formFactory
            = Utils::findOrDefault(factories, Utils::equal(&ExtraCompilerFactory::sourceType, FileType::Form));
    if (formFactory)
        setupExtraCompiler(buildDir, FileType::Form, formFactory);
    ExtraCompilerFactory *scxmlFactory
            = Utils::findOrDefault(factories, Utils::equal(&ExtraCompilerFactory::sourceType, FileType::StateChart));
    if (scxmlFactory)
        setupExtraCompiler(buildDir, FileType::StateChart, scxmlFactory);
}
