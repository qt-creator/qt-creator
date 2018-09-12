/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidrunconfiguration.h"

#include "androidconstants.h"
#include "androidglobal.h"
#include "androidtoolchain.h"
#include "androidmanager.h"
#include "adbcommandswidget.h"
#include "androidrunenvironmentaspect.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtoutputformatter.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/detailswidget.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/utilsicons.h>

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpacerItem>
#include <QWidget>

using namespace Android::Internal;
using namespace ProjectExplorer;
using namespace Utils;

namespace Android {

BaseStringListAspect::BaseStringListAspect(const QString &settingsKey, Core::Id id)
{
    setSettingsKey(settingsKey);
    setId(id);
}

BaseStringListAspect::~BaseStringListAspect() = default;

void BaseStringListAspect::addToConfigurationLayout(QFormLayout *layout)
{
    QTC_CHECK(!m_widget);
    m_widget = new AdbCommandsWidget(layout->parentWidget());
    m_widget->setCommandList(m_value);
    m_widget->setTitleText(m_label);
    layout->addRow(m_widget);
    connect(m_widget.data(), &AdbCommandsWidget::commandsChanged, this, [this] {
        m_value = m_widget->commandsList();
        emit changed();
    });
}

void BaseStringListAspect::fromMap(const QVariantMap &map)
{
    m_value = map.value(settingsKey()).toStringList();
}

void BaseStringListAspect::toMap(QVariantMap &data) const
{
    data.insert(settingsKey(), m_value);
}

QStringList BaseStringListAspect::value() const
{
    return m_value;
}

void BaseStringListAspect::setValue(const QStringList &value)
{
    m_value = value;
    if (m_widget)
        m_widget->setCommandList(m_value);
}

void BaseStringListAspect::setLabel(const QString &label)
{
    m_label = label;
}


AndroidRunConfiguration::AndroidRunConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
{
    addAspect<AndroidRunEnvironmentAspect>();
    addAspect<ArgumentsAspect>();

    auto amStartArgsAspect = addAspect<BaseStringAspect>();
    amStartArgsAspect->setId(Constants::ANDROID_AMSTARTARGS);
    amStartArgsAspect->setSettingsKey("Android.AmStartArgsKey");
    amStartArgsAspect->setLabelText(tr("Activity manager start options:"));
    amStartArgsAspect->setDisplayStyle(BaseStringAspect::LineEditDisplay);
    amStartArgsAspect->setHistoryCompleter("Android.AmStartArgs.History");

    auto warning = addAspect<BaseStringAspect>();
    warning->setLabelPixmap(Icons::WARNING.pixmap());
    warning->setValue(tr("If the \"am start\" options conflict, the application might not start."));

    auto preStartShellCmdAspect = addAspect<BaseStringListAspect>();
    preStartShellCmdAspect->setId(Constants::ANDROID_PRESTARTSHELLCMDLIST);
    preStartShellCmdAspect->setSettingsKey("Android.PreStartShellCmdListKey");
    preStartShellCmdAspect->setLabel(tr("Shell commands to run on Android device before application launch."));

    auto postStartShellCmdAspect = addAspect<BaseStringListAspect>();
    postStartShellCmdAspect->setId(Constants::ANDROID_POSTFINISHSHELLCMDLIST);
    postStartShellCmdAspect->setSettingsKey("Android.PostStartShellCmdListKey");
    postStartShellCmdAspect->setLabel(tr("Shell commands to run on Android device after application quits."));

    setOutputFormatter<QtSupport::QtOutputFormatter>();
    connect(target->project(), &Project::parsingFinished, this, [this] {
        updateTargetInformation();
    });
}

QWidget *AndroidRunConfiguration::createConfigurationWidget()
{
    auto wrapped = RunConfiguration::createConfigurationWidget();
    auto detailsWidget = qobject_cast<DetailsWidget *>(wrapped);
    QTC_ASSERT(detailsWidget, return wrapped);
    detailsWidget->setState(DetailsWidget::Expanded);
    detailsWidget->setSummaryText(tr("Android run settings"));
    return detailsWidget;
}

void AndroidRunConfiguration::updateTargetInformation()
{
    const BuildTargetInfo bti = buildTargetInfo();
    setDisplayName(bti.displayName);
    setDefaultDisplayName(bti.displayName);
}

QString AndroidRunConfiguration::disabledReason() const
{
    const BuildTargetInfo bti = buildTargetInfo();
    const QString projectFileName = bti.projectFilePath.toString();

    if (project()->isParsing())
        return tr("The project file \"%1\" is currently being parsed.").arg(projectFileName);

    if (!project()->hasParsingData()) {
        if (!bti.projectFilePath.exists())
            return tr("The project file \"%1\" does not exist.").arg(projectFileName);

        return tr("The project file \"%1\" could not be parsed.").arg(projectFileName);
    }

    return QString();
}

} // namespace Android
