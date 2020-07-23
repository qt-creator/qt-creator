/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "projectfileselectionswidget.h"

#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

#include <utils/treemodel.h>

#include <QAbstractTableModel>
#include <QBoxLayout>
#include <QHeaderView>
#include <QTreeView>

namespace QmlPreview {

class ProjectFileItem : public Utils::TreeItem
{
public:
    ProjectFileItem() = default;
    ProjectFileItem(const Utils::FilePath &f, bool d)
        : filePath(f)
        , disabled(d)
    {}

    Qt::ItemFlags flags(int) const override
    {
        return Qt::ItemIsUserCheckable | Qt::ItemIsEnabled;
    }

    QVariant data(int , int role) const override
    {
        if (role == Qt::DisplayRole)
            return filePath.toUserOutput();
        if (role == Qt::CheckStateRole) {
            if (disabled)
                return Qt::Unchecked;
            else
                return Qt::Checked;
        }
        return QVariant();
    }

    bool setData(int , const QVariant &data, int role) override
    {
        if (role != Qt::CheckStateRole)
            return false;
        disabled = (data == Qt::Unchecked);
        return true;
    }

    Utils::FilePath filePath;
    bool disabled = false;
};


ProjectFileSelectionsWidget::ProjectFileSelectionsWidget(const QString &projectSettingsKey, ProjectExplorer::FileType fileType, QWidget *parent)
    : QWidget(parent)
    , m_projectSettingsKey(projectSettingsKey)
    , m_fileType(fileType)
{
    auto model = new Utils::TreeModel<ProjectFileItem>(this);
    model->setHeader({tr("Files to test:")});
    auto updateCheckedFiles = [this, model] () {
        m_checkedFiles.clear();
        QStringList uncheckedFiles;
        model->forAllItems([&, this](ProjectFileItem *item) {
            if (item->disabled)
                uncheckedFiles.append(item->filePath.toString());
            else
                m_checkedFiles.append(item->filePath);
        });
        if (auto project = ProjectExplorer::SessionManager::startupProject())
            project->setNamedSettings(m_projectSettingsKey, uncheckedFiles);
        emit selectionChanged(m_checkedFiles);
    };

    connect(model, &QAbstractItemModel::dataChanged, updateCheckedFiles);

    auto view = new QTreeView(this);
    view->setMinimumSize(QSize(100, 100));
    view->setTextElideMode(Qt::ElideMiddle);
    view->setWordWrap(false);
    view->setUniformRowHeights(true);
    view->setModel(model);

    const auto viewLayout = new QHBoxLayout;
    viewLayout->addWidget(view);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addLayout(viewLayout);

    auto initModel = [this, model, updateCheckedFiles] (ProjectExplorer::Project *project) {
        auto refreshModel = [this, model, updateCheckedFiles] () {
            model->clear();
            if (auto project = ProjectExplorer::SessionManager::startupProject()) {
                const auto settingsDisabledFiles = project->namedSettings(m_projectSettingsKey).toStringList();

                if (auto rootProjectNode = project->rootProjectNode()) {
                    rootProjectNode->forEachNode([this, settingsDisabledFiles, model](ProjectExplorer::FileNode *fileNode) {
                        if (fileNode->fileType() == m_fileType) {
                            bool isDisabled = settingsDisabledFiles.contains(fileNode->filePath().toString());
                            model->rootItem()->appendChild(new ProjectFileItem(fileNode->filePath(), isDisabled));
                        }
                    });
                }
                updateCheckedFiles();
            }
        };
        // deploymentDataChanged is only triggered if the active project changed, so it is not a
        // problem that maybe many different targets are connected to refreshModel
        this->connect(project->activeTarget(), &ProjectExplorer::Target::deploymentDataChanged,
                model, refreshModel, Qt::UniqueConnection);
        refreshModel();
    };

    if (auto project = ProjectExplorer::SessionManager::startupProject()) {
        initModel(project);
    }

    connect(ProjectExplorer::SessionManager::instance(),
            &ProjectExplorer::SessionManager::startupProjectChanged,
            initModel);
}

Utils::FilePaths ProjectFileSelectionsWidget::checkedFiles()
{
    return m_checkedFiles;
}

} // QmlPreview
