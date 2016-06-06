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
#include "qmleventtype_test.h"
#include <qmlprofiler/qmleventtype.h>

#include <QtTest>

namespace QmlProfiler {
namespace Internal {

QmlEventTypeTest::QmlEventTypeTest(QObject *parent) : QObject(parent)
{
}

void QmlEventTypeTest::testAccessors()
{
    QmlEventType type;
    QCOMPARE(type.message(), MaximumMessage);
    QCOMPARE(type.rangeType(), MaximumRangeType);
    QCOMPARE(type.detailType(), -1);
    QVERIFY(!type.location().isValid());
    QVERIFY(type.data().isEmpty());
    QVERIFY(type.displayName().isEmpty());
    QCOMPARE(type.feature(), MaximumProfileFeature);

    type.setLocation(QmlEventLocation("blah.js", 12, 13));
    QCOMPARE(type.location().filename(), QString("blah.js"));
    QCOMPARE(type.location().line(), 12);
    QCOMPARE(type.location().column(), 13);

    type.setData("dadada");
    QCOMPARE(type.data(), QString("dadada"));

    type.setDisplayName("disdis");
    QCOMPARE(type.displayName(), QString("disdis"));

    QmlEventType type2(MaximumMessage, Javascript, 12, QmlEventLocation("lala.js", 2, 3), "nehhh",
                       "brbr");
    QCOMPARE(type2.message(), MaximumMessage);
    QCOMPARE(type2.rangeType(), Javascript);
    QCOMPARE(type2.detailType(), 12);
    QCOMPARE(type2.location(), QmlEventLocation("lala.js", 2, 3));
    QCOMPARE(type2.data(), QString("nehhh"));
    QCOMPARE(type2.displayName(), QString("brbr"));
    QCOMPARE(type2.feature(), ProfileJavaScript);
}

void QmlEventTypeTest::testFeature()
{
    const ProfileFeature features[][MaximumEventType] = {
        // Event
        { MaximumProfileFeature, ProfileInputEvents, ProfileInputEvents,
          ProfileAnimations, MaximumProfileFeature, MaximumProfileFeature },
        // RangeStart
        { MaximumProfileFeature, MaximumProfileFeature, MaximumProfileFeature,
          MaximumProfileFeature, MaximumProfileFeature, MaximumProfileFeature },
        // RangeData
        { MaximumProfileFeature, MaximumProfileFeature, MaximumProfileFeature,
          MaximumProfileFeature, MaximumProfileFeature, MaximumProfileFeature },
        // RangeLocation
        { MaximumProfileFeature, MaximumProfileFeature, MaximumProfileFeature,
          MaximumProfileFeature, MaximumProfileFeature, MaximumProfileFeature },
        // RangeEnd
        { MaximumProfileFeature, MaximumProfileFeature, MaximumProfileFeature,
          MaximumProfileFeature, MaximumProfileFeature, MaximumProfileFeature },
        // Complete
        { MaximumProfileFeature, MaximumProfileFeature, MaximumProfileFeature,
          MaximumProfileFeature, MaximumProfileFeature, MaximumProfileFeature },
        // PixmapCacheEvent
        { ProfilePixmapCache, ProfilePixmapCache, ProfilePixmapCache,
          ProfilePixmapCache, ProfilePixmapCache, ProfilePixmapCache },
        // SceneGraphFrame
        { ProfileSceneGraph, ProfileSceneGraph, ProfileSceneGraph,
          ProfileSceneGraph, ProfileSceneGraph, ProfileSceneGraph },
        // MemoryAllocation
        { ProfileMemory, ProfileMemory, ProfileMemory,
          ProfileMemory, ProfileMemory, ProfileMemory },
        // DebugMessage
        { ProfileDebugMessages, ProfileDebugMessages, ProfileDebugMessages,
          ProfileDebugMessages, ProfileDebugMessages, ProfileDebugMessages }
    };

    for (int i = 0; i < MaximumMessage; ++i) {
        for (int j = 0; j < MaximumEventType; ++j) {
            QmlEventType type(static_cast<Message>(i), MaximumRangeType, j);
            QCOMPARE(type.feature(), features[i][j]);
        }
    }

    for (int i = 0; i < MaximumRangeType; ++i) {
        QmlEventType type(MaximumMessage, static_cast<RangeType>(i));
        QCOMPARE(type.feature(), featureFromRangeType(static_cast<RangeType>(i)));
    }
}

void QmlEventTypeTest::testStreamOps()
{
    QmlEventType type(MaximumMessage, Javascript, -1, QmlEventLocation("socken.js", 12, 13),
                      "lalala", "lelele");

    QBuffer wbuffer;
    wbuffer.open(QIODevice::WriteOnly);
    QDataStream wstream(&wbuffer);
    wstream << type;

    QBuffer rbuffer;
    rbuffer.setData(wbuffer.data());
    rbuffer.open(QIODevice::ReadOnly);
    QDataStream rstream(&rbuffer);

    QmlEventType type2;
    QVERIFY(type != type2);
    rstream >> type2;

    QCOMPARE(type2, type);
}

} // namespace Internal
} // namespace QmlProfiler
