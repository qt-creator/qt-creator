// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <iwidgetplugin.h>

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

namespace QmlDesigner {

class QtQuickPlugin : public QObject, QmlDesigner::IWidgetPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QmlDesignerPlugin" FILE "qtquickplugin.json")
    Q_DISABLE_COPY(QtQuickPlugin)
    Q_INTERFACES(QmlDesigner::IWidgetPlugin)
public:
    QtQuickPlugin();
    ~QtQuickPlugin() override = default;

    QString metaInfo() const override;
    QString pluginName() const override;

};

} // namespace QmlDesigner
