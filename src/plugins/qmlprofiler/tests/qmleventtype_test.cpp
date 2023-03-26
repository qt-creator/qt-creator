// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
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
    QCOMPARE(type.message(), UndefinedMessage);
    QCOMPARE(type.rangeType(), UndefinedMessage);
    QCOMPARE(type.detailType(), -1);
    QVERIFY(!type.location().isValid());
    QVERIFY(type.data().isEmpty());
    QVERIFY(type.displayName().isEmpty());
    QCOMPARE(static_cast<ProfileFeature>(type.feature()), UndefinedProfileFeature);

    type.setLocation(QmlEventLocation("blah.js", 12, 13));
    QCOMPARE(type.location().filename(), QString("blah.js"));
    QCOMPARE(type.location().line(), 12);
    QCOMPARE(type.location().column(), 13);

    type.setData("dadada");
    QCOMPARE(type.data(), QString("dadada"));

    type.setDisplayName("disdis");
    QCOMPARE(type.displayName(), QString("disdis"));

    QmlEventType type2(UndefinedMessage, Javascript, 12, QmlEventLocation("lala.js", 2, 3), "nehhh",
                       "brbr");
    QCOMPARE(type2.message(), UndefinedMessage);
    QCOMPARE(type2.rangeType(), Javascript);
    QCOMPARE(type2.detailType(), 12);
    QCOMPARE(type2.location(), QmlEventLocation("lala.js", 2, 3));
    QCOMPARE(type2.data(), QString("nehhh"));
    QCOMPARE(type2.displayName(), QString("brbr"));
    QCOMPARE(static_cast<ProfileFeature>(type2.feature()), ProfileJavaScript);
}

void QmlEventTypeTest::testFeature()
{
    const quint8 features[][MaximumEventType] = {
        // Event
        {UndefinedProfileFeature, ProfileInputEvents, ProfileInputEvents,
         ProfileAnimations, UndefinedProfileFeature, UndefinedProfileFeature},
        // RangeStart
        {UndefinedProfileFeature, UndefinedProfileFeature, UndefinedProfileFeature,
         UndefinedProfileFeature, UndefinedProfileFeature, UndefinedProfileFeature},
        // RangeData
        {UndefinedProfileFeature, UndefinedProfileFeature, UndefinedProfileFeature,
         UndefinedProfileFeature, UndefinedProfileFeature, UndefinedProfileFeature},
        // RangeLocation
        {UndefinedProfileFeature, UndefinedProfileFeature, UndefinedProfileFeature,
         UndefinedProfileFeature, UndefinedProfileFeature, UndefinedProfileFeature},
        // RangeEnd
        {UndefinedProfileFeature, UndefinedProfileFeature, UndefinedProfileFeature,
         UndefinedProfileFeature, UndefinedProfileFeature, UndefinedProfileFeature},
        // Complete
        {UndefinedProfileFeature, UndefinedProfileFeature, UndefinedProfileFeature,
         UndefinedProfileFeature, UndefinedProfileFeature, UndefinedProfileFeature},
        // PixmapCacheEvent
        {ProfilePixmapCache, ProfilePixmapCache, ProfilePixmapCache,
         ProfilePixmapCache, ProfilePixmapCache, ProfilePixmapCache},
        // SceneGraphFrame
        {ProfileSceneGraph, ProfileSceneGraph, ProfileSceneGraph,
         ProfileSceneGraph, ProfileSceneGraph, ProfileSceneGraph},
        // MemoryAllocation
        {ProfileMemory, ProfileMemory, ProfileMemory,
         ProfileMemory, ProfileMemory, ProfileMemory},
        // DebugMessage
        {ProfileDebugMessages, ProfileDebugMessages, ProfileDebugMessages,
         ProfileDebugMessages, ProfileDebugMessages, ProfileDebugMessages},
        // ProfileQuick3D
        {ProfileQuick3D, ProfileQuick3D, ProfileQuick3D,
         ProfileQuick3D, ProfileQuick3D, ProfileQuick3D}
    };

    for (int i = 0; i < MaximumMessage; ++i) {
        for (int j = 0; j < MaximumEventType; ++j) {
            QmlEventType type(static_cast<Message>(i), UndefinedRangeType, j);
            QCOMPARE(type.feature(), features[i][j]);
        }
    }

    for (int i = 0; i < MaximumRangeType; ++i) {
        QmlEventType type(UndefinedMessage, static_cast<RangeType>(i));
        QCOMPARE(static_cast<ProfileFeature>(type.feature()),
                 featureFromRangeType(static_cast<RangeType>(i)));
    }
}

void QmlEventTypeTest::testStreamOps()
{
    QmlEventType type(UndefinedMessage, Javascript, -1, QmlEventLocation("socken.js", 12, 13),
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
    QVERIFY(type.rangeType() != type2.rangeType());

    rstream >> type2;

    QCOMPARE(type.feature(), type2.feature());
    QCOMPARE(type.message(), type2.message());
    QCOMPARE(type.rangeType(), type2.rangeType());
    QCOMPARE(type.detailType(), type2.detailType());
    QCOMPARE(type.location(), type2.location());
}

} // namespace Internal
} // namespace QmlProfiler
