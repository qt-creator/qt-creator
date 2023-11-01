// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectsettingswidget.h"

#include "autotestconstants.h"
#include "autotestplugin.h"
#include "autotesttr.h"
#include "testprojectsettings.h"
#include "testtreemodel.h"

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QComboBox>
#include <QTreeWidget>

namespace Autotest {
namespace Internal {

enum ItemDataRole  {
    BaseIdRole = Qt::UserRole + 1,
    BaseTypeRole
};

ProjectTestSettingsWidget::ProjectTestSettingsWidget(ProjectExplorer::Project *project,
                                                     QWidget *parent)
    : ProjectExplorer::ProjectSettingsWidget(parent)
    , m_projectSettings(AutotestPlugin::projectSettings(project))
{
    setGlobalSettingsId(Constants::AUTOTEST_SETTINGS_ID);

    QWidget *generalWidget;
    m_activeFrameworks = new QTreeWidget;
    m_activeFrameworks->setHeaderHidden(true);
    m_activeFrameworks->setRootIsDecorated(false);
    m_runAfterBuild = new QComboBox;
    m_runAfterBuild->addItem(Tr::tr("None"));
    m_runAfterBuild->addItem(Tr::tr("All"));
    m_runAfterBuild->addItem(Tr::tr("Selected"));
    m_runAfterBuild->setCurrentIndex(int(m_projectSettings->runAfterBuild()));

    using namespace Layouting;
    Column {
        Widget {
            bindTo(&generalWidget),
            Column {
                Row {
                    Group {
                        title(Tr::tr("Active frameworks:")),
                        Column { m_activeFrameworks },
                    },
                    st,
                },
                Row {
                    Tr::tr("Automatically run tests after build"),
                    m_runAfterBuild,
                    st,
                },
                noMargin(),
            },
        },
        noMargin(),
    }.attachTo(this);

    generalWidget->setDisabled(m_projectSettings->useGlobalSettings());

    populateFrameworks(m_projectSettings->activeFrameworks(),
                       m_projectSettings->activeTestTools());

    setUseGlobalSettings(m_projectSettings->useGlobalSettings());
    connect(this, &ProjectSettingsWidget::useGlobalSettingsChanged,
            this, [this, generalWidget](bool useGlobalSettings) {
                generalWidget->setEnabled(!useGlobalSettings);
                m_projectSettings->setUseGlobalSettings(useGlobalSettings);
                m_syncTimer.start(3000);
                m_syncType = ITestBase::Framework | ITestBase::Tool;
            });

    connect(m_activeFrameworks, &QTreeWidget::itemChanged,
            this, &ProjectTestSettingsWidget::onActiveFrameworkChanged);
    connect(m_runAfterBuild, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_projectSettings->setRunAfterBuild(RunAfterBuildMode(index));
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
        auto item = new QTreeWidgetItem(m_activeFrameworks, {frameworkOrTestTool->displayName()});
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

} // namespace Internal
} // namespace Autotest
