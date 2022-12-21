// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <iwidgetplugin.h>

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

namespace QmlDesigner {

class ComponentsPlugin : public QObject, QmlDesigner::IWidgetPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QmlDesignerPlugin" FILE "componentsplugin.json")
    Q_DISABLE_COPY(ComponentsPlugin)
    Q_INTERFACES(QmlDesigner::IWidgetPlugin)
public:
    ComponentsPlugin();
    ~ComponentsPlugin() override = default;

    QString metaInfo() const override;
    QString pluginName() const override;

};

} // namespace QmlDesigner
