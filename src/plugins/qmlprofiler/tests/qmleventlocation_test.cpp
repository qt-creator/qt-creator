// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
