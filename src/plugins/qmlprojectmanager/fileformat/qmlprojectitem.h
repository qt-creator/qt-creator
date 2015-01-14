/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLPROJECTITEM_H
#define QMLPROJECTITEM_H

#include <QObject>
#include <QSet>
#include <QStringList>

namespace QmlProjectManager {

class QmlProjectContentItem : public QObject {
    // base class for all elements that should be direct children of Project element
    Q_OBJECT

public:
    QmlProjectContentItem(QObject *parent = 0) : QObject(parent) {}
};

class QmlProjectItemPrivate;

class QmlProjectItem : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QmlProjectItem)

    Q_PROPERTY(QString sourceDirectory READ sourceDirectory NOTIFY sourceDirectoryChanged)
    Q_PROPERTY(QStringList importPaths READ importPaths WRITE setImportPaths NOTIFY importPathsChanged)
    Q_PROPERTY(QString mainFile READ mainFile WRITE setMainFile NOTIFY mainFileChanged)

public:
    QmlProjectItem(QObject *parent = 0);
    ~QmlProjectItem();

    QString sourceDirectory() const;
    void setSourceDirectory(const QString &directoryPath);

    QStringList importPaths() const;
    void setImportPaths(const QStringList &paths);

    QStringList files() const;
    bool matchesFile(const QString &filePath) const;

    QString mainFile() const;
    void setMainFile(const QString &mainFilePath);

    void appendContent(QmlProjectContentItem* contentItem);

signals:
    void qmlFilesChanged(const QSet<QString> &, const QSet<QString> &);
    void sourceDirectoryChanged();
    void importPathsChanged();
    void mainFileChanged();

protected:
    QmlProjectItemPrivate *d_ptr;
};

} // namespace QmlProjectManager

#endif // QMLPROJECTITEM_H
