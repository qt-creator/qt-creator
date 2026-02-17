// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangselectablefilesdialog.h"

#include "clangtoolstr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/find/itemviewfind.h>
#include <coreplugin/find/textfindconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QHash>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStandardItem>
#include <QTreeView>

using namespace CppEditor;
using namespace Utils;
using namespace ProjectExplorer;

namespace ClangTools {
namespace Internal {

class SelectableFilesModel : public QSortFilterProxyModel
{
public:
    SelectableFilesModel(const Project *project, QObject *parent)
        : QSortFilterProxyModel(parent)
        , m_project(project)
    {
        const auto saveSelection = [this] {
            if (m_awaitsRestore)
                return;
            m_savedSelection = minimalSelection();
            m_awaitsRestore = true;
            beginResetModel();
            m_checkStates.clear();
            endResetModel();
        };
        const auto restoreSelection = [this] {
            if (m_awaitsRestore) {
                restoreMinimalSelection(m_savedSelection);
                m_awaitsRestore = false;
            }
        };
        connect(this, &QAbstractProxyModel::sourceModelChanged, this, [=, this] {
            connect(sourceModel(), &QAbstractItemModel::layoutAboutToBeChanged, this, saveSelection);
            connect(sourceModel(), &QAbstractItemModel::layoutChanged, this, restoreSelection);
            connect(sourceModel(), &QAbstractItemModel::modelAboutToBeReset, this, saveSelection);
            connect(sourceModel(), &QAbstractItemModel::modelReset, this, restoreSelection);
            connect(sourceModel(), &QAbstractItemModel::rowsAboutToBeInserted, this, saveSelection);
            connect(sourceModel(), &QAbstractItemModel::rowsInserted, this, restoreSelection);
            connect(sourceModel(), &QAbstractItemModel::rowsAboutToBeMoved, this, saveSelection);
            connect(sourceModel(), &QAbstractItemModel::rowsRemoved, this, restoreSelection);
        });
    }

    bool hasCheckedFiles() const
    {
        return Utils::anyOf(m_checkStates, [](Qt::CheckState state) {
            return state == Qt::Checked;
        });
    }

    void reset(const FileInfos &fileInfos)
    {
        beginResetModel();
        m_fileInfos.clear();
        m_checkStates.clear();
        for (const FileInfo &fi : fileInfos)
            m_fileInfos.insert(fi.file, fi);
        endResetModel();
    }

    void selectAllFiles()
    {
        beginResetModel();
        for (int i = 0; i < rowCount(); ++i)
            setData(index(i, 0), Qt::Checked, Qt::CheckStateRole);
        endResetModel();
    }

    FileInfoSelection minimalSelection() const
    {
        FileInfoSelection selection;
        collectSelectedFiles(selection, {});
        return selection;
    }

    void restoreMinimalSelection(const FileInfoSelection &selection)
    {
        m_checkStates.clear();
        restoreMinimalSelection(selection, {});
    }

    FileInfos selectedFiles() const
    {
        FileInfos files;
        for (auto it = m_checkStates.cbegin(); it != m_checkStates.cend(); ++it) {
            if (it.key()->asFileNode() && it.value() == Qt::Checked)
                files.push_back(m_fileInfos.value(it.key()->filePath()));
        }
        return files;
    }

private:
    void collectSelectedFiles(FileInfoSelection &selection, const QModelIndex &proxyIndex) const
    {
        if (proxyIndex.isValid()
            && data(proxyIndex, Qt::CheckStateRole).value<Qt::CheckState>() == Qt::Checked) {
            Node * const node = nodeFromIndex(mapToSource(proxyIndex));
            if (node->asFileNode())
                selection.files << node->filePath();
            else
                selection.dirs << node->filePath();
        } else {
            for (int i = 0; i < rowCount(proxyIndex); ++i)
                collectSelectedFiles(selection, index(i, 0, proxyIndex));
        }
    }

    void restoreMinimalSelection(const FileInfoSelection &selection, const QModelIndex &proxyIndex)
    {
        if (proxyIndex.isValid()) {
            const Node * const node = nodeFromIndex(mapToSource(proxyIndex));
            if (selection.dirs.contains(node->filePath())
                || selection.files.contains(node->filePath())) {
                setData(proxyIndex, Qt::Checked, Qt::CheckStateRole);
            }
        }
        for (int i = 0; i < rowCount(proxyIndex); ++i)
            restoreMinimalSelection(selection, index(i, 0, proxyIndex));
    }

    Node *nodeFromIndex(const QModelIndex &sourceIndex) const
    {
        return sourceModel()->data(sourceIndex, Project::NodeRole).value<Node *>();
    }

    Qt::CheckState checkStateFromChildren(const QModelIndex &proxyIndex) const
    {
        const int childCount = rowCount(proxyIndex);
        bool hasCheckedChildren = false;
        bool hasUncheckedChildren = false;
        for (int i = 0; i < childCount; ++i) {
            const auto state
                = data(index(i, 0, proxyIndex), Qt::CheckStateRole).value<Qt::CheckState>();
            switch (state) {
            case Qt::Checked:
                hasCheckedChildren = true;
                break;
            case Qt::Unchecked:
                hasUncheckedChildren = true;
                break;
            case Qt::PartiallyChecked:
                return Qt::PartiallyChecked;
            }
            if (hasCheckedChildren && hasUncheckedChildren)
                return Qt::PartiallyChecked;
        }
        if (hasCheckedChildren)
            return Qt::Checked;
        return Qt::Unchecked;
    }

    void propagateCheckStateToChildren(const QModelIndex &proxyIndex, Qt::CheckState state)
    {
        QTC_ASSERT(state != Qt::PartiallyChecked, return);
        const int childCount = rowCount(proxyIndex);
        for (int i = 0; i < childCount; ++i) {
            const QModelIndex idx = index(i, 0, proxyIndex);
            const Qt::CheckState prevState = data(idx, Qt::CheckStateRole).value<Qt::CheckState>();
            if (prevState != state) {
                Node * const node = nodeFromIndex(mapToSource(idx));
                m_checkStates[node] = state;
                if (!node->asFileNode())
                    propagateCheckStateToChildren(idx, state);
            }
        }
        if (childCount) {
            emit dataChanged(
                index(0, 0, proxyIndex), index(childCount - 1, 0, proxyIndex), {Qt::CheckStateRole});
        }
    }

    void propagateCheckStateToParents(const QModelIndex &proxyIndex)
    {
        const QModelIndex parentIdx = parent(proxyIndex);
        if (!parentIdx.isValid())
            return;
        const Qt::CheckState prevState = data(parentIdx, Qt::CheckStateRole).value<Qt::CheckState>();
        const Qt::CheckState newState = checkStateFromChildren(parentIdx);
        if (newState != prevState) {
            m_checkStates[nodeFromIndex(mapToSource(parentIdx))] = newState;
            propagateCheckStateToParents(parentIdx);
            emit dataChanged(parentIdx, parentIdx, {Qt::CheckStateRole});
        }
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (role != Qt::CheckStateRole)
            return sourceModel()->data(mapToSource(index), role);

        Node * const node = nodeFromIndex(mapToSource(index));
        if (const auto state = m_checkStates.constFind(node); state != m_checkStates.constEnd())
            return *state;

        if (node->asFileNode())
            return Qt::Unchecked;
        return m_checkStates[node] = checkStateFromChildren(index);
    }

    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override
    {
        const QModelIndex sourceIndex = sourceModel()->index(source_row, 0, source_parent);
        const Node * const node = nodeFromIndex(sourceIndex);
        QTC_ASSERT(node, return false);
        if (node->getProject() != m_project)
            return false;
        if (node->asFileNode())
            return m_fileInfos.contains(node->filePath());

        // Prevent directory-only hierarchies.
        for (int i = 0; i < sourceModel()->rowCount(sourceIndex); ++i)
            if (filterAcceptsRow(i, sourceIndex))
                return true;
        return false;
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        Q_UNUSED(index)
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {
        if (role != Qt::CheckStateRole)
            return false;

        const auto state = value.value<Qt::CheckState>();
        Node * const node = nodeFromIndex(mapToSource(index));
        m_checkStates.insert(node, state);
        if (!node->asFileNode())
            propagateCheckStateToChildren(index, state);
        propagateCheckStateToParents(index);
        return true;
    }

    const Project * const m_project;
    QHash<FilePath, FileInfo> m_fileInfos;
    FileInfoSelection m_savedSelection;
    bool m_awaitsRestore = false;
    mutable QHash<Node *, Qt::CheckState> m_checkStates;
};

SelectableFilesDialog::SelectableFilesDialog(Project *project,
                                             const FileInfoProviders &fileInfoProviders,
                                             int initialProviderIndex)
    : QDialog(nullptr)
    , m_filesModel(new SelectableFilesModel(project, this))
    , m_fileInfoProviders(fileInfoProviders)
    , m_project(project)
{
    setWindowTitle(Tr::tr("Files to Analyze"));
    resize(700, 600);

    m_fileFilterComboBox = new QComboBox(this);
    m_fileFilterComboBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // Files View
    // Make find actions available in this dialog, e.g. Strg+F for the view.
    addAction(Core::ActionManager::command(Core::Constants::FIND_IN_DOCUMENT)->action());
    addAction(Core::ActionManager::command(Core::Constants::FIND_NEXT)->action());
    addAction(Core::ActionManager::command(Core::Constants::FIND_PREVIOUS)->action());

    m_fileView = new QTreeView;
    m_fileView->setHeaderHidden(true);
    m_filesModel->setSourceModel(ProjectTree::createProjectsModel(this));
    m_fileView->setModel(m_filesModel);

    // Filter combo box
    for (const FileInfoProvider &provider : m_fileInfoProviders) {
        m_fileFilterComboBox->addItem(provider.displayName);

        // Disable item if it has no file infos
        auto *model = qobject_cast<QStandardItemModel *>(m_fileFilterComboBox->model());
        QStandardItem *item = model->item(m_fileFilterComboBox->count() - 1);
        item->setFlags(provider.fileInfos.empty() ? item->flags() & ~Qt::ItemIsEnabled
                                                  : item->flags() | Qt::ItemIsEnabled);
    }

    int providerIndex = initialProviderIndex;
    if (m_fileInfoProviders[providerIndex].fileInfos.empty())
        providerIndex = 0;
    m_fileFilterComboBox->setCurrentIndex(providerIndex);

    onFileFilterChanged(providerIndex);
    connect(m_fileFilterComboBox, &QComboBox::currentIndexChanged,
            this, &SelectableFilesDialog::onFileFilterChanged);

    auto analyzeButton = new QPushButton(Tr::tr("Analyze"), this);
    analyzeButton->setEnabled(m_filesModel->hasCheckedFiles());

    // Buttons
    auto buttons = new QDialogButtonBox;
    buttons->setStandardButtons(QDialogButtonBox::Cancel);
    buttons->addButton(analyzeButton, QDialogButtonBox::AcceptRole);

    connect(m_filesModel, &QAbstractItemModel::dataChanged, this, [this, analyzeButton] {
        analyzeButton->setEnabled(m_filesModel->hasCheckedFiles());
    });

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    using namespace Layouting;

    Column {
        m_fileFilterComboBox,
        Core::ItemViewFind::createSearchableWrapper(m_fileView, Core::ItemViewFind::LightColored),
        buttons
    }.attachTo(this);
}

SelectableFilesDialog::~SelectableFilesDialog() = default;

FileInfos SelectableFilesDialog::fileInfos() const
{
    return m_filesModel->selectedFiles();
}

int SelectableFilesDialog::currentProviderIndex() const
{
    return m_fileFilterComboBox->currentIndex();
}

void SelectableFilesDialog::onFileFilterChanged(int index)
{
    // Remember previous filter/selection
    if (m_previousProviderIndex != -1)
        m_fileInfoProviders[m_previousProviderIndex].selection = m_filesModel->minimalSelection();
    m_previousProviderIndex = index;

    // Reset model
    const FileInfoProvider &provider = m_fileInfoProviders[index];
    m_filesModel->reset(provider.fileInfos);

    // Handle selection
    if (provider.selection.dirs.isEmpty() && provider.selection.files.isEmpty())
        m_filesModel->selectAllFiles(); // Initially, all files are selected
    else
        m_filesModel->restoreMinimalSelection(provider.selection);

    // Expand
    if (provider.expandPolicy == FileInfoProvider::All)
        m_fileView->expandAll();
    else
        m_fileView->expandToDepth(2);
}

void SelectableFilesDialog::accept()
{
    const FileInfoSelection selection = m_filesModel->minimalSelection();
    FileInfoProvider &provider = m_fileInfoProviders[m_fileFilterComboBox->currentIndex()];
    provider.onSelectionAccepted(selection);

    QDialog::accept();
}

} // namespace Internal
} // namespace ClangTools
