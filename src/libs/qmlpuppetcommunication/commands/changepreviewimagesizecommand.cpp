// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "changepreviewimagesizecommand.h"

#include <QDebug>

namespace QmlDesigner {

QDataStream &operator<<(QDataStream &out, const ChangePreviewImageSizeCommand &command)
{
    return out << command.size;
}

QDataStream &operator>>(QDataStream &in, ChangePreviewImageSizeCommand &command)
{
    return in >> command.size;
}

QDebug operator<<(QDebug debug, const ChangePreviewImageSizeCommand &command)
{
    return debug.nospace() << "ChangePreviewImageSizeCommand(" << command.size << ")";
}

} // namespace QmlDesigner
