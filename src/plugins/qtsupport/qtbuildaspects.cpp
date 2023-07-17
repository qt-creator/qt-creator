// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtbuildaspects.h"

#include "baseqtversion.h"
#include "qtsupporttr.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildpropertiessettings.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/kitmanager.h>

#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QCheckBox>

using namespace ProjectExplorer;
using namespace Utils;

namespace QtSupport {

QmlDebuggingAspect::QmlDebuggingAspect(AspectContainer *container)
    : TriStateAspect(container)
{
    setSettingsKey("EnableQmlDebugging");
    setDisplayName(Tr::tr("QML debugging and profiling:"));
    setValue(buildPropertiesSettings().qmlDebugging());
}

void QmlDebuggingAspect::addToLayout(Layouting::LayoutItem &parent)
{
    SelectionAspect::addToLayout(parent);
    const auto warningLabel = createSubWidget<InfoLabel>(QString(), InfoLabel::Warning);
    warningLabel->setElideMode(Qt::ElideNone);
    warningLabel->setVisible(false);
    parent.addRow({{}, warningLabel});
    const auto changeHandler = [this, warningLabel] {
        QString warningText;
        QTC_ASSERT(m_buildConfig, return);
        Kit *kit = m_buildConfig->kit();
        const bool supported = kit && QtVersion::isQmlDebuggingSupported(kit, &warningText);
        if (!supported) {
            setValue(TriState::Default);
        } else if (value() == TriState::Enabled) {
            warningText = Tr::tr("Might make your application vulnerable.<br/>"
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

void QmlDebuggingAspect::setBuildConfiguration(const BuildConfiguration *buildConfig)
{
    m_buildConfig = buildConfig;
}

QtQuickCompilerAspect::QtQuickCompilerAspect(AspectContainer *container)
    : TriStateAspect(container)
{
    setSettingsKey("QtQuickCompiler");
    setDisplayName(Tr::tr("Qt Quick Compiler:"));
    setValue(buildPropertiesSettings().qtQuickCompiler());
}

void QtQuickCompilerAspect::setBuildConfiguration(const BuildConfiguration *buildConfig)
{
    m_buildConfig = buildConfig;
}

void QtQuickCompilerAspect::addToLayout(Layouting::LayoutItem &parent)
{
    SelectionAspect::addToLayout(parent);
    const auto warningLabel = createSubWidget<InfoLabel>(QString(), InfoLabel::Warning);
    warningLabel->setElideMode(Qt::ElideNone);
    warningLabel->setVisible(false);
    parent.addRow({{}, warningLabel});
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
                    warningText = Tr::tr("Disables QML debugging. QML profiling will still work.");
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
