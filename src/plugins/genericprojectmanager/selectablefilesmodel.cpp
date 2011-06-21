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

#include "selectablefilesmodel.h"

#include <coreplugin/fileiconprovider.h>

#include <qtconcurrent/QtConcurrentTools>
#include <QtGui/QHBoxLayout>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QTreeView>

using namespace GenericProjectManager;
using namespace GenericProjectManager::Internal;

SelectableFilesModel::SelectableFilesModel(const QString &baseDir, QObject *parent)
    : QAbstractItemModel(parent), m_root(0), m_baseDir(baseDir), m_allFiles(true)
{
    // Dummy tree
    m_root = new Tree;
    m_root->name = "/";
    m_root->parent = 0;
}

void SelectableFilesModel::setInitialMarkedFiles(const QStringList &files)
{
    m_files = files.toSet();
    m_allFiles = false;
}

void SelectableFilesModel::setSuffixes(QSet<QString> suffixes)
{
    m_suffixes = suffixes;
}

void SelectableFilesModel::init()
{
}

void SelectableFilesModel::startParsing()
{
    // Build a tree in a future
    m_rootForFuture = new Tree;
    m_rootForFuture->name = "/";
    m_rootForFuture->parent = 0;

    connect(&m_watcher, SIGNAL(finished()), this, SLOT(buildTreeFinished()));
    m_watcher.setFuture(QtConcurrent::run(&SelectableFilesModel::run, this));
}

void SelectableFilesModel::run(QFutureInterface<void> &fi)
{
    m_futureCount = 0;
    buildTree(m_baseDir, m_rootForFuture, fi);
}

void SelectableFilesModel::buildTreeFinished()
{
    beginResetModel();
    deleteTree(m_root);
    m_root = m_rootForFuture;
    m_rootForFuture = 0;
    endResetModel();
    emit parsingFinished();
}

void SelectableFilesModel::waitForFinished()
{
    m_watcher.waitForFinished();
}

void SelectableFilesModel::cancel()
{
    m_watcher.cancel();
}

void SelectableFilesModel::buildTree(const QString &baseDir, Tree *tree, QFutureInterface<void> &fi)
{
    const QFileInfoList fileInfoList = QDir(baseDir).entryInfoList(QDir::Files |
                                                                   QDir::Dirs |
                                                                   QDir::NoDotAndDotDot |
                                                                   QDir::NoSymLinks);
    bool allChecked = true;
    bool allUnchecked = true;
    foreach (const QFileInfo &fileInfo, fileInfoList) {
        if (m_futureCount % 100) {
            emit parsingProgress(fileInfo.absoluteFilePath());
            if (fi.isCanceled())
                return;
        }
        ++m_futureCount;
        if (fileInfo.isDir()) {
            Tree *t = new Tree;
            t->parent = tree;
            t->name = fileInfo.fileName();
            t->fullPath = fileInfo.filePath();
            buildTree(fileInfo.filePath(), t, fi);
            allChecked &= t->checked == Qt::Checked;
            allUnchecked &= t->checked == Qt::Unchecked;
            tree->childDirectories.append(t);
        } else if (m_suffixes.contains(fileInfo.suffix())) {
            Tree *t = new Tree;
            t->parent = tree;
            t->name = fileInfo.fileName();
            t->checked = m_allFiles || m_files.contains(fileInfo.absoluteFilePath()) ? Qt::Checked : Qt::Unchecked;
            t->fullPath = fileInfo.filePath();
            allChecked &= t->checked == Qt::Checked;
            allUnchecked &= t->checked == Qt::Unchecked;
            tree->files.append(t);
        }
    }
    if (tree->childDirectories.isEmpty() && tree->files.isEmpty())
        tree->checked = Qt::Unchecked;
    else if (allChecked)
        tree->checked = Qt::Checked;
    else if (allUnchecked)
        tree->checked = Qt::Unchecked;
    else
        tree->checked = Qt::PartiallyChecked;
}

SelectableFilesModel::~SelectableFilesModel()
{
    deleteTree(m_root);
}

void SelectableFilesModel::deleteTree(Tree *tree)
{
    foreach (Tree *t, tree->childDirectories)
        deleteTree(t);
    foreach (Tree *t, tree->files)
        deleteTree(t);
    delete tree;
}

int SelectableFilesModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

int SelectableFilesModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 1;
    Tree *parentT = static_cast<Tree *>(parent.internalPointer());
    return parentT->childDirectories.size() + parentT->files.size();
}

QModelIndex SelectableFilesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid())
        return createIndex(row, column, m_root);
    Tree *parentT = static_cast<Tree *>(parent.internalPointer());
    if (row < parentT->childDirectories.size())
        return createIndex(row, column, parentT->childDirectories.at(row));
    else
        return createIndex(row, column, parentT->files.at(row - parentT->childDirectories.size()));
}

QModelIndex SelectableFilesModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();
    Tree *parent = static_cast<Tree *>(child.internalPointer())->parent;
    if (!parent)
        return QModelIndex();
    if (!parent->parent) //then the parent is the root
        return createIndex(0, 0, parent);
    // figure out where the parent is
    int pos = parent->parent->childDirectories.indexOf(parent);
    if (pos == -1)
        pos = parent->parent->childDirectories.size() + parent->parent->files.indexOf(parent);
    return createIndex(pos, 0, parent);
}

QVariant SelectableFilesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    Tree *t = static_cast<Tree *>(index.internalPointer());
    if (role == Qt::DisplayRole)
        return t->name;
    if (role == Qt::CheckStateRole)
        return t->checked;
    if (role == Qt::DecorationRole) {
        if (t->icon.isNull())
            t->icon = Core::FileIconProvider::instance()->icon(QFileInfo(t->fullPath));
        return t->icon;
    }
    return QVariant();
}

bool SelectableFilesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole) {
        // We can do that!
        Tree *t = static_cast<Tree *>(index.internalPointer());
        t->checked = Qt::CheckState(value.toInt());
        propagateDown(index);
        propagateUp(index);
        emit dataChanged(index, index);
    }
    return false;
}

void SelectableFilesModel::propagateUp(const QModelIndex &index)
{
    QModelIndex parent = index.parent();
    if (!parent.isValid())
        return;
    Tree *parentT = static_cast<Tree *>(parent.internalPointer());
    if (!parentT)
        return;
    bool allChecked = true;
    bool allUnchecked = true;
    for (int i = 0; i < parentT->childDirectories.size(); ++i) {
        allChecked &= parentT->childDirectories.at(i)->checked == Qt::Checked;
        allUnchecked &= parentT->childDirectories.at(i)->checked == Qt::Unchecked;
    }
    for (int i = 0; i < parentT->files.size(); ++i) {
        allChecked &= parentT->files.at(i)->checked == Qt::Checked;
        allUnchecked &= parentT->files.at(i)->checked == Qt::Unchecked;
    }
    Qt::CheckState newState = Qt::PartiallyChecked;
    if (allChecked)
        newState = Qt::Checked;
    if (allUnchecked)
        newState = Qt::Unchecked;
    if (parentT->checked != newState) {
        parentT->checked = newState;
        emit dataChanged(parent, parent);
        propagateUp(parent);
    }
}

void SelectableFilesModel::propagateDown(const QModelIndex &index)
{
    Tree *t = static_cast<Tree *>(index.internalPointer());
    for (int i = 0; i<t->childDirectories.size(); ++i) {
        t->childDirectories[i]->checked = t->checked;
        propagateDown(index.child(i, 0));
    }
    for (int i = 0; i<t->files.size(); ++i)
        t->files[i]->checked = t->checked;

    int rows = rowCount(index);
    if (rows)
        emit dataChanged(index.child(0, 0), index.child(rows-1, 0));
}

Qt::ItemFlags SelectableFilesModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
}

QStringList SelectableFilesModel::selectedPaths() const
{
    QStringList result;
    collectPaths(m_root, &result);
    return result;
}

void SelectableFilesModel::collectPaths(Tree *root, QStringList *result)  const
{
    if (root->checked == Qt::Unchecked)
        return;
    result->append(root->fullPath);
    foreach (Tree *t, root->childDirectories)
        collectPaths(t, result);
}

QStringList SelectableFilesModel::selectedFiles() const
{
    QStringList result;
    collectFiles(m_root, &result);
    return result;
}

void SelectableFilesModel::collectFiles(Tree *root, QStringList *result) const
{
    if (root->checked == Qt::Unchecked)
        return;
    foreach (Tree *t, root->childDirectories)
        collectFiles(t, result);
    foreach (Tree *t, root->files)
        if (t->checked == Qt::Checked)
            result->append(t->fullPath);
}


SelectableFilesDialog::SelectableFilesDialog(const QString &path, const QStringList files, const QSet<QString> &suffixes, QWidget *parent)
    : QDialog(parent)
{
    QVBoxLayout *layout = new QVBoxLayout();
    setLayout(layout);
    setWindowTitle(tr("Edit Files"));

    m_view = new QTreeView(this);

    m_selectableFilesModel = new SelectableFilesModel(path, this);
    m_selectableFilesModel->setInitialMarkedFiles(files);
    m_selectableFilesModel->setSuffixes(suffixes);
    m_view->setModel(m_selectableFilesModel);
    m_view->setMinimumSize(500, 400);
    m_view->setHeaderHidden(true);
    m_view->hide();
    layout->addWidget(m_view);

    m_progressLabel = new QLabel(this);
    m_progressLabel->setMaximumWidth(500);
    layout->addWidget(m_progressLabel);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(Qt::Horizontal, this);
    buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()),
            this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()),
            this, SLOT(reject()));
    layout->addWidget(buttonBox);

    connect(m_selectableFilesModel, SIGNAL(parsingProgress(QString)),
            this, SLOT(parsingProgress(QString)));
    connect(m_selectableFilesModel, SIGNAL(parsingFinished()),
            this, SLOT(parsingFinished()));

    m_selectableFilesModel->startParsing();
}

SelectableFilesDialog::~SelectableFilesDialog()
{
    m_selectableFilesModel->cancel();
    m_selectableFilesModel->waitForFinished();
}

void SelectableFilesDialog::parsingProgress(const QString &fileName)
{
    m_progressLabel->setText(tr("Generating file list...\n\n%1").arg(fileName));
}

void SelectableFilesDialog::parsingFinished()
{
    m_view->show();
    m_progressLabel->hide();
    m_view->expand(QModelIndex());
    smartExpand(m_selectableFilesModel->index(0,0, QModelIndex()));
}

void SelectableFilesDialog::smartExpand(const QModelIndex &index)
{
    if (m_view->model()->data(index, Qt::CheckStateRole) == Qt::PartiallyChecked) {
        m_view->expand(index);
        int rows = m_view->model()->rowCount(index);
        for (int i = 0; i < rows; ++i)
            smartExpand(index.child(i, 0));
    }
}

QStringList SelectableFilesDialog::selectedFiles() const
{
    return m_selectableFilesModel->selectedFiles();
}
