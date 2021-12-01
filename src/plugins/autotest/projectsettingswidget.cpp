/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "projectsettingswidget.h"

#include "autotestplugin.h"
#include "testframeworkmanager.h"
#include "testprojectsettings.h"
#include "testtreemodel.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QTreeWidget>

namespace Autotest {
namespace Internal {

enum ItemDataRole  {
    BaseIdRole = Qt::UserRole + 1,
    BaseTypeRole
};

static QSpacerItem *createSpacer(QSizePolicy::Policy horizontal, QSizePolicy::Policy vertical)
{
    return new QSpacerItem(20, 10, horizontal, vertical);
}

ProjectTestSettingsWidget::ProjectTestSettingsWidget(ProjectExplorer::Project *project,
                                                     QWidget *parent)
    : QWidget(parent)
    , m_projectSettings(AutotestPlugin::projectSettings(project))
{
    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->setContentsMargins(0, 0, 0, 0);
    m_useGlobalSettings = new QComboBox;
    m_useGlobalSettings->addItem(tr("Global"));
    m_useGlobalSettings->addItem(tr("Custom"));

    auto generalWidget = new QWidget;
    auto groupBoxLayout = new QVBoxLayout;
    groupBoxLayout->setContentsMargins(0, 0, 0, 0);
    m_activeFrameworks = new QTreeWidget;
    m_activeFrameworks->setHeaderHidden(true);
    m_activeFrameworks->setRootIsDecorated(false);
    groupBoxLayout->addWidget(new QLabel(tr("Active frameworks:")));
    groupBoxLayout->addWidget(m_activeFrameworks);
    auto horizontalLayout = new QHBoxLayout;
    horizontalLayout->addWidget(new QLabel(tr("Automatically run tests after build")));
    m_runAfterBuild = new QComboBox;
    m_runAfterBuild->addItem(tr("None"));
    m_runAfterBuild->addItem(tr("All"));
    m_runAfterBuild->addItem(tr("Selected"));
    m_runAfterBuild->setCurrentIndex(int(m_projectSettings->runAfterBuild()));
    horizontalLayout->addWidget(m_runAfterBuild);
    horizontalLayout->addItem(createSpacer(QSizePolicy::Expanding, QSizePolicy::Minimum));
    groupBoxLayout->addLayout(horizontalLayout);
    generalWidget->setLayout(groupBoxLayout);

    horizontalLayout = new QHBoxLayout;
    horizontalLayout->addWidget(m_useGlobalSettings);
    horizontalLayout->addItem(createSpacer(QSizePolicy::Expanding, QSizePolicy::Minimum));
    verticalLayout->addLayout(horizontalLayout);
    horizontalLayout = new QHBoxLayout;
    verticalLayout->addItem(createSpacer(QSizePolicy::Minimum, QSizePolicy::Fixed));
    horizontalLayout->addWidget(generalWidget);
    horizontalLayout->addItem(createSpacer(QSizePolicy::Expanding, QSizePolicy::Minimum));
    verticalLayout->addLayout(horizontalLayout);
    verticalLayout->addItem(createSpacer(QSizePolicy::Minimum, QSizePolicy::Expanding));

    m_useGlobalSettings->setCurrentIndex(m_projectSettings->useGlobalSettings() ? 0 : 1);
    generalWidget->setDisabled(m_projectSettings->useGlobalSettings());

    populateFrameworks(m_projectSettings->activeFrameworks(),
                       m_projectSettings->activeTestTools());

    connect(m_useGlobalSettings, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this, generalWidget](int index) {
        generalWidget->setEnabled(index != 0);
        m_projectSettings->setUseGlobalSettings(index == 0);
        m_syncTimer.start(3000);
        m_syncType = ITestBase::Framework | ITestBase::Tool;
    });
    connect(m_activeFrameworks, &QTreeWidget::itemChanged,
            this, &ProjectTestSettingsWidget::onActiveFrameworkChanged);
    connect(m_runAfterBuild, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        m_projectSettings->setRunAfterBuild(RunAfterBuildMode(index));
    });
    m_syncTimer.setSingleShot(true);
    connect(&m_syncTimer, &QTimer::timeout,
            [this]() {
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
    TestFrameworks sortedFrameworks = frameworks.keys();
    Utils::sort(sortedFrameworks, &ITestFramework::priority);

    auto generateItem = [this](ITestBase *frameworkOrTestTool, bool checked) {
        auto item = new QTreeWidgetItem(m_activeFrameworks, {QLatin1String(frameworkOrTestTool->name())});
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
        item->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
        item->setData(0, BaseIdRole, frameworkOrTestTool->id().toSetting());
        item->setData(0, BaseTypeRole, frameworkOrTestTool->type());
    };

    for (ITestFramework *framework : qAsConst(sortedFrameworks))
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
