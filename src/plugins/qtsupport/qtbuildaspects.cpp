// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtbuildaspects.h"

#include "baseqtversion.h"
#include "qtkitaspect.h"
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

QmlDebuggingAspect::QmlDebuggingAspect(BuildConfiguration *buildConfig)
    : TriStateAspect(buildConfig)
{
    setSettingsKey("EnableQmlDebugging");
    setDisplayName(Tr::tr("QML debugging and profiling:"));
    setValue(buildPropertiesSettings().qmlDebugging());
}

void QmlDebuggingAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    SelectionAspect::addToLayoutImpl(parent);
    Utils::InfoLabel *warningLabel = createSubWidget<InfoLabel>(QString(), InfoLabel::Warning);
    warningLabel->setElideMode(Qt::ElideNone);
    parent.addRow({Layouting::empty, warningLabel});
    const auto changeHandler = [this, warningLabel = QPointer<InfoLabel>(warningLabel)] {
        QTC_ASSERT(warningLabel, return);
        QString warningText;
        BuildConfiguration *buildConfig = qobject_cast<BuildConfiguration *>(container());
        QTC_ASSERT(buildConfig, return);
        QTC_ASSERT(buildConfig->target(), return);
        Kit *kit = buildConfig->kit();
        QTC_ASSERT(kit, return);
        const bool supported = QtVersion::isQmlDebuggingSupported(kit, &warningText);
        if (!supported) {
            setValue(TriState::Default);
        } else if (value() == TriState::Enabled) {
            warningText = Tr::tr("Might make your application vulnerable.<br/>"
                                 "Only use in a safe environment.");
        }
        warningLabel->setText(warningText);
        setVisible(supported);
        const bool warningLabelsVisible = supported && !warningText.isEmpty();
        // avoid explicitly showing the widget when it doesn't have a parent, but always
        // explicitly hide it when necessary
        if (warningLabel->parentWidget() || !warningLabelsVisible)
            warningLabel->setVisible(warningLabelsVisible);
    };
    connect(KitManager::instance(), &KitManager::kitsChanged, this, changeHandler);
    connect(this, &QmlDebuggingAspect::changed, this, changeHandler);
    changeHandler();
}

QtQuickCompilerAspect::QtQuickCompilerAspect(BuildConfiguration *buildConfig)
    : TriStateAspect(buildConfig)
{
    setSettingsKey("QtQuickCompiler");
    setDisplayName(Tr::tr("Qt Quick Compiler:"));
    setValue(buildPropertiesSettings().qtQuickCompiler());
}

void QtQuickCompilerAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    SelectionAspect::addToLayoutImpl(parent);
    const auto warningLabel = createSubWidget<InfoLabel>(QString(), InfoLabel::Warning);
    warningLabel->setElideMode(Qt::ElideNone);
    warningLabel->setVisible(false);
    parent.addRow({Layouting::empty, warningLabel});
    const auto changeHandler = [this, warningLabel = QPointer<InfoLabel>(warningLabel)] {
        QTC_ASSERT(warningLabel, return);
        QString warningText;
        BuildConfiguration *buildConfig = qobject_cast<BuildConfiguration *>(container());
        QTC_ASSERT(buildConfig, return);
        QTC_ASSERT(buildConfig->target(), return);
        Kit *kit = buildConfig->kit();
        QTC_ASSERT(kit, return);
        const bool supported = QtVersion::isQtQuickCompilerSupported(kit, &warningText);
        if (!supported)
            setValue(TriState::Default);
        if (value() == TriState::Enabled) {
            if (auto qmlDebuggingAspect = buildConfig->aspect<QmlDebuggingAspect>()) {
                if (qmlDebuggingAspect->value() == TriState::Enabled) {
                    if (QtVersion *qtVersion = QtKitAspect::qtVersion(kit)) {
                        if (qtVersion->qtVersion() < QVersionNumber(6, 0, 0))
                            warningText = Tr::tr("Disables QML debugging. QML profiling will still work.");
                    }
                }
            }
        }
        warningLabel->setText(warningText);
        setVisible(supported);
        const bool warningLabelsVisible = supported && !warningText.isEmpty();
        if (warningLabel->parentWidget())
            warningLabel->setVisible(warningLabelsVisible);
    };
    connect(KitManager::instance(), &KitManager::kitsChanged, this, changeHandler);
    connect(this, &QmlDebuggingAspect::changed, this, changeHandler);
    connect(this, &QtQuickCompilerAspect::changed, this, changeHandler);

    BuildConfiguration *buildConfig = qobject_cast<BuildConfiguration *>(container());
    if (auto qmlDebuggingAspect = buildConfig->aspect<QmlDebuggingAspect>())
        connect(qmlDebuggingAspect, &QmlDebuggingAspect::changed,  warningLabel, changeHandler);
    changeHandler();
}

} // namespace QtSupport
