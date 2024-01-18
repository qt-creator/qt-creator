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
    setLabelText(Tr::tr("Application id:"));
    //        setReadOnly(true);
}

AppManagerInstanceIdAspect::AppManagerInstanceIdAspect(Utils::AspectContainer *container)
    : StringAspect(container)
{
    setSettingsKey("ApplicationManagerPlugin.InstanceId");
    setDisplayStyle(StringAspect::LineEditDisplay);
    setLabelText(Tr::tr("AppMan instance id:"));

    makeCheckable(Utils::CheckBoxPlacement::Right, Tr::tr("Default Instance"),
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
    setLabelText(Tr::tr("Document url:"));
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
