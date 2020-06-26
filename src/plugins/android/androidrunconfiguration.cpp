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

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

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

BaseStringListAspect::BaseStringListAspect(const QString &settingsKey, Utils::Id id)
{
    setSettingsKey(settingsKey);
    setId(id);
}

BaseStringListAspect::~BaseStringListAspect() = default;

void BaseStringListAspect::addToLayout(LayoutBuilder &builder)
{
    QTC_CHECK(!m_widget);
    m_widget = new AdbCommandsWidget;
    m_widget->setCommandList(m_value);
    m_widget->setTitleText(m_label);
    builder.addItem(m_widget.data());
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


AndroidRunConfiguration::AndroidRunConfiguration(Target *target, Utils::Id id)
    : RunConfiguration(target, id)
{
    auto envAspect = addAspect<EnvironmentAspect>();
    envAspect->addSupportedBaseEnvironment(tr("Clean Environment"), {});

    addAspect<ArgumentsAspect>();

    auto amStartArgsAspect = addAspect<BaseStringAspect>();
    amStartArgsAspect->setId(Constants::ANDROID_AMSTARTARGS);
    amStartArgsAspect->setSettingsKey("Android.AmStartArgsKey");
    amStartArgsAspect->setLabelText(tr("Activity manager start options:"));
    amStartArgsAspect->setDisplayStyle(BaseStringAspect::LineEditDisplay);
    amStartArgsAspect->setHistoryCompleter("Android.AmStartArgs.History");

    auto warning = addAspect<BaseStringAspect>();
    warning->setDisplayStyle(BaseStringAspect::LabelDisplay);
    warning->setLabelPixmap(Icons::WARNING.pixmap());
    warning->setValue(tr("If the \"am start\" options conflict, the application might not start.\n"
                         "Qt Creator uses: am start -n <package_name>/<Activity_name> [-D]."));

    auto preStartShellCmdAspect = addAspect<BaseStringListAspect>();
    preStartShellCmdAspect->setId(Constants::ANDROID_PRESTARTSHELLCMDLIST);
    preStartShellCmdAspect->setSettingsKey("Android.PreStartShellCmdListKey");
    preStartShellCmdAspect->setLabel(tr("Shell commands to run on Android device before application launch."));

    auto postStartShellCmdAspect = addAspect<BaseStringListAspect>();
    postStartShellCmdAspect->setId(Constants::ANDROID_POSTFINISHSHELLCMDLIST);
    postStartShellCmdAspect->setSettingsKey("Android.PostStartShellCmdListKey");
    postStartShellCmdAspect->setLabel(tr("Shell commands to run on Android device after application quits."));

    setUpdater([this, target] {
        const BuildTargetInfo bti = buildTargetInfo();
        setDisplayName(bti.displayName);
        setDefaultDisplayName(bti.displayName);
        AndroidManager::updateGradleProperties(target, buildKey());
    });

    connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
}

} // namespace Android
