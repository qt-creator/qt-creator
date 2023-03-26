// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <nodeinstanceview.h>
#include <qmldesignerplugin.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace QmlDesigner {

class Edit3DViewConfig
{
public:
    static QList<QColor> loadColor(const char key[])
    {
        QVariant var = QmlDesignerPlugin::settings().value(key);

        if (!var.isValid())
            return {};

        auto colorNameList = var.value<QStringList>();

        return Utils::transform(colorNameList, [](const QString &colorName) {
            return QColor{colorName};
        });
    }

    static void setColors(AbstractView *view, View3DActionType type, const QList<QColor> &colorConfig)
    {
        setVariant(view, type, QVariant::fromValue(colorConfig));
    }

    template <typename T>
    static void set(AbstractView *view, View3DActionType type, const T &value)
    {
        setVariant(view, type, QVariant::fromValue(value));
    }

    static void saveColors(const QByteArray &key, const QList<QColor> &colorConfig)
    {
        QStringList colorNames = Utils::transform(colorConfig, [](const QColor &color) {
            return color.name();
        });

        saveVariant(key, QVariant::fromValue(colorNames));
    }

    static bool colorsValid(const QList<QColor> &colorConfig) { return !colorConfig.isEmpty(); }

private:
    static void setVariant(AbstractView *view, View3DActionType type, const QVariant &colorConfig)
    {
        view->emitView3DAction(type, colorConfig);
    }

    static void saveVariant(const QByteArray &key, const QVariant &colorConfig)
    {
        QmlDesignerPlugin::settings().insert(key, colorConfig);
    }
};

} // namespace QmlDesigner
