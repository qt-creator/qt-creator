#include "filefilteritems.h"
#include <qdebug.h>

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
    const QSet<QString> newFiles = filesInSubTree(QDir(m_defaultDir), QDir(projectDir), &dirsToBeWatched);

    if (newFiles != m_files) {
        // update watched files
        foreach (const QString &file, m_files - newFiles) {
            m_fsWatcher.removePath(QDir(projectDir).absoluteFilePath(file));
        }
        foreach (const QString &file, newFiles - m_files) {
            m_fsWatcher.addPath(QDir(projectDir).absoluteFilePath(file));
        }

        m_files = newFiles;

        emit filesChanged();
    }

    // update watched directories
    QSet<QString> watchedDirectories = m_fsWatcher.directories().toSet();
    foreach (const QString &dir, watchedDirectories - dirsToBeWatched) {
        m_fsWatcher.removePath(QDir(projectDir).absoluteFilePath(dir));
    }
    foreach (const QString &dir, dirsToBeWatched - watchedDirectories) {
        m_fsWatcher.addPath(QDir(projectDir).absoluteFilePath(dir));
    }
}

QSet<QString> FileFilterBaseItem::filesInSubTree(const QDir &rootDir, const QDir &dir, QSet<QString> *parsedDirs)
{
    QSet<QString> fileSet;

    if (parsedDirs)
        parsedDirs->insert(dir.absolutePath());

    foreach (const QFileInfo &file, dir.entryInfoList(QDir::Files)) {
        if (m_regex.exactMatch(file.fileName())) {
            fileSet.insert(rootDir.relativeFilePath(file.absoluteFilePath()));
        }
    }

    if (m_recursive) {
        foreach (const QFileInfo &subDir, dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            fileSet += filesInSubTree(rootDir, QDir(subDir.absoluteFilePath()));
        }
    }
    return fileSet;
}

QmlFileFilterItem::QmlFileFilterItem(QObject *parent)
    : FileFilterBaseItem(parent)
{
    setFilter(QLatin1String("*.qml"));
}

} // namespace QmlProjectManager

QML_DEFINE_TYPE(QmlProject,1,0,QmlFiles,QmlProjectManager::QmlFileFilterItem)
