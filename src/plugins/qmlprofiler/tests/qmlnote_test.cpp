// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlnote_test.h"
#include <qmlprofiler/qmlnote.h>
#include <QtTest>

namespace QmlProfiler {
namespace Internal {

QmlNoteTest::QmlNoteTest(QObject *parent) : QObject(parent)
{
}

void QmlNoteTest::testAccessors()
{
    QmlNote note;
    QCOMPARE(note.typeIndex(), -1);
    QCOMPARE(note.startTime(), -1);
    QCOMPARE(note.duration(), 0);
    QCOMPARE(note.collapsedRow(), -1);
    QVERIFY(!note.loaded());
    QVERIFY(note.text().isEmpty());

    note.setText("blah");
    QCOMPARE(note.text(), QString("blah"));

    QmlNote note2(8, 5, 9, 10, "semmeln");
    QCOMPARE(note2.typeIndex(), 8);
    QCOMPARE(note2.startTime(), 9);
    QCOMPARE(note2.duration(), 10);
    QCOMPARE(note2.collapsedRow(), 5);
    QVERIFY(!note2.loaded());
    QCOMPARE(note2.text(), QString("semmeln"));

    note2.setLoaded(true);
    QVERIFY(note2.loaded());
}

void QmlNoteTest::testStreamOps()
{
    QmlNote note(4, 1, 5, 6, "eheheh");

    QBuffer wbuffer;
    wbuffer.open(QIODevice::WriteOnly);
    QDataStream wstream(&wbuffer);
    wstream << note;

    QBuffer rbuffer;
    rbuffer.setData(wbuffer.data());
    rbuffer.open(QIODevice::ReadOnly);
    QDataStream rstream(&rbuffer);

    QmlNote note2;
    QVERIFY(note != note2);
    rstream >> note2;

    QCOMPARE(note2, note);
}

} // namespace Internal
} // namespace QmlProfiler
