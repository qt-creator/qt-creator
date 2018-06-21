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

#pragma once

#include "qmlprojectitem.h"

#include <QObject>
#include <QRegExp>
#include <QSet>
#include <QTimer>

QT_FORWARD_DECLARE_CLASS(QDir)

namespace Utils { class FileSystemWatcher; }

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

private:
    void updateFileList();
    void updateFileListNow();

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
