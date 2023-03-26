// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testsettingspage.h"

#include "autotestconstants.h"
#include "autotestplugin.h"
#include "autotesttr.h"
#include "testframeworkmanager.h"
#include "testsettings.h"
#include "testtreemodel.h"

#include <coreplugin/icore.h>
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
#include <QSpinBox>
#include <QTreeWidget>
#include <QWidget>

using namespace Utils;

namespace Autotest {
namespace Internal {

class TestSettingsWidget : public QWidget
{
public:
    explicit TestSettingsWidget(QWidget *parent = nullptr);

    void setSettings(const TestSettings &settings);
    TestSettings settings() const;

private:
    void populateFrameworksListWidget(const QHash<Id, bool> &frameworks,
                                      const QHash<Id, bool> &testTools);
    void testSettings(TestSettings &settings) const;
    void testToolsSettings(TestSettings &settings) const;
    void onFrameworkItemChanged();

    QCheckBox *m_omitInternalMsgCB;
    QCheckBox *m_omitRunConfigWarnCB;
    QCheckBox *m_limitResultOutputCB;
    QCheckBox *m_limitResultDescriptionCb;
    QSpinBox *m_limitResultDescriptionSpinBox;
    QCheckBox *m_openResultsOnStartCB;
    QCheckBox *m_openResultsOnFinishCB;
    QCheckBox *m_openResultsOnFailCB;
    QCheckBox *m_autoScrollCB;
    QCheckBox *m_displayAppCB;
    QCheckBox *m_processArgsCB;
    QComboBox *m_runAfterBuildCB;
    QSpinBox *m_timeoutSpin;
    QTreeWidget *m_frameworkTreeWidget;
    InfoLabel *m_frameworksWarn;
};

TestSettingsWidget::TestSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    resize(586, 469);

    m_omitInternalMsgCB = new QCheckBox(Tr::tr("Omit internal messages"));
    m_omitInternalMsgCB->setChecked(true);
    m_omitInternalMsgCB->setToolTip(Tr::tr("Hides internal messages by default. "
        "You can still enable them by using the test results filter."));

    m_omitRunConfigWarnCB = new QCheckBox(Tr::tr("Omit run configuration warnings"));
    m_omitRunConfigWarnCB->setToolTip(Tr::tr("Hides warnings related to a deduced run configuration."));

    m_limitResultOutputCB = new QCheckBox(Tr::tr("Limit result output"));
    m_limitResultOutputCB->setChecked(true);
    m_limitResultOutputCB->setToolTip(Tr::tr("Limits result output to 100000 characters."));

    m_limitResultDescriptionCb = new QCheckBox(Tr::tr("Limit result description:"));
    m_limitResultDescriptionCb->setToolTip(
        Tr::tr("Limit number of lines shown in test result tooltip and description."));

    m_limitResultDescriptionSpinBox = new QSpinBox;
    m_limitResultDescriptionSpinBox->setEnabled(false);
    m_limitResultDescriptionSpinBox->setMinimum(1);
    m_limitResultDescriptionSpinBox->setMaximum(1000000);
    m_limitResultDescriptionSpinBox->setValue(10);

    m_openResultsOnStartCB = new QCheckBox(Tr::tr("Open results when tests start"));
    m_openResultsOnStartCB->setToolTip(
        Tr::tr("Displays test results automatically when tests are started."));

    m_openResultsOnFinishCB = new QCheckBox(Tr::tr("Open results when tests finish"));
    m_openResultsOnFinishCB->setChecked(true);
    m_openResultsOnFinishCB->setToolTip(
        Tr::tr("Displays test results automatically when tests are finished."));

    m_openResultsOnFailCB = new QCheckBox(Tr::tr("Only for unsuccessful test runs"));
    m_openResultsOnFailCB->setToolTip(
        Tr::tr("Displays test results only if the test run contains failed, fatal or unexpectedly passed tests."));

    m_autoScrollCB = new QCheckBox(Tr::tr("Automatically scroll results"));
    m_autoScrollCB->setChecked(true);
    m_autoScrollCB->setToolTip(Tr::tr("Automatically scrolls down when new items are added and scrollbar is at bottom."));

    m_displayAppCB = new QCheckBox(Tr::tr("Group results by application"));

    m_processArgsCB = new QCheckBox(Tr::tr("Process arguments"));
    m_processArgsCB->setToolTip(
        Tr::tr("Allow passing arguments specified on the respective run configuration.\n"
           "Warning: this is an experimental feature and might lead to failing to execute the test executable."));

    m_runAfterBuildCB = new QComboBox;
    m_runAfterBuildCB->setToolTip(Tr::tr("Runs chosen tests automatically if a build succeeded."));
    m_runAfterBuildCB->addItem(Tr::tr("None"));
    m_runAfterBuildCB->addItem(Tr::tr("All"));
    m_runAfterBuildCB->addItem(Tr::tr("Selected"));

    auto timeoutLabel = new QLabel(Tr::tr("Timeout:"));
    timeoutLabel->setToolTip(Tr::tr("Timeout used when executing each test case."));

    m_timeoutSpin = new QSpinBox;
    m_timeoutSpin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_timeoutSpin->setRange(5, 36000);
    m_timeoutSpin->setValue(60);
    m_timeoutSpin->setSuffix(Tr::tr(" s"));
    m_timeoutSpin->setToolTip(
        Tr::tr("Timeout used when executing test cases. This will apply "
               "for each test case on its own, not the whole project."));

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

    Group generalGroup {
        title(Tr::tr("General")),
        Column {
            m_omitInternalMsgCB,
            m_omitRunConfigWarnCB,
            m_limitResultOutputCB,
            Row { m_limitResultDescriptionCb, m_limitResultDescriptionSpinBox, st },
            m_openResultsOnStartCB,
            m_openResultsOnFinishCB,
            Row { Space(20), m_openResultsOnFailCB },
            m_autoScrollCB,
            m_displayAppCB,
            m_processArgsCB,
            Row { Tr::tr("Automatically run"), m_runAfterBuildCB, st },
            Row { timeoutLabel, m_timeoutSpin, st },
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
    connect(m_openResultsOnFinishCB, &QCheckBox::toggled,
            m_openResultsOnFailCB, &QCheckBox::setEnabled);
    connect(m_limitResultDescriptionCb, &QCheckBox::toggled,
            m_limitResultDescriptionSpinBox, &QSpinBox::setEnabled);
}

void TestSettingsWidget::setSettings(const TestSettings &settings)
{
    m_timeoutSpin->setValue(settings.timeout / 1000); // we store milliseconds
    m_omitInternalMsgCB->setChecked(settings.omitInternalMssg);
    m_omitRunConfigWarnCB->setChecked(settings.omitRunConfigWarn);
    m_limitResultOutputCB->setChecked(settings.limitResultOutput);
    m_limitResultDescriptionCb->setChecked(settings.limitResultDescription);
    m_limitResultDescriptionSpinBox->setEnabled(settings.limitResultDescription);
    m_limitResultDescriptionSpinBox->setValue(settings.resultDescriptionMaxSize);
    m_autoScrollCB->setChecked(settings.autoScroll);
    m_processArgsCB->setChecked(settings.processArgs);
    m_displayAppCB->setChecked(settings.displayApplication);
    m_openResultsOnStartCB->setChecked(settings.popupOnStart);
    m_openResultsOnFinishCB->setChecked(settings.popupOnFinish);
    m_openResultsOnFailCB->setChecked(settings.popupOnFail);
    m_runAfterBuildCB->setCurrentIndex(int(settings.runAfterBuild));
    populateFrameworksListWidget(settings.frameworks, settings.tools);
}

TestSettings TestSettingsWidget::settings() const
{
    TestSettings result;
    result.timeout = m_timeoutSpin->value() * 1000; // we display seconds
    result.omitInternalMssg = m_omitInternalMsgCB->isChecked();
    result.omitRunConfigWarn = m_omitRunConfigWarnCB->isChecked();
    result.limitResultOutput = m_limitResultOutputCB->isChecked();
    result.limitResultDescription = m_limitResultDescriptionCb->isChecked();
    result.resultDescriptionMaxSize = m_limitResultDescriptionSpinBox->value();
    result.autoScroll = m_autoScrollCB->isChecked();
    result.processArgs = m_processArgsCB->isChecked();
    result.displayApplication = m_displayAppCB->isChecked();
    result.popupOnStart = m_openResultsOnStartCB->isChecked();
    result.popupOnFinish = m_openResultsOnFinishCB->isChecked();
    result.popupOnFail = m_openResultsOnFailCB->isChecked();
    result.runAfterBuild = RunAfterBuildMode(m_runAfterBuildCB->currentIndex());
    testSettings(result);
    testToolsSettings(result);
    return result;
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

void TestSettingsWidget::testSettings(TestSettings &settings) const
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

void TestSettingsWidget::testToolsSettings(TestSettings &settings) const
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

TestSettingsPage::TestSettingsPage(TestSettings *settings)
    : m_settings(settings)
{
    setId(Constants::AUTOTEST_SETTINGS_ID);
    setDisplayName(Tr::tr("General"));
    setCategory(Constants::AUTOTEST_SETTINGS_CATEGORY);
    setDisplayCategory(Tr::tr("Testing"));
    setCategoryIconPath(":/autotest/images/settingscategory_autotest.png");
}

QWidget *TestSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new TestSettingsWidget;
        m_widget->setSettings(*m_settings);
    }
    return m_widget;
}

void TestSettingsPage::apply()
{
    if (!m_widget) // page was not shown at all
        return;
    const TestSettings newSettings = m_widget->settings();
    const QList<Id> changedIds = Utils::filtered(newSettings.frameworksGrouping.keys(),
                                                 [newSettings, this](const Id &id) {
        return newSettings.frameworksGrouping[id] != m_settings->frameworksGrouping[id];
    });
    *m_settings = newSettings;
    m_settings->toSettings(Core::ICore::settings());

    for (ITestFramework *framework : TestFrameworkManager::registeredFrameworks()) {
        framework->setActive(m_settings->frameworks.value(framework->id(), false));
        framework->setGrouping(m_settings->frameworksGrouping.value(framework->id(), false));
    }

    for (ITestTool *testTool : TestFrameworkManager::registeredTestTools())
        testTool->setActive(m_settings->tools.value(testTool->id(), false));

    TestTreeModel::instance()->synchronizeTestFrameworks();
    TestTreeModel::instance()->synchronizeTestTools();
    if (!changedIds.isEmpty())
        TestTreeModel::instance()->rebuild(changedIds);
}

} // namespace Internal
} // namespace Autotest
