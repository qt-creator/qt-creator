/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QAbstractListModel>
#include <QDateTime>
#include <QDir>
#include <QHash>
#include <QIcon>
#include <QPair>
#include <QSet>

namespace Utils { class FileSystemWatcher; }

namespace QmlDesigner {

class SynchronousImageCache;
class AssetsLibraryDir;

class AssetsLibraryModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool isEmpty READ isEmpty WRITE setIsEmpty NOTIFY isEmptyChanged)

public:
    AssetsLibraryModel(QmlDesigner::SynchronousImageCache &fontImageCache,
                           Utils::FileSystemWatcher *fileSystemWatcher,
                           QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    void refresh();
    void setRootPath(const QString &path);
    void setSearchText(const QString &searchText);

    bool isEmpty() const;

    static const QStringList &supportedImageSuffixes();
    static const QStringList &supportedFragmentShaderSuffixes();
    static const QStringList &supportedShaderSuffixes();
    static const QStringList &supportedFontSuffixes();
    static const QStringList &supportedAudioSuffixes();
    static const QStringList &supportedVideoSuffixes();
    static const QStringList &supportedTexture3DSuffixes();

    const QSet<QString> &previewableSuffixes() const;

    static void saveExpandedState(bool expanded, const QString &assetPath);
    static bool loadExpandedState(const QString &assetPath);

    enum class DirExpandState {
        SomeExpanded,
        AllExpanded,
        AllCollapsed
    };
    Q_ENUM(DirExpandState)

    Q_INVOKABLE void toggleExpandAll(bool expand);
    Q_INVOKABLE DirExpandState getAllExpandedState() const;
    Q_INVOKABLE void deleteFiles(const QStringList &filePaths);
    Q_INVOKABLE bool renameFolder(const QString &folderPath, const QString &newName);
    Q_INVOKABLE void addNewFolder(const QString &folderPath);
    Q_INVOKABLE void deleteFolder(const QString &folderPath);
    Q_INVOKABLE QObject *rootDir() const;

signals:
    void isEmptyChanged();

private:
    const QSet<QString> &supportedSuffixes() const;

    void setIsEmpty(bool empty);

    SynchronousImageCache &m_fontImageCache;
    QHash<QString, QPair<QDateTime, QIcon>> m_iconCache;

    QString m_searchText;
    Utils::FileSystemWatcher *m_fileSystemWatcher = nullptr;
    AssetsLibraryDir *m_assetsDir = nullptr;
    bool m_isEmpty = true;

    QHash<int, QByteArray> m_roleNames;
    inline static QHash<QString, bool> m_expandedStateHash; // <assetPath, isExpanded>
};

} // namespace QmlDesigner
