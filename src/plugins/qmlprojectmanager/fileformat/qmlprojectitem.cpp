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

#include "qmlprojectitem.h"
#include "filefilteritems.h"

#include <QDebug>
#include <QDir>

namespace QmlProjectManager {

class QmlProjectItemPrivate : public QObject {
    Q_OBJECT

public:
    QString sourceDirectory;
    QStringList importPaths;
    QStringList absoluteImportPaths;
    QString mainFile;

    QList<QmlFileFilterItem*> qmlFileFilters() const;

    // content property
    QList<QmlProjectContentItem*> content;
};

QList<QmlFileFilterItem*> QmlProjectItemPrivate::qmlFileFilters() const
{
    QList<QmlFileFilterItem*> qmlFilters;
    for (int i = 0; i < content.size(); ++i) {
        QmlProjectContentItem *contentElement = content.at(i);
        QmlFileFilterItem *qmlFileFilter = qobject_cast<QmlFileFilterItem*>(contentElement);
        if (qmlFileFilter)
            qmlFilters << qmlFileFilter;
    }
    return qmlFilters;
}

QmlProjectItem::QmlProjectItem(QObject *parent) :
        QObject(parent),
        d_ptr(new QmlProjectItemPrivate)
{
//    Q_D(QmlProjectItem);
//
//    QmlFileFilter *defaultQmlFilter = new QmlFileFilter(this);
//    d->content.append(defaultQmlFilter);
}

QmlProjectItem::~QmlProjectItem()
{
    delete d_ptr;
}

QDeclarativeListProperty<QmlProjectContentItem> QmlProjectItem::content()
{
    Q_D(QmlProjectItem);
    return QDeclarativeListProperty<QmlProjectContentItem>(this, d->content);
}

QString QmlProjectItem::sourceDirectory() const
{
    Q_D(const QmlProjectItem);
    return d->sourceDirectory;
}

// kind of initialization
void QmlProjectItem::setSourceDirectory(const QString &directoryPath)
{
    Q_D(QmlProjectItem);

    if (d->sourceDirectory == directoryPath)
        return;

    d->sourceDirectory = directoryPath;

    for (int i = 0; i < d->content.size(); ++i) {
        QmlProjectContentItem *contentElement = d->content.at(i);
        FileFilterBaseItem *fileFilter = qobject_cast<FileFilterBaseItem*>(contentElement);
        if (fileFilter) {
            fileFilter->setDefaultDirectory(directoryPath);
            connect(fileFilter, SIGNAL(filesChanged(QSet<QString>,QSet<QString>)),
                    this, SIGNAL(qmlFilesChanged(QSet<QString>,QSet<QString>)));
        }
    }

    setImportPaths(d->importPaths);

    emit sourceDirectoryChanged();
}

QStringList QmlProjectItem::importPaths() const
{
    Q_D(const QmlProjectItem);
    return d->absoluteImportPaths;
}

void QmlProjectItem::setImportPaths(const QStringList &importPaths)
{
    Q_D(QmlProjectItem);

    if (d->importPaths != importPaths)
        d->importPaths = importPaths;

    // convert to absolute paths
    QStringList absoluteImportPaths;
    const QDir sourceDir(sourceDirectory());
    foreach (const QString &importPath, importPaths)
        absoluteImportPaths += QDir::cleanPath(sourceDir.absoluteFilePath(importPath));

    if (d->absoluteImportPaths == absoluteImportPaths)
        return;

    d->absoluteImportPaths = absoluteImportPaths;
    emit importPathsChanged();
}

/* Returns list of absolute paths */
QStringList QmlProjectItem::files() const
{
    Q_D(const QmlProjectItem);
    QStringList files;

    for (int i = 0; i < d->content.size(); ++i) {
        QmlProjectContentItem *contentElement = d->content.at(i);
        FileFilterBaseItem *fileFilter = qobject_cast<FileFilterBaseItem*>(contentElement);
        if (fileFilter) {
            foreach (const QString &file, fileFilter->files()) {
                if (!files.contains(file))
                    files << file;
            }
        }
    }
    return files;
}

/**
  Check whether the project would include a file path
  - regardless whether the file already exists or not.

  @param filePath: absolute file path to check
  */
bool QmlProjectItem::matchesFile(const QString &filePath) const
{
    Q_D(const QmlProjectItem);
    for (int i = 0; i < d->content.size(); ++i) {
        QmlProjectContentItem *contentElement = d->content.at(i);
        FileFilterBaseItem *fileFilter = qobject_cast<FileFilterBaseItem*>(contentElement);
        if (fileFilter) {
            if (fileFilter->matchesFile(filePath))
                return true;
        }
    }
    return false;
}

QString QmlProjectItem::mainFile() const
{
    Q_D(const QmlProjectItem);
    return d->mainFile;
}

void QmlProjectItem::setMainFile(const QString &mainFilePath)
{
    Q_D(QmlProjectItem);
    if (mainFilePath == d->mainFile)
        return;
    d->mainFile = mainFilePath;
    emit mainFileChanged();
}

} // namespace QmlProjectManager

#include "qmlprojectitem.moc"
