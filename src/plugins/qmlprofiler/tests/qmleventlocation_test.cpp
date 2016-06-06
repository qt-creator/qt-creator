/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmleventlocation_test.h"

#include <qmlprofiler/qmleventlocation.h>
#include <QtTest>

namespace QmlProfiler {
namespace Internal {

QmlEventLocationTest::QmlEventLocationTest(QObject *parent) : QObject(parent)
{
}

void QmlEventLocationTest::testCtor()
{
    QmlEventLocation location;
    QVERIFY(!location.isValid());

    QmlEventLocation location2("blah.qml", 2, 3);
    QVERIFY(location2.isValid());
    QCOMPARE(location2.filename(), QString("blah.qml"));
    QCOMPARE(location2.line(), 2);
    QCOMPARE(location2.column(), 3);
}

void QmlEventLocationTest::testStreamOps()
{
    QmlEventLocation location("socken.js", 12, 13);

    QBuffer wbuffer;
    wbuffer.open(QIODevice::WriteOnly);
    QDataStream wstream(&wbuffer);
    wstream << location;

    QBuffer rbuffer;
    rbuffer.setData(wbuffer.data());
    rbuffer.open(QIODevice::ReadOnly);
    QDataStream rstream(&rbuffer);

    QmlEventLocation location2;
    QVERIFY(location != location2);
    rstream >> location2;

    QCOMPARE(location2, location);
}

} // namespace Internal
} // namespace QmlProfiler
