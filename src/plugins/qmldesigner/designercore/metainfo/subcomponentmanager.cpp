// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "subcomponentmanager.h"
#include "metainforeader.h"

#include <externaldependenciesinterface.h>
#include <invalidmetainfoexception.h>
#include <model.h>
#include <qmldesignerconstants.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <coreplugin/messagebox.h>

#include <QDir>
#include <QDirIterator>
#include <QMessageBox>
#include <QUrl>

#include <qmljs/qmljslink.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsengine_p.h>
#include <qmljs/qmljsmodelmanagerinterface.h>

enum { debug = false };

QT_BEGIN_NAMESPACE

// Allow usage of QFileInfo in Utils::sort

static bool operator<(const QFileInfo &file1, const QFileInfo &file2)
{
    return file1.filePath() < file2.filePath();
}

QT_END_NAMESPACE

namespace QmlDesigner {
static const QString s_qmlFilePattern = QStringLiteral("*.qml");
constexpr const char AppFileName[] = "App.qml";

SubComponentManager::SubComponentManager(Model *model,
                                         ExternalDependenciesInterface &externalDependencies)
    : m_model(model)
    , m_externalDependencies{externalDependencies}
{
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged,
            this, [this](const QString &path) { parseDirectory(path); });
}

QString findFolderForImport(const QStringList &importPaths, const QString &url)
{
    if (url.isEmpty())
        return {};

    QString folderUrl = url;
    folderUrl.replace('.', '/');

    for (const QString &path : importPaths) {
        QFileInfo dirInfo = QFileInfo(path + QLatin1Char('/') + folderUrl);

        if (dirInfo.exists() && dirInfo.isDir())
            return dirInfo.canonicalFilePath();

        const QDir parentDir = dirInfo.dir();
        if (parentDir.exists()) {
            const QStringList parts = url.split('.');
            const QString lastFolder = parts.last();

            const QStringList candidates = parentDir.entryList({lastFolder + ".*"}, QDir::Dirs);
            if (!candidates.isEmpty())
                return parentDir.canonicalPath() + "/" + candidates.first();
        }
    }
    return {};
}
bool SubComponentManager::addImport(const Import &import, int index)
{
    if (debug) {
        qDebug() << Q_FUNC_INFO << index << import.file().toUtf8();
        qDebug() << Q_FUNC_INFO << index << import.url();
    }

    bool importExists = false;
    if (import.isFileImport()) {
        QFileInfo dirInfo = QFileInfo(m_filePath.resolved(import.file()).toLocalFile());
        if (dirInfo.exists() && dirInfo.isDir()) {
            const QString canonicalDirPath = dirInfo.canonicalFilePath();
            m_watcher.addPath(canonicalDirPath);
            importExists = true;
            //m_dirToQualifier.insertMulti(canonicalDirPath, import.qualifier()); ### todo: proper support for import as
        }
    } else {
        QString url = import.url();
        const QString result = findFolderForImport(importPaths(), url);
        if (!result.isEmpty()) {
            m_watcher.addPath(result);
            importExists = true;
        }
    }

    if (importExists) {
        if (index == -1  || index > m_imports.size())
            m_imports.append(import);
        else
            m_imports.insert(index, import);
    }

    return importExists;
}

void SubComponentManager::removeImport(int index)
{
    const Import import = m_imports.takeAt(index);

    if (import.isFileImport()) {
        const QFileInfo dirInfo = QFileInfo(m_filePath.resolved(import.file()).toLocalFile());
        const QString canonicalDirPath = dirInfo.canonicalFilePath();

        //m_dirToQualifier.remove(canonicalDirPath, import.qualifier()); ### todo: proper support for import as

        if (!m_dirToQualifier.contains(canonicalDirPath))
            m_watcher.removePath(canonicalDirPath);

//        const QList<QFileInfo> files = watchedFiles(canonicalDirPath);
//        for (const QFileInfo &monitoredFile : files) { ### todo: proper support for import as
//            if (!m_dirToQualifier.contains(canonicalDirPath))
//                unregisterQmlFile(monitoredFile, import.qualifier());
//        }
    } else {
            // TODO: QDeclarativeDomImport::Library
    }
}

void SubComponentManager::parseDirectories()
{
    if (!m_filePath.isEmpty()) {
        const QString file = m_filePath.toLocalFile();
        QFileInfo dirInfo = QFileInfo(QFileInfo(file).path());
        const QString canonicalPath = dirInfo.canonicalFilePath();
        if (dirInfo.exists() && dirInfo.isDir())
            parseDirectory(canonicalPath);

        const QStringList subDirs = QDir(QFileInfo(file).path()).entryList(QDir::Dirs | QDir::NoDot
                                                                           | QDir::NoDotDot);
        for (const QString &subDir : subDirs) {
            const QString canSubPath = canonicalPath + QLatin1Char('/') + subDir;
            parseDirectory(canSubPath, true, resolveDirQualifier(canSubPath));
        }
    }

    const QStringList assetPaths = quick3DAssetPaths();
    for (const auto &assetPath : assetPaths)
        parseDirectory(assetPath);

    for (const Import &import : std::as_const(m_imports)) {
        if (import.isFileImport()) {
            QFileInfo dirInfo = QFileInfo(m_filePath.resolved(import.file()).toLocalFile());
            if (dirInfo.exists() && dirInfo.isDir()) {
                const QString canPath = dirInfo.canonicalFilePath();
                parseDirectory(canPath, true, resolveDirQualifier(canPath));
            }
        } else {
            QString url = import.url();
            url.replace(QLatin1Char('.'), QLatin1Char('/'));
            QFileInfo dirInfo = QFileInfo(url);
            const QStringList paths = importPaths();
            for (const QString &path : paths) {
                QString fullUrl  = path + QLatin1Char('/') + url;
                dirInfo = QFileInfo(fullUrl);

                if (dirInfo.exists() && dirInfo.isDir()) {
                    //### todo full qualified names QString nameSpace = import.uri();
                    parseDirectory(dirInfo.canonicalFilePath(), false);
                }

                QString fullUrlVersion = path + QLatin1Char('/') + url + QLatin1Char('.') + import.version().split('.').constFirst();
                dirInfo = QFileInfo(fullUrlVersion);

                if (dirInfo.exists() && dirInfo.isDir()) {
                    //### todo full qualified names QString nameSpace = import.uri();
                    parseDirectory(dirInfo.canonicalFilePath(), false);
                }
            }
        }
    }
}

void SubComponentManager::parseDirectory(const QString &canonicalDirPath, bool addToLibrary, const TypeName &qualification)
{
    if (!model() || !model()->rewriterView())
        return;

    if (canonicalDirPath.endsWith(QLatin1String(Constants::QUICK_3D_ASSETS_FOLDER))) {
        parseQuick3DAssetsDir(canonicalDirPath);
        return;
    }

    QDir designerDir(canonicalDirPath + QLatin1String(Constants::QML_DESIGNER_SUBFOLDER));
    if (designerDir.exists()) {
        QStringList filter;
        filter << QLatin1String("*.metainfo");
        designerDir.setNameFilters(filter);

        QStringList metaFiles = designerDir.entryList(QDir::Files);
        const QFileInfoList metaInfoList = designerDir.entryInfoList(QDir::Files);
        for (const QFileInfo &metaInfoFile : metaInfoList) {
            if (model() && model()->metaInfo().itemLibraryInfo()) {
                Internal::MetaInfoReader reader(model()->metaInfo());
                reader.setQualifcation(qualification);
                try {
                    reader.readMetaInfoFile(metaInfoFile.absoluteFilePath(), true);
                } catch (const InvalidMetaInfoException &e) {
                    qWarning() << e.description();
                    const QString errorMessage = metaInfoFile.absoluteFilePath() + QLatin1Char('\n') + QLatin1Char('\n') + reader.errors().join(QLatin1Char('\n'));
                    Core::AsynchronousMessageBox::warning(QCoreApplication::translate("SubComponentManager::parseDirectory", "Invalid meta info"),
                                                           errorMessage);
                }
            }
        }
        if (!metaFiles.isEmpty())
            return;
    }

    if (debug)
        qDebug() << Q_FUNC_INFO << canonicalDirPath;

    QDir dir(canonicalDirPath);

    dir.setNameFilters(QStringList(s_qmlFilePattern));
    dir.setFilter(QDir::Files | QDir::Readable | QDir::CaseSensitive);

    QFileInfoList monitoredList = watchedFiles(canonicalDirPath);
    QFileInfoList newList;
    const QFileInfoList qmlFileList = dir.entryInfoList();
    const QString appFilePath = m_filePathDir.absoluteFilePath(AppFileName);

    for (const QFileInfo &qmlFile : qmlFileList) {
        if (QFileInfo(m_filePath.toLocalFile()) == qmlFile) {
            // do not parse main file
            continue;
        }

        if (qmlFile.absoluteFilePath() == appFilePath)
            continue;

        if (!qmlFile.fileName().at(0).isUpper()) {
            // QML sub components must be upper case
            continue;
        }
        newList << qmlFile;
    }

    Utils::sort(monitoredList);
    Utils::sort(newList);

    if (debug)
        qDebug() << "monitored list " << monitoredList.size() << "new list " << newList.size();
    auto oldIter = monitoredList.constBegin();
    auto newIter = newList.constBegin();

    while (oldIter != monitoredList.constEnd() && newIter != newList.constEnd()) {
        const QFileInfo oldFileInfo = *oldIter;
        const QFileInfo newFileInfo = *newIter;
        if (oldFileInfo == newFileInfo) {
            ++oldIter;
            ++newIter;
            continue;
        }
        if (oldFileInfo < newFileInfo) {
            const QString qualifiers = m_dirToQualifier.value(canonicalDirPath);
            for (const QChar &qualifier : qualifiers)
                unregisterQmlFile(oldFileInfo, qualifier);
            m_watcher.removePath(oldFileInfo.filePath());
            ++oldIter;
            continue;
        }
        // oldFileInfo > newFileInfo
        parseFile(newFileInfo.filePath(), addToLibrary, QString::fromUtf8(qualification));
        ++newIter;
    }

    while (oldIter != monitoredList.constEnd()) {
        const QString qualifiers = m_dirToQualifier.value(canonicalDirPath);
        for (const QChar &qualifier : qualifiers)
            unregisterQmlFile(*oldIter, qualifier);
        ++oldIter;
    }

    while (newIter != newList.constEnd()) {
        parseFile(newIter->filePath(), addToLibrary, QString::fromUtf8(qualification));
        if (debug)
            qDebug() << "m_watcher.addPath(" << newIter->filePath() << ')';
        ++newIter;
    }
}

void SubComponentManager::parseFile(const QString &canonicalFilePath, bool addToLibrary, const QString &qualification)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << canonicalFilePath;

    QFile file(canonicalFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    const QFileInfo fi(canonicalFilePath);
    const QString dir = fi.path();
    const QStringList qualifiers = m_dirToQualifier.values(dir);
    for (const QString &qualifier : qualifiers) {
        registerQmlFile(fi, qualifier, addToLibrary);
    }
    registerQmlFile(fi, qualification, addToLibrary);
}

void SubComponentManager::parseFile(const QString &canonicalFilePath)
{
    parseFile(canonicalFilePath, true, QString());
}

// dirInfo must already contain a canonical path
QFileInfoList SubComponentManager::watchedFiles(const QString &canonicalDirPath)
{
    QFileInfoList files;

    const QStringList monitoredFiles = m_watcher.files();
    for (const QString &monitoredFile : monitoredFiles) {
        QFileInfo fileInfo(monitoredFile);
        if (fileInfo.dir().absolutePath() == canonicalDirPath)
            files.append(fileInfo);
    }
    return files;
}

void SubComponentManager::unregisterQmlFile(const QFileInfo &fileInfo, const QString &qualifier)
{
    QString componentName = fileInfo.baseName();
    if (!qualifier.isEmpty())
        componentName = qualifier + QLatin1Char('.') + componentName;
}


void SubComponentManager::registerQmlFile(const QFileInfo &fileInfo, const QString &qualifier,
                                          bool addToLibrary)
{
    if (!addToLibrary || !model() || fileInfo.path().contains(QLatin1String(Constants::QUICK_3D_ASSETS_FOLDER)))
        return;

    QString componentName = fileInfo.baseName();
    const QString baseComponentName = componentName;

    QString fixedQualifier = qualifier;
    if (!qualifier.isEmpty()) {
        fixedQualifier = qualifier;
        if (qualifier.right(1) == QStringLiteral("."))
            fixedQualifier.chop(1); //remove last char if it is a dot
        componentName = fixedQualifier + QLatin1Char('.') + componentName;
    }

    if (debug)
        qDebug() << "SubComponentManager" << __FUNCTION__ << componentName;

    // Add file components to the library
    ItemLibraryEntry itemLibraryEntry;
    itemLibraryEntry.setType(componentName.toUtf8());
    itemLibraryEntry.setName(baseComponentName);
    itemLibraryEntry.setCategory(m_externalDependencies.itemLibraryImportUserComponentsTitle());
    itemLibraryEntry.setCustomComponentSource(fileInfo.absoluteFilePath());
    if (!qualifier.isEmpty())
        itemLibraryEntry.setRequiredImport(fixedQualifier);

    if (!model()->metaInfo().itemLibraryInfo()->containsEntry(itemLibraryEntry))
        model()->metaInfo().itemLibraryInfo()->addEntries({itemLibraryEntry});
}

Model *SubComponentManager::model() const
{
    return m_model.data();
}

QStringList SubComponentManager::importPaths() const
{
    if (model())
        return model()->importPaths();

    return {};
}

void SubComponentManager::parseQuick3DAssetsDir(const QString &quick3DAssetsPath)
{
    QDir quick3DAssetsDir(quick3DAssetsPath);
    QStringList assets = quick3DAssetsDir.entryList(QDir::Dirs | QDir::NoDot | QDir::NoDotDot);
    for (QString &asset : assets)
        asset.prepend(QString(Constants::QUICK_3D_ASSETS_FOLDER).mid(1) + '.');

    // Create item library entries for Quick3D assets that are imported by document
    for (auto &import : std::as_const(m_imports)) {
        if (import.isLibraryImport() && assets.contains(import.url())) {
            assets.removeOne(import.url());
            parseQuick3DAssetsItem(import.url(), quick3DAssetsPath);
        }
    }
}

// parse one asset folder under Quick3DAssets
void SubComponentManager::parseQuick3DAssetsItem(const QString &importUrl, const QString &quick3DAssetsPath)
{
    QString assetPath;
    if (!quick3DAssetsPath.isEmpty()) {
        assetPath = QDir(quick3DAssetsPath).filePath(importUrl.split('.').last());
    } else {
        // search for quick3DAssetsPath if not provided
        const auto assetPaths = quick3DAssetPaths();
        for (const auto &quick3DAssetPath : assetPaths) {
            QString path = QDir(quick3DAssetPath).filePath(importUrl.split('.').last());
            if (QFileInfo::exists(path)) {
                assetPath = path;
                break;
            }
        }
    }

    const QString defaultIconPath = QStringLiteral(":/ItemLibrary/images/item-3D_model-icon.png");
    QDirIterator qmlIt(assetPath, {"*.qml"}, QDir::Files);
    while (qmlIt.hasNext()) {
        qmlIt.next();
        const QString name = qmlIt.fileInfo().baseName();
        const QString type = importUrl + '.' + name;
        // For now we assume version is always 1.0 as that's what importer hardcodes
        ItemLibraryEntry itemLibraryEntry;
        itemLibraryEntry.setType(type.toUtf8(), 1, 0);
        itemLibraryEntry.setName(name);
        itemLibraryEntry.setCategory(::QmlDesigner::SubComponentManager::tr("My 3D Components"));
        itemLibraryEntry.setCustomComponentSource(qmlIt.fileInfo().absoluteFilePath());
        itemLibraryEntry.setRequiredImport(importUrl);
        itemLibraryEntry.setTypeIcon(QIcon(defaultIconPath));

        // load hints file if exists
        QFile hintsFile(qmlIt.fileInfo().absolutePath() + '/' + name + ".hints");
        if (hintsFile.exists() && hintsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&hintsFile);
            QHash<QString, QString> hints;
            while (!in.atEnd()) {
                QStringList hint = in.readLine().split(':');
                hints.insert(hint[0].trimmed(), hint[1].trimmed());
            }
            itemLibraryEntry.addHints(hints);
        }

        model()->metaInfo().itemLibraryInfo()->addEntries({itemLibraryEntry}, true);
    }
}

QStringList SubComponentManager::quick3DAssetPaths() const
{
    const auto impPaths = importPaths();
    QStringList retPaths;
    for (const auto &impPath : impPaths) {
        const QString assetPath = impPath + QLatin1String(Constants::QUICK_3D_ASSETS_FOLDER);
        if (QFileInfo::exists(assetPath))
            retPaths << assetPath;
    }
    return retPaths;
}

TypeName SubComponentManager::resolveDirQualifier(const QString &dirPath) const
{
    return m_filePathDir.relativeFilePath(dirPath).toUtf8();
}

/*!
  \class SubComponentManager

  Detects & monitors (potential) component files in a list of directories, and registers
  these in the metatype system.
*/

QStringList SubComponentManager::directories() const
{
    return m_watcher.directories();
}

QStringList SubComponentManager::qmlFiles() const
{
    return m_watcher.files();
}

void SubComponentManager::update(const QUrl &filePath, const Imports &imports)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << filePath << imports.size();

    QFileInfo oldDir, newDir;

    if (!m_filePath.isEmpty()) {
        const QString file = m_filePath.toLocalFile();
        oldDir = QFileInfo(QFileInfo(file).path());
        m_filePathDir = QDir();
    }
    if (!filePath.isEmpty()) {
        const QString file = filePath.toLocalFile();
        newDir = QFileInfo(QFileInfo(file).path());
        m_filePathDir.setPath(newDir.absoluteFilePath());
    }

    m_filePath = filePath;

    //
    // (implicit) import of local directory
    //
    if (oldDir != newDir) {
        if (!oldDir.filePath().isEmpty()) {
            m_dirToQualifier.remove(oldDir.canonicalFilePath(), QString());
            if (!m_dirToQualifier.contains(oldDir.canonicalFilePath()))
                m_watcher.removePath(oldDir.filePath());

            // Remove old watched asset paths
            const QStringList watchPaths = m_watcher.directories();
            const QString &quick3DAssetFolder = QLatin1String(Constants::QUICK_3D_ASSETS_FOLDER);
            for (const auto &watchPath : watchPaths) {
                if (watchPath.endsWith(quick3DAssetFolder))
                    m_watcher.removePath(watchPath);
            }
        }

        if (!newDir.filePath().isEmpty())
            m_dirToQualifier.insert(newDir.canonicalFilePath(), QString());
    }

    //
    // Imports
    //

    // skip first list items until the lists differ
    int i = 0;
    while (i < qMin(imports.size(), m_imports.size())) {
        if (!(imports.at(i) == m_imports.at(i)))
            break;
        ++i;
    }

    for (int ii = m_imports.size() - 1; ii >= i; --ii)
        removeImport(ii);

    for (int ii = i; ii < imports.size(); ++ii) {
        addImport(imports.at(ii), ii);
    }

    const QString newPath = newDir.absoluteFilePath();
    m_watcher.addPath(newPath);

    // Watch existing asset paths, including a global ones if they exist
    const auto assetPaths = quick3DAssetPaths();
    for (const auto &assetPath : assetPaths)
        m_watcher.addPath(assetPath);

    parseDirectories();
}

void SubComponentManager::addAndParseImport(const Import &import)
{
    for (const auto &existingImport : std::as_const(m_imports)) {
        if (import == existingImport)
            return;
    }

    if (!addImport(import))
        return;

    if (import.isFileImport()) {
        QFileInfo dirInfo = QFileInfo(m_filePath.resolved(import.file()).toLocalFile());
        if (dirInfo.exists() && dirInfo.isDir()) {
            const QString canPath = dirInfo.canonicalFilePath();
            parseDirectory(canPath, true, resolveDirQualifier(canPath));
        }
    } else {
        QString url = import.url();

        if (url.startsWith(QString(Constants::QUICK_3D_ASSETS_FOLDER).mid(1))) {
            parseQuick3DAssetsItem(import.url());
            return;
        }
        url.replace('.', '/');
        QFileInfo dirInfo = QFileInfo(url);
        const QStringList importPathList = importPaths();
        bool parsed = false;
        for (const QString &path : importPathList) {
            QString fullUrl  = path + '/' + url;
            dirInfo = QFileInfo(fullUrl);

            if (dirInfo.exists() && dirInfo.isDir()) {
                parseDirectory(dirInfo.canonicalFilePath(), false);
                parsed = true;
            }

            QString fullUrlVersion = path + '/' + url + '.' + import.version().split('.').constFirst();
            dirInfo = QFileInfo(fullUrlVersion);

            if (dirInfo.exists() && dirInfo.isDir()) {
                parseDirectory(dirInfo.canonicalFilePath(), false);
                parsed = true;
            }

            if (parsed)
                break;
        }
    }
}

} // namespace QmlDesigner

