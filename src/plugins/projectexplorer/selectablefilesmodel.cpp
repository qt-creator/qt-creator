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

#include "selectablefilesmodel.h"
#include "projectexplorerconstants.h"

#include <coreplugin/fileiconprovider.h>
#include <coreplugin/icore.h>

#include <utils/QtConcurrentTools>
#include <utils/algorithm.h>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeView>
#include <QDir>
#include <utils/pathchooser.h>

namespace ProjectExplorer {

SelectableFilesModel::SelectableFilesModel(QObject *parent)
    : QAbstractItemModel(parent), m_root(0), m_allFiles(true)
{
    connect(&m_watcher, SIGNAL(finished()), this, SLOT(buildTreeFinished()));

    m_root = new Tree;
    m_root->parent = 0;
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

void SelectableFilesModel::startParsing(const QString &baseDir)
{
    m_watcher.cancel();
    m_watcher.waitForFinished();

    m_baseDir = baseDir;
    // Build a tree in a future
    m_rootForFuture = new Tree;
    m_rootForFuture->name = QLatin1Char('/');
    m_rootForFuture->parent = 0;
    m_rootForFuture->fullPath = baseDir;
    m_rootForFuture->isDir = true;

    m_watcher.setFuture(QtConcurrent::run(&SelectableFilesModel::run, this));
}

void SelectableFilesModel::run(QFutureInterface<void> &fi)
{
    m_futureCount = 0;
    buildTree(m_baseDir, m_rootForFuture, fi, 5);
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

void SelectableFilesModel::cancel()
{
    m_watcher.cancel();
    m_watcher.waitForFinished();
}

bool SelectableFilesModel::filter(Tree *t)
{
    if (t->isDir)
        return false;
    if (m_files.contains(t->fullPath))
        return false;

    auto matchesTreeName = [t](const Glob &g) {
        return g.isMatch(t->name);
    };

    //If none of the "show file" filters match just return
    if (!Utils::anyOf(m_showFilesFilter, matchesTreeName))
        return true;

    return Utils::anyOf(m_hideFilesFilter, matchesTreeName);
}

void SelectableFilesModel::buildTree(const QString &baseDir, Tree *tree, QFutureInterface<void> &fi, int symlinkDepth)
{
    if (symlinkDepth == 0)
        return;
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
            Tree *t = new Tree;
            t->parent = tree;
            t->name = fileInfo.fileName();
            t->fullPath = fileInfo.filePath();
            t->isDir = true;
            buildTree(fileInfo.filePath(), t, fi, symlinkDepth - fileInfo.isSymLink());
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
    m_watcher.cancel();
    m_watcher.waitForFinished();
    deleteTree(m_root);
}

void SelectableFilesModel::deleteTree(Tree *tree)
{
    if (!tree)
        return;
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
    if (!child.internalPointer())
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
            t->icon = Core::FileIconProvider::icon(t->fullPath);
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

void SelectableFilesModel::applyFilter(const QString &showFilesfilter, const QString &hideFilesfilter)
{
    m_showFilesFilter = parseFilter(showFilesfilter);
    m_hideFilesFilter = parseFilter(hideFilesfilter);
    applyFilter(createIndex(0, 0, m_root));
}

void SelectableFilesModel::selectAllFiles()
{
    selectAllFiles(m_root);
}

void SelectableFilesModel::selectAllFiles(Tree *root)
{
    root->checked = Qt::Checked;

    foreach (Tree *t, root->childDirectories)
        selectAllFiles(t);

    foreach (Tree *t, root->visibleFiles)
        t->checked = Qt::Checked;
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
// SelectableFilesDialogs
//////////

SelectableFilesDialogEditFiles::SelectableFilesDialogEditFiles(const QString &path,
                                                               const QStringList &files,
                                                               QWidget *parent)
    : QDialog(parent)
{
    QVBoxLayout *layout = new QVBoxLayout();
    setLayout(layout);
    setWindowTitle(tr("Edit Files"));

    m_view = new QTreeView(this);

    createShowFileFilterControls(layout);
    createHideFileFilterControls(layout);
    createApplyButton(layout);

    m_selectableFilesModel = new SelectableFilesModel(this);
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

    connect(m_selectableFilesModel, SIGNAL(parsingProgress(QString)),
            this, SLOT(parsingProgress(QString)));
    connect(m_selectableFilesModel, SIGNAL(parsingFinished()),
            this, SLOT(parsingFinished()));

    m_selectableFilesModel->startParsing(path);
}

void SelectableFilesDialogEditFiles::createHideFileFilterControls(QVBoxLayout *layout)
{
    QHBoxLayout *hbox = new QHBoxLayout;
    m_hideFilesFilterLabel = new QLabel;
    m_hideFilesFilterLabel->setText(tr("Hide files matching:"));
    m_hideFilesFilterLabel->hide();
    hbox->addWidget(m_hideFilesFilterLabel);
    m_hideFilesfilterLineEdit = new QLineEdit;

    const QString filter = Core::ICore::settings()->value(QLatin1String(Constants::HIDE_FILE_FILTER_SETTING),
                                                          QLatin1String(Constants::HIDE_FILE_FILTER_DEFAULT)).toString();
    m_hideFilesfilterLineEdit->setText(filter);
    m_hideFilesfilterLineEdit->hide();
    hbox->addWidget(m_hideFilesfilterLineEdit);
    layout->addLayout(hbox);
}

void SelectableFilesDialogEditFiles::createShowFileFilterControls(QVBoxLayout *layout)
{
    QHBoxLayout *hbox = new QHBoxLayout;
    m_showFilesFilterLabel = new QLabel;
    m_showFilesFilterLabel->setText(tr("Show files matching:"));
    m_showFilesFilterLabel->hide();
    hbox->addWidget(m_showFilesFilterLabel);
    m_showFilesfilterLineEdit = new QLineEdit;

    const QString filter = Core::ICore::settings()->value(QLatin1String(Constants::SHOW_FILE_FILTER_SETTING),
                                                          QLatin1String(Constants::SHOW_FILE_FILTER_DEFAULT)).toString();
    m_showFilesfilterLineEdit->setText(filter);
    m_showFilesfilterLineEdit->hide();
    hbox->addWidget(m_showFilesfilterLineEdit);
    layout->addLayout(hbox);
}

void SelectableFilesDialogEditFiles::createApplyButton(QVBoxLayout *layout)
{
    QHBoxLayout *hbox = new QHBoxLayout;

    QSpacerItem *horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hbox->addItem(horizontalSpacer);

    m_applyFilterButton = new QPushButton(tr("Apply Filter"), this);
    m_applyFilterButton->hide();
    hbox->addWidget(m_applyFilterButton);
    layout->addLayout(hbox);

    connect(m_applyFilterButton, SIGNAL(clicked()), this, SLOT(applyFilter()));
}

SelectableFilesDialogEditFiles::~SelectableFilesDialogEditFiles()
{
    m_selectableFilesModel->cancel();
}

void SelectableFilesDialogEditFiles::parsingProgress(const QString &fileName)
{
    m_progressLabel->setText(tr("Generating file list...\n\n%1").arg(fileName));
}

void SelectableFilesDialogEditFiles::parsingFinished()
{
    m_hideFilesFilterLabel->show();
    m_hideFilesfilterLineEdit->show();

    m_showFilesFilterLabel->show();
    m_showFilesfilterLineEdit->show();

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

void SelectableFilesDialogEditFiles::smartExpand(const QModelIndex &index)
{
    if (m_view->model()->data(index, Qt::CheckStateRole) == Qt::PartiallyChecked) {
        m_view->expand(index);
        int rows = m_view->model()->rowCount(index);
        for (int i = 0; i < rows; ++i)
            smartExpand(index.child(i, 0));
    }
}

QStringList SelectableFilesDialogEditFiles::selectedFiles() const
{
    return m_selectableFilesModel->selectedFiles();
}

void SelectableFilesDialogEditFiles::applyFilter()
{
    const QString showFilesFilter = m_showFilesfilterLineEdit->text();
    Core::ICore::settings()->setValue(QLatin1String(Constants::SHOW_FILE_FILTER_SETTING), showFilesFilter);

    const QString hideFilesFilter = m_hideFilesfilterLineEdit->text();
    Core::ICore::settings()->setValue(QLatin1String(Constants::HIDE_FILE_FILTER_SETTING), hideFilesFilter);

    m_selectableFilesModel->applyFilter(showFilesFilter, hideFilesFilter);
}

SelectableFilesDialogAddDirectory::SelectableFilesDialogAddDirectory(const QString &path,
                                                const QStringList &files, QWidget *parent) :
    SelectableFilesDialogEditFiles(path, files, parent)
{
    setWindowTitle(tr("Add Existing Directory"));

    connect(m_selectableFilesModel, SIGNAL(parsingFinished()), this, SLOT(parsingFinished()));

    createPathChooser(static_cast<QVBoxLayout*>(layout()), path);
}

void SelectableFilesDialogAddDirectory::createPathChooser(QVBoxLayout *layout, const QString &path)
{
    QHBoxLayout *hbox = new QHBoxLayout;

    m_pathChooser = new Utils::PathChooser;
    m_pathChooser->setPath(path);
    m_pathChooser->setHistoryCompleter(QLatin1String("PE.AddToProjectDir.History"));
    m_sourceDirectoryLabel = new QLabel(tr("Source directory:"));
    hbox->addWidget(m_sourceDirectoryLabel);

    hbox->addWidget(m_pathChooser);
    layout->insertLayout(0, hbox);

    m_startParsingButton = new QPushButton(tr("Start Parsing"));
    hbox->addWidget(m_startParsingButton);

    connect(m_pathChooser, SIGNAL(validChanged(bool)), this, SLOT(validityOfDirectoryChanged(bool)));
    connect(m_startParsingButton, SIGNAL(clicked()), this, SLOT(startParsing()));
}

void SelectableFilesDialogAddDirectory::validityOfDirectoryChanged(bool validState)
{
    m_startParsingButton->setEnabled(validState);
}

void SelectableFilesDialogAddDirectory::parsingFinished()
{
    m_selectableFilesModel->selectAllFiles();
    m_selectableFilesModel->applyFilter(m_showFilesfilterLineEdit->text(),
                                        m_hideFilesfilterLineEdit->text());

    setWidgetsEnabled(true);
}

void SelectableFilesDialogAddDirectory::startParsing()
{
    setWidgetsEnabled(false);

    m_selectableFilesModel->startParsing(m_pathChooser->path());
}

void SelectableFilesDialogAddDirectory::setWidgetsEnabled(bool enabled)
{
    m_hideFilesfilterLineEdit->setEnabled(enabled);
    m_showFilesfilterLineEdit->setEnabled(enabled);
    m_applyFilterButton->setEnabled(enabled);
    m_view->setEnabled(enabled);
    m_pathChooser->setEnabled(enabled);
    m_startParsingButton->setVisible(enabled);

    m_progressLabel->setVisible(!enabled);
}

} // namespace ProjectExplorer


