// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmldesignerbaseplugin.h"

#include "studio/studiosettingspage.h"

#include "studio/studiostyle.h"

#include <designersettings.h>

#include <coreplugin/icore.h>
#include <utils/appinfo.h>
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

const char experimentalFeatures[] = "QML/Designer/UseExperimentalFeatures";

QmlDesignerBasePlugin *global;
}

QmlDesignerBasePlugin::QmlDesignerBasePlugin()
{
    global = this;
}

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

bool QmlDesignerBasePlugin::experimentalFeaturesEnabled()
{
    return Core::ICore::settings()->value(experimentalFeaturesSettingsKey(), false).toBool();
}

QByteArray QmlDesignerBasePlugin::experimentalFeaturesSettingsKey()
{
    QString version = Utils::appInfo().displayVersion;
    version.remove('.');

    return QByteArray(experimentalFeatures) + version.toLatin1();
}

void QmlDesignerBasePlugin::enbableLiteMode()
{
    global->m_enableLiteMode = true;
}

bool QmlDesignerBasePlugin::isLiteModeEnabled()
{
    return global->m_enableLiteMode;
}

bool QmlDesignerBasePlugin::initialize(const QStringList &arguments, QString *)
{
    if (arguments.contains("-qml-lite-designer"))
        enbableLiteMode();

    d = std::make_unique<Data>();
    if (Core::ICore::settings()->value("QML/Designer/StandAloneMode", false).toBool())
        d->studioConfigSettingsPage = std::make_unique<StudioConfigSettingsPage>();
    return true;
}

} // namespace QmlDesigner
