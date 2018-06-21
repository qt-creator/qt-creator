/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#pragma once

#include "parameters.h"
#include "qstringparser/qstringparser.h"

#include <QString>

#include <QPointF>
#include <QRectF>
#include <QDateTime>

#define QARK_BASIC_SAVELOAD(TYPE) \
    template<class Archive> \
    inline void save(Archive &archive, TYPE v, const Parameters &) \
    { \
        archive.write(v); \
    } \
    template<class Archive> \
    inline void load(Archive &archive, TYPE &v, const Parameters &) \
    { \
        archive.read(&v); \
    }

namespace qark {

QARK_BASIC_SAVELOAD(char)
QARK_BASIC_SAVELOAD(signed char)
QARK_BASIC_SAVELOAD(unsigned char)
QARK_BASIC_SAVELOAD(short int)
QARK_BASIC_SAVELOAD(unsigned short int)
QARK_BASIC_SAVELOAD(int)
QARK_BASIC_SAVELOAD(unsigned int)
QARK_BASIC_SAVELOAD(long int)
QARK_BASIC_SAVELOAD(unsigned long int)
QARK_BASIC_SAVELOAD(long long int)
QARK_BASIC_SAVELOAD(unsigned long long int)
QARK_BASIC_SAVELOAD(float)
QARK_BASIC_SAVELOAD(double)
QARK_BASIC_SAVELOAD(long double)
QARK_BASIC_SAVELOAD(bool)

QARK_BASIC_SAVELOAD(QString)

#undef QARK_BASIC_SAVELOAD

// QPointF

template<class Archive>
inline void save(Archive &archive, const QPointF &point, const Parameters &)
{
    archive.write(QString("x:%1;y:%2").arg(point.x()).arg(point.y()));
}

template<class Archive>
inline void load(Archive &archive, QPointF &point, const Parameters &)
{
    QString s;
    archive.read(&s);
    if (QStringParser(s).parse("x:%1;y:%2")
            .arg(point, &QPointF::setX).arg(point, &QPointF::setY).failed()) {
        throw typename Archive::FileFormatException();
    }
}

// QRectF

template<class Archive>
inline void save(Archive &archive, const QRectF &rect, const Parameters &)
{
    archive.write(QString("x:%1;y:%2;w:%3;h:%4")
                  .arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height()));
}

template<class Archive>
inline void load(Archive &archive, QRectF &point, const Parameters &)
{
    QString s;
    archive.read(&s);
    if (QStringParser(s).parse("x:%1;y:%2;w:%3;h:%4")
            .arg(point, &QRectF::setX).arg(point, &QRectF::setY)
            .arg(point, &QRectF::setWidth).arg(point, &QRectF::setHeight).failed()) {
        throw typename Archive::FileFormatException();
    }
}

// QDateTime

template<class Archive>
inline void save(Archive &archive, const QDateTime &dateTime, const Parameters &)
{
    archive << dateTime.toMSecsSinceEpoch();
}

template<class Archive>
inline void load(Archive &archive, QDateTime &dateTime, const Parameters &)
{
    qint64 t;
    archive >> t;
    dateTime = QDateTime::fromMSecsSinceEpoch(t);
}

} // namespace qark
