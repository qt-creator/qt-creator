/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise QML Live Preview Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

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
