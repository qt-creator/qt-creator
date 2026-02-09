// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <auxiliarydata.h>
#include <qmldesignerplugin.h>

#include <qmldesigner/settings/designersettings.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace QmlDesigner {

class Edit3DViewConfig
{
public:

    static void setColors(AbstractView *view, const AuxiliaryDataKeyView &auxProp, const QList<QColor> &colorConfig)
    {
        QVariant param;
        if (auxProp.name == "edit3dGridColor")
            param = colorConfig.isEmpty() ? QColor() : colorConfig[0];
        else
            param = QVariant::fromValue(colorConfig);
        view->rootModelNode().setAuxiliaryData(auxProp, param);
    }

    template <typename T>
    static void set(AbstractView *view, const AuxiliaryDataKeyView &auxProp, const T &value)
    {
        view->rootModelNode().setAuxiliaryData(auxProp, value);
    }
};

} // namespace QmlDesigner
