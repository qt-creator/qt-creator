/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <QtTest>

// Don't do this at home. This is test code, not production.
#define protected public
#define private public

#include <private/qdatetime_p.h>
#include <private/qfile_p.h>
#include <private/qfileinfo_p.h>
#include <private/qobject_p.h>

class tst_offsets : public QObject
{
    Q_OBJECT

public:
    tst_offsets();

private slots:
    void offsets();
    void offsets_data();
};

tst_offsets::tst_offsets()
{
    qDebug("Qt Version   : %s", qVersion());
    qDebug("Pointer size : %d", int(sizeof(void *)));
}

void tst_offsets::offsets()
{
    QFETCH(int, actual);
    QFETCH(int, expected32);
    QFETCH(int, expected64);
    int expect = sizeof(void *) == 4 ? expected32 : expected64;
    QCOMPARE(actual, expect);
}

void tst_offsets::offsets_data()
{
    QTest::addColumn<int>("actual");
    QTest::addColumn<int>("expected32");
    QTest::addColumn<int>("expected64");

    const int qtVersion = QT_VERSION;

    {
        QFilePrivate *p = 0;
        QTestData &data = QTest::newRow("QFilePrivate::fileName")
                << int((char *)&p->fileName - (char *)p);
        if (qtVersion > 0x50200)
            data << 176 << 272;
        else if (qtVersion >= 0x50000)
            data << 176 << 280;
        else
            data << 140 << 232;
    }

    {
        QFileInfoPrivate *p = 0;
        QTestData &data = QTest::newRow("QFileInfoPrivate::filePath")
                << int((char *)&p->fileEntry.m_filePath - (char *)p);
        data << 4 << 8;
    }

    {
        QTestData &data = QTest::newRow("sizeof(QObjectData)")
                << int(sizeof(QObjectData));
        data << 28 << 48; // vptr + 3 ptr + 2 int + ptr
    }

    {
        QObjectPrivate *p = 0;
        QTestData &data = QTest::newRow("QObjectPrivate::extraData")
                << int((char *)&p->extraData - (char *)p);
        if (qtVersion >= 0x50000)
            data << 28 << 48;    // sizeof(QObjectData)
        else
            data << 32 << 56;    // sizeof(QObjectData) + 1 ptr
    }

#if QT_VERSION < 0x50000
    {
        QObjectPrivate *p = 0;
        QTestData &data = QTest::newRow("QObjectPrivate::objectName")
                << int((char *)&p->objectName - (char *)p);
        data << 28 << 48;    // sizeof(QObjectData)
    }
#endif

    {
        QDateTimePrivate *p = 0;
#if QT_VERSION < 0x50000
        QTest::newRow("QDateTimePrivate::date")
            << int((char *)&p->date - (char *)p) << 4 << 4;
        QTest::newRow("QDateTimePrivate::time")
            << int((char *)&p->time - (char *)p) << 8 << 8;
        QTest::newRow("QDateTimePrivate::spec")
            << int((char *)&p->spec - (char *)p) << 12 << 12;
        QTest::newRow("QDateTimePrivate::utcOffset")
            << int((char *)&p->utcOffset - (char *)p) << 16 << 16;
#elif QT_VERSION < 0x50200
        QTest::newRow("QDateTimePrivate::date")
            << int((char *)&p->date - (char *)p) << 4 << 8;
        QTest::newRow("QDateTimePrivate::time")
            << int((char *)&p->time - (char *)p) << 12 << 16;
        QTest::newRow("QDateTimePrivate::spec")
            << int((char *)&p->spec - (char *)p) << 16 << 20;
        QTest::newRow("QDateTimePrivate::utcOffset")
            << int((char *)&p->utcOffset - (char *)p) << 20 << 24;
#else
        QTest::newRow("QDateTimePrivate::m_msecs")
            << int((char *)&p->m_msecs - (char *)p) << 4 << 8;
        QTest::newRow("QDateTimePrivate::m_spec")
            << int((char *)&p->m_spec - (char *)p) << 12 << 16;
        QTest::newRow("QDateTimePrivate::m_offsetFromUtc")
            << int((char *)&p->m_offsetFromUtc - (char *)p) << 16 << 20;
        QTest::newRow("QDateTimePrivate::m_timeZone")
            << int((char *)&p->m_timeZone - (char *)p) << 20 << 24;
        QTest::newRow("QDateTimePrivate::m_status")
            << int((char *)&p->m_status - (char *)p) << 24 << 32;
#endif
    }

}


QTEST_APPLESS_MAIN(tst_offsets);

#include "tst_offsets.moc"

