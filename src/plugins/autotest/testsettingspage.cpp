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

#include "autotestconstants.h"
#include "testframeworkmanager.h"
#include "testsettingspage.h"
#include "testsettings.h"
#include "testtreemodel.h"

#include <coreplugin/icore.h>

#include <utils/utilsicons.h>

namespace Autotest {
namespace Internal {

TestSettingsWidget::TestSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    m_ui.setupUi(this);

    m_ui.frameworksWarnIcon->setVisible(false);
    m_ui.frameworksWarnIcon->setPixmap(Utils::Icons::WARNING.pixmap());
    m_ui.frameworksWarn->setVisible(false);
    m_ui.frameworksWarn->setText(tr("No active test frameworks."));
    m_ui.frameworksWarn->setToolTip(tr("You will not be able to use the AutoTest plugin without "
                                       "having at least one active test framework."));
    connect(m_ui.frameworkListWidget, &QListWidget::itemChanged,
            this, &TestSettingsWidget::onFrameworkItemChanged);
}

void TestSettingsWidget::setSettings(const TestSettings &settings)
{
    m_ui.timeoutSpin->setValue(settings.timeout / 1000); // we store milliseconds
    m_ui.omitInternalMsgCB->setChecked(settings.omitInternalMssg);
    m_ui.omitRunConfigWarnCB->setChecked(settings.omitRunConfigWarn);
    m_ui.limitResultOutputCB->setChecked(settings.limitResultOutput);
    m_ui.autoScrollCB->setChecked(settings.autoScroll);
    m_ui.alwaysParseCB->setChecked(settings.alwaysParse);
    populateFrameworksListWidget(settings.frameworks);
}

TestSettings TestSettingsWidget::settings() const
{
    TestSettings result;
    result.timeout = m_ui.timeoutSpin->value() * 1000; // we display seconds
    result.omitInternalMssg = m_ui.omitInternalMsgCB->isChecked();
    result.omitRunConfigWarn = m_ui.omitRunConfigWarnCB->isChecked();
    result.limitResultOutput = m_ui.limitResultOutputCB->isChecked();
    result.autoScroll = m_ui.autoScrollCB->isChecked();
    result.alwaysParse = m_ui.alwaysParseCB->isChecked();
    result.frameworks = frameworks();
    return result;
}

void TestSettingsWidget::populateFrameworksListWidget(const QHash<Core::Id, bool> &frameworks)
{
    TestFrameworkManager *frameworkManager = TestFrameworkManager::instance();
    const QList<Core::Id> &registered = frameworkManager->sortedRegisteredFrameworkIds();
    m_ui.frameworkListWidget->clear();
    for (const Core::Id &id : registered) {
        QListWidgetItem *item = new QListWidgetItem(frameworkManager->frameworkNameForId(id),
                                                    m_ui.frameworkListWidget);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
        item->setCheckState(frameworks.value(id) ? Qt::Checked : Qt::Unchecked);
        item->setData(Qt::UserRole, id.toSetting());
    }
}

QHash<Core::Id, bool> TestSettingsWidget::frameworks() const
{
    const int itemCount = m_ui.frameworkListWidget->count();
    QHash<Core::Id, bool> frameworks;
    for (int row = 0; row < itemCount; ++row) {
        if (QListWidgetItem *item = m_ui.frameworkListWidget->item(row)) {
            frameworks.insert(Core::Id::fromSetting(item->data(Qt::UserRole)),
                              item->checkState() == Qt::Checked);
        }
    }
    return frameworks;
}

void TestSettingsWidget::onFrameworkItemChanged()
{
    for (int row = 0, count = m_ui.frameworkListWidget->count(); row < count; ++row) {
        if (m_ui.frameworkListWidget->item(row)->checkState() == Qt::Checked) {
            m_ui.frameworksWarn->setVisible(false);
            m_ui.frameworksWarnIcon->setVisible(false);
            return;
        }
    }
    m_ui.frameworksWarn->setVisible(true);
    m_ui.frameworksWarnIcon->setVisible(true);
}

TestSettingsPage::TestSettingsPage(const QSharedPointer<TestSettings> &settings)
    : m_settings(settings), m_widget(0)
{
    setId("A.AutoTest.0.General");
    setDisplayName(tr("General"));
    setCategory(Constants::AUTOTEST_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("AutoTest", Constants::AUTOTEST_SETTINGS_TR));
    setCategoryIcon(Utils::Icon(":/images/autotest.png"));
}

TestSettingsPage::~TestSettingsPage()
{
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
    bool frameworkSyncNecessary = newSettings.frameworks != m_settings->frameworks;
    *m_settings = newSettings;
    m_settings->toSettings(Core::ICore::settings());
    if (m_settings->alwaysParse)
        TestTreeModel::instance()->enableParsingFromSettings();
    else
        TestTreeModel::instance()->disableParsingFromSettings();
    TestFrameworkManager::instance()->activateFrameworksFromSettings(m_settings);
    if (frameworkSyncNecessary)
        TestTreeModel::instance()->syncTestFrameworks();
}

} // namespace Internal
} // namespace Autotest
