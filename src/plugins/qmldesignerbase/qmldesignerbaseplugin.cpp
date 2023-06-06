// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmldesignerbaseplugin.h"

#include "studio/studiosettingspage.h"

#include "studio/studiostyle.h"
#include "utils/designersettings.h"

#include <coreplugin/icore.h>
#include <utils/uniqueobjectptr.h>

#include <QApplication>

namespace QmlDesigner {


class QmlDesignerBasePlugin::Data
{
public:
    DesignerSettings settings;
    StudioStyle *style = nullptr;
    std::unique_ptr<StudioConfigSettingsPage> studioConfigSettingsPage;

    Data()
        : settings(Core::ICore::settings())
    {}
};

namespace {
QmlDesignerBasePlugin *global;
}

QmlDesignerBasePlugin::QmlDesignerBasePlugin()
{
    global = this;
};

QmlDesignerBasePlugin::~QmlDesignerBasePlugin() = default;

DesignerSettings &QmlDesignerBasePlugin::settings()
{
    return global->d->settings;
}

QStyle *QmlDesignerBasePlugin::style()
{
    if (!global->d->style)
        global->d->style = new StudioStyle(QApplication::style());

    return global->d->style;
}

StudioConfigSettingsPage *QmlDesignerBasePlugin::studioConfigSettingsPage()
{
    return global->d->studioConfigSettingsPage.get();
}

bool QmlDesignerBasePlugin::initialize(const QStringList &, QString *)
{
    d = std::make_unique<Data>();
    if (Core::ICore::settings()->value("QML/Designer/StandAloneMode", false).toBool())
        d->studioConfigSettingsPage = std::make_unique<StudioConfigSettingsPage>();
    return true;
}

} // namespace QmlDesigner
