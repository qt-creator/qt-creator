// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef QUICK3D_MODULE

#include "qquick3darealight_p.h"
#include <QtQuick3D/private/qquick3dobject_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>

namespace QmlDesigner::Internal {

float QQuick3DAreaLight::width() const
{
    return m_width;
}

float QQuick3DAreaLight::height() const
{
    return m_height;
}

void QQuick3DAreaLight::setWidth(float width)
{
    m_width = width;
    emit widthChanged();
}

void QQuick3DAreaLight::setHeight(float height)
{
    m_height = height;
    emit heightChanged();
}

}

#endif
