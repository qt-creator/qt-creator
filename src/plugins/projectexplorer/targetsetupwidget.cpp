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
#include <extensionsystem/pluginmanager.h>
#include <utils/detailsbutton.h>
#include <utils/detailswidget.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>

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

TargetSetupWidget::TargetSetupWidget(Kit *k,
                                     const QString &projectPath,
                                     const QList<BuildInfo *> &infoList) :
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

    foreach (BuildInfo *info, infoList)
        addBuildInfo(info, false);

    setProjectPath(projectPath);

    connect(m_detailsWidget, &Utils::DetailsWidget::checked,
            this, &TargetSetupWidget::targetCheckBoxToggled);

    connect(KitManager::instance(), &KitManager::kitUpdated,
            this, &TargetSetupWidget::handleKitUpdate);

    connect(m_manageButton, &QAbstractButton::clicked, this, &TargetSetupWidget::manageKit);
}

TargetSetupWidget::~TargetSetupWidget()
{
    qDeleteAll(m_infoList);
    m_infoList.clear();
}

Kit *TargetSetupWidget::kit()
{
    return m_kit;
}

void TargetSetupWidget::clearKit()
{
    m_kit = nullptr;
}

bool TargetSetupWidget::isKitSelected() const
{
    if (!m_detailsWidget->isChecked())
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
    if (isImport && !m_haveImported) {
        // disable everything on first import
        for (int i = 0; i < m_enabled.count(); ++i) {
            m_enabled[i] = false;
            m_checkboxes[i]->setChecked(false);
        }
        m_selected = 0;

        m_haveImported = true;
    }

    int pos = m_pathChoosers.count();
    m_enabled.append(true);
    ++m_selected;

    m_infoList << info;

    auto checkbox = new QCheckBox;
    checkbox->setText(info->displayName);
    checkbox->setChecked(m_enabled.at(pos));
    checkbox->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    m_newBuildsLayout->addWidget(checkbox, pos * 2, 0);

    auto pathChooser = new Utils::PathChooser();
    pathChooser->setExpectedKind(Utils::PathChooser::Directory);
    pathChooser->setFileName(info->buildDirectory);
    pathChooser->setHistoryCompleter(QLatin1String("TargetSetup.BuildDir.History"));
    pathChooser->setReadOnly(isImport);
    m_newBuildsLayout->addWidget(pathChooser, pos * 2, 1);

    auto reportIssuesLabel = new QLabel;
    reportIssuesLabel->setIndent(32);
    m_newBuildsLayout->addWidget(reportIssuesLabel, pos * 2 + 1, 0, 1, 2);
    reportIssuesLabel->setVisible(false);

    connect(checkbox, &QAbstractButton::toggled, this, &TargetSetupWidget::checkBoxToggled);
    connect(pathChooser, &Utils::PathChooser::rawPathChanged, this, &TargetSetupWidget::pathChanged);

    m_checkboxes.append(checkbox);
    m_pathChoosers.append(pathChooser);
    m_reportIssuesLabels.append(reportIssuesLabel);

    m_issues.append(false);
    reportIssues(pos);

    emit selectedToggled();
}

void TargetSetupWidget::targetCheckBoxToggled(bool b)
{
    if (m_ignoreChange)
        return;
    m_detailsWidget->widget()->setEnabled(b);
    if (b) {
        foreach (bool error, m_issues) {
            if (error) {
                m_detailsWidget->setState(Utils::DetailsWidget::Expanded);
                break;
            }
        }
    } else {
        m_detailsWidget->setState(Utils::DetailsWidget::Collapsed);
    }
    emit selectedToggled();
}

void TargetSetupWidget::manageKit()
{
    KitOptionsPage *page = ExtensionSystem::PluginManager::getObject<KitOptionsPage>();
    if (!page || !m_kit)
        return;

    page->showKit(m_kit);
    Core::ICore::showOptionsDialog(Constants::KITS_SETTINGS_PAGE_ID, parentWidget());
}

void TargetSetupWidget::setProjectPath(const QString &projectPath)
{
    if (!m_kit)
        return;

    m_projectPath = projectPath;
    clear();

    IBuildConfigurationFactory *factory
            = IBuildConfigurationFactory::find(m_kit, projectPath);

    if (!factory)
        return;

    QList<BuildInfo *> infoList
            = factory->availableSetups(m_kit, projectPath);
    foreach (BuildInfo *info, infoList)
        addBuildInfo(info, false);
}

void TargetSetupWidget::expandWidget()
{
    m_detailsWidget->setState(Utils::DetailsWidget::Expanded);
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
    for (int i = 0; i < m_infoList.count(); ++i) {
        if (m_enabled.at(i))
            result.append(m_infoList.at(i));
    }
    return result;
}

void TargetSetupWidget::clear()
{
    qDeleteAll(m_checkboxes);
    m_checkboxes.clear();
    qDeleteAll(m_pathChoosers);
    m_pathChoosers.clear();
    qDeleteAll(m_reportIssuesLabels);
    m_reportIssuesLabels.clear();
    qDeleteAll(m_infoList);
    m_infoList.clear();

    m_issues.clear();
    m_enabled.clear();
    m_selected = 0;
    m_haveImported = false;

    emit selectedToggled();
}

void TargetSetupWidget::checkBoxToggled(bool b)
{
    auto box = qobject_cast<QCheckBox *>(sender());
    if (!box)
        return;
    int index = m_checkboxes.indexOf(box);
    if (index == -1)
        return;
    if (m_enabled[index] == b)
        return;
    m_selected += b ? 1 : -1;
    m_enabled[index] = b;
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
    if (!pathChooser)
        return;
    int index = m_pathChoosers.indexOf(pathChooser);
    if (index == -1)
        return;
    m_infoList[index]->buildDirectory = pathChooser->fileName();
    reportIssues(index);
}

void TargetSetupWidget::reportIssues(int index)
{
    QPair<Task::TaskType, QString> issues = findIssues(m_infoList.at(index));
    QLabel *reportIssuesLabel = m_reportIssuesLabels.at(index);
    reportIssuesLabel->setText(issues.second);
    bool error = issues.first != Task::Unknown;
    reportIssuesLabel->setVisible(error);
    m_issues[index] = error;
}

QPair<Task::TaskType, QString> TargetSetupWidget::findIssues(const BuildInfo *info)
{
    if (m_projectPath.isEmpty())
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

} // namespace Internal
} // namespace ProjectExplorer
