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

#include "simplifytype.h"

#include <QString>
#include <QtTest>

using namespace Debugger;
using namespace Internal;

const char *description[] =
{
    "g++_stdstring",
    "g++_stdwstring",
    "g++_5stdstring",
    "g++_5stdwstring",
    "g++_stringmap",
    "g++_wstringmap",
    "g++_stringlist",
    "g++_stringset",
    "g++_stringvector",
    "g++_wstringvector",
    "g++_unordered_set",
    "g++_unordered_multiset",
    "g++_unordered_map",
    "g++_unordered_multimap",
    "g++_stdvector_int_ptr",
    "g++_stdmap_char_ptr",

    "libc++_stringvector",
    "libc++_unordered_map",
    "libc++_hash_node",

    "msvc_stdstring",
    "msvc_stdwstring",
    "msvc_stringmap",
    "msvc_wstringmap",
    "msvc_stringlist",
    "msvc_stringset",
    "msvc_stringvector",
    "msvc_wstringvector",

    "std_shared_ptr",

    "boost_shared_ptr",
    "boost_unordered_set",
};

const char *input[] =
{
// g++
"std::string",
"std::wstring",
"std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >",
"std::__cxx11::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >",
"std::map<std::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::less<std::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::pair<std::basic_string<char, std::char_traits<char>, std::allocator<char> > const, std::basic_string<char, std::char_traits<char>, std::allocator<char> > > > >",
"std::map<std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >, std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >, std::less<std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> > >, std::allocator<std::pair<std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> > const, std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> > > > >",
"std::list<std::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::basic_string<char, std::char_traits<char>, std::allocator<char> > > >",
"std::set<std::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::less<std::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::basic_string<char, std::char_traits<char>, std::allocator<char> > > >",
"std::vector<std::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::basic_string<char, std::char_traits<char>, std::allocator<char> > > >",
"std::vector<std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >, std::allocator<std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> > > >",

"std::unordered_set<int, std::hash<int>, std::equal_to<int>, std::allocator<int> >",
"std::unordered_multiset<int, std::hash<int>, std::equal_to<int>, std::allocator<int> >",
"std::unordered_map<int, int, std::hash<int>, std::equal_to<int>, std::allocator<std::pair<int const, int> > >",
"std::unordered_multimap<int, int, std::hash<int>, std::equal_to<int>, std::allocator<std::pair<int const, int> > >",

"std::vector<int *, std::allocator<int*> >",
"std::map<const char *, Foo, std::less<const char *>, std::allocator<std::pair<char const* const, Foo> > >",

// libc++
"std::__1::vector<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> >, std::__1::allocator<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> > > >",

"std::__1::unordered_map<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> >, float, std::__1::hash<char, std::__1::char_traits<char>, std::__1::allocator<char> >, std::__1::equal_to<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> > >, std::__1::allocator<std::__1::pair<const std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> >, float> > >",

"std::__1::__hash_node<int, void *>::value_type",

// MSVC
"class std::basic_string<char,std::char_traits<char>,std::allocator<char> >",
"class std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> >",
"class std::map<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,std::basic_string<char,std::char_traits<char>,std::allocator<char> > > > >",
"class std::map<std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> >,std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> >,std::less<std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> > >,std::allocator<std::pair<std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> > const ,std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> > > > >",
"class std::list<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >",
"class std::set<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >",
"class std::vector<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >",
"class std::vector<std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> >,std::allocator<std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> > > >",
// std
"std::shared_ptr<int>::element_type",
// boost
"boost::shared_ptr<int>::element_type",
"boost::unordered_set<int, boost::hash<int>, std::equal_to<int>, std::allocator<int> >",
};

const char *output[] =
{
    // Gcc
    "std::string",
    "std::wstring",
    "std::string",
    "std::wstring",
    "std::map<std::string, std::string>",
    "std::map<std::wstring, std::wstring>",
    "std::list<std::string>",
    "std::set<std::string>",
    "std::vector<std::string>",
    "std::vector<std::wstring>",
    "std::unordered_set<int>",
    "std::unordered_multiset<int>",
    "std::unordered_map<int, int>",
    "std::unordered_multimap<int, int>",
    "std::vector<int *>",
    "std::map<const char *, Foo>",
    // libc++
    "std::vector<std::string>",
    "std::unordered_map<std::string, float>",
    "int",
    // MSVC
    "std::string",
    "std::wstring",
    "std::map<std::string, std::string>",
    "std::map<std::wstring, std::wstring>",
    "std::list<std::string>",
    "std::set<std::string>",
    "std::vector<std::string>",
    "std::vector<std::wstring>",
    // std
    "int",
    // boost
    "int",
    "boost::unordered_set<int>",
};

class SimplifyTypesTest : public QObject
{
    Q_OBJECT

public:
    SimplifyTypesTest();

private Q_SLOTS:
    void test();
    void test_data();
};

SimplifyTypesTest::SimplifyTypesTest()
{
}

void SimplifyTypesTest::test()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);
    const QString output = simplifyType(input);
    QCOMPARE(output, expected);
}

void SimplifyTypesTest::test_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");
    const size_t count = sizeof(input)/sizeof(const char *);
    for (size_t i = 0; i < count; i++ )
        QTest::newRow(description[i]) << QString::fromLatin1(input[i])
                                      << QString::fromLatin1(output[i]);
}

QTEST_APPLESS_MAIN(SimplifyTypesTest);

#include "tst_simplifytypes.moc"
