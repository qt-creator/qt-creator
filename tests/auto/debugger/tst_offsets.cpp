// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

#include <QtGlobal>
#include <QLibraryInfo>

#include <private/qdatetime_p.h>
#include <private/qdir_p.h>
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

#if QT_VERSION >= 0x60000
OFFSET_ACCESS(QFileSystemEntry, QDirPrivate, dirEntry);

#if QT_VERSION < 0x60600
OFFSET_ACCESS(QStringList, QDirPrivate, files);
OFFSET_ACCESS(QFileInfoList, QDirPrivate, fileInfos);
OFFSET_ACCESS(QFileSystemEntry, QDirPrivate, absoluteDirEntry);
#else
using FileCache = QDirPrivate::FileCache;

OFFSET_ACCESS(QDirPrivate::FileCache, QDirPrivate, fileCache);
OFFSET_ACCESS(QStringList, FileCache, files);
OFFSET_ACCESS(QFileInfoList, FileCache, fileInfos);
OFFSET_ACCESS(QFileSystemEntry, FileCache, absoluteDirEntry);
#endif
#endif

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
#if BOOST_VERSION <= (1 * 100000 + 64 * 100)
typedef boost::unordered::detail::table_impl<Uset_types>::table UsetTable;
#else
typedef boost::unordered::detail::table<Uset_types> UsetTable;
#endif
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

QT_BEGIN_NAMESPACE
extern quintptr Q_CORE_EXPORT qtHookData[];
QT_END_NAMESPACE

void tst_offsets::offsets_data()
{
    QTest::addColumn<int>("actual");
    QTest::addColumn<int>("expected32");
    QTest::addColumn<int>("expected64");

    const int qtVersion = QT_VERSION;
    const quintptr qtTypeVersion = qtHookData[6];

    if (qtTypeVersion >= 22)
#ifdef Q_OS_WIN
#   ifdef Q_CC_MSVC
        OFFSET_TEST(QFilePrivate, fileName) << 0 << 424;
#   else // MinGW
        OFFSET_TEST(QFilePrivate, fileName) << 0 << 424;
#   endif
#else
        OFFSET_TEST(QFilePrivate, fileName) << 300 << 424;
#endif
    else if (qtTypeVersion >= 20)
#ifdef Q_OS_WIN
#   ifdef Q_CC_MSVC
        OFFSET_TEST(QFilePrivate, fileName) << 0 << 304;
#   else // MinGW
        OFFSET_TEST(QFilePrivate, fileName) << 0 << 304;
#   endif
#else
        OFFSET_TEST(QFilePrivate, fileName) << 196 << 304;
#endif
    else if (qtVersion > 0x50600 && qtTypeVersion >= 17)
#ifdef Q_OS_WIN
#   ifdef Q_CC_MSVC
        OFFSET_TEST(QFilePrivate, fileName) << 164 << 224;
#   else // MinGW
        OFFSET_TEST(QFilePrivate, fileName) << 160 << 224;
#   endif
#else
        OFFSET_TEST(QFilePrivate, fileName) << 156 << 224;
#endif
    else if (qtVersion >= 0x50700)
#ifdef Q_OS_WIN
#   ifdef Q_CC_MSVC
        OFFSET_TEST(QFilePrivate, fileName) << 176 << 248;
#   else // MinGW
        OFFSET_TEST(QFilePrivate, fileName) << 172 << 248;
#   endif
#else
        OFFSET_TEST(QFilePrivate, fileName) << 168 << 248;
#endif
    else if (qtVersion >= 0x50600)
#ifdef Q_OS_WIN
#   ifdef Q_CC_MSVC
        OFFSET_TEST(QFilePrivate, fileName) << 184 << 248;
#   else // MinGW
        OFFSET_TEST(QFilePrivate, fileName) << 180 << 248;
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

    // Qt5: vptr + 3 ptr + 2 int + ptr
    // Qt6: vptr + objectlist + 8 unit:1 + uint:24 + int + ptr + bindingstorage (+ ptr)
    int size32 = qtVersion >= 0x60000 ? 56 : 28;
    int size64 = qtVersion >= 0x60000 ? 72 : 48;
    if (qtTypeVersion >= 21) { // the additional ptr was introduced with qtTypeVersion 21
        size32 += 4;
        size64 += 8;
    }
    QTest::newRow("sizeof(QObjectData)") << int(sizeof(QObjectData)) << size32 << size64;

    if (qtVersion >= 0x50000)
        OFFSET_TEST(QObjectPrivate, extraData) << size32 << size64; // sizeof(QObjectData)
    else
        OFFSET_TEST(QObjectPrivate, extraData) << size32 + 4 << size64 + 8; // sizeof(QObjectData) + 1 ptr

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
#elif QT_VERSION < 0x50e00
        OFFSET_TEST(QDateTimePrivate, m_msecs) << 0 << 0;
        OFFSET_TEST(QDateTimePrivate, m_status) << 8 << 8;
        OFFSET_TEST(QDateTimePrivate, m_offsetFromUtc) << 12 << 12;
        OFFSET_TEST(QDateTimePrivate, m_timeZone) << 20 << 24;
#else
        OFFSET_TEST(QDateTimePrivate, m_msecs) << 8 << 8;
        OFFSET_TEST(QDateTimePrivate, m_status) << 4 << 4;
        OFFSET_TEST(QDateTimePrivate, m_offsetFromUtc) << 16 << 16;
        OFFSET_TEST(QDateTimePrivate, m_timeZone) << 20 << 24;
#endif

#if QT_VERSION >= 0x60000
#if QT_VERSION < 0x60600
        OFFSET_TEST(QDirPrivate, dirEntry) << 40 << 96;
        OFFSET_TEST(QDirPrivate, files) << 4 << 8;
        OFFSET_TEST(QDirPrivate, fileInfos) << 16 << 32;
        OFFSET_TEST(QDirPrivate, absoluteDirEntry) << 72 << 152;
#else
        OFFSET_TEST(QDirPrivate, fileCache) << 52 << 104;
        OFFSET_TEST(QDirPrivate, dirEntry) << 24 << 48;
        OFFSET_TEST(FileCache, files) << 4 << 8;
        OFFSET_TEST(FileCache, fileInfos) << 16 << 32;
        OFFSET_TEST(FileCache, absoluteDirEntry) << 32 << 64;
#endif
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

