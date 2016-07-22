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

#include "projectpanelfactory.h"

#include "project.h"
#include "projectwindow.h"
#include "propertiespanel.h"

using namespace ProjectExplorer::Internal;
using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

static QList<ProjectPanelFactory *> s_factories;

// Standard second level for the generic case: i.e. all except for the Build/Run page
class ProjectPanelItem : public TreeItem
{
public:
    using WidgetCreator = std::function<QWidget *(Project *Project)>;

    ProjectPanelItem(ProjectPanelFactory *factory, Project *project,
                     const WidgetCreator &widgetCreator)
        : m_factory(factory), m_project(project), m_widgetCreator(widgetCreator)
    {}

    ~ProjectPanelItem() { delete m_widget; }

    QVariant data(int column, int role) const override;
    Qt::ItemFlags flags(int column) const override;
    bool setData(int column, const QVariant &, int role) override;

protected:
    ProjectPanelFactory *m_factory = nullptr;
    QPointer<Project> m_project;
    WidgetCreator m_widgetCreator;

    mutable QPointer<QWidget> m_widget = nullptr;
};

QVariant ProjectPanelItem::data(int column, int role) const
{
    Q_UNUSED(column)
    if (role == Qt::DisplayRole) {
        if (m_factory)
            return m_factory->displayName();
    }

//    if (role == Qt::DecorationRole) {
//        if (m_factory)
//            return QIcon(m_factory->icon());
//    }

    if (role == ActiveWidgetRole) {
        if (!m_widget) {
            auto panelsWidget = new PanelsWidget;
            auto panel = new PropertiesPanel;
            panel->setDisplayName(m_factory->displayName());
            QWidget *widget = m_widgetCreator(m_project);
            panel->setWidget(widget);
            panel->setIcon(QIcon(m_factory->icon()));
            panelsWidget->addPropertiesPanel(panel);
            panelsWidget->setFocusProxy(widget);
            m_widget = panelsWidget;
        }

        return QVariant::fromValue<QWidget *>(m_widget.data());
    }

    if (role == ActiveIndexRole)  // We are the active one.
        return QVariant::fromValue(index());

    return QVariant();
}

Qt::ItemFlags ProjectPanelItem::flags(int column) const
{
    if (m_factory && m_project) {
        if (!m_factory->supports(m_project))
            return Qt::ItemIsSelectable;
    }
    return TreeItem::flags(column);
}

bool ProjectPanelItem::setData(int column, const QVariant &, int role)
{
    if (role == ItemActivaterRole) {
        // Bubble up
        return parent()->setData(column, QVariant::fromValue(static_cast<TreeItem *>(this)), role);
    }

    return false;
}

} // Internal

ProjectPanelFactory::ProjectPanelFactory()
    : m_supportsFunction([] (Project *) { return true; })
{ }

int ProjectPanelFactory::priority() const
{
    return m_priority;
}

void ProjectPanelFactory::setPriority(int priority)
{
    m_priority = priority;
}

QString ProjectPanelFactory::displayName() const
{
    return m_displayName;
}

void ProjectPanelFactory::setDisplayName(const QString &name)
{
    m_displayName = name;
}

void ProjectPanelFactory::registerFactory(ProjectPanelFactory *factory)
{
    auto it = std::lower_bound(s_factories.begin(), s_factories.end(), factory,
        [](ProjectPanelFactory *a, ProjectPanelFactory *b)  {
            return (a->priority() == b->priority() && a < b) || a->priority() < b->priority();
        });

    s_factories.insert(it, factory);
}

QList<ProjectPanelFactory *> ProjectPanelFactory::factories()
{
    return s_factories;
}

TreeItem *ProjectPanelFactory::createSelectorItem(Project *project)
{
    if (m_selectorItemCreator)
        return m_selectorItemCreator(project);
    return new Internal::ProjectPanelItem(this, project, m_widgetCreator);
}

void ProjectPanelFactory::destroyFactories()
{
    qDeleteAll(s_factories);
    s_factories.clear();
}

QString ProjectPanelFactory::icon() const
{
    return m_icon;
}

void ProjectPanelFactory::setIcon(const QString &icon)
{
    m_icon = icon;
}

void ProjectPanelFactory::setCreateWidgetFunction(const WidgetCreator &createWidgetFunction)
{
    m_widgetCreator = createWidgetFunction;
}

void ProjectPanelFactory::setSelectorItemCreator(const SelectorItemCreator &selectorCreator)
{
    m_selectorItemCreator = selectorCreator;
}

bool ProjectPanelFactory::supports(Project *project)
{
    return m_supportsFunction(project);
}

void ProjectPanelFactory::setSupportsFunction(std::function<bool (Project *)> function)
{
    m_supportsFunction = function;
}

} // namespace ProjectExplorer
