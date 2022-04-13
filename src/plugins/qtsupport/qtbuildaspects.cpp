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

#include "qtbuildaspects.h"

#include "baseqtversion.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildpropertiessettings.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/kitmanager.h>

#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QLayout>

using namespace ProjectExplorer;
using namespace Utils;

namespace QtSupport {

QmlDebuggingAspect::QmlDebuggingAspect(BuildConfiguration *buildConfig)
    : m_buildConfig(buildConfig)
{
    setSettingsKey("EnableQmlDebugging");
    setDisplayName(tr("QML debugging and profiling:"));
    setValue(ProjectExplorerPlugin::buildPropertiesSettings().qmlDebugging.value());
}

void QmlDebuggingAspect::addToLayout(LayoutBuilder &builder)
{
    SelectionAspect::addToLayout(builder);
    const auto warningLabel = createSubWidget<InfoLabel>(QString(), InfoLabel::Warning);
    warningLabel->setElideMode(Qt::ElideNone);
    warningLabel->setVisible(false);
    builder.addRow({{}, warningLabel});
    const auto changeHandler = [this, warningLabel] {
        QString warningText;
        QTC_ASSERT(m_buildConfig, return);
        Kit *kit = m_buildConfig->kit();
        const bool supported = kit && QtVersion::isQmlDebuggingSupported(kit, &warningText);
        if (!supported) {
            setValue(TriState::Default);
        } else if (value() == TriState::Enabled) {
            warningText = tr("Might make your application vulnerable.<br/>"
                             "Only use in a safe environment.");
        }
        warningLabel->setText(warningText);
        setVisible(supported);
        const bool warningLabelsVisible = supported && !warningText.isEmpty();
        if (warningLabel->parentWidget())
            warningLabel->setVisible(warningLabelsVisible);
    };
    connect(KitManager::instance(), &KitManager::kitsChanged, warningLabel, changeHandler);
    connect(this, &QmlDebuggingAspect::changed, warningLabel, changeHandler);
    changeHandler();
}

QtQuickCompilerAspect::QtQuickCompilerAspect(BuildConfiguration *buildConfig)
    : m_buildConfig(buildConfig)
{
    setSettingsKey("QtQuickCompiler");
    setDisplayName(tr("Qt Quick Compiler:"));
    setValue(ProjectExplorerPlugin::buildPropertiesSettings().qtQuickCompiler.value());
}

void QtQuickCompilerAspect::addToLayout(LayoutBuilder &builder)
{
    SelectionAspect::addToLayout(builder);
    const auto warningLabel = createSubWidget<InfoLabel>(QString(), InfoLabel::Warning);
    warningLabel->setElideMode(Qt::ElideNone);
    warningLabel->setVisible(false);
    builder.addRow({{}, warningLabel});
    const auto changeHandler = [this, warningLabel] {
        QString warningText;
        QTC_ASSERT(m_buildConfig, return);
        Kit *kit = m_buildConfig->kit();
        const bool supported = kit
                && QtVersion::isQtQuickCompilerSupported(kit, &warningText);
        if (!supported)
            setValue(TriState::Default);
        if (value() == TriState::Enabled) {
            if (auto qmlDebuggingAspect = m_buildConfig->aspect<QmlDebuggingAspect>()) {
                if (qmlDebuggingAspect->value() == TriState::Enabled)
                    warningText = tr("Disables QML debugging. QML profiling will still work.");
            }
        }
        warningLabel->setText(warningText);
        setVisible(supported);
        const bool warningLabelsVisible = supported && !warningText.isEmpty();
        if (warningLabel->parentWidget())
            warningLabel->setVisible(warningLabelsVisible);
    };
    connect(KitManager::instance(), &KitManager::kitsChanged, warningLabel, changeHandler);
    connect(this, &QmlDebuggingAspect::changed, warningLabel, changeHandler);
    connect(this, &QtQuickCompilerAspect::changed, warningLabel, changeHandler);
    if (auto qmlDebuggingAspect = m_buildConfig->aspect<QmlDebuggingAspect>())
        connect(qmlDebuggingAspect, &QmlDebuggingAspect::changed,  warningLabel, changeHandler);
    changeHandler();
}

} // namespace QtSupport
