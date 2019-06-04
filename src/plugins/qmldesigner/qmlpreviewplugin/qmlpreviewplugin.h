/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <iwidgetplugin.h>

#include <QMetaType>

QT_FORWARD_DECLARE_CLASS(QAction)

namespace QmlDesigner {

class QmlPreviewPlugin : public QObject, QmlDesigner::IWidgetPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QmlDesignerPlugin" FILE "qmlpreviewplugin.json")

    Q_DISABLE_COPY(QmlPreviewPlugin)
    Q_INTERFACES(QmlDesigner::IWidgetPlugin)

public:
    QmlPreviewPlugin();
    ~QmlPreviewPlugin() override = default;

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
