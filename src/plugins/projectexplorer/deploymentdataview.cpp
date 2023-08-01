// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "deploymentdataview.h"

#include "buildsystem.h"
#include "deployconfiguration.h"
#include "deploymentdata.h"
#include "projectexplorertr.h"
#include "target.h"

#include <utils/qtcassert.h>
#include <utils/treemodel.h>

#include <QAbstractTableModel>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class DeploymentDataItem : public TreeItem
{
public:
    DeploymentDataItem() = default;
    DeploymentDataItem(const DeployableFile &file, bool isEditable)
        : file(file), isEditable(isEditable) {}

    Qt::ItemFlags flags(int column) const override
    {
        Qt::ItemFlags f = TreeItem::flags(column);
        if (isEditable)
            f |= Qt::ItemIsEditable;
        return f;
    }

    QVariant data(int column, int role) const override
    {
        if (role == Qt::DisplayRole || role == Qt::EditRole)
            return column == 0 ? file.localFilePath().toUserOutput() : file.remoteDirectory();
        return {};
    }

    bool setData(int column, const QVariant &data, int role) override
    {
        if (role != Qt::EditRole)
            return false;
        if (column == 0)
            file = DeployableFile(FilePath::fromSettings(data), file.remoteDirectory());
        else if (column == 1)
            file = DeployableFile(file.localFilePath(), data.toString());
        return true;
    }

    DeployableFile file;
    bool isEditable = false;
};

DeploymentDataView::DeploymentDataView(DeployConfiguration *dc)
{
    auto model = new TreeModel<DeploymentDataItem>(this);
    model->setHeader({Tr::tr("Source File Path"), Tr::tr("Target Directory")});

    auto view = new QTreeView(this);
    view->setMinimumSize(QSize(100, 100));
    view->setTextElideMode(Qt::ElideMiddle);
    view->setWordWrap(false);
    view->setUniformRowHeights(true);
    view->setModel(model);

    const auto buttonsLayout = new QVBoxLayout;
    const auto addButton = new QPushButton(Tr::tr("Add"));
    const auto removeButton = new QPushButton(Tr::tr("Remove"));
    buttonsLayout->addWidget(addButton);
    buttonsLayout->addWidget(removeButton);
    buttonsLayout->addStretch(1);

    const auto viewLayout = new QHBoxLayout;
    viewLayout->addWidget(view);
    viewLayout->addLayout(buttonsLayout);

    auto label = new QLabel(Tr::tr("Files to deploy:"), this);
    const auto sourceCheckBox = new QCheckBox(Tr::tr("Override deployment data from build system"));
    sourceCheckBox->setChecked(dc->usesCustomDeploymentData());

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(label);
    layout->addWidget(sourceCheckBox);
    layout->addLayout(viewLayout);

    const auto updateModel = [dc, model, view] {
        model->clear();
        for (const DeployableFile &file : dc->target()->deploymentData().allFiles()) {
            model->rootItem()->appendChild(
                        new DeploymentDataItem(file, dc->usesCustomDeploymentData()));
        }

        QHeaderView *header = view->header();
        header->setSectionResizeMode(0, QHeaderView::Interactive);
        header->setSectionResizeMode(1, QHeaderView::Interactive);
        view->resizeColumnToContents(0);
        view->resizeColumnToContents(1);
        if (header->sectionSize(0) + header->sectionSize(1) < header->width())
            header->setSectionResizeMode(1, QHeaderView::Stretch);
    };

    const auto deploymentDataFromModel = [model] {
        DeploymentData deployData;
        for (int i = 0; i < model->rowCount(); ++i) {
            const auto item = static_cast<DeploymentDataItem *>(
                        model->itemForIndex(model->index(i, 0)));
            if (!item->file.localFilePath().isEmpty() && !item->file.remoteDirectory().isEmpty())
                deployData.addFile(item->file);
        }
        return deployData;
    };

    const auto updateButtons = [dc, view, addButton, removeButton] {
        addButton->setEnabled(dc->usesCustomDeploymentData());
        removeButton->setEnabled(dc->usesCustomDeploymentData()
                                 && view->selectionModel()->hasSelection());
    };

    connect(dc->target(), &Target::deploymentDataChanged, this, [dc, updateModel] {
        if (!dc->usesCustomDeploymentData())
            updateModel();
    });
    connect(sourceCheckBox, &QCheckBox::toggled, this, [dc, updateModel, updateButtons](bool checked) {
        dc->setUseCustomDeploymentData(checked);
        updateModel();
        updateButtons();
    });
    connect(addButton, &QPushButton::clicked, this, [model, view] {
        const auto newItem = new DeploymentDataItem(DeployableFile(), true);
        model->rootItem()->appendChild(newItem);
        view->edit(model->indexForItem(newItem));
    });
    connect(removeButton, &QPushButton::clicked, this, [dc, model, view, deploymentDataFromModel] {
        const QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
        if (!selectedIndexes.isEmpty()) {
            model->destroyItem(model->itemForIndex(selectedIndexes.first()));
            dc->setCustomDeploymentData(deploymentDataFromModel());
        }
    });
    connect(model, &QAbstractItemModel::dataChanged, this, [dc, deploymentDataFromModel] {
        if (dc->usesCustomDeploymentData())
            dc->setCustomDeploymentData(deploymentDataFromModel());
    });
    connect(view->selectionModel(), &QItemSelectionModel::selectionChanged, this, [updateButtons] {
        updateButtons();
    });
    updateModel();
    updateButtons();
}

} // Internal
} // ProjectExplorer
