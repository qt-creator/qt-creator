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

#include "qmakesettings.h"

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
    m_warnAgainstUnalignedBuildDir.setLabelText(tr("Warn if a project's source and "
            "build directories are not at the same level"));
    m_warnAgainstUnalignedBuildDir.setToolTip(tr("Qmake has subtle bugs that "
            "can be triggered if source and build directory are not at the same level."));

    registerAspect(&m_alwaysRunQmake);
    m_alwaysRunQmake.setSettingsKey("QmakeProjectManager/AlwaysRunQmake");
    m_alwaysRunQmake.setLabelText(tr("Run qmake on every build"));
    m_alwaysRunQmake.setToolTip(tr("This option can help to prevent failures on "
            "incremental builds, but might slow them down unnecessarily in the general case."));

    registerAspect(&m_ignoreSystemFunction);
    m_ignoreSystemFunction.setSettingsKey("QmakeProjectManager/RunSystemFunction");
    m_ignoreSystemFunction.setLabelText(tr("Ignore qmake's system() function when parsing a project"));
    m_ignoreSystemFunction.setToolTip(tr("Checking this option avoids unwanted side effects, "
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
    Q_DECLARE_TR_FUNCTIONS(QmakeProjectManager::Internal::QmakeSettingsPage)

public:
    SettingsWidget()
    {
        auto &s = QmakeSettings::instance();
        using namespace Layouting;
        Column {
            s.m_warnAgainstUnalignedBuildDir,
            s.m_alwaysRunQmake,
            s.m_ignoreSystemFunction,
            Stretch()
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
    setDisplayName(SettingsWidget::tr("Qmake"));
    setCategory(ProjectExplorer::Constants::BUILD_AND_RUN_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new SettingsWidget; });
}

} // namespace Internal
} // namespace QmakeProjectManager
