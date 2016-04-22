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

#include <utils/runextensions.h>
#include <utils/algorithm.h>

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeView>
#include <QDir>
#include <utils/pathchooser.h>

namespace ProjectExplorer {

Tree::~Tree()
{
    qDeleteAll(childDirectories);
    qDeleteAll(files);
}

SelectableFilesModel::SelectableFilesModel(QObject *parent) : QAbstractItemModel(parent)
{
    connect(&m_watcher, &QFutureWatcherBase::finished,
            this, &SelectableFilesModel::buildTreeFinished);

    connect(this, &SelectableFilesModel::dataChanged, this, [this] { emit checkedFilesChanged(); });
    connect(this, &SelectableFilesModel::modelReset, this, [this] { emit checkedFilesChanged(); });

    m_root = new Tree;
}

void SelectableFilesModel::setInitialMarkedFiles(const Utils::FileNameList &files)
{
    m_files = files.toSet();
    m_allFiles = files.isEmpty();
}

void SelectableFilesModel::startParsing(const Utils::FileName &baseDir)
{
    m_watcher.cancel();
    m_watcher.waitForFinished();

    m_baseDir = baseDir;
    // Build a tree in a future
    m_rootForFuture = new Tree;
    m_rootForFuture->name = baseDir.toUserOutput();
    m_rootForFuture->fullPath = baseDir;
    m_rootForFuture->isDir = true;

    m_watcher.setFuture(Utils::runAsync(&SelectableFilesModel::run, this));
}

void SelectableFilesModel::run(QFutureInterface<void> &fi)
{
    m_futureCount = 0;
    buildTree(m_baseDir, m_rootForFuture, fi, 5);
}

void SelectableFilesModel::buildTreeFinished()
{
    beginResetModel();
    delete m_root;
    m_root = m_rootForFuture;
    m_rootForFuture = nullptr;
    m_outOfBaseDirFiles
            = Utils::filtered(m_files, [this](const Utils::FileName &fn) { return !fn.isChildOf(m_baseDir); });

    endResetModel();
    emit parsingFinished();
}

void SelectableFilesModel::cancel()
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

    //If one of the "show file" filters matches just return false
    if (Utils::anyOf(m_showFilesFilter, matchesTreeName))
        return FilterState::CHECKED;

    return Utils::anyOf(m_hideFilesFilter, matchesTreeName) ? FilterState::HIDDEN : FilterState::SHOWN;
}

void SelectableFilesModel::buildTree(const Utils::FileName &baseDir, Tree *tree,
                                     QFutureInterface<void> &fi, int symlinkDepth)
{
    if (symlinkDepth == 0)
        return;

    const QFileInfoList fileInfoList = QDir(baseDir.toString()).entryInfoList(QDir::Files |
                                                                              QDir::Dirs |
                                                                              QDir::NoDotAndDotDot);
    bool allChecked = true;
    bool allUnchecked = true;
    foreach (const QFileInfo &fileInfo, fileInfoList) {
        Utils::FileName fn = Utils::FileName(fileInfo);
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
    cancel();
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

void SelectableFilesModel::propagateDown(const QModelIndex &index)
{
    auto t = static_cast<Tree *>(index.internalPointer());
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

Utils::FileNameList SelectableFilesModel::selectedPaths() const
{
    Utils::FileNameList result;
    collectPaths(m_root, &result);
    return result;
}

void SelectableFilesModel::collectPaths(Tree *root, Utils::FileNameList *result)  const
{
    if (root->checked == Qt::Unchecked)
        return;
    result->append(root->fullPath);
    foreach (Tree *t, root->childDirectories)
        collectPaths(t, result);
}

Utils::FileNameList SelectableFilesModel::selectedFiles() const
{
    Utils::FileNameList result = m_outOfBaseDirFiles.toList();
    collectFiles(m_root, &result);
    return result;
}

Utils::FileNameList SelectableFilesModel::preservedFiles() const
{
    return m_outOfBaseDirFiles.toList();
}

bool SelectableFilesModel::hasCheckedFiles() const
{
    return m_root->checked != Qt::Unchecked;
}

void SelectableFilesModel::collectFiles(Tree *root, Utils::FileNameList *result) const
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
    QList<Glob> filter = parseFilter(showFilesfilter);
    bool mustApply = filter != m_showFilesFilter;
    m_showFilesFilter = filter;

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

    foreach (Tree *t, root->childDirectories)
        selectAllFiles(t);

    foreach (Tree *t, root->visibleFiles)
        t->checked = Qt::Checked;

    emit checkedFilesChanged();
}

Qt::CheckState SelectableFilesModel::applyFilter(const QModelIndex &index)
{
    bool allChecked = true;
    bool allUnchecked = true;
    auto t = static_cast<Tree *>(index.internalPointer());

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
            removeBlock = (filter(t->visibleFiles.at(visibleIndex)) == FilterState::HIDDEN);
        } else if (removeBlock != (filter(t->visibleFiles.at(visibleIndex)) == FilterState::HIDDEN)) {
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
            removeBlock = (filter(t->visibleFiles.at(visibleIndex)) == FilterState::HIDDEN);
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
        beginInsertRows(index, visibleIndex, visibleIndex + newIndex - startOfBlock - 1);
        for (int i= newIndex - 1; i >= startOfBlock; --i)
            t->visibleFiles.insert(visibleIndex, newRows.at(i));
        endInsertRows();
        visibleIndex = visibleIndex + newIndex-startOfBlock;
        visibleEnd = visibleEnd + newIndex-startOfBlock;
        if (newIndex >= newEnd)
            break;
    }
    if (newIndex != newEnd) {
        beginInsertRows(index, visibleIndex, visibleIndex + newEnd - newIndex - 1);
        for (int i = newEnd - 1; i >= newIndex; --i)
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
// SelectableFilesWidget
//////////

namespace {

enum class SelectableFilesWidgetRows {
    BaseDirectory, ShowFileFilter, HideFileFilter, ApplyButton, View, Progress, PreservedInformation
};

} // namespace

SelectableFilesWidget::SelectableFilesWidget(QWidget *parent) :
    QWidget(parent),
    m_baseDirChooser(new Utils::PathChooser),
    m_baseDirLabel(new QLabel),
    m_startParsingButton(new QPushButton),
    m_showFilesFilterLabel(new QLabel),
    m_showFilesFilterEdit(new QLineEdit),
    m_hideFilesFilterLabel(new QLabel),
    m_hideFilesFilterEdit(new QLineEdit),
    m_applyFilterButton(new QPushButton),
    m_view(new QTreeView),
    m_preservedFilesLabel(new QLabel),
    m_progressLabel(new QLabel)
{
    const QString showFilter
            = Core::ICore::settings()->value(QLatin1String(Constants::SHOW_FILE_FILTER_SETTING),
                                             QLatin1String(Constants::SHOW_FILE_FILTER_DEFAULT)).toString();
    const QString hideFilter
            = Core::ICore::settings()->value(QLatin1String(Constants::HIDE_FILE_FILTER_SETTING),
                                             QLatin1String(Constants::HIDE_FILE_FILTER_DEFAULT)).toString();

    auto layout = new QGridLayout(this);
    layout->setMargin(0);

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
            this, [this]() { startParsing(m_baseDirChooser->fileName()); });

    m_showFilesFilterLabel->setText(tr("Show files matching:"));
    m_showFilesFilterEdit->setText(showFilter);
    layout->addWidget(m_showFilesFilterLabel, static_cast<int>(SelectableFilesWidgetRows::ShowFileFilter), 0);
    layout->addWidget(m_showFilesFilterEdit, static_cast<int>(SelectableFilesWidgetRows::ShowFileFilter), 1, 1, 3);

    m_hideFilesFilterLabel->setText(tr("Hide files matching:"));
    m_hideFilesFilterEdit->setText(hideFilter);
    layout->addWidget(m_hideFilesFilterLabel, static_cast<int>(SelectableFilesWidgetRows::HideFileFilter), 0);
    layout->addWidget(m_hideFilesFilterEdit, static_cast<int>(SelectableFilesWidgetRows::HideFileFilter), 1, 1, 3);

    m_applyFilterButton->setText(tr("Apply Filter"));
    layout->addWidget(m_applyFilterButton, static_cast<int>(SelectableFilesWidgetRows::ApplyButton), 3);

    connect(m_applyFilterButton, &QAbstractButton::clicked,
            this, &SelectableFilesWidget::applyFilter);

    m_view->setMinimumSize(500, 400);
    m_view->setHeaderHidden(true);
    layout->addWidget(m_view, static_cast<int>(SelectableFilesWidgetRows::View), 0, 1, 4);

    layout->addWidget(m_preservedFilesLabel, static_cast<int>(SelectableFilesWidgetRows::PreservedInformation), 0, 1, 4);

    m_progressLabel->setMaximumWidth(500);
    layout->addWidget(m_progressLabel, static_cast<int>(SelectableFilesWidgetRows::Progress), 0, 1, 4);
}

SelectableFilesWidget::SelectableFilesWidget(const Utils::FileName &path,
                                             const Utils::FileNameList &files, QWidget *parent) :
    SelectableFilesWidget(parent)
{
    resetModel(path, files);
}

void SelectableFilesWidget::setAddFileFilter(const QString &filter)
{
    m_showFilesFilterEdit->setText(filter);
}

void SelectableFilesWidget::setBaseDirEditable(bool edit)
{
    m_baseDirLabel->setVisible(edit);
    m_baseDirChooser->lineEdit()->setVisible(edit);
    m_baseDirChooser->buttonAtIndex(0)->setVisible(edit);
    m_startParsingButton->setVisible(edit);
}

Utils::FileNameList SelectableFilesWidget::selectedFiles() const
{
    return m_model ? m_model->selectedFiles() : Utils::FileNameList();
}

Utils::FileNameList SelectableFilesWidget::selectedPaths() const
{
    return m_model ? m_model->selectedPaths() : Utils::FileNameList();
}

bool SelectableFilesWidget::hasFilesSelected() const
{
    return m_model ? m_model->hasCheckedFiles() : false;
}

void SelectableFilesWidget::resetModel(const Utils::FileName &path, const Utils::FileNameList &files)
{
    m_view->setModel(nullptr);

    delete m_model;
    m_model = new SelectableFilesModel(this);

    m_model->setInitialMarkedFiles(files);
    connect(m_model, &SelectableFilesModel::parsingProgress,
            this, &SelectableFilesWidget::parsingProgress);
    connect(m_model, &SelectableFilesModel::parsingFinished,
            this, &SelectableFilesWidget::parsingFinished);
    connect(m_model, &SelectableFilesModel::checkedFilesChanged,
            this, &SelectableFilesWidget::selectedFilesChanged);

    m_baseDirChooser->setFileName(path);
    m_view->setModel(m_model);

    startParsing(path);
}

void SelectableFilesWidget::cancelParsing()
{
    if (m_model)
        m_model->cancel();
}

void SelectableFilesWidget::enableWidgets(bool enabled)
{
    m_hideFilesFilterEdit->setEnabled(enabled);
    m_showFilesFilterEdit->setEnabled(enabled);
    m_applyFilterButton->setEnabled(enabled);
    m_view->setEnabled(enabled);
    m_baseDirChooser->setEnabled(enabled);

    m_startParsingButton->setEnabled(enabled);

    m_progressLabel->setVisible(!enabled);
    m_preservedFilesLabel->setVisible(m_model && !m_model->preservedFiles().isEmpty());
}

void SelectableFilesWidget::applyFilter()
{
    if (m_model)
        m_model->applyFilter(m_showFilesFilterEdit->text(), m_hideFilesFilterEdit->text());
}

void SelectableFilesWidget::baseDirectoryChanged(bool validState)
{
    m_startParsingButton->setEnabled(validState);
}

void SelectableFilesWidget::startParsing(const Utils::FileName &baseDir)
{
    if (!m_model)
        return;

    enableWidgets(false);
    applyFilter();
    m_model->startParsing(baseDir);
}

void SelectableFilesWidget::parsingProgress(const Utils::FileName &fileName)
{
    m_progressLabel->setText(tr("Generating file list...\n\n%1").arg(fileName.toUserOutput()));
}

void SelectableFilesWidget::parsingFinished()
{
    if (!m_model)
        return;

    smartExpand(m_model->index(0,0, QModelIndex()));

    const Utils::FileNameList preservedFiles = m_model->preservedFiles();
    m_preservedFilesLabel->setText(tr("Not showing %n files that are outside of the base directory.\n"
                                      "These files are preserved.", 0, preservedFiles.count()));

    enableWidgets(true);
}

void SelectableFilesWidget::smartExpand(const QModelIndex &index)
{
    if (m_view->model()->data(index, Qt::CheckStateRole) == Qt::PartiallyChecked) {
        m_view->expand(index);
        int rows = m_view->model()->rowCount(index);
        for (int i = 0; i < rows; ++i)
            smartExpand(index.child(i, 0));
    }
}

//////////
// SelectableFilesDialogs
//////////

SelectableFilesDialogEditFiles::SelectableFilesDialogEditFiles(const Utils::FileName &path,
                                                               const Utils::FileNameList &files,
                                                               QWidget *parent) :
    QDialog(parent),
    m_filesWidget(new SelectableFilesWidget(path, files))
{
    setWindowTitle(tr("Edit Files"));

    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_filesWidget);

    m_filesWidget->setBaseDirEditable(false);

    auto buttonBox = new QDialogButtonBox(Qt::Horizontal, this);
    buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
    layout->addWidget(buttonBox);
}

Utils::FileNameList SelectableFilesDialogEditFiles::selectedFiles() const
{
    return m_filesWidget->selectedFiles();
}

//////////
// SelectableFilesDialogAddDirectory
//////////


SelectableFilesDialogAddDirectory::SelectableFilesDialogAddDirectory(const Utils::FileName &path,
                                                                     const Utils::FileNameList &files,
                                                                     QWidget *parent) :
    SelectableFilesDialogEditFiles(path, files, parent)
{
    setWindowTitle(tr("Add Existing Directory"));

    m_filesWidget->setBaseDirEditable(true);
}

} // namespace ProjectExplorer


