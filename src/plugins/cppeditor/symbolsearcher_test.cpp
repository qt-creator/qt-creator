// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "symbolsearcher_test.h"

#include "cppindexingsupport.h"
#include "cpptoolstestcase.h"

#include <coreplugin/find/searchresultwindow.h>

#include <utils/async.h>

#include <QTest>

using namespace Utils;

namespace {

inline QString _(const QByteArray &ba) { return QString::fromLatin1(ba, ba.size()); }

class ResultData;
using ResultDataList = QList<ResultData>;

class ResultData
{
public:
    ResultData() = default;
    ResultData(const QString &symbolName, const QString &scope)
        : m_symbolName(symbolName), m_scope(scope) {}

    bool operator==(const ResultData &other) const
    {
        return m_symbolName == other.m_symbolName && m_scope == other.m_scope;
    }

    static ResultDataList fromSearchResultList(const Utils::SearchResultItems &entries)
    {
        ResultDataList result;
        for (const Utils::SearchResultItem &entry : entries)
            result << ResultData(entry.lineText(), entry.path().join(QLatin1String("::")));
        return result;
    }

    /// For debugging and creating reference data
    static void printFilterEntries(const ResultDataList &entries)
    {
        QTextStream out(stdout);
        for (const ResultData &entry : entries) {
            out << "<< ResultData(_(\"" << entry.m_symbolName << "\"), _(\""
                << entry.m_scope <<  "\"))" << '\n';
        }
    }

public:
    QString m_symbolName;
    QString m_scope;
};

} // anonymous namespace

Q_DECLARE_METATYPE(ResultData)

namespace CppEditor::Internal {

namespace  {
class SymbolSearcherTestCase : public CppEditor::Tests::TestCase
{
public:
    SymbolSearcherTestCase(const FilePath &testFile,
                           const SearchParameters &searchParameters,
                           const ResultDataList &expectedResults)
    {
        QVERIFY(succeededSoFar());
        QVERIFY(parseFiles({testFile}));

        QFuture<Utils::SearchResultItem> search
            = Utils::asyncRun(&Internal::searchForSymbols, CppModelManager::snapshot(),
                              searchParameters, QSet<FilePath>{testFile});
        search.waitForFinished();
        ResultDataList results = ResultData::fromSearchResultList(search.results());
        QCOMPARE(results, expectedResults);
    }
};
} // anonymous namespace

void SymbolSearcherTest::test()
{
    QFETCH(FilePath, testFile);
    QFETCH(SearchParameters, searchParameters);
    QFETCH(ResultDataList, expectedResults);

    SymbolSearcherTestCase(testFile, searchParameters, expectedResults);
}

void SymbolSearcherTest::test_data()
{
    QTest::addColumn<FilePath>("testFile");
    QTest::addColumn<SearchParameters>("searchParameters");
    QTest::addColumn<ResultDataList>("expectedResults");

    const FilePath testFile = SRCDIR "/../../../tests/cppsymbolsearcher/testdata_basic/file1.cpp";
    QVERIFY(testFile.isReadableFile());

    SearchParameters searchParameters;

    // Check All Symbol Types
    searchParameters = SearchParameters();
    searchParameters.text = _("");
    searchParameters.flags = {};
    searchParameters.types = SymbolType::AllTypes;
    searchParameters.scope = SearchGlobal;
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
            << ResultData(_("MyNamespace::MyClass MY_CLASS"), _(""))
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
            << ResultData(_("MyClass MY_OTHER_CLASS"), _(""))
            << ResultData(_("main()"), _(""))

        );

    // Check Classes
    searchParameters = SearchParameters();
    searchParameters.text = _("myclass");
    searchParameters.flags = {};
    searchParameters.types = SymbolType::Classes;
    searchParameters.scope = SearchGlobal;
    QTest::newRow("BuiltinSymbolSearcher::Classes")
        << testFile
        << searchParameters
        << (ResultDataList()
            << ResultData(_("MyClass"), _(""))
            << ResultData(_("MyClass"), _("MyNamespace"))
            << ResultData(_("MyClass"), _("<anonymous namespace>"))
        );

    // Check Functions
    searchParameters = SearchParameters();
    searchParameters.text = _("fun");
    searchParameters.flags = {};
    searchParameters.types = SymbolType::Functions;
    searchParameters.scope = SearchGlobal;
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
    searchParameters = SearchParameters();
    searchParameters.text = _("enum");
    searchParameters.flags = {};
    searchParameters.types = SymbolType::Enums;
    searchParameters.scope = SearchGlobal;
    QTest::newRow("BuiltinSymbolSearcher::Enums")
        << testFile
        << searchParameters
        << (ResultDataList()
            << ResultData(_("MyEnum"), _(""))
            << ResultData(_("MyEnum"), _("MyNamespace"))
            << ResultData(_("MyEnum"), _("<anonymous namespace>"))
        );

    // Check Declarations
    searchParameters = SearchParameters();
    searchParameters.text = _("myvar");
    searchParameters.flags = {};
    searchParameters.types = SymbolType::Declarations;
    searchParameters.scope = SearchGlobal;
    QTest::newRow("BuiltinSymbolSearcher::Declarations")
        << testFile
        << searchParameters
        << (ResultDataList()
            << ResultData(_("int myVariable"), _(""))
            << ResultData(_("int myVariable"), _("MyNamespace"))
            << ResultData(_("int myVariable"), _("<anonymous namespace>"))
        );
}

} // namespace CppEditor::Internal
