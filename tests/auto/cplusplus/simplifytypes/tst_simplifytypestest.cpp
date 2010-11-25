/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include <cplusplus/CppRewriter.h>

#include <QtCore/QString>
#include <QtTest/QtTest>

const char *description[] =
{
    "g++_stdstring",
    "g++_stdwstring",
    "g++_stringmap",
    "g++_wstringmap",
    "g++_stringlist",
    "g++_stringset",
    "g++_stringvector",
    "g++_wstringvector",
    "msvc_stdstring",
    "msvc_stdwstring",
    "msvc_stringmap",
    "msvc_wstringmap",
    "msvc_stringlist",
    "msvc_stringset",
    "msvc_stringvector",
    "msvc_wstringvector",
};

const char *input[] =
{
// g++
"std::string",
"std::wstring",
"std::map<std::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::less<std::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::pair<std::basic_string<char, std::char_traits<char>, std::allocator<char> > const, std::basic_string<char, std::char_traits<char>, std::allocator<char> > > > >",
"std::map<std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >, std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >, std::less<std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> > >, std::allocator<std::pair<std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> > const, std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> > > > >",
"std::list<std::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::basic_string<char, std::char_traits<char>, std::allocator<char> > > >",
"std::set<std::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::less<std::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::basic_string<char, std::char_traits<char>, std::allocator<char> > > >",
"std::vector<std::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::basic_string<char, std::char_traits<char>, std::allocator<char> > > >",
"std::vector<std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >, std::allocator<std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> > > >",
// MSVC
"class std::basic_string<char,std::char_traits<char>,std::allocator<char> >",
"class std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> >",
"class std::map<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,std::basic_string<char,std::char_traits<char>,std::allocator<char> > > > >",
"class std::map<std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> >,std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> >,std::less<std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> > >,std::allocator<std::pair<std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> > const ,std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> > > > >",
"class std::list<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >",
"class std::set<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >",
"class std::vector<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >",
"class std::vector<std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> >,std::allocator<std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> > > >"
};

const char *output[] =
{
    "std::string",
    "std::wstring",
    "std::map<std::string, std::string>",
    "std::map<std::wstring, std::wstring>",
    "std::list<std::string>",
    "std::set<std::string>",
    "std::vector<std::string>",
    "std::vector<std::wstring>",
    "std::string",
    "std::wstring",
    "std::map<std::string, std::string>",
    "std::map<std::wstring, std::wstring>",
    "std::list<std::string>",
    "std::set<std::string>",
    "std::vector<std::string>",
    "std::vector<std::wstring>",
};

class SimplifyTypesTest : public QObject
{
    Q_OBJECT

public:
    SimplifyTypesTest();

private Q_SLOTS:
    void testCase1();
    void testCase1_data();
};

SimplifyTypesTest::SimplifyTypesTest()
{
}

void SimplifyTypesTest::testCase1()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);
    const QString output = CPlusPlus::simplifySTLType(input);
    const bool ok = output == expected;
    if (!ok) {
        const QString msg = QString::fromAscii("Failure: got '%1' where '%2' was expected for '%3'")
                .arg(output, expected, input);
        QWARN(qPrintable(msg));
    }
    QVERIFY2(ok, "Failure");
}

void SimplifyTypesTest::testCase1_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");
    const size_t count = sizeof(input)/sizeof(const char *);
    for (size_t i = 0; i < count; i++ )
        QTest::newRow(description[i]) << QString::fromAscii(input[i])
                                      << QString::fromAscii(output[i]);
}

QTEST_APPLESS_MAIN(SimplifyTypesTest);

#include "tst_simplifytypestest.moc"
