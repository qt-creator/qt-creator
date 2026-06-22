// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectsettingswidget.h"

#include "autotestconstants.h"
#include "autotestplugin.h"
#include "autotesttr.h"
#include "testcodeparser.h"
#include "testprojectsettings.h"
#include "testtreemodel.h"

#include <projectexplorer/projectpanelfactory.h>

#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QPushButton>
#include <QTimer>
#include <QTreeWidget>

using namespace ProjectExplorer;

namespace Autotest::Internal {

enum ItemDataRole  {
    BaseIdRole = Qt::UserRole + 1,
    BaseTypeRole
};

class ProjectTestSettingsWidget : public QWidget
{
public:
    explicit ProjectTestSettingsWidget(Project *project);

private:
    void populateFrameworks(const QHash<Autotest::ITestFramework *, bool> &frameworks,
                            const QHash<Autotest::ITestTool *, bool> &testTools);
    void populatePathFilters(const QStringList &filters);
    void onActiveFrameworkChanged(QTreeWidgetItem *item, int column);
    TestProjectSettings *m_projectSettings;
    QWidget *m_generalWidget = nullptr;
    QTreeWidget m_activeFrameworks;
    QTreeWidget m_pathFilters;
    QTimer m_syncTimer;
    int m_syncType = 0;
};

ProjectTestSettingsWidget::ProjectTestSettingsWidget(Project *project)
    : m_projectSettings(testProjectSettings(project))
{
    m_activeFrameworks.setHeaderHidden(true);
    m_activeFrameworks.setRootIsDecorated(false);

    m_pathFilters.setHeaderHidden(true);
    m_pathFilters.setRootIsDecorated(false);

    QLabel *filterLabel = new QLabel(Tr::tr("Wildcard expressions for filtering:"), this);
    QPushButton *addFilter = new QPushButton(Tr::tr("Add"), this);
    QPushButton *removeFilter = new QPushButton(Tr::tr("Remove"), this);
    removeFilter->setEnabled(false);

    // clang-format off
    using namespace Layouting;
    Column {
        m_projectSettings->useGlobalSettings,
        Widget {
            bindTo(&m_generalWidget),
            Column {
                Row {
                    Group {
                        title(Tr::tr("Active Test Frameworks")),
                        Column { &m_activeFrameworks },
                    },
                    st,
                },
                Row { m_projectSettings->runAfterBuild, st },
                noMargin,
            },
        },
        Row { // explicitly outside of the global settings
            Group {
                title(Tr::tr("Limit Files to Path Patterns")),
                groupChecker(m_projectSettings->limitToFilter.groupChecker()),
                Column {
                    filterLabel,
                    Row {
                        Column { &m_pathFilters },
                        Column { addFilter, removeFilter, st },
                    },
                },
            },
        },
        noMargin,
        st,
    }.attachTo(this);
    // clang-format on

    m_generalWidget->setDisabled(m_projectSettings->useGlobalSettings());

    populateFrameworks(m_projectSettings->activeTestFrameworks(),
                       m_projectSettings->activeTestTools());

    populatePathFilters(m_projectSettings->pathFilters());

    m_projectSettings->useGlobalSettings.addOnChanged(this, [this] {
        m_generalWidget->setEnabled(!m_projectSettings->useGlobalSettings());
        m_syncTimer.start(3000);
        m_syncType = ITestBase::Framework | ITestBase::Tool;
    });

    connect(&m_activeFrameworks, &QTreeWidget::itemChanged,
            this, &ProjectTestSettingsWidget::onActiveFrameworkChanged);

    auto itemsToStringList = [this] {
        QStringList items;
        const QTreeWidgetItem *rootItem = m_pathFilters.invisibleRootItem();
        for (int i = 0, count = rootItem->childCount(); i < count; ++i) {
            auto expr = rootItem->child(i)->data(0, Qt::DisplayRole).toString();
            items.append(expr);
        }
        return items;
    };
    auto triggerRescan = [] {
        TestCodeParser *parser = TestTreeModel::instance()->parser();
        parser->emitUpdateTestTree();
    };

    m_projectSettings->limitToFilter.addOnChanged(this, [triggerRescan] {
        triggerRescan();
    });
    connect(&m_pathFilters, &QTreeWidget::itemSelectionChanged,
            this, [this, removeFilter] {
        removeFilter->setEnabled(!m_pathFilters.selectedItems().isEmpty());
    });
    connect(m_pathFilters.model(), &QAbstractItemModel::dataChanged,
            this, [this, itemsToStringList, triggerRescan]
            (const QModelIndex &tl, const QModelIndex &br, const QList<int> &roles) {
        if (!roles.contains(Qt::DisplayRole))
            return;
        if (tl != br)
            return;
        m_pathFilters.model()->setData(tl, tl.data(), Qt::ToolTipRole);
        m_projectSettings->setPathFilters(itemsToStringList());
        triggerRescan();
    });
    connect(addFilter, &QPushButton::clicked, this, [this] {
        m_projectSettings->addPathFilter("*");
        populatePathFilters(m_projectSettings->pathFilters());
        const QTreeWidgetItem *root = m_pathFilters.invisibleRootItem();
        QTreeWidgetItem *lastChild =  root->child(root->childCount() - 1);
        const QModelIndex index = m_pathFilters.indexFromItem(lastChild, 0);
        m_pathFilters.edit(index);
    });
    connect(removeFilter, &QPushButton::clicked, this, [this, itemsToStringList, triggerRescan] {
        const QList<QTreeWidgetItem *> selected = m_pathFilters.selectedItems();
        QTC_ASSERT(selected.size() == 1, return);
        m_pathFilters.invisibleRootItem()->removeChild(selected.first());
        delete selected.first();
        m_projectSettings->setPathFilters(itemsToStringList());
        triggerRescan();
    });
    m_syncTimer.setSingleShot(true);
    connect(&m_syncTimer, &QTimer::timeout, this, [this] {
        auto testTreeModel = TestTreeModel::instance();
        if (m_syncType & ITestBase::Framework)
            testTreeModel->synchronizeTestFrameworks();
        if (m_syncType & ITestBase::Tool)
            testTreeModel->synchronizeTestTools();
        m_syncType = ITestBase::None;
    });
}

void ProjectTestSettingsWidget::populateFrameworks(const QHash<ITestFramework *, bool> &frameworks,
                                                   const QHash<ITestTool *, bool> &testTools)
{
    const TestFrameworks sortedFrameworks = Utils::sorted(frameworks.keys(),
                                                          &ITestFramework::priority);

    auto generateItem = [this](ITestBase *frameworkOrTestTool, bool checked) {
        auto item = new QTreeWidgetItem(&m_activeFrameworks, {frameworkOrTestTool->displayName()});
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
        item->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
        item->setData(0, BaseIdRole, frameworkOrTestTool->id().toSetting());
        item->setData(0, BaseTypeRole, frameworkOrTestTool->type());
    };

    for (ITestFramework *framework : sortedFrameworks)
        generateItem(framework, frameworks.value(framework));

    // FIXME: testTools aren't sorted and we cannot use priority here
    auto end = testTools.cend();
    for (auto it = testTools.cbegin(); it != end; ++it)
        generateItem(it.key(), it.value());
}

void ProjectTestSettingsWidget::populatePathFilters(const QStringList &filters)
{
    m_pathFilters.clear();
    for (const QString &filter : filters) {
        auto item = new QTreeWidgetItem(&m_pathFilters, {filter});
        item->setData(0, Qt::ToolTipRole, filter);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
    }
}

void ProjectTestSettingsWidget::onActiveFrameworkChanged(QTreeWidgetItem *item, int column)
{
    auto id = Utils::Id::fromSetting(item->data(column, BaseIdRole));
    int type = item->data(column, BaseTypeRole).toInt();
    if (type == ITestBase::Framework)
        m_projectSettings->activateFramework(id, item->data(0, Qt::CheckStateRole) == Qt::Checked);
    else if (type == ITestBase::Tool)
        m_projectSettings->activateTestTool(id, item->data(0, Qt::CheckStateRole) == Qt::Checked);
    else
        QTC_ASSERT(! "unexpected test base type", return);
    m_syncTimer.start(3000);
    m_syncType |= type;
}

class AutotestProjectPanelFactory final : public ProjectPanelFactory
{
public:
    AutotestProjectPanelFactory()
    {
        setPriority(666);
        setDisplayName(Tr::tr("Testing"));
        setCreateWidgetFunction([](Project *project) {
            return new ProjectTestSettingsWidget(project);
        });
    }
};

void setupAutotestProjectPanel()
{
    static AutotestProjectPanelFactory theAutotestProjectPanelFactory;
}

} // Autotest::Internal
