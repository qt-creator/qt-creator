// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

#define QMLDESIGNER_WIDGETPLUGIN_INTERFACE "com.Digia.QmlDesigner.IWidgetPlugin.v10"

namespace QmlDesigner {

class IWidgetPlugin
{
public:
    virtual ~IWidgetPlugin() = default;

    virtual QString metaInfo() const  = 0;
    virtual QString pluginName() const = 0;
};

} // namespace QmlDesigner

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(QmlDesigner::IWidgetPlugin, QMLDESIGNER_WIDGETPLUGIN_INTERFACE)
QT_END_NAMESPACE
