/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "lldbengineguest.h"

#include <QLocalSocket>
#include <QCoreApplication>
#include <QSocketNotifier>
#include <QQueue>

#include <cstdio>

// #define DO_STDIO_DEBUG 1
#ifdef DO_STDIO_DEBUG
#define D_STDIO0(x) qDebug(x)
#define D_STDIO1(x,a1) qDebug(x,a1)
#define D_STDIO2(x,a1,a2) qDebug(x,a1,a2)
#define D_STDIO3(x,a1,a2,a3) qDebug(x,a1,a2,a3)
#else
#define D_STDIO0(x)
#define D_STDIO1(x,a1)
#define D_STDIO2(x,a1,a2)
#define D_STDIO3(x,a1,a2,a3)
#endif

class Stdio : public QIODevice
{
    Q_OBJECT
public:
    QSocketNotifier notify;
    Stdio()
        : QIODevice()
        , notify(fileno(stdin), QSocketNotifier::Read)
        , buckethead(0)
    {
        setvbuf(stdin , NULL , _IONBF , 0);
        setvbuf(stdout , NULL , _IONBF , 0);
        setOpenMode(QIODevice::ReadWrite | QIODevice::Unbuffered);
        connect(&notify, SIGNAL(activated(int)), this, SLOT(activated()));
    }
    virtual qint64 bytesAvailable () const
    {
        qint64 r = QIODevice::bytesAvailable();
        foreach (const QByteArray &bucket, buckets)
            r += bucket.size();
        r-= buckethead;
        return r;
    }

    virtual qint64 readData (char * data, qint64 maxSize)
    {
        D_STDIO1("readData %lli",maxSize);
        qint64 size = maxSize;
        while (size > 0) {
            if (!buckets.size()) {
                D_STDIO1("done prematurely with %lli", maxSize - size);
                return maxSize - size;
            }
            QByteArray &bucket = buckets.head();
            if ((size + buckethead) >= bucket.size()) {
                int d =  bucket.size() - buckethead;
                D_STDIO3("read (over bucket) d: %i buckethead: %i bucket.size(): %i",
                        d, buckethead,  bucket.size());
                memcpy(data, bucket.data() + buckethead, d);
                data += d;
                size -= d;
                buckets.dequeue();
                buckethead = 0;
            } else {
                D_STDIO1("read (in bucket) size: %lli", size);
                memcpy(data, bucket.data() + buckethead, size);
                data += size;
                buckethead += size;
                size = 0;
            }
        }
        D_STDIO1("done with %lli",(maxSize - size));
        return maxSize - size;
    }

    virtual qint64 writeData (const char * data, qint64 maxSize)
    {
        return ::write(fileno(stdout), data, maxSize);
    }

    QQueue<QByteArray> buckets;
    int buckethead;

private slots:
    void activated()
    {
        QByteArray a;
        a.resize(1000);
        int ret = ::read(fileno(stdin), a.data(), 1000);
        if (ret == 0)
            ::exit(0);
        assert(ret <= 1000);
        D_STDIO1("activated %i", ret);
        a.resize(ret);
        buckets.enqueue(a);
        emit readyRead();
    }
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    qDebug() << "guest engine operational";

    Debugger::Internal::LldbEngineGuest lldb;


    Stdio stdio;
    lldb.setHostDevice(&stdio);

    return app.exec();
}

extern "C" {
extern const unsigned char lldbVersionString[] __attribute__ ((used)) = "@(#)PROGRAM:lldb  PROJECT:lldb-26" "\n";
extern const double lldbVersionNumber __attribute__ ((used)) = (double)26.;
extern const double LLDBVersionNumber __attribute__ ((used)) = (double)26.;
}

#include "main.moc"
