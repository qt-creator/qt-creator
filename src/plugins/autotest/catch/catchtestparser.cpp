/****************************************************************************
**
** Copyright (C) 2019 Jochen Seemann
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

#include "catchtestparser.h"

#include "catchcodeparser.h"
#include "catchframework.h"
#include "catchtreeitem.h"

#include <cpptools/cppmodelmanager.h>
#include <cpptools/projectpart.h>
#include <utils/qtcassert.h>

#include <QRegularExpression>

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
    const QStringList validSectionMacros = {
        QStringLiteral("SECTION"), QStringLiteral("WHEN")
    };
    return isCatchTestCaseMacro(macroName) || validSectionMacros.contains(macroName);
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

    for (const QString &include : snapshot.allIncludesForDocument(doc->fileName())) {
        for (const QString &catchHeader : catchHeaders) {
            if (include.endsWith(catchHeader))
                return true;
        }
    }

    for (const QString &catchHeader : catchHeaders) {
        if (CppParser::precompiledHeaderContains(snapshot,
                                                 Utils::FilePath::fromString(doc->fileName()),
                                                 catchHeader))
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

bool CatchTestParser::processDocument(QFutureInterface<TestParseResultPtr> futureInterface,
                                      const Utils::FilePath &fileName)
{
    CPlusPlus::Document::Ptr doc = document(fileName);
    if (doc.isNull() || !includesCatchHeader(doc, m_cppSnapshot))
        return false;

    const CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    const QString &filePath = doc->fileName();
    const QByteArray &fileContent = getFileContent(fileName);

    if (!hasCatchNames(doc)) {
        const QRegularExpression regex("\\b(SCENARIO|(TEMPLATE_(PRODUCT_)?)?TEST_CASE(_METHOD)?|"
                                       "TEMPLATE_TEST_CASE(_METHOD)?_SIG|"
                                       "TEMPLATE_PRODUCT_TEST_CASE(_METHOD)?_SIG|"
                                       "TEMPLATE_LIST_TEST_CASE_METHOD|METHOD_AS_TEST_CASE|"
                                       "REGISTER_TEST_CASE)");
        if (!regex.match(QString::fromUtf8(fileContent)).hasMatch())
            return false;
    }


    const QList<CppTools::ProjectPart::Ptr> projectParts = modelManager->projectPart(fileName);
    if (projectParts.isEmpty()) // happens if shutting down while parsing
        return false;
    Utils::FilePath proFile;
    const CppTools::ProjectPart::Ptr projectPart = projectParts.first();
    proFile = Utils::FilePath::fromString(projectPart->projectFile);

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

    futureInterface.reportResult(TestParseResultPtr(parseResult));

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
