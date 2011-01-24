#include "filefilteritems.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtGui/QImageReader>

namespace QmlProjectManager {

FileFilterBaseItem::FileFilterBaseItem(QObject *parent) :
        QmlProjectContentItem(parent),
        m_recurse(RecurseDefault)
{
    m_updateFileListTimer.setSingleShot(true);
    m_updateFileListTimer.setInterval(50);
    connect(&m_dirWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(updateFileList()));
    connect(&m_updateFileListTimer, SIGNAL(timeout()), this, SLOT(updateFileListNow()));
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

    updateFileListNow();
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

    m_regExpList.clear();
    m_fileSuffixes.clear();

    foreach (const QString &pattern, filter.split(QLatin1Char(';'))) {
        if (pattern.isEmpty())
            continue;
        // decide if it's a canonical pattern like *.x
        if (pattern.startsWith(QLatin1String("*."))) {
            const QString suffix = pattern.right(pattern.size() - 1);
            if (!suffix.contains(QLatin1Char('*'))
                    && !suffix.contains(QLatin1Char('?'))
                    && !suffix.contains(QLatin1Char('['))) {
                m_fileSuffixes << suffix;
                continue;
            }
        }
        m_regExpList << QRegExp(pattern, Qt::CaseInsensitive, QRegExp::Wildcard);
    }

    updateFileList();
}

bool FileFilterBaseItem::recursive() const
{
    bool recursive;
    if (m_recurse == Recurse) {
        recursive = true;
    } else if (m_recurse == DoNotRecurse) {
        recursive = false;
    } else {  // RecurseDefault
        if (m_explicitFiles.isEmpty()) {
            recursive = true;
        } else {
            recursive = false;
        }
    }
    return recursive;
}

void FileFilterBaseItem::setRecursive(bool recurse)
{
    bool oldRecursive = recursive();

    if (recurse) {
        m_recurse = Recurse;
    } else {
            m_recurse = DoNotRecurse;
    }

    if (recurse != oldRecursive)
        updateFileList();
}

QStringList FileFilterBaseItem::pathsProperty() const
{
    return m_explicitFiles;
}

void FileFilterBaseItem::setPathsProperty(const QStringList &path)
{
    m_explicitFiles = path;
    updateFileList();
}

QStringList FileFilterBaseItem::files() const
{
    return m_files.toList();
}

/**
  Check whether filter matches a file path - regardless whether the file already exists or not.

  @param filePath: absolute file path
  */
bool FileFilterBaseItem::matchesFile(const QString &filePath) const
{
    foreach (const QString &explicitFile, m_explicitFiles) {
        if (absolutePath(explicitFile) == filePath)
            return true;
    }

    const QString &fileName = QFileInfo(filePath).fileName();

    if (!fileMatches(fileName))
        return false;

    const QDir fileDir = QFileInfo(filePath).absoluteDir();
    foreach (const QString &watchedDirectory, m_dirWatcher.directories()) {
        if (QDir(watchedDirectory) == fileDir)
            return true;
    }

    return false;
}

QString FileFilterBaseItem::absolutePath(const QString &path) const
{
    if (QFileInfo(path).isAbsolute())
        return path;
    return QDir(absoluteDir()).absoluteFilePath(path);
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
    if (!m_updateFileListTimer.isActive())
        m_updateFileListTimer.start();
}

void FileFilterBaseItem::updateFileListNow()
{
    if (m_updateFileListTimer.isActive())
        m_updateFileListTimer.stop();

    const QString projectDir = absoluteDir();
    if (projectDir.isEmpty())
        return;

    QSet<QString> dirsToBeWatched;
    QSet<QString> newFiles;
    foreach (const QString &explicitPath, m_explicitFiles) {
        newFiles << absolutePath(explicitPath);
    }
    if ((!m_fileSuffixes.isEmpty() || !m_regExpList.isEmpty()) && m_explicitFiles.isEmpty())
        newFiles += filesInSubTree(QDir(m_defaultDir), QDir(projectDir), &dirsToBeWatched);

    if (newFiles != m_files) {
        QSet<QString> addedFiles = newFiles;
        QSet<QString> removedFiles = m_files;
        QSet<QString> unchanged = newFiles;
        unchanged.intersect(m_files);
        addedFiles.subtract(unchanged);
        removedFiles.subtract(unchanged);

        m_files = newFiles;
        emit filesChanged(addedFiles, removedFiles);
    }

    // update watched directories
    const QSet<QString> oldDirs = m_dirWatcher.directories().toSet();
    const QSet<QString> unwatchDirs = oldDirs - dirsToBeWatched;
    const QSet<QString> watchDirs = dirsToBeWatched - oldDirs;

    if (!unwatchDirs.isEmpty())
        m_dirWatcher.removeDirectories(unwatchDirs.toList());
    if (!watchDirs.isEmpty())
        m_dirWatcher.addDirectories(watchDirs.toList());
}

bool FileFilterBaseItem::fileMatches(const QString &fileName) const
{
    foreach (const QString &suffix, m_fileSuffixes) {
        if (fileName.endsWith(suffix, Qt::CaseInsensitive)) {
            return true;
        }
    }

    foreach (const QRegExp &filter, m_regExpList) {
        if (filter.exactMatch(fileName)) {
            return true;
        }
    }

    return false;
}

QSet<QString> FileFilterBaseItem::filesInSubTree(const QDir &rootDir, const QDir &dir, QSet<QString> *parsedDirs)
{
    QSet<QString> fileSet;

    if (parsedDirs)
        parsedDirs->insert(dir.absolutePath());

    foreach (const QFileInfo &file, dir.entryInfoList(QDir::Files)) {
        const QString fileName = file.fileName();

        if (fileMatches(fileName))
            fileSet.insert(file.absoluteFilePath());
    }

    if (recursive()) {
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

void JsFileFilterItem::setFilter(const QString &filter)
{
    FileFilterBaseItem::setFilter(filter);
    emit filterChanged();
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

void ImageFileFilterItem::setFilter(const QString &filter)
{
    FileFilterBaseItem::setFilter(filter);
    emit filterChanged();
}

CssFileFilterItem::CssFileFilterItem(QObject *parent)
    : FileFilterBaseItem(parent)
{
    setFilter(QLatin1String("*.css"));
}

void CssFileFilterItem::setFilter(const QString &filter)
{
    FileFilterBaseItem::setFilter(filter);
    emit filterChanged();
}

OtherFileFilterItem::OtherFileFilterItem(QObject *parent)
    : FileFilterBaseItem(parent)
{
}

void OtherFileFilterItem::setFilter(const QString &filter)
{
    FileFilterBaseItem::setFilter(filter);
    emit filterChanged();
}

} // namespace QmlProjectManager

