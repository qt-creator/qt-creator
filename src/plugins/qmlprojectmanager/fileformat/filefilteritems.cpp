#include "filefilteritems.h"
#include <qdebug.h>
#include <QtGui/QImageReader>

namespace QmlProjectManager {

FileFilterBaseItem::FileFilterBaseItem(QObject *parent) :
        QmlProjectContentItem(parent),
        m_recursive(false)
{
    connect(&m_fsWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(updateFileList()));
    connect(&m_fsWatcher, SIGNAL(fileChanged(QString)), this, SLOT(updateFileList()));
}

QString FileFilterBaseItem::directory() const
{
    return m_rootDir;
}

void FileFilterBaseItem::setDirectory(const QString &dirPath)
{
    if (m_rootDir == dirPath)
        return;
    m_rootDir = dirPath;
    emit directoryChanged();

    updateFileList();
}

void FileFilterBaseItem::setDefaultDirectory(const QString &dirPath)
{
    if (m_defaultDir == dirPath)
        return;
    m_defaultDir = dirPath;

    updateFileList();
}

QString FileFilterBaseItem::filter() const
{
    return m_filter;
}

void FileFilterBaseItem::setFilter(const QString &filter)
{
    if (filter == m_filter)
        return;
    m_filter = filter;
    m_regex.setPattern(m_filter);
    m_regex.setPatternSyntax(QRegExp::Wildcard);

    emit filterChanged();
    updateFileList();
}

bool FileFilterBaseItem::recursive() const
{
    return m_recursive;
}

void FileFilterBaseItem::setRecursive(bool recursive)
{
    if (recursive == m_recursive)
        return;
    m_recursive = recursive;
    updateFileList();
}

QString FileFilterBaseItem::pathsProperty() const
{
    return QStringList(m_explicitFiles.toList()).join(",");
}

void FileFilterBaseItem::setPathsProperty(const QString &path)
{
    // we support listening paths both in an array, and in one string
    m_explicitFiles.clear();
    foreach (const QString &subpath, path.split(QLatin1Char(','), QString::SkipEmptyParts)) {
        m_explicitFiles += subpath.trimmed();
    }
    updateFileList();
}

QStringList FileFilterBaseItem::files() const
{
    return m_files.toList();
}

QString FileFilterBaseItem::absoluteDir() const
{
    QString absoluteDir;
    if (QFileInfo(m_rootDir).isAbsolute()) {
        absoluteDir = m_rootDir;
    } else if (!m_defaultDir.isEmpty()) {
        absoluteDir = m_defaultDir + QLatin1Char('/') + m_rootDir;
    }

    return absoluteDir;
}

void FileFilterBaseItem::updateFileList()
{
    const QString projectDir = absoluteDir();
    if (projectDir.isEmpty())
        return;

    QSet<QString> dirsToBeWatched;
    QSet<QString> newFiles;
    foreach (const QString &explicitPath, m_explicitFiles) {
        if (QFileInfo(explicitPath).isAbsolute()) {
            newFiles << explicitPath;
        } else {
            newFiles << QDir(projectDir).absoluteFilePath(explicitPath);
        }
    }
    if (m_regex.isValid() && m_explicitFiles.isEmpty())
        newFiles += filesInSubTree(QDir(m_defaultDir), QDir(projectDir), &dirsToBeWatched);

    if (newFiles != m_files) {
        // update watched files
        const QSet<QString> unwatchFiles = QSet<QString>(m_files - newFiles);
        const QSet<QString> watchFiles = QSet<QString>(newFiles - m_files);
        if (!unwatchFiles.isEmpty())
            m_fsWatcher.removePaths(unwatchFiles.toList());
        if (!watchFiles.isEmpty())
            m_fsWatcher.addPaths(QSet<QString>(newFiles - m_files).toList());

        m_files = newFiles;

        emit filesChanged();
    }

    // update watched directories
    const QSet<QString> watchedDirectories = m_fsWatcher.directories().toSet();
    const QSet<QString> unwatchDirs = watchedDirectories - dirsToBeWatched;
    const QSet<QString> watchDirs = dirsToBeWatched - watchedDirectories;

    if (!unwatchDirs.isEmpty())
        m_fsWatcher.removePaths(unwatchDirs.toList());
    if (!watchDirs.isEmpty())
        m_fsWatcher.addPaths(watchDirs.toList());
}

QSet<QString> FileFilterBaseItem::filesInSubTree(const QDir &rootDir, const QDir &dir, QSet<QString> *parsedDirs)
{
    QSet<QString> fileSet;

    if (parsedDirs)
        parsedDirs->insert(dir.absolutePath());

    foreach (const QFileInfo &file, dir.entryInfoList(QDir::Files)) {
        if (m_regex.exactMatch(file.fileName())) {
            fileSet.insert(file.absoluteFilePath());
        }
    }

    if (m_recursive) {
        foreach (const QFileInfo &subDir, dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            fileSet += filesInSubTree(rootDir, QDir(subDir.absoluteFilePath()), parsedDirs);
        }
    }
    return fileSet;
}

QmlFileFilterItem::QmlFileFilterItem(QObject *parent)
    : FileFilterBaseItem(parent)
{
    setFilter(QLatin1String("*.qml"));
}


JsFileFilterItem::JsFileFilterItem(QObject *parent)
    : FileFilterBaseItem(parent)
{
    setFilter(QLatin1String("*.js"));
}

ImageFileFilterItem::ImageFileFilterItem(QObject *parent)
    : FileFilterBaseItem(parent)
{
    QString filter;
    // supported image formats according to
    QList<QByteArray> extensions = QImageReader::supportedImageFormats();
    foreach (const QByteArray &extension, extensions) {
        filter.append(QString("*.%1;").arg(QString::fromAscii(extension)));
    }
    setFilter(filter);
}

} // namespace QmlProjectManager

QML_DEFINE_TYPE(QmlProject,1,0,QmlFiles,QmlProjectManager::QmlFileFilterItem)
QML_DEFINE_TYPE(QmlProject,1,0,JavaScriptFiles,QmlProjectManager::JsFileFilterItem)
QML_DEFINE_TYPE(QmlProject,1,0,ImageFiles,QmlProjectManager::ImageFileFilterItem)
