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

#ifndef SELECTABLEFILESMODEL_H
#define SELECTABLEFILESMODEL_H

#include <QAbstractItemModel>
#include <QSet>
#include <QFutureInterface>
#include <QFutureWatcher>
#include <QFileSystemModel>
#include <QDialog>
#include <QTreeView>
#include <QLabel>

namespace GenericProjectManager {
namespace Internal {

struct Tree
{
    QString name;
    Qt::CheckState checked;
    QList<Tree *> childDirectories;
    QList<Tree *> files;
    QList<Tree *> visibleFiles;
    QIcon icon;
    QString fullPath;
    bool isDir;
    Tree *parent;
};

struct Glob
{
    enum Mode { EXACT, ENDSWITH, REGEXP };
    Mode mode;
    QString matchString;
    mutable QRegExp matchRegexp;
};

class SelectableFilesModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    SelectableFilesModel(const QString &baseDir, QObject *parent);
    ~SelectableFilesModel();

    void setInitialMarkedFiles(const QStringList &files);

    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    QStringList selectedFiles() const;
    QStringList selectedPaths() const;
    QStringList preservedFiles() const;

    // only call this once
    void startParsing();
    void waitForFinished();
    void cancel();
    void applyFilter(const QString &filter);

signals:
    void parsingFinished();
    void parsingProgress(const QString &filename);

private slots:
    void buildTreeFinished();

private:
    QList<Glob> parseFilter(const QString &filter);
    Qt::CheckState applyFilter(const QModelIndex &index);
    bool filter(Tree *t);
    void init();
    void run(QFutureInterface<void> &fi);
    void collectFiles(Tree *root, QStringList *result) const;
    void collectPaths(Tree *root, QStringList *result) const;
    void buildTree(const QString &baseDir, Tree *tree, QFutureInterface<void> &fi);
    void deleteTree(Tree *tree);
    void propagateUp(const QModelIndex &index);
    void propagateDown(const QModelIndex &index);
    Tree *m_root;
    // Used in the future thread need to all not used after calling startParsing
    QString m_baseDir;
    QSet<QString> m_files;
    QStringList m_outOfBaseDirFiles;
    QFutureWatcher<void> m_watcher;
    Tree *m_rootForFuture;
    int m_futureCount;
    bool m_allFiles;
    QList<Glob> m_filter;
};

class SelectableFilesDialog : public QDialog
{
    Q_OBJECT

public:
    SelectableFilesDialog(const QString &path, const QStringList files, QWidget *parent);
    ~SelectableFilesDialog();
    QStringList selectedFiles() const;

private slots:
    void applyFilter();
    void parsingProgress(const QString &fileName);
    void parsingFinished();

private:
    void smartExpand(const QModelIndex &index);
    SelectableFilesModel *m_selectableFilesModel;
    QLabel *m_filterLabel;
    QLineEdit *m_filterLineEdit;
    QPushButton *m_applyFilterButton;
    QTreeView *m_view;
    QLabel *m_preservedFiles;
    QLabel *m_progressLabel;
};

} // namespace Internal
} // namespace GenericProjectManager

#endif // SELECTABLEFILESMODEL_H
