/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    m_kit(k),
    m_haveImported(false),
    m_ignoreChange(false),
    m_selected(0)
{
    Q_ASSERT(m_kit);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    QVBoxLayout *vboxLayout = new QVBoxLayout();
    setLayout(vboxLayout);
    vboxLayout->setContentsMargins(0, 0, 0, 0);
    m_detailsWidget = new Utils::DetailsWidget(this);
    m_detailsWidget->setUseCheckBox(true);
    m_detailsWidget->setChecked(false);
    m_detailsWidget->setSummaryFontBold(true);
    m_detailsWidget->setToolTip(m_kit->toHtml());
    vboxLayout->addWidget(m_detailsWidget);

    Utils::FadingWidget *panel = new Utils::FadingWidget(m_detailsWidget);
    QHBoxLayout *panelLayout = new QHBoxLayout(panel);
    m_manageButton = new QPushButton(KitConfigWidget::msgManage());
    panelLayout->addWidget(m_manageButton);
    m_detailsWidget->setToolWidget(panel);

    handleKitUpdate(m_kit);

    QWidget *widget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    widget->setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);

    QWidget *w = new QWidget;
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

    connect(m_detailsWidget, SIGNAL(checked(bool)),
            this, SLOT(targetCheckBoxToggled(bool)));

    connect(KitManager::instance(), SIGNAL(kitUpdated(ProjectExplorer::Kit*)),
            this, SLOT(handleKitUpdate(ProjectExplorer::Kit*)));

    connect(m_manageButton, SIGNAL(clicked()), this, SLOT(manageKit()));
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
    m_kit = 0;
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

    QCheckBox *checkbox = new QCheckBox;
    checkbox->setText(info->displayName);
    checkbox->setChecked(m_enabled.at(pos));
    checkbox->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    m_newBuildsLayout->addWidget(checkbox, pos * 2, 0);

    Utils::PathChooser *pathChooser = new Utils::PathChooser();
    pathChooser->setExpectedKind(Utils::PathChooser::Directory);
    pathChooser->setFileName(info->buildDirectory);
    pathChooser->setHistoryCompleter(QLatin1String("TargetSetup.BuildDir.History"));
    pathChooser->setReadOnly(isImport);
    m_newBuildsLayout->addWidget(pathChooser, pos * 2, 1);

    QLabel *reportIssuesLabel = new QLabel;
    reportIssuesLabel->setIndent(32);
    m_newBuildsLayout->addWidget(reportIssuesLabel, pos * 2 + 1, 0, 1, 2);
    reportIssuesLabel->setVisible(false);

    connect(checkbox, SIGNAL(toggled(bool)),
            this, SLOT(checkBoxToggled(bool)));

    connect(pathChooser, SIGNAL(rawPathChanged(QString)),
            this, SLOT(pathChanged()));

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
    QCheckBox *box = qobject_cast<QCheckBox *>(sender());
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
    Utils::PathChooser *pathChooser = qobject_cast<Utils::PathChooser *>(sender());
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
