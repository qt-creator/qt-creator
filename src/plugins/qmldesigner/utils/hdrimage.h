// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "qmldesignerutils_global.h"

#include <QImage>
#include <QPixmap>
#include <QString>

namespace QmlDesigner {

class QMLDESIGNERUTILS_EXPORT HdrImage
{
public:
    HdrImage(const QString &fileName);

    QString fileName() const { return m_fileName; }
    const QImage &image() const { return m_image; }
    QPixmap toPixmap() const;

private:
    void loadHdr();

    QImage m_image;
    QString m_fileName;
    QByteArray m_buf;
};

} // namespace QmlDesigner
