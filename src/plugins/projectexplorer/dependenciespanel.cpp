// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dependenciespanel.h"

#include "project.h"
#include "projectexplorertr.h"
#include "projectmanager.h"
#include "projectpanelfactory.h"
#include "projectsettingswidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/session.h>

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/fsengine/fileiconprovider.h>

#include <QAbstractListModel>
#include <QCheckBox>
#include <QGridLayout>
#include <QMessageBox>
#include <QSize>
#include <QSpacerItem>
#include <QTreeView>

namespace ProjectExplorer::Internal {

class DependenciesModel : public QAbstractListModel
{
public:
    explicit DependenciesModel(Project *project)
        : m_project(project)
    {
        resetModel();

        connect(ProjectManager::instance(), &ProjectManager::projectRemoved,
                this, &DependenciesModel::resetModel);
        connect(ProjectManager::instance(), &ProjectManager::projectAdded,
                this, &DependenciesModel::resetModel);
        connect(Core::SessionManager::instance(), &Core::SessionManager::sessionLoaded,
                this, &DependenciesModel::resetModel);
    }

    int rowCount(const QModelIndex &index) const override;
    int columnCount(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
    void resetModel();

    Project *m_project;
    QList<Project *> m_projects;
};

void DependenciesModel::resetModel()
{
    beginResetModel();
    m_projects = ProjectManager::projects();
    m_projects.removeAll(m_project);
    Utils::sort(m_projects, [](Project *a, Project *b) {
        return a->displayName() < b->displayName();
    });
    endResetModel();
}

int DependenciesModel::rowCount(const QModelIndex &index) const
{
    return index.isValid() ? 0 : m_projects.isEmpty() ? 1 : m_projects.size();
}

int DependenciesModel::columnCount(const QModelIndex &index) const
{
    return index.isValid() ? 0 : 1;
}

QVariant DependenciesModel::data(const QModelIndex &index, int role) const
{
    if (m_projects.isEmpty())
        return role == Qt::DisplayRole
            ? Tr::tr("<No other projects in this session>")
            : QVariant();

    const Project *p = m_projects.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        return p->displayName();
    case Qt::ToolTipRole:
        return p->projectFilePath().toUserOutput();
    case Qt::CheckStateRole:
        return ProjectManager::hasDependency(m_project, p) ? Qt::Checked : Qt::Unchecked;
    case Qt::DecorationRole:
        return Utils::FileIconProvider::icon(p->projectFilePath());
    default:
        return {};
    }
}

bool DependenciesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole) {
        Project *p = m_projects.at(index.row());
        const auto c = static_cast<Qt::CheckState>(value.toInt());

        if (c == Qt::Checked) {
            if (ProjectManager::addDependency(m_project, p)) {
                emit dataChanged(index, index);
                return true;
            } else {
                QMessageBox::warning(Core::ICore::dialogParent(), Tr::tr("Unable to Add Dependency"),
                                     Tr::tr("This would create a circular dependency."));
            }
        } else if (c == Qt::Unchecked) {
            if (ProjectManager::hasDependency(m_project, p)) {
                ProjectManager::removeDependency(m_project, p);
                emit dataChanged(index, index);
                return true;
            }
        }
    }
    return false;
}

Qt::ItemFlags DependenciesModel::flags(const QModelIndex &index) const
{
    if (m_projects.isEmpty())
        return Qt::NoItemFlags;

    Qt::ItemFlags rc = QAbstractListModel::flags(index);
    if (index.column() == 0)
        rc |= Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
    return rc;
}

//
// DependenciesView
//

class DependenciesView : public QTreeView
{
public:
    explicit DependenciesView(QWidget *parent);

    QSize sizeHint() const override;
    void setModel(QAbstractItemModel *model) override;

private:
    void updateSizeHint();

    QSize m_sizeHint;
};

DependenciesView::DependenciesView(QWidget *parent)
    : QTreeView(parent)
{
    m_sizeHint = QSize(250, 250);
    setUniformRowHeights(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    setRootIsDecorated(false);
}

QSize DependenciesView::sizeHint() const
{
    return m_sizeHint;
}

void DependenciesView::setModel(QAbstractItemModel *newModel)
{
    if (QAbstractItemModel *oldModel = model()) {
        disconnect(oldModel, &QAbstractItemModel::rowsInserted,
                   this, &DependenciesView::updateSizeHint);
        disconnect(oldModel, &QAbstractItemModel::rowsRemoved,
                   this, &DependenciesView::updateSizeHint);
        disconnect(oldModel, &QAbstractItemModel::modelReset,
                   this, &DependenciesView::updateSizeHint);
        disconnect(oldModel, &QAbstractItemModel::layoutChanged,
                   this, &DependenciesView::updateSizeHint);
    }

    QTreeView::setModel(newModel);

    if (newModel) {
        connect(newModel, &QAbstractItemModel::rowsInserted,
                this, &DependenciesView::updateSizeHint);
        connect(newModel, &QAbstractItemModel::rowsRemoved,
                this, &DependenciesView::updateSizeHint);
        connect(newModel, &QAbstractItemModel::modelReset,
                this, &DependenciesView::updateSizeHint);
        connect(newModel, &QAbstractItemModel::layoutChanged,
                this, &DependenciesView::updateSizeHint);
    }
    updateSizeHint();
}

void DependenciesView::updateSizeHint()
{
    if (!model()) {
        m_sizeHint = QSize(250, 250);
        return;
    }

    int heightOffset = size().height() - viewport()->height();

    int heightPerRow = sizeHintForRow(0);
    if (heightPerRow == -1)
        heightPerRow = 30;
    int rows = qMin(qMax(model()->rowCount(), 2), 10);
    int height = rows * heightPerRow + heightOffset;
    if (m_sizeHint.height() != height) {
        m_sizeHint.setHeight(height);
        updateGeometry();
    }
}

//
// DependenciesWidget
//

class DependenciesWidget : public ProjectSettingsWidget
{
public:
    explicit DependenciesWidget(Project *project)
        : m_model(project)
    {
        setUseGlobalSettingsCheckBoxVisible(false);
        setUseGlobalSettingsLabelVisible(false);
        auto vbox = new QVBoxLayout(this);
        vbox->setContentsMargins(0, 0, 0, 0);
        m_detailsContainer = new Utils::DetailsWidget(this);
        m_detailsContainer->setState(Utils::DetailsWidget::NoSummary);
        vbox->addWidget(m_detailsContainer);

        auto detailsWidget = new QWidget(m_detailsContainer);
        m_detailsContainer->setWidget(detailsWidget);
        auto layout = new QGridLayout(detailsWidget);
        layout->setContentsMargins(0, -1, 0, -1);
        auto treeView = new DependenciesView(this);
        treeView->setModel(&m_model);
        treeView->setHeaderHidden(true);
        layout->addWidget(treeView, 0 ,0);
        layout->addItem(new QSpacerItem(0, 0 , QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 1);

        m_cascadeSetActiveCheckBox = new QCheckBox;
        m_cascadeSetActiveCheckBox->setText(Tr::tr("Synchronize configuration"));
        m_cascadeSetActiveCheckBox->setToolTip(Tr::tr("Synchronize active kit, build, and deploy configuration between projects."));
        m_cascadeSetActiveCheckBox->setChecked(ProjectManager::isProjectConfigurationCascading());
        connect(m_cascadeSetActiveCheckBox, &QCheckBox::toggled,
                ProjectManager::instance(), &ProjectManager::setProjectConfigurationCascading);
        layout->addWidget(m_cascadeSetActiveCheckBox, 1, 0, 2, 1);
        m_deployCheckBox = new QCheckBox;
        m_deployCheckBox->setText(Tr::tr("Deploy dependencies"));
        m_deployCheckBox->setToolTip(
            Tr::tr("Do not just build dependencies, but deploy them as well."));
        m_deployCheckBox->setChecked(ProjectManager::deployProjectDependencies());
        connect(m_deployCheckBox, &QCheckBox::toggled,
                ProjectManager::instance(), &ProjectManager::setDeployProjectDependencies);
        layout->addWidget(m_deployCheckBox, 3, 0, 2, 1);
    }

private:
    DependenciesModel m_model;
    Utils::DetailsWidget *m_detailsContainer;
    QCheckBox *m_cascadeSetActiveCheckBox;
    QCheckBox *m_deployCheckBox;
};

class DependenciesProjectPanelFactory final : public ProjectPanelFactory
{
public:
    DependenciesProjectPanelFactory()
    {
        setPriority(50);
        setDisplayName(Tr::tr("Dependencies"));
        setCreateWidgetFunction([](Project *project) { return new DependenciesWidget(project); });
    }
};

void setupDependenciesProjectPanel()
{
    static DependenciesProjectPanelFactory theDependenciesProjectPanelFactory;
}

} // ProjectExplorer::Internal
