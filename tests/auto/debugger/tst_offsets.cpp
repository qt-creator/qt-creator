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

#include <QtTest>

#ifdef HAS_BOOST
#include <boost/version.hpp>
#if BOOST_VERSION >= (1 * 100000 + 54 * 100)
#ifdef Q_CC_CLANG
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif
#include <boost/unordered/unordered_set.hpp>
#else
#undef HAS_BOOST
#endif
#endif

#include <private/qdatetime_p.h>
#include <private/qfile_p.h>
#include <private/qfileinfo_p.h>
#include <private/qobject_p.h>

// Based on http://bloglitb.blogspot.com/2011/12/access-to-private-members-safer.html

template<typename Class, typename Type, Type Value, typename Tag>
class Access
{
    friend int offset(Tag) {
        Class *c = 0;
        return (char*)&(c->*Value) - (char*)c; // UB. Don't try at home.
    }
};

#define OFFSET_ACCESS0(Name, Tag, Type, Class, Member) \
    struct Tag {}; \
    int offset(Tag); \
    template class Access<Class, Type Class::*, &Class::Member, Tag>; \
    const int Name = offset(Tag())

#define OFFSET_ACCESS(Type, Class, Member) \
    OFFSET_ACCESS0(Class ## _ ## Member, tag_ ## Class ## _ ## Member, Type, Class, Member)

#define OFFSET_TEST(Class, Member) \
        QTest::newRow(#Class "::" #Member) << Class ## _ ## Member


OFFSET_ACCESS(QString, QFilePrivate, fileName);
OFFSET_ACCESS(QString, QFileSystemEntry, m_filePath);
OFFSET_ACCESS(QFileSystemEntry, QFileInfoPrivate, fileEntry);
OFFSET_ACCESS(QObjectPrivate::ExtraData*, QObjectPrivate, extraData);

#if QT_VERSION < 0x50000
OFFSET_ACCESS(QString, QObjectPrivate, objectName);
#endif

#if QT_VERSION < 0x50200
OFFSET_ACCESS(QDate, QDateTimePrivate, date);
OFFSET_ACCESS(QTime, QDateTimePrivate, time);
OFFSET_ACCESS(Qt::TimeSpec, QDateTimePrivate, spec);
OFFSET_ACCESS(int, QDateTimePrivate, utcOffset);
#elif QT_VERSION < 0x50800
OFFSET_ACCESS(qint64, QDateTimePrivate, m_msecs);
OFFSET_ACCESS(Qt::TimeSpec, QDateTimePrivate, m_spec);
OFFSET_ACCESS(int, QDateTimePrivate, m_offsetFromUtc);
OFFSET_ACCESS(QTimeZone, QDateTimePrivate, m_timeZone);
OFFSET_ACCESS(QDateTimePrivate::StatusFlags, QDateTimePrivate, m_status);
#else
OFFSET_ACCESS(qint64, QDateTimePrivate, m_msecs);
OFFSET_ACCESS(int, QDateTimePrivate, m_offsetFromUtc);
OFFSET_ACCESS(QTimeZone, QDateTimePrivate, m_timeZone);
OFFSET_ACCESS(QDateTimePrivate::StatusFlags, QDateTimePrivate, m_status);
#endif


#ifdef HAS_BOOST
typedef boost::unordered::unordered_set<int>::key_type T_;
typedef boost::unordered::unordered_set<int>::hasher H_;
typedef boost::unordered::unordered_set<int>::key_equal P_;
typedef boost::unordered::unordered_set<int>::allocator_type A_;

typedef boost::unordered::detail::set<A_, T_, H_, P_> Uset_types;
typedef boost::unordered::detail::table_impl<Uset_types>::table UsetTable;
typedef boost::unordered_set<int> Uset;

OFFSET_ACCESS(Uset::table, Uset, table_);
OFFSET_ACCESS(std::size_t, UsetTable, size_);
OFFSET_ACCESS(std::size_t, UsetTable, bucket_count_);
OFFSET_ACCESS(UsetTable::bucket_pointer, UsetTable, buckets_);
#endif


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

    if (qtVersion >= 0x50600)
#ifdef Q_OS_WIN
#   ifdef Q_CC_MSVC
        OFFSET_TEST(QFilePrivate, fileName) << 184 << 248;
#   else // MinGW
        OFFSET_TEST(QFilePrivate, fileName) << 164 << 248;
#   endif
#else
        OFFSET_TEST(QFilePrivate, fileName) << 168 << 248;
#endif
    else if (qtVersion >= 0x50500)
#ifdef Q_OS_WIN
#   ifdef Q_CC_MSVC
        OFFSET_TEST(QFilePrivate, fileName) << 176 << 248;
#   else // MinGW
        OFFSET_TEST(QFilePrivate, fileName) << 164 << 248;
#   endif
#else
        OFFSET_TEST(QFilePrivate, fileName) << 164 << 248;
#endif
    else if (qtVersion >= 0x50400)
#ifdef Q_OS_WIN
#   ifdef Q_CC_MSVC
        OFFSET_TEST(QFilePrivate, fileName) << 196 << 272;
#   else // MinGW
        OFFSET_TEST(QFilePrivate, fileName) << 188 << 272;
#   endif
#else
        OFFSET_TEST(QFilePrivate, fileName) << 180 << 272;
#endif
    else if (qtVersion > 0x50200)
#ifdef Q_OS_WIN
#   ifdef Q_CC_MSVC
        OFFSET_TEST(QFilePrivate, fileName) << 184 << 272;
#   else // MinGW
        OFFSET_TEST(QFilePrivate, fileName) << 180 << 272;
#   endif
#else
        OFFSET_TEST(QFilePrivate, fileName) << 176 << 272;
#endif
    else if (qtVersion >= 0x50000)
        OFFSET_TEST(QFilePrivate, fileName) << 176 << 280;
    else
#ifdef Q_OS_WIN
        OFFSET_TEST(QFilePrivate, fileName) << 144 << 232;
#else
        OFFSET_TEST(QFilePrivate, fileName) << 140 << 232;
#endif

        OFFSET_TEST(QFileSystemEntry, m_filePath) << 0 << 0;
        OFFSET_TEST(QFileInfoPrivate, fileEntry) << 4 << 8;

        QTest::newRow("sizeof(QObjectData)") << int(sizeof(QObjectData))
            << 28 << 48; // vptr + 3 ptr + 2 int + ptr

    if (qtVersion >= 0x50000)
        OFFSET_TEST(QObjectPrivate, extraData) << 28 << 48; // sizeof(QObjectData)
    else
        OFFSET_TEST(QObjectPrivate, extraData) << 32 << 56; // sizeof(QObjectData) + 1 ptr

#if QT_VERSION < 0x50000
        OFFSET_TEST(QObjectPrivate, objectName) << 28 << 48;  // sizeof(QObjectData)
#endif

#if QT_VERSION < 0x50000
        OFFSET_TEST(QDateTimePrivate, date) << 4 << 4;
        OFFSET_TEST(QDateTimePrivate, time) << 8 << 8;
        OFFSET_TEST(QDateTimePrivate, spec) << 12 << 12;
        OFFSET_TEST(QDateTimePrivate, utcOffset) << 16 << 16;
#elif QT_VERSION < 0x50200
#   ifdef Q_OS_WIN
        OFFSET_TEST(QDateTimePrivate, date) << 8 << 8;
        OFFSET_TEST(QDateTimePrivate, time) << 16 << 16;
        OFFSET_TEST(QDateTimePrivate, spec) << 20 << 20;
        OFFSET_TEST(QDateTimePrivate, utcOffset) << 24 << 24;
#   else
        OFFSET_TEST(QDateTimePrivate, date) << 4 << 8;
        OFFSET_TEST(QDateTimePrivate, time) << 12 << 16;
        OFFSET_TEST(QDateTimePrivate, spec) << 16 << 20;
        OFFSET_TEST(QDateTimePrivate, utcOffset) << 20 << 24;
#   endif
#elif QT_VERSION < 0x50800
#   ifdef Q_OS_WIN
        OFFSET_TEST(QDateTimePrivate, m_msecs) << 8 << 8;
        OFFSET_TEST(QDateTimePrivate, m_spec) << 16 << 16;
        OFFSET_TEST(QDateTimePrivate, m_offsetFromUtc) << 20 << 20;
        OFFSET_TEST(QDateTimePrivate, m_timeZone) << 24 << 24;
        OFFSET_TEST(QDateTimePrivate, m_status) << 28 << 32;
#   else
        OFFSET_TEST(QDateTimePrivate, m_msecs) << 4 << 8;
        OFFSET_TEST(QDateTimePrivate, m_spec) << 12 << 16;
        OFFSET_TEST(QDateTimePrivate, m_offsetFromUtc) << 16 << 20;
        OFFSET_TEST(QDateTimePrivate, m_timeZone) << 20 << 24;
        OFFSET_TEST(QDateTimePrivate, m_status) << 24 << 32;
#   endif
#else
        OFFSET_TEST(QDateTimePrivate, m_msecs) << 0 << 0;
        OFFSET_TEST(QDateTimePrivate, m_status) << 8 << 8;
        OFFSET_TEST(QDateTimePrivate, m_offsetFromUtc) << 12 << 12;
        OFFSET_TEST(QDateTimePrivate, m_timeZone) << 20 << 24;
#endif

#ifdef HAS_BOOST
        OFFSET_TEST(Uset, table_) << 0 << 0;
        OFFSET_TEST(UsetTable, size_) << 8 << 16;
        OFFSET_TEST(UsetTable, bucket_count_) << 4 << 8;
        OFFSET_TEST(UsetTable, buckets_) << 20 << 40;
#endif
}


QTEST_APPLESS_MAIN(tst_offsets);

#include "tst_offsets.moc"

