// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <iwidgetplugin.h>

QT_FORWARD_DECLARE_CLASS(QAction)

namespace QmlDesigner {

class StudioPlugin : public QObject, QmlDesigner::IWidgetPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QmlDesignerPlugin" FILE "studioplugin.json")

    Q_DISABLE_COPY(StudioPlugin)
    Q_INTERFACES(QmlDesigner::IWidgetPlugin)

public:
    StudioPlugin();
    ~StudioPlugin() override = default;

    QString metaInfo() const override;
    QString pluginName() const override;
};

} // namespace QmlDesigner
