// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "icongizmoimageprovider.h"

namespace QmlDesigner {
namespace Internal {

IconGizmoImageProvider::IconGizmoImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage IconGizmoImageProvider::requestImage(const QString &id,
                                            [[maybe_unused]] QSize *size,
                                            [[maybe_unused]] const QSize &requestedSize)
{
    // id format: <file name>:<color name>
    QStringList parts = id.split(':');
    if (parts.size() == 2) {
        QImage image(QStringLiteral("://qtquickplugin/mockfiles/images/%1").arg(parts[0]));

        // Recolorize non-transparent image pixels
        QColor targetColor(parts[1]);
        int r = targetColor.red();
        int g = targetColor.green();
        int b = targetColor.blue();
        int size = image.sizeInBytes();
        uchar *byte = image.bits();
        for (int i = 0; i < size; i += 4) {
            // Skip if alpha is zero
            if (*(byte + 3) != 0) {
                // Average between target color and current color
                *byte = uchar((int(*byte) + b) / 2);
                ++byte;
                *byte = uchar((int(*byte) + g) / 2);
                ++byte;
                *byte = uchar((int(*byte) + r) / 2);
                ++byte;
                // Preserve alpha
                ++byte;
            } else {
                byte += 4;
            }
        }
        return image;
    } else {
        return {};
    }
}

}
}
