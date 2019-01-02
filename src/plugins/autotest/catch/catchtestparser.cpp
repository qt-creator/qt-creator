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

#include "catchtreeitem.h"

#include <cpptools/cppmodelmanager.h>
#include <cpptools/projectpart.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>
#include <utils/qtcassert.h>

#include <QRegularExpression>

namespace Autotest {
namespace Internal {

struct CatchTestCaseSpec
{
    QString name;
    QStringList tags;
};

static bool isCatchTestCaseMacro(const QString &macroName)
{
    const QStringList validTestCaseMacros = {
        QStringLiteral("TEST_CASE"), QStringLiteral("SCENARIO")
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

static inline QString stringLiteralToQString(const CPlusPlus::StringLiteral *literal)
{
    return QString::fromLatin1(literal->chars(), int(literal->size()));
}

static QStringList parseTags(const QString &tagsString)
{
    QStringList tagsList;

    const QRegularExpression tagRegEx("\\[(.*?)\\]",QRegularExpression::CaseInsensitiveOption);
    int pos = 0;
    QRegularExpressionMatch it = tagRegEx.match(tagsString, pos);
    while (it.hasMatch()) {
        tagsList.append(it.captured(1));
        pos += it.capturedLength();
        it = tagRegEx.match(tagsString, pos);
    }
    return tagsList;
}

class CatchTestVisitor : public CPlusPlus::ASTVisitor
{
public:
    explicit CatchTestVisitor(CPlusPlus::Document::Ptr doc)
        : CPlusPlus::ASTVisitor(doc->translationUnit()) , m_document(doc) {}
    bool visit(CPlusPlus::FunctionDefinitionAST *ast) override;

    QMap<CatchTestCaseSpec, TestCodeLocationAndType> testFunctions() const { return m_testFunctions; }

private:
    bool isValidTestCase(unsigned int macroTokenIndex, QString &testCaseName, QStringList &tags);

    CPlusPlus::Document::Ptr m_document;
    CPlusPlus::Overview m_overview;
    QMap<CatchTestCaseSpec, TestCodeLocationAndType> m_testFunctions;
};

bool CatchTestVisitor::visit(CPlusPlus::FunctionDefinitionAST *ast)
{
    if (!ast || !ast->declarator || !ast->declarator->core_declarator)
        return false;

    CPlusPlus::DeclaratorIdAST *id = ast->declarator->core_declarator->asDeclaratorId();
    if (!id || !id->name)
        return false;

    const QString prettyName = m_overview.prettyName(id->name->name);
    if (!isCatchTestCaseMacro(prettyName))
        return false;

    QString testName;
    QStringList tags;
    if (!isValidTestCase(ast->firstToken(), testName, tags))
        return false;

    CatchTestCaseSpec spec;
    spec.name = testName;
    if (prettyName == "SCENARIO")
        spec.name.prepend("Scenario: ");  // TODO maybe better as a flag?

    spec.tags = tags;

    TestCodeLocationAndType location;
    location.m_type = TestTreeItem::TestCase;
    location.m_name = m_document->fileName();
    getTokenStartPosition(ast->firstToken(), &location.m_line, &location.m_column);

    m_testFunctions.insert(spec, location);
    return true;
}

bool CatchTestVisitor::isValidTestCase(unsigned int macroTokenIndex, QString &testCaseName, QStringList &tags)
{
    QTC_ASSERT(testCaseName.isEmpty(), return false);
    QTC_ASSERT(tags.isEmpty(), return false);

    unsigned int end = tokenCount();
    ++macroTokenIndex;

    enum ParseMode { TestCaseMode, TagsMode, Ignored } mode = TestCaseMode;
    QString captured;
    while (macroTokenIndex < end) {
        CPlusPlus::Token token = tokenAt(macroTokenIndex);
        if (token.kind() == CPlusPlus::T_RPAREN) {
            if (mode == TagsMode)
                tags = parseTags(captured);
            else
                testCaseName = captured;
            break;
        } else if (token.kind() == CPlusPlus::T_COMMA) {
            if (mode == TestCaseMode) {
                mode = TagsMode;
                testCaseName = captured;
                captured = QString();
            } else if (mode == TagsMode) {
                mode = Ignored;
            }
        } else if (token.kind() == CPlusPlus::T_STRING_LITERAL) {
            if (mode != Ignored)
                captured += stringLiteralToQString(stringLiteral(macroTokenIndex));
        }
        ++macroTokenIndex;
    }

    return !testCaseName.isEmpty();
}

inline bool operator<(const CatchTestCaseSpec &spec1, const CatchTestCaseSpec &spec2)
{
    if (spec1.name != spec2.name)
        return spec1.name < spec2.name;
    if (spec1.tags != spec2.tags)
        return spec1.tags < spec2.tags;

    return false;
}

static bool includesCatchHeader(const CPlusPlus::Document::Ptr &doc,
                                const CPlusPlus::Snapshot &snapshot)
{
    static const QString catchHeader("catch.hpp");
    for (const CPlusPlus::Document::Include &inc : doc->resolvedIncludes()) {
        if (inc.resolvedFileName().endsWith(catchHeader))
            return true;
    }

    for (const QString &include : snapshot.allIncludesForDocument(doc->fileName())) {
        if (include.endsWith(catchHeader))
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

static bool handleCatchDocument(QFutureInterface<TestParseResultPtr> futureInterface,
                                const CPlusPlus::Document::Ptr &doc,
                                const CPlusPlus::Snapshot &snapshot,
                                ITestFramework *framework)
{
    const CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    const QString &filePath = doc->fileName();
    const QByteArray &fileContent = CppParser::getFileContent(filePath);
    CPlusPlus::Document::Ptr document = snapshot.preprocessedDocument(fileContent, filePath);
    document->check();
    CPlusPlus::AST *ast = document->translationUnit()->ast();
    CatchTestVisitor visitor(document);
    visitor.accept(ast);

    QMap<CatchTestCaseSpec, TestCodeLocationAndType> result = visitor.testFunctions();
    QString proFile;
    const QList<CppTools::ProjectPart::Ptr> &ppList = modelManager->projectPart(filePath);
    if (ppList.size())
        proFile = ppList.first()->projectFile;
    else
        return false;

    CatchParseResult *parseResult = new CatchParseResult(framework);
    parseResult->itemType = TestTreeItem::TestCase;
    parseResult->fileName = filePath;
    parseResult->name = filePath;
    parseResult->displayName = filePath;
    QList<CppTools::ProjectPart::Ptr> projectParts = modelManager->projectPart(doc->fileName());
    if (projectParts.isEmpty()) // happens if shutting down while parsing
        return false;
    parseResult->proFile = projectParts.first()->projectFile;

    for (const auto &testSpec : result.keys()) {
        const TestCodeLocationAndType &testLocation = result.value(testSpec);
        CatchParseResult *testCase = new CatchParseResult(framework);
        testCase->fileName = filePath;
        testCase->name = testSpec.name;
        testCase->proFile = proFile;
        testCase->itemType = TestTreeItem::TestFunction;
        testCase->line = testLocation.m_line;
        testCase->column = testLocation.m_column;

        parseResult->children.append(testCase);
    }

    futureInterface.reportResult(TestParseResultPtr(parseResult));

    return !result.keys().isEmpty();
}

bool CatchTestParser::processDocument(QFutureInterface<TestParseResultPtr> futureInterface, const QString &fileName)
{
    CPlusPlus::Document::Ptr doc = document(fileName);
    if (doc.isNull() || !includesCatchHeader(doc, m_cppSnapshot) || !hasCatchNames(doc))
        return false;

    return handleCatchDocument(futureInterface, doc, m_cppSnapshot, framework());
}

TestTreeItem *CatchParseResult::createTestTreeItem() const
{
    if (itemType == TestTreeItem::Root)
        return nullptr;

    CatchTreeItem *item = new CatchTreeItem(framework, name, fileName, itemType);
    item->setProFile(proFile);
    item->setLine(line);
    item->setColumn(column);

    for (const TestParseResult *testSet : children)
        item->appendChild(testSet->createTestTreeItem());

    return item;
}

} // namespace Internal
} // namespace Autotest
