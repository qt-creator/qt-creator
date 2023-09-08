// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <auxiliarydata.h>
#include <qmldesignerplugin.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace QmlDesigner {

class Edit3DViewConfig
{
public:
    static QColor loadColor(const char key[])
    {
        QVariant var = QmlDesignerPlugin::settings().value(key);

        if (!var.isValid())
            return {};

        return QColor{var.value<QString>()};
    }

    static QList<QColor> loadColors(const char key[])
    {
        QVariant var = QmlDesignerPlugin::settings().value(key);

        if (!var.isValid())
            return {};

        auto colorNameList = var.value<QStringList>();

        return Utils::transform(colorNameList, [](const QString &colorName) {
            return QColor{colorName};
        });
    }

    static QVariant load(const QByteArray &key, const QVariant &defaultValue = {})
    {
        return QmlDesignerPlugin::settings().value(key, defaultValue);
    }

    static void save(const QByteArray &key, const QVariant &value)
    {
        QmlDesignerPlugin::settings().insert(key, value);
    }

    static void setColors(AbstractView *view, const AuxiliaryDataKeyView &auxProp, const QList<QColor> &colorConfig)
    {
        QVariant param;
        if (auxProp.name == "edit3dGridColor")
            param = colorConfig.isEmpty() ? QColor() : colorConfig[0];
        else
            param = QVariant::fromValue(colorConfig);
        setVariant(view, auxProp, param);
    }

    template <typename T>
    static void set(AbstractView *view, const AuxiliaryDataKeyView &auxProp, const T &value)
    {
        setVariant(view, auxProp, QVariant::fromValue(value));
    }

    static void saveColors(const QByteArray &key, const QList<QColor> &colorConfig)
    {
        QStringList colorNames = Utils::transform(colorConfig, [](const QColor &color) {
            return color.name();
        });

        save(key, QVariant::fromValue(colorNames));
    }

    static bool colorsValid(const QList<QColor> &colorConfig) { return !colorConfig.isEmpty(); }

private:
    static void setVariant(AbstractView *view, const AuxiliaryDataKeyView &auxProp, const QVariant &value)
    {
        view->rootModelNode().setAuxiliaryData(auxProp, value);
    }
};

} // namespace QmlDesigner
