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

#include <QObject>

namespace QmlDesigner {

class AssetsLibraryDirsModel;
class AssetsLibraryFilesModel;

class AssetsLibraryDir : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString dirName READ dirName NOTIFY dirNameChanged)
    Q_PROPERTY(QString dirPath READ dirPath NOTIFY dirPathChanged)
    Q_PROPERTY(bool dirExpanded READ dirExpanded WRITE setDirExpanded NOTIFY dirExpandedChanged)
    Q_PROPERTY(bool dirVisible READ dirVisible WRITE setDirVisible NOTIFY dirVisibleChanged)
    Q_PROPERTY(int dirDepth READ dirDepth NOTIFY dirDepthChanged)
    Q_PROPERTY(QObject *filesModel READ filesModel NOTIFY filesModelChanged)
    Q_PROPERTY(QObject *dirsModel READ dirsModel NOTIFY dirsModelChanged)

public:
    AssetsLibraryDir(const QString &path, int depth, bool expanded = true, QObject *parent = nullptr);

    QString dirName() const;
    QString dirPath() const;
    int dirDepth() const;

    bool dirExpanded() const;
    bool dirVisible() const;
    void setDirExpanded(bool expand);
    void setDirVisible(bool visible);

    QObject *filesModel() const;
    QObject *dirsModel() const;

    QList<AssetsLibraryDir *> childAssetsDirs() const;

    void addDir(AssetsLibraryDir *assetsDir);
    void addFile(const QString &filePath);

signals:
    void dirNameChanged();
    void dirPathChanged();
    void dirDepthChanged();
    void dirExpandedChanged();
    void dirVisibleChanged();
    void filesModelChanged();
    void dirsModelChanged();

private:
    QString m_dirPath;
    int m_dirDepth = 0;
    bool m_dirExpanded = true;
    bool m_dirVisible = true;
    AssetsLibraryDirsModel *m_dirsModel = nullptr;
    AssetsLibraryFilesModel *m_filesModel = nullptr;
};

} // namespace QmlDesigner
