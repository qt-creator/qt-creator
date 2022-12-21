// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <iwidgetplugin.h>


namespace QmlDesigner {
class AssetExporter;
class AssetExporterView;
class AssetExporterPlugin : public QObject, QmlDesigner::IWidgetPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QmlDesignerPlugin" FILE "assetexporterplugin.json")

    Q_DISABLE_COPY(AssetExporterPlugin)
    Q_INTERFACES(QmlDesigner::IWidgetPlugin)

public:
    AssetExporterPlugin();

    QString metaInfo() const final;
    QString pluginName() const final;

private:
    void onExport();
    void addActions();
    void updateActions();

    AssetExporterView *m_view = nullptr;
};

} // namespace QmlDesigner
