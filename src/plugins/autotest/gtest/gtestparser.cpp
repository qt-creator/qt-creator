// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gtestparser.h"

#include "gtesttreeitem.h"
#include "gtestvisitors.h"
#include "gtest_utils.h"

#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/projectpart.h>

#include <QPromise>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

using namespace Utils;

namespace Autotest {
namespace Internal {

TestTreeItem *GTestParseResult::createTestTreeItem() const
{
    if (itemType != TestTreeItem::TestSuite && itemType != TestTreeItem::TestCase)
        return nullptr;
    GTestTreeItem *item = new GTestTreeItem(framework, name, fileName, itemType);
    item->setProFile(proFile);
    item->setLine(line);
    item->setColumn(column);

    if (parameterized)
        item->setState(GTestTreeItem::Parameterized);
    if (typed)
        item->setState(GTestTreeItem::Typed);
    if (disabled)
        item->setState(GTestTreeItem::Disabled);
    for (const TestParseResult *testSet : children)
        item->appendChild(testSet->createTestTreeItem());
    return item;
}

static bool includesGTest(const CPlusPlus::Document::Ptr &doc,
                          const CPlusPlus::Snapshot &snapshot)
{
    static const QString gtestH("gtest/gtest.h");
    for (const CPlusPlus::Document::Include &inc : doc->resolvedIncludes()) {
        if (inc.resolvedFileName().endsWith(gtestH))
            return true;
    }

    for (const FilePath &include : snapshot.allIncludesForDocument(doc->filePath())) {
        if (include.path().endsWith(gtestH))
            return true;
    }

    return CppParser::precompiledHeaderContains(snapshot, doc->filePath(), gtestH);
}

static bool hasGTestNames(const CPlusPlus::Document::Ptr &document)
{
    for (const CPlusPlus::Document::MacroUse &macro : document->macroUses()) {
        if (!macro.isFunctionLike())
            continue;
        if (GTestUtils::isGTestMacro(QLatin1String(macro.macro().name()))) {
            const QVector<CPlusPlus::Document::Block> args = macro.arguments();
            if (args.size() != 2)
                continue;
            return true;
        }
    }
    return false;
}

bool GTestParser::processDocument(QPromise<TestParseResultPtr> &promise,
                                  const FilePath &fileName)
{
    CPlusPlus::Document::Ptr doc = document(fileName);
    if (doc.isNull() || !includesGTest(doc, m_cppSnapshot))
        return false;

    const QByteArray &fileContent = getFileContent(fileName);
    if (!hasGTestNames(doc)) {
        static const QRegularExpression regex("\\b(TEST(_[FP])?|TYPED_TEST(_P)?|(GTEST_TEST))");
        if (!regex.match(QString::fromUtf8(fileContent)).hasMatch())
            return false;
    }

    const FilePath filePath = doc->filePath();
    CPlusPlus::Document::Ptr document = m_cppSnapshot.preprocessedDocument(fileContent, fileName);
    document->check();
    CPlusPlus::AST *ast = document->translationUnit()->ast();
    GTestVisitor visitor(document);
    visitor.accept(ast);

    const QMap<GTestCaseSpec, GTestCodeLocationList> result = visitor.gtestFunctions();
    FilePath proFile;
    const QList<CppEditor::ProjectPart::ConstPtr> &ppList =
        CppEditor::CppModelManager::projectPart(filePath);
    if (!ppList.isEmpty())
        proFile = FilePath::fromString(ppList.first()->projectFile);
    else
        return false; // happens if shutting down while parsing

    for (auto it = result.cbegin(); it != result.cend(); ++it) {
        const GTestCaseSpec &testSpec = it.key();
        GTestParseResult *parseResult = new GTestParseResult(framework());
        parseResult->itemType = TestTreeItem::TestSuite;
        parseResult->fileName = fileName;
        parseResult->name = testSpec.testCaseName;
        parseResult->parameterized = testSpec.parameterized;
        parseResult->typed = testSpec.typed;
        parseResult->disabled = testSpec.disabled;
        parseResult->proFile = proFile;

        for (const GTestCodeLocationAndType &location : it.value()) {
            GTestParseResult *testSet = new GTestParseResult(framework());
            testSet->name = location.m_name;
            testSet->fileName = fileName;
            testSet->line = location.m_line;
            testSet->column = location.m_column;
            testSet->disabled = location.m_state & GTestTreeItem::Disabled;
            testSet->itemType = location.m_type;
            testSet->proFile = proFile;

            parseResult->children.append(testSet);
        }

        promise.addResult(TestParseResultPtr(parseResult));
    }
    return !result.isEmpty();
}

} // namespace Internal
} // namespace Autotest
