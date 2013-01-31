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

#include "selectablefilesmodel.h"
#include "genericprojectconstants.h"

#include <coreplugin/fileiconprovider.h>
#include <coreplugin/icore.h>

#include <utils/QtConcurrentTools>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeView>

namespace GenericProjectManager {
namespace Internal {

SelectableFilesModel::SelectableFilesModel(const QString &baseDir, QObject *parent)
    : QAbstractItemModel(parent), m_root(0), m_baseDir(baseDir), m_allFiles(true)
{
    // Dummy tree
    m_root = new Tree;
    m_root->name = QLatin1String("/");
    m_root->parent = 0;
    m_root->fullPath = m_baseDir;
    m_root->isDir = true;
}

void SelectableFilesModel::setInitialMarkedFiles(const QStringList &files)
{
    m_files = files.toSet();
    m_outOfBaseDirFiles.clear();
    QString base = m_baseDir + QLatin1Char('/');
    foreach (const QString &file, m_files)
        if (!file.startsWith(base))
            m_outOfBaseDirFiles.append(file);

    m_allFiles = false;
}

void SelectableFilesModel::init()
{
}

void SelectableFilesModel::startParsing()
{
    // Build a tree in a future
    m_rootForFuture = new Tree;
    m_rootForFuture->name = QLatin1String("/");
    m_rootForFuture->parent = 0;
    m_rootForFuture->fullPath = m_baseDir;
    m_rootForFuture->isDir = true;

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

bool SelectableFilesModel::filter(Tree *t)
{
    if (t->isDir)
        return false;
    if (m_files.contains(t->fullPath))
        return false;
    foreach (const Glob &g, m_filter) {
        if (g.mode == Glob::EXACT) {
            if (g.matchString == t->name)
                return true;
        } else if (g.mode == Glob::ENDSWITH) {
            if (t->name.endsWith(g.matchString))
                return true;
        } else if (g.mode == Glob::REGEXP) {
            if (g.matchRegexp.exactMatch(t->name))
                return true;
        }
    }
    return false;
}

void SelectableFilesModel::buildTree(const QString &baseDir, Tree *tree, QFutureInterface<void> &fi)
{
    const QFileInfoList fileInfoList = QDir(baseDir).entryInfoList(QDir::Files |
                                                                   QDir::Dirs |
                                                                   QDir::NoDotAndDotDot);
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
            if (fileInfo.isSymLink())
                continue;
            Tree *t = new Tree;
            t->parent = tree;
            t->name = fileInfo.fileName();
            t->fullPath = fileInfo.filePath();
            t->isDir = true;
            buildTree(fileInfo.filePath(), t, fi);
            allChecked &= t->checked == Qt::Checked;
            allUnchecked &= t->checked == Qt::Unchecked;
            tree->childDirectories.append(t);
        } else {
            Tree *t = new Tree;
            t->parent = tree;
            t->name = fileInfo.fileName();
            t->checked = m_allFiles || m_files.contains(fileInfo.absoluteFilePath()) ? Qt::Checked : Qt::Unchecked;
            t->fullPath = fileInfo.filePath();
            t->isDir = false;
            allChecked &= t->checked == Qt::Checked;
            allUnchecked &= t->checked == Qt::Unchecked;
            tree->files.append(t);
            if (!filter(t))
                tree->visibleFiles.append(t);
        }
    }
    if (tree->childDirectories.isEmpty() && tree->visibleFiles.isEmpty())
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
    return parentT->childDirectories.size() + parentT->visibleFiles.size();
}

QModelIndex SelectableFilesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid())
        return createIndex(row, column, m_root);
    Tree *parentT = static_cast<Tree *>(parent.internalPointer());
    if (row < parentT->childDirectories.size())
        return createIndex(row, column, parentT->childDirectories.at(row));
    else
        return createIndex(row, column, parentT->visibleFiles.at(row - parentT->childDirectories.size()));
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
        pos = parent->parent->childDirectories.size() + parent->parent->visibleFiles.indexOf(parent);
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
    for (int i = 0; i < parentT->visibleFiles.size(); ++i) {
        allChecked &= parentT->visibleFiles.at(i)->checked == Qt::Checked;
        allUnchecked &= parentT->visibleFiles.at(i)->checked == Qt::Unchecked;
    }
    Qt::CheckState newState = Qt::PartiallyChecked;
    if (parentT->childDirectories.isEmpty() && parentT->visibleFiles.isEmpty())
        newState = Qt::Unchecked;
    else if (allChecked)
        newState = Qt::Checked;
    else if (allUnchecked)
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
    QStringList result = m_outOfBaseDirFiles;
    collectFiles(m_root, &result);
    return result;
}

QStringList SelectableFilesModel::preservedFiles() const
{
    return m_outOfBaseDirFiles;
}

void SelectableFilesModel::collectFiles(Tree *root, QStringList *result) const
{
    if (root->checked == Qt::Unchecked)
        return;
    foreach (Tree *t, root->childDirectories)
        collectFiles(t, result);
    foreach (Tree *t, root->visibleFiles)
        if (t->checked == Qt::Checked)
            result->append(t->fullPath);
}

QList<Glob> SelectableFilesModel::parseFilter(const QString &filter)
{
    QList<Glob> result;
    QStringList list = filter.split(QLatin1Char(';'), QString::SkipEmptyParts);
    foreach (const QString &e, list) {
        QString entry = e.trimmed();
        Glob g;
        if (entry.indexOf(QLatin1Char('*')) == -1 && entry.indexOf(QLatin1Char('?')) == -1) {
            g.mode = Glob::EXACT;
            g.matchString = entry;
        } else if (entry.startsWith(QLatin1Char('*')) && entry.indexOf(QLatin1Char('*'), 1) == -1
                   && entry.indexOf(QLatin1Char('?'), 1) == -1) {
            g.mode = Glob::ENDSWITH;
            g.matchString = entry.mid(1);
        } else {
            g.mode = Glob::REGEXP;
            g.matchRegexp = QRegExp(entry, Qt::CaseInsensitive, QRegExp::Wildcard);
        }
        result.append(g);
    }
    return result;
}

void SelectableFilesModel::applyFilter(const QString &filter)
{
    m_filter = parseFilter(filter);
    applyFilter(createIndex(0, 0, m_root));
}

Qt::CheckState SelectableFilesModel::applyFilter(const QModelIndex &index)
{
    bool allChecked = true;
    bool allUnchecked = true;
    Tree *t = static_cast<Tree *>(index.internalPointer());

    for (int i=0; i < t->childDirectories.size(); ++i) {
        Qt::CheckState childCheckState = applyFilter(index.child(i, 0));
        if (childCheckState == Qt::Checked)
            allUnchecked = false;
        else if (childCheckState == Qt::Unchecked)
            allChecked = false;
        else
            allChecked = allUnchecked = false;
    }

    int visibleIndex = 0;
    int visibleEnd = t->visibleFiles.size();
    int startOfBlock = 0;

    bool removeBlock = false;
    // first remove filtered out rows..
    for (;visibleIndex < visibleEnd; ++visibleIndex) {
        if (startOfBlock == visibleIndex) {
            removeBlock = filter(t->visibleFiles.at(visibleIndex));
        } else if (removeBlock != filter(t->visibleFiles.at(visibleIndex))) {
            if (removeBlock) {
                beginRemoveRows(index, startOfBlock, visibleIndex - 1);
                for (int i=startOfBlock; i < visibleIndex; ++i)
                    t->visibleFiles[i]->checked = Qt::Unchecked;
                t->visibleFiles.erase(t->visibleFiles.begin() + startOfBlock,
                                      t->visibleFiles.begin() + visibleIndex);
                endRemoveRows();
                visibleIndex = startOfBlock; // start again at startOfBlock
                visibleEnd = t->visibleFiles.size();
            }
            removeBlock = filter(t->visibleFiles.at(visibleIndex));
            startOfBlock = visibleIndex;
        }
    }
    if (removeBlock) {
        beginRemoveRows(index, startOfBlock, visibleEnd - 1);
        for (int i=startOfBlock; i < visibleEnd; ++i)
            t->visibleFiles[i]->checked = Qt::Unchecked;
        t->visibleFiles.erase(t->visibleFiles.begin() + startOfBlock,
                              t->visibleFiles.begin() + visibleEnd);
        endRemoveRows();
    }

    // Figure out which rows should be visible
    QList<Tree *> newRows;
    for (int i=0; i < t->files.size(); ++i)
        if (!filter(t->files.at(i)))
            newRows.append(t->files.at(i));
    // now add them!
    startOfBlock = 0;
    visibleIndex = 0;
    visibleEnd = t->visibleFiles.size();
    int newIndex = 0;
    int newEnd = newRows.size();
    while (true) {
        while (visibleIndex < visibleEnd && newIndex < newEnd &&
               t->visibleFiles.at(visibleIndex) == newRows.at(newIndex)) {
            ++newIndex;
            ++visibleIndex;
        }
        if (visibleIndex >= visibleEnd || newIndex >= newEnd)
            break;
        startOfBlock = newIndex;
        while (newIndex < newEnd &&
               t->visibleFiles.at(visibleIndex) != newRows.at(newIndex)) {
            ++newIndex;
        }
        // end of block = newIndex
        beginInsertRows(index, visibleIndex, visibleIndex + newIndex-startOfBlock-1);
        for (int i= newIndex - 1; i >= startOfBlock; --i)
            t->visibleFiles.insert(visibleIndex, newRows.at(i));
        endInsertRows();
        visibleIndex = visibleIndex + newIndex-startOfBlock;
        visibleEnd = visibleEnd + newIndex-startOfBlock;
        if (newIndex >= newEnd)
            break;
    }
    if (newIndex != newEnd) {
        beginInsertRows(index, visibleIndex, visibleIndex + newEnd-newIndex-1);
        for (int i=newEnd-1; i >=newIndex; --i)
            t->visibleFiles.insert(visibleIndex, newRows.at(i));
        endInsertRows();
    }

    for (int i=0; i < t->visibleFiles.size(); ++i) {
        if (t->visibleFiles.at(i)->checked == Qt::Checked)
            allUnchecked = false;
        else
            allChecked = false;
    }

    Qt::CheckState newState = Qt::PartiallyChecked;
    if (t->childDirectories.isEmpty() && t->visibleFiles.isEmpty())
        newState = Qt::Unchecked;
    else if (allChecked)
        newState = Qt::Checked;
    else if (allUnchecked)
        newState = Qt::Unchecked;
    if (t->checked != newState) {
        t->checked = newState;
        emit dataChanged(index, index);
    }

    return newState;
}

//////////
// SelectableFilesDialog
//////////

SelectableFilesDialog::SelectableFilesDialog(const QString &path, const QStringList files, QWidget *parent)
    : QDialog(parent)
{
    QVBoxLayout *layout = new QVBoxLayout();
    setLayout(layout);
    setWindowTitle(tr("Edit Files"));

    m_view = new QTreeView(this);

    QHBoxLayout *hbox = new QHBoxLayout;
    m_filterLabel = new QLabel(this);
    m_filterLabel->setText(tr("Hide files matching:"));
    m_filterLabel->hide();
    hbox->addWidget(m_filterLabel);
    m_filterLineEdit = new QLineEdit(this);

    const QString filter = Core::ICore::settings()->value(QLatin1String(Constants::FILEFILTER_SETTING),
                                                          QLatin1String(Constants::FILEFILTER_DEFAULT)).toString();
    m_filterLineEdit->setText(filter);
    m_filterLineEdit->hide();
    hbox->addWidget(m_filterLineEdit);
    m_applyFilterButton = new QPushButton(tr("Apply Filter"), this);
    m_applyFilterButton->hide();
    hbox->addWidget(m_applyFilterButton);
    layout->addLayout(hbox);

    m_selectableFilesModel = new SelectableFilesModel(path, this);
    m_selectableFilesModel->setInitialMarkedFiles(files);
    m_view->setModel(m_selectableFilesModel);
    m_view->setMinimumSize(500, 400);
    m_view->setHeaderHidden(true);
    m_view->hide();
    layout->addWidget(m_view);

    m_preservedFiles = new QLabel;
    m_preservedFiles->hide();
    layout->addWidget(m_preservedFiles);

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

    connect(m_applyFilterButton, SIGNAL(clicked()), this, SLOT(applyFilter()));

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
    m_filterLabel->show();
    m_filterLineEdit->show();
    m_applyFilterButton->show();
    m_view->show();
    m_progressLabel->hide();
    m_view->expand(QModelIndex());
    smartExpand(m_selectableFilesModel->index(0,0, QModelIndex()));
    applyFilter();
    const QStringList &preservedFiles = m_selectableFilesModel->preservedFiles();
    if (preservedFiles.isEmpty()) {
        m_preservedFiles->hide();
    } else {
        m_preservedFiles->show();
        m_preservedFiles->setText(tr("Not showing %n files that are outside of the base directory.\nThese files are preserved.", 0, preservedFiles.count()));
    }
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

void SelectableFilesDialog::applyFilter()
{
    const QString filter = m_filterLineEdit->text();
    Core::ICore::settings()->setValue(QLatin1String(Constants::FILEFILTER_SETTING), filter);
    m_selectableFilesModel->applyFilter(filter);
}

} // namespace Internal
} // namespace GenericProjectManager
