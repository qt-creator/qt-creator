// Copyright (C) 2019 Jochen Seemann
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "catchtestparser.h"

#include "catchcodeparser.h"
#include "catchtestframework.h"
#include "catchtreeitem.h"

#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/projectpart.h>

#include <utils/qtcassert.h>

#include <QPromise>
#include <QRegularExpression>

using namespace Utils;

namespace Autotest {
namespace Internal {

static bool isCatchTestCaseMacro(const QString &macroName)
{
    const QStringList validTestCaseMacros = {
        QStringLiteral("TEST_CASE"), QStringLiteral("SCENARIO"),
        QStringLiteral("TEMPLATE_TEST_CASE"), QStringLiteral("TEMPLATE_PRODUCT_TEST_CASE"),
        QStringLiteral("TEMPLATE_LIST_TEST_CASE"),
        QStringLiteral("TEMPLATE_TEST_CASE_SIG"), QStringLiteral("TEMPLATE_PRODUCT_TEST_CASE_SIG"),
        QStringLiteral("TEST_CASE_METHOD"), QStringLiteral("TEMPLATE_TEST_CASE_METHOD"),
        QStringLiteral("TEMPLATE_PRODUCT_TEST_CASE_METHOD"),
        QStringLiteral("TEST_CASE_METHOD"), QStringLiteral("TEMPLATE_TEST_CASE_METHOD_SIG"),
        QStringLiteral("TEMPLATE_PRODUCT_TEST_CASE_METHOD_SIG"),
        QStringLiteral("TEMPLATE_TEST_CASE_METHOD"),
        QStringLiteral("TEMPLATE_LIST_TEST_CASE_METHOD"),
        QStringLiteral("METHOD_AS_TEST_CASE"), QStringLiteral("REGISTER_TEST_CASE")
    };
    return validTestCaseMacros.contains(macroName);
}

static bool isCatchMacro(const QString &macroName)
{
    QString unprefixed = macroName.startsWith("CATCH_") ? macroName.mid(6) : macroName;
    const QStringList validSectionMacros = {
        QStringLiteral("SECTION"), QStringLiteral("WHEN")
    };
    return isCatchTestCaseMacro(unprefixed) || validSectionMacros.contains(unprefixed);
}

static bool includesCatchHeader(const CPlusPlus::Document::Ptr &doc,
                                const CPlusPlus::Snapshot &snapshot)
{
    static const QStringList catchHeaders{"catch.hpp", // v2
                                          "catch_all.hpp", // v3 - new approach
                                          "catch_amalgamated.hpp",
                                          "catch_test_macros.hpp",
                                          "catch_template_test_macros.hpp"
                                         };
    for (const CPlusPlus::Document::Include &inc : doc->resolvedIncludes()) {
        for (const QString &catchHeader : catchHeaders) {
            if (inc.resolvedFileName().endsWith(catchHeader))
                return true;
        }
    }

    for (const FilePath &include : snapshot.allIncludesForDocument(doc->filePath())) {
        for (const QString &catchHeader : catchHeaders) {
            if (include.endsWith(catchHeader))
                return true;
        }
    }

    for (const QString &catchHeader : catchHeaders) {
        if (CppParser::precompiledHeaderContains(snapshot, doc->filePath(), catchHeader))
            return true;
    }
    return false;
}

static bool hasCatchNames(const CPlusPlus::Document::Ptr &document)
{
    for (const CPlusPlus::Document::MacroUse &macro : document->macroUses()) {
        if (!macro.isFunctionLike())
            continue;

        if (isCatchMacro(QLatin1String(macro.macro().name()))) {
            const QVector<CPlusPlus::Document::Block> args = macro.arguments();
            if (args.size() < 1)
                continue;
            return true;
        }
    }
    return false;
}

bool CatchTestParser::processDocument(QPromise<TestParseResultPtr> &promise,
                                      const FilePath &fileName)
{
    CPlusPlus::Document::Ptr doc = document(fileName);
    if (doc.isNull() || !includesCatchHeader(doc, m_cppSnapshot))
        return false;

    const QString &filePath = doc->filePath().toString();
    const QByteArray &fileContent = getFileContent(fileName);

    if (!hasCatchNames(doc)) {
        static const QRegularExpression regex("\\b(CATCH_)?"
                                              "(SCENARIO|(TEMPLATE_(PRODUCT_)?)?TEST_CASE(_METHOD)?|"
                                              "TEMPLATE_TEST_CASE(_METHOD)?_SIG|"
                                              "TEMPLATE_PRODUCT_TEST_CASE(_METHOD)?_SIG|"
                                              "TEMPLATE_LIST_TEST_CASE_METHOD|METHOD_AS_TEST_CASE|"
                                              "REGISTER_TEST_CASE)");
        if (!regex.match(QString::fromUtf8(fileContent)).hasMatch())
            return false;
    }


    const QList<CppEditor::ProjectPart::ConstPtr> projectParts
        = CppEditor::CppModelManager::projectPart(fileName);
    if (projectParts.isEmpty()) // happens if shutting down while parsing
        return false;
    FilePath proFile;
    const CppEditor::ProjectPart::ConstPtr projectPart = projectParts.first();
    proFile = FilePath::fromString(projectPart->projectFile);

    CatchCodeParser codeParser(fileContent, projectPart->languageFeatures);
    const CatchTestCodeLocationList foundTests = codeParser.findTests();

    CatchParseResult *parseResult = new CatchParseResult(framework());
    parseResult->itemType = TestTreeItem::TestSuite;
    parseResult->fileName = fileName;
    parseResult->name = filePath;
    parseResult->displayName = filePath;
    parseResult->proFile = proFile;

    for (const CatchTestCodeLocationAndType & testLocation : foundTests) {
        CatchParseResult *testCase = new CatchParseResult(framework());
        testCase->fileName = fileName;
        testCase->name = testLocation.m_name;
        testCase->proFile = proFile;
        testCase->itemType = testLocation.m_type;
        testCase->line = testLocation.m_line;
        testCase->column = testLocation.m_column;
        testCase->states = testLocation.states;

        parseResult->children.append(testCase);
    }

    promise.addResult(TestParseResultPtr(parseResult));

    return !foundTests.isEmpty();
}

TestTreeItem *CatchParseResult::createTestTreeItem() const
{
    if (itemType == TestTreeItem::Root)
        return nullptr;

    CatchTreeItem *item = new CatchTreeItem(framework, name, fileName, itemType);
    item->setProFile(proFile);
    item->setLine(line);
    item->setColumn(column);
    item->setStates(states);

    for (const TestParseResult *testSet : children)
        item->appendChild(testSet->createTestTreeItem());

    return item;
}

} // namespace Internal
} // namespace Autotest
