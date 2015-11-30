/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef IMAGECONTAINER_H
#define IMAGECONTAINER_H

#include <QMetaType>
#include <QImage>

namespace QmlDesigner {

class ImageContainer
{
    friend QDataStream &operator>>(QDataStream &in, ImageContainer &container);
    friend bool operator ==(const ImageContainer &first, const ImageContainer &second);
    friend bool operator <(const ImageContainer &first, const ImageContainer &second);

public:
    ImageContainer();
    ImageContainer(qint32 instanceId, const QImage &image, qint32 keyNumber);

    qint32 instanceId() const;
    QImage image() const;
    qint32 keyNumber() const;

    void setImage(const QImage &image);

    static void removeSharedMemorys(const QVector<qint32> &keyNumberVector);

private:
    QImage m_image;
    qint32 m_instanceId;
    qint32 m_keyNumber;
};

QDataStream &operator<<(QDataStream &out, const ImageContainer &container);
QDataStream &operator>>(QDataStream &in, ImageContainer &container);

bool operator ==(const ImageContainer &first, const ImageContainer &second);
bool operator <(const ImageContainer &first, const ImageContainer &second);
QDebug operator <<(QDebug debug, const ImageContainer &container);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ImageContainer)

#endif // IMAGECONTAINER_H
