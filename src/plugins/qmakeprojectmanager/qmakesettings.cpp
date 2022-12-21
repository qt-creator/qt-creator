// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmakesettings.h"
#include "qmakeprojectmanagertr.h"

#include <coreplugin/icore.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>

#include <QXmlStreamWriter>

using namespace Utils;

namespace QmakeProjectManager {
namespace Internal {

QmakeSettings::QmakeSettings()
{
    setAutoApply(false);

    registerAspect(&m_warnAgainstUnalignedBuildDir);
    m_warnAgainstUnalignedBuildDir.setSettingsKey("QmakeProjectManager/WarnAgainstUnalignedBuildDir");
    m_warnAgainstUnalignedBuildDir.setDefaultValue(HostOsInfo::isWindowsHost());
    m_warnAgainstUnalignedBuildDir.setLabelText(Tr::tr("Warn if a project's source and "
            "build directories are not at the same level"));
    m_warnAgainstUnalignedBuildDir.setToolTip(Tr::tr("Qmake has subtle bugs that "
            "can be triggered if source and build directory are not at the same level."));

    registerAspect(&m_alwaysRunQmake);
    m_alwaysRunQmake.setSettingsKey("QmakeProjectManager/AlwaysRunQmake");
    m_alwaysRunQmake.setLabelText(Tr::tr("Run qmake on every build"));
    m_alwaysRunQmake.setToolTip(Tr::tr("This option can help to prevent failures on "
            "incremental builds, but might slow them down unnecessarily in the general case."));

    registerAspect(&m_ignoreSystemFunction);
    m_ignoreSystemFunction.setSettingsKey("QmakeProjectManager/RunSystemFunction");
    m_ignoreSystemFunction.setLabelText(Tr::tr("Ignore qmake's system() function when parsing a project"));
    m_ignoreSystemFunction.setToolTip(Tr::tr("Checking this option avoids unwanted side effects, "
             "but may result in inexact parsing results."));
    // The settings value has been stored with the opposite meaning for a while.
    // Avoid changing the stored value, but flip it on read/write:
    const auto invertBoolVariant = [](const QVariant &v) { return QVariant(!v.toBool()); };
    m_ignoreSystemFunction.setFromSettingsTransformation(invertBoolVariant);
    m_ignoreSystemFunction.setToSettingsTransformation(invertBoolVariant);

    readSettings(Core::ICore::settings());
}

bool QmakeSettings::warnAgainstUnalignedBuildDir()
{
    return instance().m_warnAgainstUnalignedBuildDir.value();
}

bool QmakeSettings::alwaysRunQmake()
{
    return instance().m_alwaysRunQmake.value();
}

bool QmakeSettings::runSystemFunction()
{
    return !instance().m_ignoreSystemFunction.value(); // Note: negated.
}

QmakeSettings &QmakeSettings::instance()
{
    static QmakeSettings theSettings;
    return theSettings;
}

class SettingsWidget final : public Core::IOptionsPageWidget
{
public:
    SettingsWidget()
    {
        auto &s = QmakeSettings::instance();
        using namespace Layouting;
        Column {
            s.m_warnAgainstUnalignedBuildDir,
            s.m_alwaysRunQmake,
            s.m_ignoreSystemFunction,
            st
        }.attachTo(this);
    }

    void apply() final
    {
        auto &s = QmakeSettings::instance();
        if (s.isDirty()) {
            s.apply();
            s.writeSettings(Core::ICore::settings());
        }
    }
};

QmakeSettingsPage::QmakeSettingsPage()
{
    setId("K.QmakeProjectManager.QmakeSettings");
    setDisplayName(Tr::tr("Qmake"));
    setCategory(ProjectExplorer::Constants::BUILD_AND_RUN_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new SettingsWidget; });
}

} // namespace Internal
} // namespace QmakeProjectManager
