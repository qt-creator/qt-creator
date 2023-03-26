// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <iwidgetplugin.h>

#include <QMetaType>

QT_FORWARD_DECLARE_CLASS(QAction)

namespace QmlDesigner {

class QmlPreviewWidgetPlugin : public QObject, QmlDesigner::IWidgetPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QmlDesignerPlugin" FILE "qmlpreviewplugin.json")

    Q_DISABLE_COPY(QmlPreviewWidgetPlugin)
    Q_INTERFACES(QmlDesigner::IWidgetPlugin)

public:
    QmlPreviewWidgetPlugin();
    ~QmlPreviewWidgetPlugin() override = default;

    QString metaInfo() const override;
    QString pluginName() const override;

    static void stopAllRunControls();
    static void setQmlFile();
    static QObject *getPreviewPlugin();

    static float zoomFactor();
    static void setZoomFactor(float zoomFactor);
    static void setLanguageLocale(const QString &locale);
signals:
    void fpsChanged(quint16 frames);

private slots:
    void handleRunningPreviews();

private:
    QAction *m_previewToggleAction = nullptr;
};

} // namespace QmlDesigner
