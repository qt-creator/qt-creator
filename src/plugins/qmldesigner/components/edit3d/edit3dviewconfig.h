// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <view3dactioncommand.h>

#include <nodeinstanceview.h>
#include <qmldesignerplugin.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace QmlDesigner {

class Edit3DViewConfig
{
public:
    static QList<QColor> load(const char key[])
    {
        QVariant var = DesignerSettings::getValue(key);

        if (!var.isValid())
            return {};

        auto colorNameList = var.value<QStringList>();

        return Utils::transform(colorNameList, [](const QString &colorName) {
            return QColor{colorName};
        });
    }

    static void set(View3DActionCommand::Type type, const QList<QColor> &colorConfig)
    {
        if (colorConfig.size() == 1)
            set(type, colorConfig.at(0));
        else
            setVariant(type, QVariant::fromValue(colorConfig));
    }

    static void set(View3DActionCommand::Type type, const QColor &color)
    {
        setVariant(type, QVariant::fromValue(color));
    }

    static void save(const QByteArray &key, const QList<QColor> &colorConfig)
    {
        QStringList colorNames = Utils::transform(colorConfig, [](const QColor &color) {
            return color.name();
        });

        saveVariant(key, QVariant::fromValue(colorNames));
    }

    static void save(const QByteArray &key, const QColor &color)
    {
        saveVariant(key, QVariant::fromValue(color.name()));
    }

    static bool isValid(const QList<QColor> &colorConfig) { return !colorConfig.isEmpty(); }

private:
    static void setVariant(View3DActionCommand::Type type, const QVariant &colorConfig)
    {
        auto view = QmlDesignerPlugin::instance()->viewManager().nodeInstanceView();
        View3DActionCommand cmd(type, colorConfig);
        view->view3DAction(cmd);
    }

    static void saveVariant(const QByteArray &key, const QVariant &colorConfig)
    {
        DesignerSettings::setValue(key, colorConfig);
    }
};

} // namespace QmlDesigner
