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

#include "testsettingspage.h"

#include "autotestconstants.h"
#include "autotestplugin.h"
#include "testframeworkmanager.h"
#include "testsettings.h"
#include "testtreemodel.h"

#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/id.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

namespace Autotest {
namespace Internal {

TestSettingsWidget::TestSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    m_ui.setupUi(this);

    m_ui.frameworksWarn->setVisible(false);
    m_ui.frameworksWarn->setElideMode(Qt::ElideNone);
    m_ui.frameworksWarn->setType(Utils::InfoLabel::Warning);
    connect(m_ui.frameworkTreeWidget, &QTreeWidget::itemChanged,
            this, &TestSettingsWidget::onFrameworkItemChanged);
    connect(m_ui.resetChoicesButton, &QPushButton::clicked,
            this, [] { AutotestPlugin::clearChoiceCache(); });
    connect(m_ui.openResultsOnFinishCB, &QCheckBox::toggled,
            m_ui.openResultsOnFailCB, &QCheckBox::setEnabled);
    connect(m_ui.limitResultDescriptionCb, &QCheckBox::toggled,
            m_ui.limitResultDescriptionSpinBox, &QSpinBox::setEnabled);
}

void TestSettingsWidget::setSettings(const TestSettings &settings)
{
    m_ui.timeoutSpin->setValue(settings.timeout / 1000); // we store milliseconds
    m_ui.omitInternalMsgCB->setChecked(settings.omitInternalMssg);
    m_ui.omitRunConfigWarnCB->setChecked(settings.omitRunConfigWarn);
    m_ui.limitResultOutputCB->setChecked(settings.limitResultOutput);
    m_ui.limitResultDescriptionCb->setChecked(settings.limitResultDescription);
    m_ui.limitResultDescriptionSpinBox->setEnabled(settings.limitResultDescription);
    m_ui.limitResultDescriptionSpinBox->setValue(settings.resultDescriptionMaxSize);
    m_ui.autoScrollCB->setChecked(settings.autoScroll);
    m_ui.processArgsCB->setChecked(settings.processArgs);
    m_ui.displayAppCB->setChecked(settings.displayApplication);
    m_ui.openResultsOnStartCB->setChecked(settings.popupOnStart);
    m_ui.openResultsOnFinishCB->setChecked(settings.popupOnFinish);
    m_ui.openResultsOnFailCB->setChecked(settings.popupOnFail);
    m_ui.runAfterBuildCB->setCurrentIndex(int(settings.runAfterBuild));
    populateFrameworksListWidget(settings.frameworks, settings.tools);
}

TestSettings TestSettingsWidget::settings() const
{
    TestSettings result;
    result.timeout = m_ui.timeoutSpin->value() * 1000; // we display seconds
    result.omitInternalMssg = m_ui.omitInternalMsgCB->isChecked();
    result.omitRunConfigWarn = m_ui.omitRunConfigWarnCB->isChecked();
    result.limitResultOutput = m_ui.limitResultOutputCB->isChecked();
    result.limitResultDescription = m_ui.limitResultDescriptionCb->isChecked();
    result.resultDescriptionMaxSize = m_ui.limitResultDescriptionSpinBox->value();
    result.autoScroll = m_ui.autoScrollCB->isChecked();
    result.processArgs = m_ui.processArgsCB->isChecked();
    result.displayApplication = m_ui.displayAppCB->isChecked();
    result.popupOnStart = m_ui.openResultsOnStartCB->isChecked();
    result.popupOnFinish = m_ui.openResultsOnFinishCB->isChecked();
    result.popupOnFail = m_ui.openResultsOnFailCB->isChecked();
    result.runAfterBuild = RunAfterBuildMode(m_ui.runAfterBuildCB->currentIndex());
    testSettings(result);
    testToolsSettings(result);
    return result;
}

namespace {

enum TestBaseInfo
{
    BaseId = Qt::UserRole,
    BaseType
};

}

void TestSettingsWidget::populateFrameworksListWidget(const QHash<Utils::Id, bool> &frameworks,
                                                      const QHash<Utils::Id, bool> &testTools)
{
    const TestFrameworks &registered = TestFrameworkManager::registeredFrameworks();
    m_ui.frameworkTreeWidget->clear();
    for (const ITestFramework *framework : registered) {
        const Utils::Id id = framework->id();
        auto item = new QTreeWidgetItem(m_ui.frameworkTreeWidget, QStringList(QLatin1String(framework->name())));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
        item->setCheckState(0, frameworks.value(id) ? Qt::Checked : Qt::Unchecked);
        item->setData(0, BaseId, id.toSetting());
        item->setData(0, BaseType, ITestBase::Framework);
        item->setData(1, Qt::CheckStateRole, framework->grouping() ? Qt::Checked : Qt::Unchecked);
        item->setToolTip(0, tr("Enable or disable test frameworks to be handled by the AutoTest "
                               "plugin."));
        QString toolTip = framework->groupingToolTip();
        if (toolTip.isEmpty())
            toolTip = tr("Enable or disable grouping of test cases by folder.");
        item->setToolTip(1, toolTip);
    }
    // ...and now the test tools
    const TestTools &registeredTools = TestFrameworkManager::registeredTestTools();
    for (const ITestTool *testTool : registeredTools) {
        const Utils::Id id = testTool->id();
        auto item = new QTreeWidgetItem(m_ui.frameworkTreeWidget, {QLatin1String(testTool->name())});
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
        item->setCheckState(0, testTools.value(id) ? Qt::Checked : Qt::Unchecked);
        item->setData(0, BaseId, id.toSetting());
        item->setData(0, BaseType, ITestBase::Tool);
    }
}

void TestSettingsWidget::testSettings(TestSettings &settings) const
{
    const QAbstractItemModel *model = m_ui.frameworkTreeWidget->model();
    QTC_ASSERT(model, return);
    const int itemCount = TestFrameworkManager::registeredFrameworks().size();
    QTC_ASSERT(itemCount <= model->rowCount(), return);
    for (int row = 0; row < itemCount; ++row) {
        QModelIndex idx = model->index(row, 0);
        const Utils::Id id = Utils::Id::fromSetting(idx.data(BaseId));
        settings.frameworks.insert(id, idx.data(Qt::CheckStateRole) == Qt::Checked);
        idx = model->index(row, 1);
        settings.frameworksGrouping.insert(id, idx.data(Qt::CheckStateRole) == Qt::Checked);
    }
}

void TestSettingsWidget::testToolsSettings(TestSettings &settings) const
{
    const QAbstractItemModel *model = m_ui.frameworkTreeWidget->model();
    QTC_ASSERT(model, return);
    // frameworks are listed before tools
    int row = TestFrameworkManager::registeredFrameworks().size();
    const int end = model->rowCount();
    QTC_ASSERT(row <= end, return);
    for ( ; row < end; ++row) {
        const QModelIndex idx = model->index(row, 0);
        const Utils::Id id = Utils::Id::fromSetting(idx.data(BaseId));
        settings.tools.insert(id, idx.data(Qt::CheckStateRole) == Qt::Checked);
    }
}

void TestSettingsWidget::onFrameworkItemChanged()
{
    bool atLeastOneEnabled = false;
    int mixed = ITestBase::None;
    if (QAbstractItemModel *model = m_ui.frameworkTreeWidget->model()) {
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
            m_ui.frameworksWarn->setText(tr("No active test frameworks or tools."));
            m_ui.frameworksWarn->setToolTip(tr("You will not be able to use the AutoTest plugin "
                                               "without having at least one active test framework."));
        } else {
            m_ui.frameworksWarn->setText(tr("Mixing test frameworks and test tools."));
            m_ui.frameworksWarn->setToolTip(tr("Mixing test frameworks and test tools can lead "
                                               "to duplicating run information when using "
                                                "\"Run All Tests\", for example."));
        }
    }
    m_ui.frameworksWarn->setVisible(!atLeastOneEnabled
                                    || (mixed == (ITestBase::Framework | ITestBase::Tool)));
}

TestSettingsPage::TestSettingsPage(TestSettings *settings)
    : m_settings(settings)
{
    setId("A.AutoTest.0.General");
    setDisplayName(tr("General"));
    setCategory(Constants::AUTOTEST_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("AutoTest", Constants::AUTOTEST_SETTINGS_TR));
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
    const QList<Utils::Id> changedIds = Utils::filtered(newSettings.frameworksGrouping.keys(),
                                                       [newSettings, this] (const Utils::Id &id) {
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
