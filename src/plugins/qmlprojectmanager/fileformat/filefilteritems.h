#ifndef FILEFILTERITEMS_H
#define FILEFILTERITEMS_H

#include <QDir>
#include <QObject>
#include <QSet>
#include <qdeclarative.h>
#include <QFileSystemWatcher>

#include "qmlprojectitem.h"

namespace QmlProjectManager {

class FileFilterBaseItem : public QmlProjectContentItem {
    Q_OBJECT

    Q_PROPERTY(QString directory READ directory WRITE setDirectory NOTIFY directoryChanged)
    Q_PROPERTY(bool recursive READ recursive WRITE setRecursive NOTIFY recursiveChanged)
    Q_PROPERTY(QStringList paths READ pathsProperty WRITE setPathsProperty NOTIFY pathsPropertyChanged)

    Q_PROPERTY(QStringList files READ files NOTIFY filesChanged DESIGNABLE false)

public:
    FileFilterBaseItem(QObject *parent = 0);

    QString directory() const;
    void setDirectory(const QString &directoryPath);

    void setDefaultDirectory(const QString &directoryPath);

    QString filter() const;
    void setFilter(const QString &filter);

    bool recursive() const;
    void setRecursive(bool recursive);

    QStringList pathsProperty() const;
    void setPathsProperty(const QStringList &paths);

    virtual QStringList files() const;
    bool matchesFile(const QString &filePath) const;

signals:
    void directoryChanged();
    void recursiveChanged();
    void pathsChanged();
    void filesChanged();
    void filterChanged();

private slots:
    void updateFileList();

private:
    QString absolutePath(const QString &path) const;
    QString absoluteDir() const;

    QSet<QString> filesInSubTree(const QDir &rootDir, const QDir &dir, QSet<QString> *parsedDirs = 0);

    QString m_rootDir;
    QString m_defaultDir;

    QString m_filter;
    QList<QRegExp> m_regExpList;

    enum RecursiveOption {
        Recurse,
        DoNotRecurse,
        RecurseDefault // not set explicitly
    };

    RecursiveOption m_recurse;

    QStringList m_explicitFiles;

    QFileSystemWatcher m_fsWatcher;

    QSet<QString> m_files;

    friend class ProjectItem;
};

class QmlFileFilterItem : public FileFilterBaseItem {
    Q_OBJECT

public:
    QmlFileFilterItem(QObject *parent = 0);
};

class JsFileFilterItem : public FileFilterBaseItem {
    Q_OBJECT
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged())

public:
    JsFileFilterItem(QObject *parent = 0);
};

class ImageFileFilterItem : public FileFilterBaseItem {
    Q_OBJECT
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged())

public:
    ImageFileFilterItem(QObject *parent = 0);
};

class CssFileFilterItem : public FileFilterBaseItem {
    Q_OBJECT
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged())

public:
    CssFileFilterItem(QObject *parent = 0);
};

} // namespace QmlProjectManager

QML_DECLARE_TYPE(QmlProjectManager::QmlFileFilterItem)
QML_DECLARE_TYPE(QmlProjectManager::JsFileFilterItem)
QML_DECLARE_TYPE(QmlProjectManager::ImageFileFilterItem)
QML_DECLARE_TYPE(QmlProjectManager::CssFileFilterItem)

#endif // FILEFILTERITEMS_HPROJECTITEM_H
