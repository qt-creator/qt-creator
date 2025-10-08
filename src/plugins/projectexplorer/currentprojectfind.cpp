// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "currentprojectfind.h"

#include "allprojectsfind.h"
#include "project.h"
#include "projectexplorertr.h"
#include "projectmanager.h"
#include "projecttree.h"

#include <utils/shutdownguard.h>
#include <utils/treemodel.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QGridLayout>
#include <QPushButton>

using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace ProjectExplorer::Internal {
namespace {
class ProjectItem : public TreeItem
{
public:
    ProjectItem(const Project *project, BaseTreeModel *m) : m_project(project)
    {
        QObject::connect(project, &Project::displayNameChanged, m, [this] {
            const QModelIndex idx = index();
            emit model()->dataChanged(idx, idx);
        });
    }

    bool isForProject(const Project *p) const { return m_project == p; }

private:
    QVariant data(int column, int role) const override
    {
        if (column != 0)
            return {};
        if (role == Qt::DisplayRole)
            return m_project->displayName();
        if (role == Qt::UserRole)
            return QVariant::fromValue(m_project);
        return {};
    }

    const Project * const m_project;
};

class ProjectsModel : public BaseTreeModel
{
public:
    ProjectsModel(QObject *parent) : BaseTreeModel(parent)
    {
        for (const Project * const p : ProjectManager::projects())
            rootItem()->appendChild(new ProjectItem(p, this));
        connect(ProjectManager::instance(), &ProjectManager::projectAdded, this, [this](Project *p) {
            rootItem()->appendChild(new ProjectItem(p, this));
        });
        connect(ProjectManager::instance(), &ProjectManager::aboutToRemoveProject,
                         this, [this](Project *p) {
            for (int i = 0; i < rootItem()->childCount(); ++i) {
                if (static_cast<ProjectItem *>(rootItem()->childAt(i))->isForProject(p)) {
                    rootItem()->removeChildAt(i);
                    break;
                }
            }
        });
    }

    QModelIndex indexForProject(const Project *p) const
    {
        for (int i = 0; i < rootItem()->childCount(); ++i) {
            if (static_cast<ProjectItem *>(rootItem()->childAt(i))->isForProject(p))
                return index(i, 0);
        }
        return {};
    }
};
} // namespace

class CurrentProjectFind final : public TextEditor::BaseFileFind
{
public:
    CurrentProjectFind();

private:
    QString id() const final { return QLatin1String("Single Project"); }
    QString displayName() const final { return Tr::tr("Single Project"); }
    QString label() const final;

    bool isEnabled() const final;

    Utils::Store save() const final;
    void restore(const Utils::Store &s) final;

    // deprecated
    QByteArray settingsKey() const final;

    TextEditor::FileContainerProvider fileContainerProvider() const final;
    Utils::FindFlags supportedFindFlags() const final;
    void setupSearch(Core::SearchResult *search) final;
    QString toolTip() const override;
    QWidget *createConfigWidget() override;

    const Project *selectedProject() const;
    FilePath selectedProjectPath() const;

    void selectCurrentProject();

    QPointer<QComboBox> m_projectsComboBox;
    QPointer<QWidget> m_configWidget;
};

CurrentProjectFind::CurrentProjectFind()
{
    const auto projectListChangedHandler = [this] { emit enabledChanged(isEnabled()); };
    connect(ProjectManager::instance(), &ProjectManager::projectAdded,
            this, projectListChangedHandler);
    connect(ProjectManager::instance(), &ProjectManager::projectRemoved,
            this, projectListChangedHandler);
}

bool CurrentProjectFind::isEnabled() const
{
    return ProjectManager::hasProjects() && BaseFileFind::isEnabled();
}

FileContainerProvider CurrentProjectFind::fileContainerProvider() const
{
    return [nameFilters = fileNameFilters(), exclusionFilters = fileExclusionFilters(),
            projectFilePath = selectedProjectPath()] {
        for (Project * const p : ProjectManager::projects()) {
            if (p->projectFilePath() == projectFilePath)
                return AllProjectsFind::filesForProjects(nameFilters, exclusionFilters, {p});
        }
        return FileContainer();
    };
}

QString CurrentProjectFind::toolTip() const
{
    // last arg is filled by BaseFileFind::runNewSearch
    return Tr::tr("Filter: %1\nExcluding: %2\n%3")
        .arg(fileNameFilters().join(','))
        .arg(fileExclusionFilters().join(','));
}

QString CurrentProjectFind::label() const
{
    return Tr::tr("Project \"%1\":").arg(m_projectsComboBox->currentText());
}

QWidget *CurrentProjectFind::createConfigWidget()
{
    if (!m_configWidget) {
        m_configWidget = new QWidget;
        const auto label = new QLabel(Tr::tr("Project:"));
        m_projectsComboBox = new QComboBox;
        QSizePolicy p = m_projectsComboBox->sizePolicy();
        p.setHorizontalStretch(1);
        m_projectsComboBox->setSizePolicy(p);
        const auto projectsModel = new ProjectsModel(this);
        const auto sortModel = new SortModel(this);
        sortModel->setSourceModel(projectsModel);
        const auto sort = [sortModel] { sortModel->sort(0); };
        sort();
        connect(ProjectManager::instance(), &ProjectManager::projectDisplayNameChanged,
                sortModel, [sort] { sort(); });
        connect(ProjectManager::instance(), &ProjectManager::projectAdded,
                sortModel, [sort] { sort(); });
        m_projectsComboBox->setModel(sortModel);
        const auto currentButton = new QPushButton(Tr::tr("Current"));
        selectCurrentProject();
        connect(currentButton, &QPushButton::clicked,
                this, &CurrentProjectFind::selectCurrentProject);
        auto projectsLayout = new QHBoxLayout;
        projectsLayout->addWidget(m_projectsComboBox);
        projectsLayout->addWidget(currentButton);
        auto gridLayout = new QGridLayout(m_configWidget);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        m_configWidget->setLayout(gridLayout);
        const QList<QPair<QWidget *, QWidget *>> patternWidgets = createPatternWidgets();
        gridLayout->addWidget(label, 0, 0, Qt::AlignRight);
        gridLayout->addLayout(projectsLayout, 0, 1);
        int row = 1;
        for (const QPair<QWidget *, QWidget *> &p : patternWidgets) {
            gridLayout->addWidget(p.first, row, 0, Qt::AlignRight);
            gridLayout->addWidget(p.second, row, 1);
            ++row;
        }
        m_configWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    }
    return m_configWidget;
}

const Project *CurrentProjectFind::selectedProject() const
{
    if (!m_projectsComboBox)
        return nullptr;
    return m_projectsComboBox->currentData().value<const Project *>();
}

FilePath CurrentProjectFind::selectedProjectPath() const
{
    if (const Project * const p = selectedProject())
        return p->projectFilePath();
    return {};
}

void CurrentProjectFind::selectCurrentProject()
{
    if (m_projectsComboBox) {
        const auto sortModel = static_cast<SortModel *>(m_projectsComboBox->model());
        const auto projectsModel = static_cast<ProjectsModel *>(sortModel->sourceModel());
        const QModelIndex idx = projectsModel->indexForProject(ProjectTree::currentProject());
        m_projectsComboBox->setCurrentIndex(sortModel->mapFromSource(idx).row());
    }
}

void CurrentProjectFind::setupSearch(Core::SearchResult *search)
{
    connect(this, &IFindFilter::enabledChanged, search,
            [search, projectFilePath = selectedProjectPath()] {
        for (Project * const p : ProjectManager::projects()) {
            if (p->projectFilePath() == projectFilePath)
                return search->setSearchAgainEnabled(true);
        }
        search->setSearchAgainEnabled(false);
    });
}

const char kDefaultInclusion[] = "*";
const char kDefaultExclusion[] = "";

Store CurrentProjectFind::save() const
{
    Store s;
    writeCommonSettings(s, kDefaultInclusion, kDefaultExclusion);
    return s;
}

void CurrentProjectFind::restore(const Store &s)
{
    readCommonSettings(s, kDefaultInclusion, kDefaultExclusion);
}

QByteArray CurrentProjectFind::settingsKey() const
{
    return "CurrentProjectFind";
}

FindFlags CurrentProjectFind::supportedFindFlags() const
{
    return BaseFileFind::supportedFindFlags() | DontFindGeneratedFiles;
}

void setupCurrentProjectFind()
{
    static GuardedObject<CurrentProjectFind> theCurrentProjectFind;
}

} // ProjectExplorer::Internal
