// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "ktximage.h"

#include <QFile>
#include <QFileInfo>
#include <QDebug>

#include <cmath>

namespace QmlDesigner {

// Ktx images currently support only image metadata

static QByteArray fileToByteArray(QString const &filename)
{
    QFile file(filename);
    QFileInfo info(file);

    if (info.exists() && file.open(QFile::ReadOnly)) {
        // Read data until we have what we need
        // Content is:
        //    Byte[12] identifier
        //    UInt32 endianness
        //    UInt32
        //    UInt32
        //    UInt32
        //    Uint32
        //    Uint32
        //    UInt32 pixelWidth
        //    UInt32 pixelHeight
        //    ...
        return file.read(44);
    }

    return {};
}

KtxImage::KtxImage(const QString &fileName)
    : m_fileName(fileName)
{
    loadKtx();
}

QSize KtxImage::dimensions() const
{
    return m_dim;
}

void KtxImage::loadKtx()
{
    QByteArray buf(fileToByteArray(m_fileName));

    auto handleError = [this](const QString &error) {
        qWarning() << QStringLiteral("Failed to load KTX image '%1': %2.").arg(m_fileName, error).toUtf8();
    };

    if (buf.isEmpty()) {
        handleError("File open failed");
        return;
    }

    constexpr char ktxIdentifier[12] = {
        '\xAB', 'K',    'T',  'X',  ' ',    '1',
        '1',    '\xBB', '\r', '\n', '\x1A', '\n'
    };

    if (!buf.startsWith(ktxIdentifier)) {
        handleError("Non-KTX file");
        return;
    }

    if (buf.size() < 44) {
        handleError("Missing metadata");
        return;
    }

    quint32 w = 0;
    quint32 h = 0;

    if (*reinterpret_cast<const quint32 *>(buf.data() + 12) == 0x01020304) {
        // File endianness is different from our endianness
        QByteArray convBuf(4, 0);
        auto convertEndianness = [&convBuf, &buf](int idx) -> quint32 {
            for (int i = 0; i < 4; ++i)
                convBuf[i] = buf[idx + 3 - i];
            return *reinterpret_cast<const quint32 *>(convBuf.data());
        };
        w = convertEndianness(36);
        h = convertEndianness(40);
    } else {
        w = *reinterpret_cast<const quint32 *>(buf.data() + 36);
        h = *reinterpret_cast<const quint32 *>(buf.data() + 40);
    }

    m_dim = QSize(w, h);
}

} // namespace QmlDesigner
