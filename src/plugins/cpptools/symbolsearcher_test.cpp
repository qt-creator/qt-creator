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

#include "cpptoolsplugin.h"

#include "builtinindexingsupport.h"
#include "cppmodelmanager.h"
#include "cpptoolstestcase.h"
#include "searchsymbols.h"

#include <coreplugin/testdatadir.h>
#include <coreplugin/find/searchresultwindow.h>
#include <utils/runextensions.h>

#include <QtTest>

using namespace CppTools;
using namespace CppTools::Internal;

namespace {

QTC_DECLARE_MYTESTDATADIR("../../../tests/cppsymbolsearcher/")

inline QString _(const QByteArray &ba) { return QString::fromLatin1(ba, ba.size()); }

class ResultData
{
public:
    typedef QList<ResultData> ResultDataList;

    ResultData() {}
    ResultData(const QString &symbolName, const QString &scope)
        : m_symbolName(symbolName), m_scope(scope) {}

    bool operator==(const ResultData &other) const
    {
        return m_symbolName == other.m_symbolName && m_scope == other.m_scope;
    }

    static ResultDataList fromSearchResultList(const QList<Core::SearchResultItem> &entries)
    {
        ResultDataList result;
        foreach (const Core::SearchResultItem &entry, entries)
            result << ResultData(entry.text, entry.path.join(QLatin1String("::")));
        return result;
    }

    /// For debugging and creating reference data
    static void printFilterEntries(const ResultDataList &entries)
    {
        QTextStream out(stdout);
        foreach (const ResultData entry, entries) {
            out << "<< ResultData(_(\"" << entry.m_symbolName << "\"), _(\""
                << entry.m_scope <<  "\"))" << endl;
        }
    }

public:
    QString m_symbolName;
    QString m_scope;
};

typedef ResultData::ResultDataList ResultDataList;

class SymbolSearcherTestCase : public Tests::TestCase
{
public:
    /// Takes no ownership of indexingSupportToUse
    SymbolSearcherTestCase(const QString &testFile,
                           CppIndexingSupport *indexingSupportToUse,
                           const SymbolSearcher::Parameters &searchParameters,
                           const ResultDataList &expectedResults)
        : m_indexingSupportToRestore(0)
        , m_indexingSupportToUse(indexingSupportToUse)
    {
        QVERIFY(succeededSoFar());

        QVERIFY(m_indexingSupportToUse);
        QVERIFY(parseFiles(testFile));
        m_indexingSupportToRestore = m_modelManager->indexingSupport();
        m_modelManager->setIndexingSupport(m_indexingSupportToUse);

        CppIndexingSupport *indexingSupport = m_modelManager->indexingSupport();
        const QScopedPointer<SymbolSearcher> symbolSearcher(
            indexingSupport->createSymbolSearcher(searchParameters, QSet<QString>() << testFile));
        QFuture<Core::SearchResultItem> search
            = Utils::runAsync(&SymbolSearcher::runSearch, symbolSearcher.data());
        search.waitForFinished();
        ResultDataList results = ResultData::fromSearchResultList(search.results());
        QCOMPARE(results, expectedResults);
    }

    ~SymbolSearcherTestCase()
    {
        if (m_indexingSupportToRestore)
            m_modelManager->setIndexingSupport(m_indexingSupportToRestore);
    }

private:
    CppIndexingSupport *m_indexingSupportToRestore;
    CppIndexingSupport *m_indexingSupportToUse;
};

} // anonymous namespace

Q_DECLARE_METATYPE(ResultData)

QT_BEGIN_NAMESPACE
namespace QTest {

template<> char *toString(const ResultData &data)
{
    QByteArray ba = "\"" + data.m_symbolName.toUtf8() + "\", \"" + data.m_scope.toUtf8() + "\"";
    return qstrdup(ba.data());
}

} // namespace QTest
QT_END_NAMESPACE

void CppToolsPlugin::test_builtinsymbolsearcher()
{
    QFETCH(QString, testFile);
    QFETCH(SymbolSearcher::Parameters, searchParameters);
    QFETCH(ResultDataList, expectedResults);

    QScopedPointer<CppIndexingSupport> builtinIndexingSupport(new BuiltinIndexingSupport);
    SymbolSearcherTestCase(testFile,
                           builtinIndexingSupport.data(),
                           searchParameters,
                           expectedResults);
}

void CppToolsPlugin::test_builtinsymbolsearcher_data()
{
    QTest::addColumn<QString>("testFile");
    QTest::addColumn<SymbolSearcher::Parameters>("searchParameters");
    QTest::addColumn<ResultDataList>("expectedResults");

    MyTestDataDir testDirectory(QLatin1String("testdata_basic"));
    const QString testFile = testDirectory.file(QLatin1String("file1.cpp"));

    SymbolSearcher::Parameters searchParameters;

    // Check All Symbol Types
    searchParameters = SymbolSearcher::Parameters();
    searchParameters.text = _("");
    searchParameters.flags = 0;
    searchParameters.types = SearchSymbols::AllTypes;
    searchParameters.scope = SymbolSearcher::SearchGlobal;
    QTest::newRow("BuiltinSymbolSearcher::AllTypes")
        << testFile
        << searchParameters
        << (ResultDataList()
            << ResultData(_("int myVariable"), _(""))
            << ResultData(_("myFunction(bool, int)"), _(""))
            << ResultData(_("MyEnum"), _(""))
            << ResultData(_("int V1"), _("MyEnum"))
            << ResultData(_("int V2"), _("MyEnum"))
            << ResultData(_("MyClass"), _(""))
            << ResultData(_("MyClass()"), _("MyClass"))
            << ResultData(_("functionDeclaredOnly()"), _("MyClass"))
            << ResultData(_("functionDefinedInClass(bool, int)"), _("MyClass"))
            << ResultData(_("functionDefinedOutSideClass(char)"), _("MyClass"))
            << ResultData(_("functionDefinedOutSideClass(char)"), _("MyClass"))
            << ResultData(_("int myVariable"), _("MyNamespace"))
            << ResultData(_("myFunction(bool, int)"), _("MyNamespace"))
            << ResultData(_("MyEnum"), _("MyNamespace"))
            << ResultData(_("int V1"), _("MyNamespace::MyEnum"))
            << ResultData(_("int V2"), _("MyNamespace::MyEnum"))
            << ResultData(_("MyClass"), _("MyNamespace"))
            << ResultData(_("MyClass()"), _("MyNamespace::MyClass"))
            << ResultData(_("functionDeclaredOnly()"), _("MyNamespace::MyClass"))
            << ResultData(_("functionDefinedInClass(bool, int)"), _("MyNamespace::MyClass"))
            << ResultData(_("functionDefinedOutSideClass(char)"), _("MyNamespace::MyClass"))
            << ResultData(_("functionDefinedOutSideClassAndNamespace(float)"),
                          _("MyNamespace::MyClass"))
            << ResultData(_("functionDefinedOutSideClass(char)"), _("MyNamespace::MyClass"))
            << ResultData(_("functionDefinedOutSideClassAndNamespace(float)"),
                          _("MyNamespace::MyClass"))
            << ResultData(_("int myVariable"), _("<anonymous namespace>"))
            << ResultData(_("myFunction(bool, int)"), _("<anonymous namespace>"))
            << ResultData(_("MyEnum"), _("<anonymous namespace>"))
            << ResultData(_("int V1"), _("<anonymous namespace>::MyEnum"))
            << ResultData(_("int V2"), _("<anonymous namespace>::MyEnum"))
            << ResultData(_("MyClass"), _("<anonymous namespace>"))
            << ResultData(_("MyClass()"), _("<anonymous namespace>::MyClass"))
            << ResultData(_("functionDeclaredOnly()"), _("<anonymous namespace>::MyClass"))
            << ResultData(_("functionDefinedInClass(bool, int)"),
                          _("<anonymous namespace>::MyClass"))
            << ResultData(_("functionDefinedOutSideClass(char)"),
                          _("<anonymous namespace>::MyClass"))
            << ResultData(_("functionDefinedOutSideClass(char)"),
                          _("<anonymous namespace>::MyClass"))
            << ResultData(_("main()"), _(""))

        );

    // Check Classes
    searchParameters = SymbolSearcher::Parameters();
    searchParameters.text = _("myclass");
    searchParameters.flags = 0;
    searchParameters.types = SymbolSearcher::Classes;
    searchParameters.scope = SymbolSearcher::SearchGlobal;
    QTest::newRow("BuiltinSymbolSearcher::Classes")
        << testFile
        << searchParameters
        << (ResultDataList()
            << ResultData(_("MyClass"), _(""))
            << ResultData(_("MyClass"), _("MyNamespace"))
            << ResultData(_("MyClass"), _("<anonymous namespace>"))
        );

    // Check Functions
    searchParameters = SymbolSearcher::Parameters();
    searchParameters.text = _("fun");
    searchParameters.flags = 0;
    searchParameters.types = SymbolSearcher::Functions;
    searchParameters.scope = SymbolSearcher::SearchGlobal;
    QTest::newRow("BuiltinSymbolSearcher::Functions")
        << testFile
        << searchParameters
        << (ResultDataList()
            << ResultData(_("myFunction(bool, int)"), _(""))
            << ResultData(_("functionDefinedInClass(bool, int)"), _("MyClass"))
            << ResultData(_("functionDefinedOutSideClass(char)"), _("MyClass"))
            << ResultData(_("myFunction(bool, int)"), _("MyNamespace"))
            << ResultData(_("functionDefinedInClass(bool, int)"), _("MyNamespace::MyClass"))
            << ResultData(_("functionDefinedOutSideClass(char)"), _("MyNamespace::MyClass"))
            << ResultData(_("functionDefinedOutSideClassAndNamespace(float)"),
                          _("MyNamespace::MyClass"))
            << ResultData(_("myFunction(bool, int)"), _("<anonymous namespace>"))
            << ResultData(_("functionDefinedInClass(bool, int)"),
                          _("<anonymous namespace>::MyClass"))
            << ResultData(_("functionDefinedOutSideClass(char)"),
                          _("<anonymous namespace>::MyClass"))
        );

    // Check Enums
    searchParameters = SymbolSearcher::Parameters();
    searchParameters.text = _("enum");
    searchParameters.flags = 0;
    searchParameters.types = SymbolSearcher::Enums;
    searchParameters.scope = SymbolSearcher::SearchGlobal;
    QTest::newRow("BuiltinSymbolSearcher::Enums")
        << testFile
        << searchParameters
        << (ResultDataList()
            << ResultData(_("MyEnum"), _(""))
            << ResultData(_("MyEnum"), _("MyNamespace"))
            << ResultData(_("MyEnum"), _("<anonymous namespace>"))
        );

    // Check Declarations
    searchParameters = SymbolSearcher::Parameters();
    searchParameters.text = _("myvar");
    searchParameters.flags = 0;
    searchParameters.types = SymbolSearcher::Declarations;
    searchParameters.scope = SymbolSearcher::SearchGlobal;
    QTest::newRow("BuiltinSymbolSearcher::Declarations")
        << testFile
        << searchParameters
        << (ResultDataList()
            << ResultData(_("int myVariable"), _(""))
            << ResultData(_("int myVariable"), _("MyNamespace"))
            << ResultData(_("int myVariable"), _("<anonymous namespace>"))
        );
}
