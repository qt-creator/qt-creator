/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "selectablefilesmodel.h"
#include "projectexplorerconstants.h"

#include <coreplugin/fileiconprovider.h>
#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/fancylineedit.h>
#include <utils/pathchooser.h>
#include <utils/runextensions.h>
#include <utils/stringutils.h>

#include <QDialogButtonBox>
#include <QDir>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeView>

namespace ProjectExplorer {

const char HIDE_FILE_FILTER_DEFAULT[] = "Makefile*; *.o; *.lo; *.la; *.obj; *~; *.files;"
                                        " *.config; *.creator; *.user*; *.includes; *.autosave";
const char SELECT_FILE_FILTER_DEFAULT[] = "*.c; *.cc; *.cpp; *.cp; *.cxx; *.c++; *.h; *.hh; *.hpp; *.hxx;";

SelectableFilesModel::SelectableFilesModel(QObject *parent) : QAbstractItemModel(parent)
{
    m_root = new Tree;
}

void SelectableFilesModel::setInitialMarkedFiles(const Utils::FilePaths &files)
{
    m_files = Utils::toSet(files);
    m_allFiles = files.isEmpty();
}

void SelectableFilesFromDirModel::startParsing(const Utils::FilePath &baseDir)
{
    m_watcher.cancel();
    m_watcher.waitForFinished();

    m_baseDir = baseDir;
    // Build a tree in a future
    m_rootForFuture = new Tree;
    m_rootForFuture->name = baseDir.toUserOutput();
    m_rootForFuture->fullPath = baseDir;
    m_rootForFuture->isDir = true;

    m_watcher.setFuture(Utils::runAsync(&SelectableFilesFromDirModel::run, this));
}

void SelectableFilesFromDirModel::run(QFutureInterface<void> &fi)
{
    m_futureCount = 0;
    buildTree(m_baseDir, m_rootForFuture, fi, 5);
}

void SelectableFilesFromDirModel::buildTreeFinished()
{
    beginResetModel();
    delete m_root;
    m_root = m_rootForFuture;
    m_rootForFuture = nullptr;
    m_outOfBaseDirFiles
            = Utils::filtered(m_files, [this](const Utils::FilePath &fn) { return !fn.isChildOf(m_baseDir); });

    endResetModel();
    emit parsingFinished();
}

void SelectableFilesFromDirModel::cancel()
{
    m_watcher.cancel();
    m_watcher.waitForFinished();
}

SelectableFilesModel::FilterState SelectableFilesModel::filter(Tree *t)
{
    if (t->isDir)
        return FilterState::SHOWN;
    if (m_files.contains(t->fullPath))
        return FilterState::CHECKED;

    auto matchesTreeName = [t](const Glob &g) {
        return g.isMatch(t->name);
    };

    if (Utils::anyOf(m_selectFilesFilter, matchesTreeName))
        return FilterState::CHECKED;

    return Utils::anyOf(m_hideFilesFilter, matchesTreeName) ? FilterState::HIDDEN : FilterState::SHOWN;
}

void SelectableFilesFromDirModel::buildTree(const Utils::FilePath &baseDir, Tree *tree,
                                            QFutureInterface<void> &fi, int symlinkDepth)
{
    if (symlinkDepth == 0)
        return;

    const QFileInfoList fileInfoList = QDir(baseDir.toString()).entryInfoList(QDir::Files |
                                                                              QDir::Dirs |
                                                                              QDir::NoDotAndDotDot);
    bool allChecked = true;
    bool allUnchecked = true;
    for (const QFileInfo &fileInfo : fileInfoList) {
        Utils::FilePath fn = Utils::FilePath::fromFileInfo(fileInfo);
        if (m_futureCount % 100) {
            emit parsingProgress(fn);
            if (fi.isCanceled())
                return;
        }
        ++m_futureCount;
        if (fileInfo.isDir()) {
            auto t = new Tree;
            t->parent = tree;
            t->name = fileInfo.fileName();
            t->fullPath = fn;
            t->isDir = true;
            buildTree(fn, t, fi, symlinkDepth - fileInfo.isSymLink());
            allChecked &= t->checked == Qt::Checked;
            allUnchecked &= t->checked == Qt::Unchecked;
            tree->childDirectories.append(t);
        } else {
            auto t = new Tree;
            t->parent = tree;
            t->name = fileInfo.fileName();
            FilterState state = filter(t);
            t->checked = ((m_allFiles && state == FilterState::CHECKED)
                          || m_files.contains(fn)) ? Qt::Checked : Qt::Unchecked;
            t->fullPath = fn;
            t->isDir = false;
            allChecked &= t->checked == Qt::Checked;
            allUnchecked &= t->checked == Qt::Unchecked;
            tree->files.append(t);
            if (state != FilterState::HIDDEN)
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
    delete m_root;
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
    auto parentT = static_cast<Tree *>(parent.internalPointer());
    return parentT->childDirectories.size() + parentT->visibleFiles.size();
}

QModelIndex SelectableFilesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid())
        return createIndex(row, column, m_root);
    auto parentT = static_cast<Tree *>(parent.internalPointer());
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
    auto parent = static_cast<Tree *>(child.internalPointer())->parent;
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
    auto t = static_cast<Tree *>(index.internalPointer());
    if (role == Qt::DisplayRole)
        return t->name;
    if (role == Qt::CheckStateRole)
        return t->checked;
    if (role == Qt::DecorationRole) {
        if (t->icon.isNull())
            t->icon = Core::FileIconProvider::icon(t->fullPath.toFileInfo());
        return t->icon;
    }
    return QVariant();
}

bool SelectableFilesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole) {
        // We can do that!
        auto t = static_cast<Tree *>(index.internalPointer());
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
    auto parentT = static_cast<Tree *>(parent.internalPointer());
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

void SelectableFilesModel::propagateDown(const QModelIndex &idx)
{
    auto t = static_cast<Tree *>(idx.internalPointer());
    for (int i = 0; i<t->childDirectories.size(); ++i) {
        t->childDirectories[i]->checked = t->checked;
        propagateDown(index(i, 0, idx));
    }
    for (int i = 0; i<t->files.size(); ++i)
        t->files[i]->checked = t->checked;

    int rows = rowCount(idx);
    if (rows)
        emit dataChanged(index(0, 0, idx), index(rows-1, 0, idx));
}

Qt::ItemFlags SelectableFilesModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
}

Utils::FilePaths SelectableFilesModel::selectedPaths() const
{
    Utils::FilePaths result;
    collectPaths(m_root, &result);
    return result;
}

void SelectableFilesModel::collectPaths(Tree *root, Utils::FilePaths *result)  const
{
    if (root->checked == Qt::Unchecked)
        return;
    result->append(root->fullPath);
    for (Tree *t : qAsConst(root->childDirectories))
        collectPaths(t, result);
}

Utils::FilePaths SelectableFilesModel::selectedFiles() const
{
    Utils::FilePaths result = Utils::toList(m_outOfBaseDirFiles);
    collectFiles(m_root, &result);
    return result;
}

Utils::FilePaths SelectableFilesModel::preservedFiles() const
{
    return Utils::toList(m_outOfBaseDirFiles);
}

bool SelectableFilesModel::hasCheckedFiles() const
{
    return m_root->checked != Qt::Unchecked;
}

void SelectableFilesModel::collectFiles(Tree *root, Utils::FilePaths *result) const
{
    if (root->checked == Qt::Unchecked)
        return;
    for (Tree *t : qAsConst(root->childDirectories))
        collectFiles(t, result);
    for (Tree *t : qAsConst(root->visibleFiles))
        if (t->checked == Qt::Checked)
            result->append(t->fullPath);
}

QList<Glob> SelectableFilesModel::parseFilter(const QString &filter)
{
    QList<Glob> result;
    const QStringList list = filter.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString &e : list) {
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
            const QString re = QRegularExpression::wildcardToRegularExpression(entry);
            g.matchRegexp = QRegularExpression(re, QRegularExpression::CaseInsensitiveOption);
        }
        result.append(g);
    }
    return result;
}

void SelectableFilesModel::applyFilter(const QString &selectFilesfilter, const QString &hideFilesfilter)
{
    QList<Glob> filter = parseFilter(selectFilesfilter);
    bool mustApply = filter != m_selectFilesFilter;
    m_selectFilesFilter = filter;

    filter = parseFilter(hideFilesfilter);
    mustApply = mustApply || (filter != m_hideFilesFilter);
    m_hideFilesFilter = filter;

    if (mustApply)
        applyFilter(createIndex(0, 0, m_root));
}

void SelectableFilesModel::selectAllFiles()
{
    selectAllFiles(m_root);
}

void SelectableFilesModel::selectAllFiles(Tree *root)
{
    root->checked = Qt::Checked;

    for (Tree *t : qAsConst(root->childDirectories))
        selectAllFiles(t);

    for (Tree *t : qAsConst(root->visibleFiles))
        t->checked = Qt::Checked;

    emit checkedFilesChanged();
}

Qt::CheckState SelectableFilesModel::applyFilter(const QModelIndex &idx)
{
    bool allChecked = true;
    bool allUnchecked = true;
    auto t = static_cast<Tree *>(idx.internalPointer());

    for (int i=0; i < t->childDirectories.size(); ++i) {
        Qt::CheckState childCheckState = applyFilter(index(i, 0, idx));
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
            removeBlock = (filter(t->visibleFiles.at(visibleIndex)) == FilterState::HIDDEN);
        } else if (removeBlock != (filter(t->visibleFiles.at(visibleIndex)) == FilterState::HIDDEN)) {
            if (removeBlock) {
                beginRemoveRows(idx, startOfBlock, visibleIndex - 1);
                for (int i=startOfBlock; i < visibleIndex; ++i)
                    t->visibleFiles[i]->checked = Qt::Unchecked;
                t->visibleFiles.erase(t->visibleFiles.begin() + startOfBlock,
                                      t->visibleFiles.begin() + visibleIndex);
                endRemoveRows();
                visibleIndex = startOfBlock; // start again at startOfBlock
                visibleEnd = t->visibleFiles.size();
            }
            removeBlock = (filter(t->visibleFiles.at(visibleIndex)) == FilterState::HIDDEN);
            startOfBlock = visibleIndex;
        }
    }
    if (removeBlock) {
        beginRemoveRows(idx, startOfBlock, visibleEnd - 1);
        for (int i=startOfBlock; i < visibleEnd; ++i)
            t->visibleFiles[i]->checked = Qt::Unchecked;
        t->visibleFiles.erase(t->visibleFiles.begin() + startOfBlock,
                              t->visibleFiles.begin() + visibleEnd);
        endRemoveRows();
    }

    // Figure out which rows should be visible
    QList<Tree *> newRows;
    for (int i=0; i < t->files.size(); ++i) {
        if (filter(t->files.at(i)) != FilterState::HIDDEN)
            newRows.append(t->files.at(i));
    }
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
        beginInsertRows(idx, visibleIndex, visibleIndex + newIndex - startOfBlock - 1);
        for (int i= newIndex - 1; i >= startOfBlock; --i)
            t->visibleFiles.insert(visibleIndex, newRows.at(i));
        endInsertRows();
        visibleIndex = visibleIndex + newIndex-startOfBlock;
        visibleEnd = visibleEnd + newIndex-startOfBlock;
        if (newIndex >= newEnd)
            break;
    }
    if (newIndex != newEnd) {
        beginInsertRows(idx, visibleIndex, visibleIndex + newEnd - newIndex - 1);
        for (int i = newEnd - 1; i >= newIndex; --i)
            t->visibleFiles.insert(visibleIndex, newRows.at(i));
        endInsertRows();
    }

    for (int i=0; i < t->visibleFiles.size(); ++i) {
        Tree * const fileNode = t->visibleFiles.at(i);
        fileNode->checked = filter(fileNode) == FilterState::CHECKED ? Qt::Checked : Qt::Unchecked;
        if (fileNode->checked)
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
        emit dataChanged(idx, idx);
    }

    return newState;
}

//////////
// SelectableFilesWidget
//////////

namespace {

enum class SelectableFilesWidgetRows {
    BaseDirectory, SelectFileFilter, HideFileFilter, ApplyButton, View, Progress, PreservedInformation
};

} // namespace

SelectableFilesWidget::SelectableFilesWidget(QWidget *parent) :
    QWidget(parent),
    m_baseDirChooser(new Utils::PathChooser),
    m_baseDirLabel(new QLabel),
    m_startParsingButton(new QPushButton),
    m_selectFilesFilterLabel(new QLabel),
    m_selectFilesFilterEdit(new Utils::FancyLineEdit),
    m_hideFilesFilterLabel(new QLabel),
    m_hideFilesFilterEdit(new Utils::FancyLineEdit),
    m_applyFiltersButton(new QPushButton),
    m_view(new QTreeView),
    m_preservedFilesLabel(new QLabel),
    m_progressLabel(new QLabel)
{
    const QString selectFilter
            = Core::ICore::settings()->value("GenericProject/ShowFileFilter",
                                             QLatin1String(SELECT_FILE_FILTER_DEFAULT)).toString();
    const QString hideFilter
            = Core::ICore::settings()->value("GenericProject/FileFilter",
                                             QLatin1String(HIDE_FILE_FILTER_DEFAULT)).toString();

    auto layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_baseDirLabel->setText(tr("Source directory:"));
    m_baseDirChooser->setHistoryCompleter(QLatin1String("PE.AddToProjectDir.History"));
    m_startParsingButton->setText(tr("Start Parsing"));
    layout->addWidget(m_baseDirLabel, static_cast<int>(SelectableFilesWidgetRows::BaseDirectory), 0);
    layout->addWidget(m_baseDirChooser->lineEdit(), static_cast<int>(SelectableFilesWidgetRows::BaseDirectory), 1);
    layout->addWidget(m_baseDirChooser->buttonAtIndex(0), static_cast<int>(SelectableFilesWidgetRows::BaseDirectory), 2);
    layout->addWidget(m_startParsingButton, static_cast<int>(SelectableFilesWidgetRows::BaseDirectory), 3);

    connect(m_baseDirChooser, &Utils::PathChooser::validChanged,
            this, &SelectableFilesWidget::baseDirectoryChanged);
    connect(m_startParsingButton, &QAbstractButton::clicked,
            this, [this]() { startParsing(m_baseDirChooser->filePath()); });

    m_selectFilesFilterLabel->setText(tr("Select files matching:"));
    m_selectFilesFilterEdit->setText(selectFilter);
    layout->addWidget(m_selectFilesFilterLabel, static_cast<int>(SelectableFilesWidgetRows::SelectFileFilter), 0);
    layout->addWidget(m_selectFilesFilterEdit, static_cast<int>(SelectableFilesWidgetRows::SelectFileFilter), 1, 1, 3);

    m_hideFilesFilterLabel->setText(tr("Hide files matching:"));
    m_hideFilesFilterEdit->setText(hideFilter);
    layout->addWidget(m_hideFilesFilterLabel, static_cast<int>(SelectableFilesWidgetRows::HideFileFilter), 0);
    layout->addWidget(m_hideFilesFilterEdit, static_cast<int>(SelectableFilesWidgetRows::HideFileFilter), 1, 1, 3);

    m_applyFiltersButton->setText(tr("Apply Filters"));
    layout->addWidget(m_applyFiltersButton, static_cast<int>(SelectableFilesWidgetRows::ApplyButton), 3);

    connect(m_applyFiltersButton, &QAbstractButton::clicked,
            this, &SelectableFilesWidget::applyFilter);

    m_view->setMinimumSize(500, 400);
    m_view->setHeaderHidden(true);
    layout->addWidget(m_view, static_cast<int>(SelectableFilesWidgetRows::View), 0, 1, 4);

    layout->addWidget(m_preservedFilesLabel, static_cast<int>(SelectableFilesWidgetRows::PreservedInformation), 0, 1, 4);

    m_progressLabel->setMaximumWidth(500);
    layout->addWidget(m_progressLabel, static_cast<int>(SelectableFilesWidgetRows::Progress), 0, 1, 4);
}

SelectableFilesWidget::SelectableFilesWidget(const Utils::FilePath &path,
                                             const Utils::FilePaths &files, QWidget *parent) :
    SelectableFilesWidget(parent)
{
    resetModel(path, files);
}

void SelectableFilesWidget::setAddFileFilter(const QString &filter)
{
    m_selectFilesFilterEdit->setText(filter);
    if (m_applyFiltersButton->isEnabled())
        applyFilter();
    else
        m_filteringScheduled = true;
}

void SelectableFilesWidget::setBaseDirEditable(bool edit)
{
    m_baseDirLabel->setVisible(edit);
    m_baseDirChooser->lineEdit()->setVisible(edit);
    m_baseDirChooser->buttonAtIndex(0)->setVisible(edit);
    m_startParsingButton->setVisible(edit);
}

Utils::FilePaths SelectableFilesWidget::selectedFiles() const
{
    return m_model ? m_model->selectedFiles() : Utils::FilePaths();
}

Utils::FilePaths SelectableFilesWidget::selectedPaths() const
{
    return m_model ? m_model->selectedPaths() : Utils::FilePaths();
}

bool SelectableFilesWidget::hasFilesSelected() const
{
    return m_model ? m_model->hasCheckedFiles() : false;
}

void SelectableFilesWidget::resetModel(const Utils::FilePath &path, const Utils::FilePaths &files)
{
    m_view->setModel(nullptr);

    delete m_model;
    m_model = new SelectableFilesFromDirModel(this);

    m_model->setInitialMarkedFiles(files);
    connect(m_model, &SelectableFilesFromDirModel::parsingProgress,
            this, &SelectableFilesWidget::parsingProgress);
    connect(m_model, &SelectableFilesFromDirModel::parsingFinished,
            this, &SelectableFilesWidget::parsingFinished);
    connect(m_model, &SelectableFilesModel::checkedFilesChanged,
            this, &SelectableFilesWidget::selectedFilesChanged);

    m_baseDirChooser->setFilePath(path);
    m_view->setModel(m_model);

    startParsing(path);
}

void SelectableFilesWidget::cancelParsing()
{
    if (m_model)
        m_model->cancel();
}

void SelectableFilesWidget::enableFilterHistoryCompletion(const QString &keyPrefix)
{
    m_selectFilesFilterEdit->setHistoryCompleter(keyPrefix + ".select", true);
    m_hideFilesFilterEdit->setHistoryCompleter(keyPrefix + ".hide", true);
}

void SelectableFilesWidget::enableWidgets(bool enabled)
{
    m_hideFilesFilterEdit->setEnabled(enabled);
    m_selectFilesFilterEdit->setEnabled(enabled);
    m_applyFiltersButton->setEnabled(enabled);
    m_view->setEnabled(enabled);
    m_baseDirChooser->setEnabled(enabled);

    m_startParsingButton->setEnabled(enabled);

    m_progressLabel->setVisible(!enabled);
    m_preservedFilesLabel->setVisible(m_model && !m_model->preservedFiles().isEmpty());
}

void SelectableFilesWidget::applyFilter()
{
    m_filteringScheduled = false;
    if (m_model)
        m_model->applyFilter(m_selectFilesFilterEdit->text(), m_hideFilesFilterEdit->text());
}

void SelectableFilesWidget::baseDirectoryChanged(bool validState)
{
    m_startParsingButton->setEnabled(validState);
}

void SelectableFilesWidget::startParsing(const Utils::FilePath &baseDir)
{
    if (!m_model)
        return;

    enableWidgets(false);
    applyFilter();
    m_model->startParsing(baseDir);
}

void SelectableFilesWidget::parsingProgress(const Utils::FilePath &fileName)
{
    m_progressLabel->setText(tr("Generating file list...\n\n%1").arg(fileName.toUserOutput()));
}

void SelectableFilesWidget::parsingFinished()
{
    if (!m_model)
        return;

    smartExpand(m_model->index(0,0, QModelIndex()));

    const Utils::FilePaths preservedFiles = m_model->preservedFiles();
    m_preservedFilesLabel->setText(tr("Not showing %n files that are outside of the base directory.\n"
                                      "These files are preserved.", nullptr, preservedFiles.count()));

    enableWidgets(true);
    if (m_filteringScheduled)
        applyFilter();
}

void SelectableFilesWidget::smartExpand(const QModelIndex &idx)
{
    QAbstractItemModel *model = m_view->model();
    if (model->data(idx, Qt::CheckStateRole) == Qt::PartiallyChecked) {
        m_view->expand(idx);
        int rows = model->rowCount(idx);
        for (int i = 0; i < rows; ++i)
            smartExpand(model->index(i, 0, idx));
    }
}

//////////
// SelectableFilesDialogs
//////////

SelectableFilesDialogEditFiles::SelectableFilesDialogEditFiles(const Utils::FilePath &path,
                                                               const Utils::FilePaths &files,
                                                               QWidget *parent) :
    QDialog(parent),
    m_filesWidget(new SelectableFilesWidget(path, files))
{
    setWindowTitle(tr("Edit Files"));

    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_filesWidget);

    m_filesWidget->setBaseDirEditable(false);
    m_filesWidget->enableFilterHistoryCompletion(Constants::ADD_FILES_DIALOG_FILTER_HISTORY_KEY);

    auto buttonBox = new QDialogButtonBox(Qt::Horizontal, this);
    buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
    layout->addWidget(buttonBox);
}

Utils::FilePaths SelectableFilesDialogEditFiles::selectedFiles() const
{
    return m_filesWidget->selectedFiles();
}


//////////
// SelectableFilesDialogAddDirectory
//////////


SelectableFilesDialogAddDirectory::SelectableFilesDialogAddDirectory(const Utils::FilePath &path,
                                                                     const Utils::FilePaths &files,
                                                                     QWidget *parent) :
    SelectableFilesDialogEditFiles(path, files, parent)
{
    setWindowTitle(tr("Add Existing Directory"));

    m_filesWidget->setBaseDirEditable(true);
}

SelectableFilesFromDirModel::SelectableFilesFromDirModel(QObject *parent)
    : SelectableFilesModel(parent)
{
    connect(&m_watcher, &QFutureWatcherBase::finished,
            this, &SelectableFilesFromDirModel::buildTreeFinished);

    connect(this, &SelectableFilesFromDirModel::dataChanged,
            this, [this] { emit checkedFilesChanged(); });
    connect(this, &SelectableFilesFromDirModel::modelReset,
            this, [this] { emit checkedFilesChanged(); });
}

SelectableFilesFromDirModel::~SelectableFilesFromDirModel()
{
    cancel();
}

} // namespace ProjectExplorer


