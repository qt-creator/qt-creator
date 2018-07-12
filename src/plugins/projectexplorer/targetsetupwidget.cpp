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

#include "targetsetupwidget.h"

#include "buildconfiguration.h"
#include "buildinfo.h"
#include "projectexplorerconstants.h"
#include "kit.h"
#include "kitconfigwidget.h"
#include "kitmanager.h"
#include "kitoptionspage.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/detailsbutton.h>
#include <utils/detailswidget.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

namespace ProjectExplorer {
namespace Internal {

// -------------------------------------------------------------------------
// TargetSetupWidget
// -------------------------------------------------------------------------

TargetSetupWidget::TargetSetupWidget(Kit *k, const QString &projectPath) :
    m_kit(k)
{
    Q_ASSERT(m_kit);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto vboxLayout = new QVBoxLayout();
    setLayout(vboxLayout);
    vboxLayout->setContentsMargins(0, 0, 0, 0);
    m_detailsWidget = new Utils::DetailsWidget(this);
    m_detailsWidget->setUseCheckBox(true);
    m_detailsWidget->setChecked(false);
    m_detailsWidget->setSummaryFontBold(true);
    m_detailsWidget->setToolTip(m_kit->toHtml());
    vboxLayout->addWidget(m_detailsWidget);

    auto panel = new Utils::FadingWidget(m_detailsWidget);
    auto panelLayout = new QHBoxLayout(panel);
    m_manageButton = new QPushButton(KitConfigWidget::msgManage());
    panelLayout->addWidget(m_manageButton);
    m_detailsWidget->setToolWidget(panel);

    handleKitUpdate(m_kit);

    auto widget = new QWidget;
    auto layout = new QVBoxLayout;
    widget->setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);

    auto w = new QWidget;
    m_newBuildsLayout = new QGridLayout;
    m_newBuildsLayout->setMargin(0);
    if (Utils::HostOsInfo::isMacHost())
        m_newBuildsLayout->setSpacing(0);
    w->setLayout(m_newBuildsLayout);
    layout->addWidget(w);

    widget->setEnabled(false);
    m_detailsWidget->setWidget(widget);

    setProjectPath(projectPath);

    connect(m_detailsWidget, &Utils::DetailsWidget::checked,
            this, &TargetSetupWidget::targetCheckBoxToggled);

    connect(KitManager::instance(), &KitManager::kitUpdated,
            this, &TargetSetupWidget::handleKitUpdate);

    connect(m_manageButton, &QAbstractButton::clicked, this, &TargetSetupWidget::manageKit);
}

Kit *TargetSetupWidget::kit() const
{
    return m_kit;
}

void TargetSetupWidget::clearKit()
{
    m_kit = nullptr;
}

bool TargetSetupWidget::isKitSelected() const
{
    if (!m_kit || !m_detailsWidget->isChecked())
        return false;

    return !selectedBuildInfoList().isEmpty();
}

void TargetSetupWidget::setKitSelected(bool b)
{
    // Only check target if there are build configurations possible
    b &= !selectedBuildInfoList().isEmpty();
    m_ignoreChange = true;
    m_detailsWidget->setChecked(b);
    m_detailsWidget->widget()->setEnabled(b);
    m_ignoreChange = false;
}

void TargetSetupWidget::addBuildInfo(BuildInfo *info, bool isImport)
{
    QTC_ASSERT(info && info->kitId == m_kit->id(), return);

    if (isImport && !m_haveImported) {
        // disable everything on first import
        for (BuildInfoStore &store : m_infoStore) {
            store.isEnabled = false;
            store.checkbox->setChecked(false);
        }
        m_selected = 0;

        m_haveImported = true;
    }

    const auto pos = static_cast<int>(m_infoStore.size());

    BuildInfoStore store;
    store.buildInfo = info;
    store.isEnabled = true;
    ++m_selected;

    if (info->factory()) {
        store.checkbox = new QCheckBox;
        store.checkbox->setText(info->displayName);
        store.checkbox->setChecked(store.isEnabled);
        store.checkbox->setAttribute(Qt::WA_LayoutUsesWidgetRect);
        m_newBuildsLayout->addWidget(store.checkbox, pos * 2, 0);

        store.pathChooser = new Utils::PathChooser();
        store.pathChooser->setExpectedKind(Utils::PathChooser::Directory);
        store.pathChooser->setFileName(info->buildDirectory);
        store.pathChooser->setHistoryCompleter(QLatin1String("TargetSetup.BuildDir.History"));
        store.pathChooser->setReadOnly(isImport);
        m_newBuildsLayout->addWidget(store.pathChooser, pos * 2, 1);

        store.issuesLabel = new QLabel;
        store.issuesLabel->setIndent(32);
        m_newBuildsLayout->addWidget(store.issuesLabel, pos * 2 + 1, 0, 1, 2);
        store.issuesLabel->setVisible(false);

        connect(store.checkbox, &QAbstractButton::toggled, this, &TargetSetupWidget::checkBoxToggled);
        connect(store.pathChooser, &Utils::PathChooser::rawPathChanged, this, &TargetSetupWidget::pathChanged);
    }

    store.hasIssues = false;
    m_infoStore.emplace_back(std::move(store));

    reportIssues(pos);

    emit selectedToggled();
}

void TargetSetupWidget::targetCheckBoxToggled(bool b)
{
    if (m_ignoreChange)
        return;
    m_detailsWidget->widget()->setEnabled(b);
    m_detailsWidget->setState(b && Utils::contains(m_infoStore, &BuildInfoStore::hasIssues)
                              ? Utils::DetailsWidget::Expanded
                              : Utils::DetailsWidget::Collapsed);
    emit selectedToggled();
}

void TargetSetupWidget::manageKit()
{
    if (!m_kit)
        return;

    if (auto kitPage = KitOptionsPage::instance()) {
        kitPage->showKit(m_kit);
        Core::ICore::showOptionsDialog(Constants::KITS_SETTINGS_PAGE_ID, parentWidget());
    }
}

void TargetSetupWidget::setProjectPath(const QString &projectPath)
{
    if (!m_kit)
        return;

    m_projectPath = projectPath;
    clear();

    for (BuildInfo *info : buildInfoList(m_kit, projectPath))
        addBuildInfo(info, false);
}

void TargetSetupWidget::expandWidget()
{
    m_detailsWidget->setState(Utils::DetailsWidget::Expanded);
}

QList<BuildInfo *> TargetSetupWidget::buildInfoList(const Kit *k, const QString &projectPath)
{
    const IBuildConfigurationFactory *const factory
            = IBuildConfigurationFactory::find(k, projectPath);
    if (factory)
        return factory->availableSetups(k, projectPath);

    auto *info = new BuildInfo(nullptr);
    info->kitId = k->id();
    return QList<BuildInfo *>({info});
}

void TargetSetupWidget::handleKitUpdate(Kit *k)
{
    if (k != m_kit)
        return;

    m_detailsWidget->setIcon(k->icon());
    m_detailsWidget->setSummaryText(k->displayName());
}

QList<const BuildInfo *> TargetSetupWidget::selectedBuildInfoList() const
{
    QList<const BuildInfo *> result;
    for (const BuildInfoStore &store : m_infoStore) {
        if (store.isEnabled)
            result.append(store.buildInfo);
    }
    return result;
}

void TargetSetupWidget::clear()
{
    m_infoStore.clear();

    m_selected = 0;
    m_haveImported = false;

    emit selectedToggled();
}

void TargetSetupWidget::checkBoxToggled(bool b)
{
    auto box = qobject_cast<QCheckBox *>(sender());
    if (!box)
        return;
    auto it = std::find_if(m_infoStore.begin(), m_infoStore.end(),
                           [box](const BuildInfoStore &store) { return store.checkbox == box; });
    QTC_ASSERT(it != m_infoStore.end(), return);
    if (it->isEnabled == b)
        return;
    m_selected += b ? 1 : -1;
    it->isEnabled = b;
    if ((m_selected == 0 && !b) || (m_selected == 1 && b)) {
        emit selectedToggled();
        m_detailsWidget->setChecked(b);
    }
}

void TargetSetupWidget::pathChanged()
{
    if (m_ignoreChange)
        return;
    auto pathChooser = qobject_cast<Utils::PathChooser *>(sender());
    QTC_ASSERT(pathChooser, return);

    auto it = std::find_if(m_infoStore.begin(), m_infoStore.end(),
                           [pathChooser](const BuildInfoStore &store) {
        return store.pathChooser == pathChooser;
    });
    QTC_ASSERT(it != m_infoStore.end(), return);
    it->buildInfo->buildDirectory = pathChooser->fileName();
    reportIssues(static_cast<int>(std::distance(m_infoStore.begin(), it)));
}

void TargetSetupWidget::reportIssues(int index)
{
    const auto size = static_cast<int>(m_infoStore.size());
    QTC_ASSERT(index >= 0 && index < size, return);

    BuildInfoStore &store = m_infoStore[static_cast<size_t>(index)];
    if (store.issuesLabel) {
        QPair<Task::TaskType, QString> issues = findIssues(store.buildInfo);
        store.issuesLabel->setText(issues.second);
        store.hasIssues = issues.first != Task::Unknown;
        store.issuesLabel->setVisible(store.hasIssues);
    }
}

QPair<Task::TaskType, QString> TargetSetupWidget::findIssues(const BuildInfo *info)
{
    if (m_projectPath.isEmpty() || !info->factory())
        return qMakePair(Task::Unknown, QString());

    QString buildDir = info->buildDirectory.toString();
    QList<Task> issues = info->reportIssues(m_projectPath, buildDir);

    QString text;
    Task::TaskType highestType = Task::Unknown;
    foreach (const Task &t, issues) {
        if (!text.isEmpty())
            text.append(QLatin1String("<br>"));
        // set severity:
        QString severity;
        if (t.type == Task::Error) {
            highestType = Task::Error;
            severity = tr("<b>Error:</b> ", "Severity is Task::Error");
        } else if (t.type == Task::Warning) {
            if (highestType == Task::Unknown)
                highestType = Task::Warning;
            severity = tr("<b>Warning:</b> ", "Severity is Task::Warning");
        }
        text.append(severity + t.description);
    }
    if (!text.isEmpty())
        text = QLatin1String("<nobr>") + text;
    return qMakePair(highestType, text);
}

TargetSetupWidget::BuildInfoStore::~BuildInfoStore()
{
    delete buildInfo;
    delete checkbox;
    delete label;
    delete issuesLabel;
    delete pathChooser;
}

TargetSetupWidget::BuildInfoStore::BuildInfoStore(TargetSetupWidget::BuildInfoStore &&other)
{
    std::swap(other.buildInfo, buildInfo);
    std::swap(other.checkbox, checkbox);
    std::swap(other.label, label);
    std::swap(other.issuesLabel, issuesLabel);
    std::swap(other.pathChooser, pathChooser);
    std::swap(other.isEnabled, isEnabled);
    std::swap(other.hasIssues, hasIssues);
}

} // namespace Internal
} // namespace ProjectExplorer
