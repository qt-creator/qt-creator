// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QMetaType>
#include <QSize>

namespace QmlDesigner {

class ChangePreviewImageSizeCommand
{
public:
    ChangePreviewImageSizeCommand() = default;
    ChangePreviewImageSizeCommand(const QSize &size)
        : size(size)
    {}

    friend QDataStream &operator<<(QDataStream &out, const ChangePreviewImageSizeCommand &command);
    friend QDataStream &operator>>(QDataStream &in, ChangePreviewImageSizeCommand &command);
    friend QDebug operator<<(QDebug debug, const ChangePreviewImageSizeCommand &command);

public:
    QSize size;
};

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ChangePreviewImageSizeCommand)
