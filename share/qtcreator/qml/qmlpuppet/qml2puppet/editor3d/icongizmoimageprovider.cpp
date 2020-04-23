/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "icongizmoimageprovider.h"

namespace QmlDesigner {
namespace Internal {

IconGizmoImageProvider::IconGizmoImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage IconGizmoImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(size)
    Q_UNUSED(requestedSize)

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
