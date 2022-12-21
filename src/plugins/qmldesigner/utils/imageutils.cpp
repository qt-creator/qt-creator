// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "imageutils.h"

#include <QFile>
#include <QFileInfo>
#include <QImageReader>

namespace QmlDesigner {

QString QmlDesigner::ImageUtils::imageInfo(const QString &path)
{
    QFileInfo info(path);
    if (!info.exists())
        return {};

    int width = 0;
    int height = 0;
    if (info.suffix() == "hdr") {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return {};

        while (!file.atEnd()) {
            QByteArray line = file.readLine();
            if (sscanf(line.constData(), "-Y %d +X %d", &height, &width))
                break;
        }
    } else {
        QSize size = QImageReader(path).size();
        width = size.width();
        height = size.height();
    }

    if (width == 0 && height == 0)
        return {};

    return QLatin1String("%1 x %2\n%3 (%4)")
            .arg(QString::number(width),
                 QString::number(height),
                 QLocale::system().formattedDataSize(info.size(), 2, QLocale::DataSizeTraditionalFormat),
                 info.suffix());
}

} // namespace QmlDesigner
