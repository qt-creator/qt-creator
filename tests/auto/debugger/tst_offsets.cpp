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

#include <private/qfile_p.h>

class tst_offsets : public QObject
{
    Q_OBJECT

public:
    tst_offsets() {}

private slots:
    void offsets();
    void offsets_data();
};

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
        QTestData &data = QTest::newRow("File::fileName")
                << int((char *)&p->fileName - (char *)p);
        if (qtVersion >= 0x50200)
            data << 176 << 272;
        else if (qtVersion >= 0x50000)
            data << 180 << 280;
        else
            data << 140 << 232;
    }

}


QTEST_APPLESS_MAIN(tst_offsets);

#include "tst_offsets.moc"

