// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#pragma once

#include "qmldesignerutils_global.h"

#include <QImage>
#include <QPixmap>
#include <QSize>
#include <QString>

namespace QmlDesigner {

class QMLDESIGNERUTILS_EXPORT KtxImage
{
public:
    KtxImage(const QString &fileName);

    QString fileName() const { return m_fileName; }
    QSize dimensions() const;

private:
    void loadKtx();

    QString m_fileName;
    QSize m_dim;
};

} // namespace QmlDesigner
