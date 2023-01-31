// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmldesignerbaseplugin.h"

#include "utils/designersettings.h"

#include <coreplugin/icore.h>

namespace QmlDesigner {

class QmlDesignerBasePlugin::Data
{
public:
    DesignerSettings settings{Core::ICore::instance()->settings()};
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

bool QmlDesignerBasePlugin::initialize(const QStringList &, QString *)
{
    d = std::make_unique<Data>();

    return true;
}

} // namespace QmlDesigner
