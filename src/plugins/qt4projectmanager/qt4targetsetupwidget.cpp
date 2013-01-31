/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qt4targetsetupwidget.h"

#include "buildconfigurationinfo.h"
#include "qt4buildconfiguration.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/kitoptionspage.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/detailsbutton.h>
#include <utils/detailswidget.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>

#include <QCheckBox>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

namespace Qt4ProjectManager {

// -------------------------------------------------------------------------
// Qt4TargetSetupWidget
// -------------------------------------------------------------------------

Qt4TargetSetupWidget::Qt4TargetSetupWidget(ProjectExplorer::Kit *k,
                                           const QString &proFilePath,
                                           const QList<BuildConfigurationInfo> &infoList) :
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
    m_manageButton = new QPushButton(tr("Manage..."));
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

    foreach (const BuildConfigurationInfo &info, infoList)
        addBuildConfigurationInfo(info);

    setProFilePath(proFilePath);

    connect(m_detailsWidget, SIGNAL(checked(bool)),
            this, SLOT(targetCheckBoxToggled(bool)));

    connect(ProjectExplorer::KitManager::instance(), SIGNAL(kitUpdated(ProjectExplorer::Kit*)),
            this, SLOT(handleKitUpdate(ProjectExplorer::Kit*)));

    connect(m_manageButton, SIGNAL(clicked()), this, SLOT(manageKit()));
}

Qt4TargetSetupWidget::~Qt4TargetSetupWidget()
{ }

ProjectExplorer::Kit *Qt4TargetSetupWidget::kit()
{
    return m_kit;
}

void Qt4TargetSetupWidget::clearKit()
{
    m_kit = 0;
}

bool Qt4TargetSetupWidget::isKitSelected() const
{
    if (!m_detailsWidget->isChecked())
        return false;

    return !selectedBuildConfigurationInfoList().isEmpty();
}

void Qt4TargetSetupWidget::setKitSelected(bool b)
{
    // Only check target if there are build configurations possible
    b &= !selectedBuildConfigurationInfoList().isEmpty();
    m_ignoreChange = true;
    m_detailsWidget->setChecked(b);
    m_detailsWidget->widget()->setEnabled(b);
    m_ignoreChange = false;

    m_detailsWidget->setState(b ? Utils::DetailsWidget::Expanded : Utils::DetailsWidget::Collapsed);
}

void Qt4TargetSetupWidget::addBuildConfigurationInfo(const BuildConfigurationInfo &info, bool importing)
{
    if (importing) {
        if (!m_haveImported) {
            // disable everything on first import
            for (int i = 0; i < m_enabled.count(); ++i) {
                m_enabled[i] = false;
                m_checkboxes[i]->setChecked(false);
            }
            m_selected = 0;
        }

        m_haveImported = true;
    }
    int pos = m_pathChoosers.count();
    m_enabled.append(true);
    ++m_selected;

    m_infoList.append(info);

    QCheckBox *checkbox = new QCheckBox;
    checkbox->setText(Qt4BuildConfigurationFactory::buildConfigurationDisplayName(info));
    checkbox->setChecked(m_enabled.at(pos));
    checkbox->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    m_newBuildsLayout->addWidget(checkbox, pos * 2, 0);

    Utils::PathChooser *pathChooser = new Utils::PathChooser();
    pathChooser->setExpectedKind(Utils::PathChooser::Directory);
    pathChooser->setPath(info.directory);
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(m_kit);
    if (!version)
        return;
    pathChooser->setReadOnly(!version->supportsShadowBuilds() || importing);
    m_newBuildsLayout->addWidget(pathChooser, pos * 2, 1);

    QLabel *reportIssuesLabel = new QLabel;
    reportIssuesLabel->setIndent(32);
    m_newBuildsLayout->addWidget(reportIssuesLabel, pos * 2 + 1, 0, 1, 2);
    reportIssuesLabel->setVisible(false);

    connect(checkbox, SIGNAL(toggled(bool)),
            this, SLOT(checkBoxToggled(bool)));

    connect(pathChooser, SIGNAL(changed(QString)),
            this, SLOT(pathChanged()));

    m_checkboxes.append(checkbox);
    m_pathChoosers.append(pathChooser);
    m_reportIssuesLabels.append(reportIssuesLabel);

    m_issues.append(false);
    reportIssues(pos);

    emit selectedToggled();
}

void Qt4TargetSetupWidget::targetCheckBoxToggled(bool b)
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
    }
    emit selectedToggled();
}

void Qt4TargetSetupWidget::manageKit()
{
    ProjectExplorer::KitOptionsPage *page =
            ExtensionSystem::PluginManager::instance()->getObject<ProjectExplorer::KitOptionsPage>();
    if (!page || !m_kit)
        return;

    page->showKit(m_kit);
    Core::ICore::showOptionsDialog(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY,
                                   ProjectExplorer::Constants::KITS_SETTINGS_PAGE_ID);
}

void Qt4TargetSetupWidget::setProFilePath(const QString &proFilePath)
{
    if (!m_kit)
        return;

    m_proFilePath = proFilePath;
    clear();

    QList<BuildConfigurationInfo> infoList
            = Qt4BuildConfigurationFactory::availableBuildConfigurations(m_kit, proFilePath);
    foreach (const BuildConfigurationInfo &info, infoList)
        addBuildConfigurationInfo(info);
}

void Qt4TargetSetupWidget::handleKitUpdate(ProjectExplorer::Kit *k)
{
    if (k != m_kit)
        return;

    m_detailsWidget->setIcon(k->icon());
    m_detailsWidget->setSummaryText(k->displayName());
}

QList<BuildConfigurationInfo> Qt4TargetSetupWidget::selectedBuildConfigurationInfoList() const
{
    QList<BuildConfigurationInfo> result;
    for (int i = 0; i < m_infoList.count(); ++i) {
        if (m_enabled.at(i))
            result.append(m_infoList.at(i));
    }
    return result;
}

QList<BuildConfigurationInfo> Qt4TargetSetupWidget::allBuildConfigurationInfoList() const
{
    return m_infoList;
}

void Qt4TargetSetupWidget::clear()
{
    qDeleteAll(m_checkboxes);
    m_checkboxes.clear();
    qDeleteAll(m_pathChoosers);
    m_pathChoosers.clear();
    qDeleteAll(m_reportIssuesLabels);
    m_reportIssuesLabels.clear();

    m_infoList.clear();
    m_issues.clear();
    m_enabled.clear();
    m_selected = 0;
    m_haveImported = false;

    emit selectedToggled();
}

void Qt4TargetSetupWidget::checkBoxToggled(bool b)
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

void Qt4TargetSetupWidget::pathChanged()
{
    if (m_ignoreChange)
        return;
    Utils::PathChooser *pathChooser = qobject_cast<Utils::PathChooser *>(sender());
    if (!pathChooser)
        return;
    int index = m_pathChoosers.indexOf(pathChooser);
    if (index == -1)
        return;
    m_infoList[index].directory = pathChooser->path();
    reportIssues(index);
}

void Qt4TargetSetupWidget::reportIssues(int index)
{
    QPair<ProjectExplorer::Task::TaskType, QString> issues = findIssues(m_infoList.at(index));
    QLabel *reportIssuesLabel = m_reportIssuesLabels.at(index);
    reportIssuesLabel->setText(issues.second);
    bool error = issues.first != ProjectExplorer::Task::Unknown;
    reportIssuesLabel->setVisible(error);
    m_issues[index] = error;
}

QPair<ProjectExplorer::Task::TaskType, QString> Qt4TargetSetupWidget::findIssues(const BuildConfigurationInfo &info)
{
    if (m_proFilePath.isEmpty())
        return qMakePair(ProjectExplorer::Task::Unknown, QString());

    QString buildDir = info.directory;
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(m_kit);
    if (!version)
        return qMakePair(ProjectExplorer::Task::Unknown, QString());

    QList<ProjectExplorer::Task> issues = version->reportIssues(m_proFilePath, buildDir);

    QString text;
    ProjectExplorer::Task::TaskType highestType = ProjectExplorer::Task::Unknown;
    foreach (const ProjectExplorer::Task &t, issues) {
        if (!text.isEmpty())
            text.append(QLatin1String("<br>"));
        // set severity:
        QString severity;
        if (t.type == ProjectExplorer::Task::Error) {
            highestType = ProjectExplorer::Task::Error;
            severity = tr("<b>Error:</b> ", "Severity is Task::Error");
        } else if (t.type == ProjectExplorer::Task::Warning) {
            if (highestType == ProjectExplorer::Task::Unknown)
                highestType = ProjectExplorer::Task::Warning;
            severity = tr("<b>Warning:</b> ", "Severity is Task::Warning");
        }
        text.append(severity + t.description);
    }
    if (!text.isEmpty())
        text = QLatin1String("<nobr>") + text;
    return qMakePair(highestType, text);
}

} // namespace Qt4ProjectManager
