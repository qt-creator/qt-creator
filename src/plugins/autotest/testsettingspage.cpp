// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testsettingspage.h"

#include "autotestconstants.h"
#include "autotestplugin.h"
#include "autotesttr.h"
#include "testframeworkmanager.h"
#include "testsettings.h"
#include "testtreemodel.h"

#include <utils/algorithm.h>
#include <utils/id.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QTreeWidget>

using namespace Utils;

namespace Autotest::Internal {

class TestSettingsWidget : public Core::IOptionsPageWidget
{
public:
    TestSettingsWidget();

private:
    void populateFrameworksListWidget(const QHash<Id, bool> &frameworks,
                                      const QHash<Id, bool> &testTools);
    void testSettings(NonAspectSettings &settings) const;
    void testToolsSettings(NonAspectSettings &settings) const;
    void onFrameworkItemChanged();

    QTreeWidget *m_frameworkTreeWidget;
    InfoLabel *m_frameworksWarn;
};

TestSettingsWidget::TestSettingsWidget()
{
    auto timeoutLabel = new QLabel(Tr::tr("Timeout:"));
    timeoutLabel->setToolTip(Tr::tr("Timeout used when executing each test case."));
    auto scanThreadLabel = new QLabel(Tr::tr("Scan threads:"));
    scanThreadLabel->setToolTip("Number of worker threads used when scanning for tests.");

    m_frameworkTreeWidget = new QTreeWidget;
    m_frameworkTreeWidget->setRootIsDecorated(false);
    m_frameworkTreeWidget->setHeaderHidden(false);
    m_frameworkTreeWidget->setColumnCount(2);
    m_frameworkTreeWidget->header()->setDefaultSectionSize(150);
    m_frameworkTreeWidget->setToolTip(Tr::tr("Selects the test frameworks to be handled by the AutoTest plugin."));

    QTreeWidgetItem *item = m_frameworkTreeWidget->headerItem();
    item->setText(0, Tr::tr("Framework"));
    item->setToolTip(0, Tr::tr("Selects the test frameworks to be handled by the AutoTest plugin."));
    item->setText(1, Tr::tr("Group"));
    item->setToolTip(1, Tr::tr("Enables grouping of test cases."));

    m_frameworksWarn = new InfoLabel;
    m_frameworksWarn->setVisible(false);
    m_frameworksWarn->setElideMode(Qt::ElideNone);
    m_frameworksWarn->setType(InfoLabel::Warning);

    using namespace Layouting;

    PushButton resetChoicesButton {
        text(Tr::tr("Reset Cached Choices")),
        tooltip(Tr::tr("Clear all cached choices of run configurations for "
                       "tests where the executable could not be deduced.")),
        onClicked([] { AutotestPlugin::clearChoiceCache(); }, this)
    };

    TestSettings &s = Internal::testSettings();
    Group generalGroup {
        title(Tr::tr("General")),
        Column {
            Row { scanThreadLabel, s.scanThreadLimit, st },
            s.omitInternalMsg,
            s.omitRunConfigWarn,
            s.limitResultOutput,
            Row { s.limitResultDescription, s.resultDescriptionMaxSize, st },
            s.popupOnStart,
            s.popupOnFinish,
            Row { Space(20), s.popupOnFail },
            s.autoScroll,
            s.displayApplication,
            s.processArgs,
            Row { Tr::tr("Automatically run"), s.runAfterBuild, st },
            Row { timeoutLabel, s.timeout, st },
            Row { resetChoicesButton, st }
         }
    };

    Group activeFrameworks {
        title(Tr::tr("Active Test Frameworks")),
        Column {
            m_frameworkTreeWidget,
            m_frameworksWarn,
        }
    };

    Column {
        Row {
            Column { generalGroup, st },
            Column { activeFrameworks, st }
        },
        st
    }.attachTo(this);

    connect(m_frameworkTreeWidget, &QTreeWidget::itemChanged,
            this, &TestSettingsWidget::onFrameworkItemChanged);

    populateFrameworksListWidget(s.frameworks, s.tools);

    setOnApply([this] {
        TestSettings &s = Internal::testSettings();

        NonAspectSettings tmp;
        testSettings(tmp);
        testToolsSettings(tmp);

        const QList<Utils::Id> changedIds = Utils::filtered(tmp.frameworksGrouping.keys(),
           [&tmp, &s](Utils::Id id) {
            return tmp.frameworksGrouping[id] != s.frameworksGrouping[id];
        });

        testSettings(s);
        testToolsSettings(s);
        s.toSettings();

        for (ITestFramework *framework : TestFrameworkManager::registeredFrameworks()) {
            framework->setActive(s.frameworks.value(framework->id(), false));
            framework->setGrouping(s.frameworksGrouping.value(framework->id(), false));
        }

        for (ITestTool *testTool : TestFrameworkManager::registeredTestTools())
            testTool->setActive(s.tools.value(testTool->id(), false));

        TestTreeModel::instance()->synchronizeTestFrameworks();
        TestTreeModel::instance()->synchronizeTestTools();
        if (!changedIds.isEmpty())
            TestTreeModel::instance()->rebuild(changedIds);
    });
}

enum TestBaseInfo
{
    BaseId = Qt::UserRole,
    BaseType
};

void TestSettingsWidget::populateFrameworksListWidget(const QHash<Id, bool> &frameworks,
                                                      const QHash<Id, bool> &testTools)
{
    const TestFrameworks &registered = TestFrameworkManager::registeredFrameworks();
    m_frameworkTreeWidget->clear();
    for (const ITestFramework *framework : registered) {
        const Id id = framework->id();
        auto item = new QTreeWidgetItem(m_frameworkTreeWidget, {framework->displayName()});
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
        item->setCheckState(0, frameworks.value(id) ? Qt::Checked : Qt::Unchecked);
        item->setData(0, BaseId, id.toSetting());
        item->setData(0, BaseType, ITestBase::Framework);
        item->setData(1, Qt::CheckStateRole, framework->grouping() ? Qt::Checked : Qt::Unchecked);
        item->setToolTip(0, Tr::tr("Enable or disable test frameworks to be handled by the "
                                   "AutoTest plugin."));
        QString toolTip = framework->groupingToolTip();
        if (toolTip.isEmpty())
            toolTip = Tr::tr("Enable or disable grouping of test cases by folder.");
        item->setToolTip(1, toolTip);
    }
    // ...and now the test tools
    const TestTools &registeredTools = TestFrameworkManager::registeredTestTools();
    for (const ITestTool *testTool : registeredTools) {
        const Id id = testTool->id();
        auto item = new QTreeWidgetItem(m_frameworkTreeWidget, {testTool->displayName()});
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
        item->setCheckState(0, testTools.value(id) ? Qt::Checked : Qt::Unchecked);
        item->setData(0, BaseId, id.toSetting());
        item->setData(0, BaseType, ITestBase::Tool);
    }
}

void TestSettingsWidget::testSettings(NonAspectSettings &settings) const
{
    const QAbstractItemModel *model = m_frameworkTreeWidget->model();
    QTC_ASSERT(model, return);
    const int itemCount = TestFrameworkManager::registeredFrameworks().size();
    QTC_ASSERT(itemCount <= model->rowCount(), return);
    for (int row = 0; row < itemCount; ++row) {
        QModelIndex idx = model->index(row, 0);
        const Id id = Id::fromSetting(idx.data(BaseId));
        settings.frameworks.insert(id, idx.data(Qt::CheckStateRole) == Qt::Checked);
        idx = model->index(row, 1);
        settings.frameworksGrouping.insert(id, idx.data(Qt::CheckStateRole) == Qt::Checked);
    }
}

void TestSettingsWidget::testToolsSettings(NonAspectSettings &settings) const
{
    const QAbstractItemModel *model = m_frameworkTreeWidget->model();
    QTC_ASSERT(model, return);
    // frameworks are listed before tools
    int row = TestFrameworkManager::registeredFrameworks().size();
    const int end = model->rowCount();
    QTC_ASSERT(row <= end, return);
    for ( ; row < end; ++row) {
        const QModelIndex idx = model->index(row, 0);
        const Id id = Id::fromSetting(idx.data(BaseId));
        settings.tools.insert(id, idx.data(Qt::CheckStateRole) == Qt::Checked);
    }
}

void TestSettingsWidget::onFrameworkItemChanged()
{
    bool atLeastOneEnabled = false;
    int mixed = ITestBase::None;
    if (QAbstractItemModel *model = m_frameworkTreeWidget->model()) {
        for (int row = 0, count = model->rowCount(); row < count; ++row) {
            const QModelIndex idx = model->index(row, 0);
            if (idx.data(Qt::CheckStateRole) == Qt::Checked) {
                atLeastOneEnabled = true;
                mixed |= idx.data(BaseType).toInt();
            }
        }
    }

    if (!atLeastOneEnabled || (mixed == (ITestBase::Framework | ITestBase::Tool))) {
        if (!atLeastOneEnabled) {
            m_frameworksWarn->setText(Tr::tr("No active test frameworks or tools."));
            m_frameworksWarn->setToolTip(Tr::tr("You will not be able to use the AutoTest plugin "
                                                "without having at least one active test framework."));
        } else {
            m_frameworksWarn->setText(Tr::tr("Mixing test frameworks and test tools."));
            m_frameworksWarn->setToolTip(Tr::tr("Mixing test frameworks and test tools can lead "
                                                "to duplicating run information when using "
                                                "\"Run All Tests\", for example."));
        }
    }
    m_frameworksWarn->setVisible(!atLeastOneEnabled
                                    || (mixed == (ITestBase::Framework | ITestBase::Tool)));
}

// TestSettingsPage

TestSettingsPage::TestSettingsPage()
{
    setId(Constants::AUTOTEST_SETTINGS_ID);
    setDisplayName(Tr::tr("General"));
    setCategory(Constants::AUTOTEST_SETTINGS_CATEGORY);
    setDisplayCategory(Tr::tr("Testing"));
    setCategoryIconPath(":/autotest/images/settingscategory_autotest.png");
    setWidgetCreator([] { return new TestSettingsWidget; });
}

} // Autotest::Internal
