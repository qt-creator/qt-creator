// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QRegularExpression>
#include <QSet>
#include <QTimer>
#include <QJsonArray>

QT_FORWARD_DECLARE_CLASS(QDir)

namespace Utils { class FileSystemWatcher; }

namespace QmlProjectManager {

using FileFilterItemPtr = std::unique_ptr<class FileFilterItem>;
using FileFilterItems = std::vector<FileFilterItemPtr>;

class FileFilterItem : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString directory READ directory WRITE setDirectory NOTIFY directoryChanged)
    Q_PROPERTY(bool recursive READ recursive WRITE setRecursive NOTIFY recursiveChanged)
    Q_PROPERTY(QStringList paths READ pathsProperty WRITE setPathsProperty)

    Q_PROPERTY(QStringList files READ files NOTIFY filesChanged DESIGNABLE false)

public:
    FileFilterItem();
    FileFilterItem(const QString &directory, const QStringList &filters);

    QString directory() const;
    void setDirectory(const QString &directoryPath);

    void setDefaultDirectory(const QString &directoryPath);

    QStringList filters() const;
    void setFilters(const QStringList &filter);

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
    QSet<QString> filesInSubTree(const QDir &rootDir, const QDir &dir, QSet<QString> *parsedDirs = nullptr);
    Utils::FileSystemWatcher *dirWatcher();
    QStringList watchedDirectories() const;

    QString m_rootDir;
    QString m_defaultDir;

    QStringList m_filter;
    // simple "*.png" patterns are stored in m_fileSuffixes, otherwise store in m_regExpList
    QList<QString> m_fileSuffixes;
    QList<QRegularExpression> m_regExpList;

    enum RecursiveOption {
        Recurse,
        DoNotRecurse,
        RecurseDefault // not set explicitly
    };

    RecursiveOption m_recurse = RecurseDefault;

    QStringList m_explicitFiles;

    QSet<QString> m_files;
    Utils::FileSystemWatcher *m_dirWatcher = nullptr;
    QTimer m_updateFileListTimer;

    friend class ProjectItem;
    void initTimer();
};

} // namespace QmlProjectManager
