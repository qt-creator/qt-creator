#ifndef FILEFILTERITEMS_H
#define FILEFILTERITEMS_H

#include <QDir>
#include <QObject>
#include <QSet>
#include <qml.h>

#include "qmlprojectitem.h"

namespace QmlProjectManager {

class FileFilterBaseItem : public QmlProjectContentItem {
    Q_OBJECT

    Q_PROPERTY(QString directory READ directory WRITE setDirectory NOTIFY directoryChanged)
    Q_PROPERTY(bool recursive READ recursive WRITE setRecursive NOTIFY recursiveChanged)

    Q_PROPERTY(QList<QString> files READ files NOTIFY filesChanged)

public:
    FileFilterBaseItem(QObject *parent = 0);

    QString directory() const;
    void setDirectory(const QString &directoryPath);

    void setDefaultDirectory(const QString &directoryPath);

    QString filter() const;
    void setFilter(const QString &filter);

    bool recursive() const;
    void setRecursive(bool recursive);

    virtual QStringList files() const;

signals:
    void directoryChanged();
    void recursiveChanged();
    void filterChanged();
    void filesChanged();

private:
    QString absoluteDir() const;

    void updateFileList();
    QSet<QString> filesInSubTree(const QDir &rootDir, const QDir &dir);

    QString m_rootDir;
    QString m_defaultDir;

    QString m_filter;
    QRegExp m_regex;
    bool m_recursive;

    QSet<QString> m_files;

    friend class ProjectItem;
};

class QmlFileFilterItem : public FileFilterBaseItem {
    Q_OBJECT

public:
    QmlFileFilterItem(QObject *parent = 0);
};

} // namespace QmlProjectManager

QML_DECLARE_TYPE(QmlProjectManager::QmlFileFilterItem)

#endif // FILEFILTERITEMS_HPROJECTITEM_H
