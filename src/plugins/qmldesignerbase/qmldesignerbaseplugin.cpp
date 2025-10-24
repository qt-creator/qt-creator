// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmldesignerbaseplugin.h"

#include <designersettings.h>
#include <studioquickutils.h>
#include <studiovalidator.h>
#include <windowmanager.h>

#include <coreplugin/icore.h>
#include <utils/appinfo.h>
#include <utils/uniqueobjectptr.h>

#include <QApplication>

namespace QmlDesigner {

class QmlDesignerBasePlugin::Data
{
public:
    DesignerSettings settings;

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

void QmlDesignerBasePlugin::enableLiteMode()
{
    global->m_enableLiteMode = true;
}

bool QmlDesignerBasePlugin::isLiteModeEnabled()
{
    return global->m_enableLiteMode;
}

Utils::Result<> QmlDesignerBasePlugin::initialize(const QStringList &arguments)
{
    if (arguments.contains("-qml-lite-designer"))
        enableLiteMode();

    WindowManager::registerDeclarativeType();
    StudioQuickUtils::registerDeclarativeType();
    StudioIntValidator::registerDeclarativeType();
    StudioDoubleValidator::registerDeclarativeType();

    d = std::make_unique<Data>();

    return Utils::ResultOk;
}

} // namespace QmlDesigner
