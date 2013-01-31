/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef FILEFILTERITEMS_H
#define FILEFILTERITEMS_H

#include "qmlprojectitem.h"

#include <QObject>
#include <QSet>
#include <QTimer>

#include <qdeclarative.h>

QT_FORWARD_DECLARE_CLASS(QDir)

namespace Utils {
class FileSystemWatcher;
}

namespace QmlProjectManager {

class FileFilterBaseItem : public QmlProjectContentItem {
    Q_OBJECT

    Q_PROPERTY(QString directory READ directory WRITE setDirectory NOTIFY directoryChanged)
    Q_PROPERTY(bool recursive READ recursive WRITE setRecursive NOTIFY recursiveChanged)
    Q_PROPERTY(QStringList paths READ pathsProperty WRITE setPathsProperty)

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
    void filesChanged(const QSet<QString> &added, const QSet<QString> &removed);

private slots:
    void updateFileList();
    void updateFileListNow();

private:
    QString absolutePath(const QString &path) const;
    QString absoluteDir() const;

    bool fileMatches(const QString &fileName) const;
    QSet<QString> filesInSubTree(const QDir &rootDir, const QDir &dir, QSet<QString> *parsedDirs = 0);
    Utils::FileSystemWatcher *dirWatcher();
    QStringList watchedDirectories() const;

    QString m_rootDir;
    QString m_defaultDir;

    QString m_filter;
    // simple "*.png" patterns are stored in m_fileSuffixes, otherwise store in m_regExpList
    QList<QString> m_fileSuffixes;
    QList<QRegExp> m_regExpList;

    enum RecursiveOption {
        Recurse,
        DoNotRecurse,
        RecurseDefault // not set explicitly
    };

    RecursiveOption m_recurse;

    QStringList m_explicitFiles;

    QSet<QString> m_files;
    Utils::FileSystemWatcher *m_dirWatcher;
    QTimer m_updateFileListTimer;


    friend class ProjectItem;
};

class QmlFileFilterItem : public FileFilterBaseItem {
    Q_OBJECT

public:
    QmlFileFilterItem(QObject *parent = 0);
};

class JsFileFilterItem : public FileFilterBaseItem {
    Q_OBJECT
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

    void setFilter(const QString &filter);

signals:
    void filterChanged();

public:
    JsFileFilterItem(QObject *parent = 0);
};

class ImageFileFilterItem : public FileFilterBaseItem {
    Q_OBJECT
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

    void setFilter(const QString &filter);

signals:
    void filterChanged();

public:
    ImageFileFilterItem(QObject *parent = 0);
};

class CssFileFilterItem : public FileFilterBaseItem {
    Q_OBJECT
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

    void setFilter(const QString &filter);

signals:
    void filterChanged();

public:
    CssFileFilterItem(QObject *parent = 0);
};

class OtherFileFilterItem : public FileFilterBaseItem {
    Q_OBJECT
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

    void setFilter(const QString &filter);

signals:
    void filterChanged();

public:
    OtherFileFilterItem(QObject *parent = 0);
};

} // namespace QmlProjectManager

QML_DECLARE_TYPE(QmlProjectManager::QmlFileFilterItem)
QML_DECLARE_TYPE(QmlProjectManager::JsFileFilterItem)
QML_DECLARE_TYPE(QmlProjectManager::ImageFileFilterItem)
QML_DECLARE_TYPE(QmlProjectManager::CssFileFilterItem)
QML_DECLARE_TYPE(QmlProjectManager::OtherFileFilterItem)

#endif // FILEFILTERITEMS_HPROJECTITEM_H
