// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appmanagerstringaspect.h"

#include "appmanagertr.h"

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>

#include <QFileInfo>
#include <QLayout>
#include <QPushButton>

using namespace Utils;

namespace AppManager {
namespace Internal {

AppManagerIdAspect::AppManagerIdAspect(Utils::AspectContainer *container)
    : StringAspect(container)
{
    setSettingsKey("ApplicationManagerPlugin.ApplicationId");
    setDisplayStyle(StringAspect::LineEditDisplay);
    setLabelText(Tr::tr("Application ID:"));
    //        setReadOnly(true);
}

AppManagerInstanceIdAspect::AppManagerInstanceIdAspect(Utils::AspectContainer *container)
    : StringAspect(container)
{
    setSettingsKey("ApplicationManagerPlugin.InstanceId");
    setDisplayStyle(StringAspect::LineEditDisplay);
    setLabelText(Tr::tr("Application Manager instance ID:"));

    makeCheckable(Utils::CheckBoxPlacement::Right, Tr::tr("Default instance"),
                  "ApplicationManagerPlugin.InstanceIdDefault");
    setChecked(true);

    addDataExtractor(this, &AppManagerInstanceIdAspect::operator(), &Data::value);
}

QString AppManagerInstanceIdAspect::operator()() const
{
    return !isChecked() ? value() : QString();
}

AppManagerDocumentUrlAspect::AppManagerDocumentUrlAspect(Utils::AspectContainer *container)
    : StringAspect(container)
{
    setSettingsKey("ApplicationManagerPlugin.DocumentUrl");
    setDisplayStyle(StringAspect::LineEditDisplay);
    setLabelText(Tr::tr("Document URL:"));
}

AppManagerCustomizeAspect::AppManagerCustomizeAspect(Utils::AspectContainer *container)
    : BoolAspect(container)
{
    setSettingsKey("ApplicationManagerPlugin.CustomizeStep");
    setLabelText(Tr::tr("Customize step"));
    setToolTip(Tr::tr("Disables the automatic updates based on the current run configuration and "
                      "allows customizing the values."));
}

AppManagerRestartIfRunningAspect::AppManagerRestartIfRunningAspect(Utils::AspectContainer *container)
    : BoolAspect(container)
{
    setSettingsKey("ApplicationManagerPlugin.RestartIfRunning");
    setLabelText(Tr::tr("Restart if running:"));
    setToolTip(Tr::tr("Restarts the application in case it is already running."));
}

AppManagerControllerAspect::AppManagerControllerAspect(Utils::AspectContainer *container)
    : FilePathAspect(container)
{
    setSettingsKey("ApplicationManagerPlugin.AppControllerPath");
    setExpectedKind(Utils::PathChooser::ExistingCommand);
    setLabelText(Tr::tr("Controller:"));
}

AppManagerPackagerAspect::AppManagerPackagerAspect(Utils::AspectContainer *container)
    : FilePathAspect(container)
{
    setSettingsKey("ApplicationManagerPlugin.AppPackagerPath");
    setExpectedKind(Utils::PathChooser::ExistingCommand);
    setLabelText(Tr::tr("Packager:"));
}


} // namespace Internal
} // namespace AppManager
