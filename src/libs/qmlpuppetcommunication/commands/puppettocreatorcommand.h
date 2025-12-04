// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmetatype.h>
#include <QDataStream>
#include <QVariant>

namespace QmlDesigner {

class PuppetToCreatorCommand
{
public:
    enum Type {
        None,
        ActiveSceneChanged,
        ActiveViewportChanged,
        BakeLightsAborted,
        BakeLightsFinished,
        BakeLightsProgress,
        Edit3DMouseCursor,
        Edit3DToolState,
        Import3DPreviewIcon,
        Import3DPreviewImage,
        Import3DSupport,
        NodeAtPos,
        Render3DView,
        RenderModelNodePreviewImage
    };

    PuppetToCreatorCommand(Type type, const QVariant &data);
    PuppetToCreatorCommand() = default;

    Type type() const { return m_type; }
    QVariant data() const { return m_data; }

private:
    Type m_type = None;
    QVariant m_data;

    friend QDataStream &operator>>(QDataStream &in, PuppetToCreatorCommand &command);
};

QDataStream &operator<<(QDataStream &out, const PuppetToCreatorCommand &command);
QDataStream &operator>>(QDataStream &in, PuppetToCreatorCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::PuppetToCreatorCommand)
