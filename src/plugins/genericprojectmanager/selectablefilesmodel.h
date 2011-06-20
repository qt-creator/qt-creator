/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef SELECTABLEFILESMODEL_H
#define SELECTABLEFILESMODEL_H

#include <QtCore/QAbstractItemModel>
#include <QtCore/QSet>
#include <QtGui/QFileSystemModel>
#include <QtGui/QFileIconProvider>
#include <QtGui/QDialog>
#include <QtGui/QTreeView>

namespace GenericProjectManager {
namespace Internal {


struct Tree
{
    QString name;
    Qt::CheckState checked;
    QList<Tree *> childDirectories;
    QList<Tree *> files;
    bool isDir;
    QIcon icon;
    QString fullPath;
    Tree *parent;
};

class SelectableFilesModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    SelectableFilesModel(const QString &baseDir, const QStringList &files, const QSet<QString> &suffixes, QObject *parent);
    ~SelectableFilesModel();

    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    QStringList selectedFiles() const;
private:
    void collectFiles(Tree *root, QStringList *result) const;
    void buildTree(const QString &baseDir, Tree *tree);
    void deleteTree(Tree *tree);
    void propagateUp(const QModelIndex &index);
    void propagateDown(const QModelIndex &index);
    QString m_baseDir;
    QSet<QString> m_files;
    QSet<QString> m_suffixes;
    Tree *m_root;
    QFileIconProvider m_iconProvider;
};

class SelectableFilesDialog : public QDialog
{
    Q_OBJECT
public:
    SelectableFilesDialog(const QString &path, const QStringList files, const QSet<QString> &suffixes, QWidget *parent);
    QStringList selectedFiles() const;
private:
    void smartExpand(QTreeView *view, const QModelIndex &index);
    SelectableFilesModel *m_selectableFilesModel;
};

}
}


#endif // SELECTABLEFILESMODEL_H
