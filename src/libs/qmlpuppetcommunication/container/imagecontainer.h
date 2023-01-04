// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QImage>
#include <QMetaType>
#include <QRectF>

namespace QmlDesigner {

class ImageContainer
{
    friend QDataStream &operator>>(QDataStream &in, ImageContainer &container);
    friend bool operator==(const ImageContainer &first, const ImageContainer &second);
    friend bool operator<(const ImageContainer &first, const ImageContainer &second);

public:
    ImageContainer();
    ImageContainer(qint32 instanceId, const QImage &image, qint32 keyNumber);

    qint32 instanceId() const;
    QImage image() const;
    qint32 keyNumber() const;
    QRectF rect() const;

    void setImage(const QImage &image);
    void setRect(const QRectF &rectangle);

    static void removeSharedMemorys(const QVector<qint32> &keyNumberVector);

private:
    QImage m_image;
    qint32 m_instanceId;
    qint32 m_keyNumber;
    QRectF m_rect;
};

QDataStream &operator<<(QDataStream &out, const ImageContainer &container);
QDataStream &operator>>(QDataStream &in, ImageContainer &container);

bool operator==(const ImageContainer &first, const ImageContainer &second);
bool operator<(const ImageContainer &first, const ImageContainer &second);
QDebug operator<<(QDebug debug, const ImageContainer &container);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ImageContainer)
