// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "boosttestparser.h"

#include "boostcodeparser.h"
#include "boosttesttreeitem.h"

#include <cppeditor/cppmodelmanager.h>

#include <QMap>
#include <QPromise>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

using namespace Utils;

namespace Autotest {
namespace Internal {

namespace BoostTestUtils {
static const QStringList relevant = {
    QStringLiteral("BOOST_AUTO_TEST_CASE"), QStringLiteral("BOOST_TEST_CASE"),
    QStringLiteral("BOOST_DATA_TEST_CASE"), QStringLiteral("BOOST_FIXTURE_TEST_CASE"),
    QStringLiteral("BOOST_PARAM_TEST_CASE"), QStringLiteral("BOOST_DATA_TEST_CASE_F"),
    QStringLiteral("BOOST_AUTO_TEST_CASE_TEMPLATE"),
    QStringLiteral("BOOST_FIXTURE_TEST_CASE_TEMPLATE"),
};

bool isBoostTestMacro(const QString &macro)
{
    return relevant.contains(macro);
}
} // BoostTestUtils

TestTreeItem *BoostTestParseResult::createTestTreeItem() const
{
    if (itemType == TestTreeItem::Root)
        return nullptr;

    BoostTestTreeItem *item = new BoostTestTreeItem(framework, displayName, fileName, itemType);
    item->setProFile(proFile);
    item->setLine(line);
    item->setColumn(column);
    item->setStates(state);
    item->setFullName(name);

    for (const TestParseResult *funcParseResult : children)
        item->appendChild(funcParseResult->createTestTreeItem());
    return item;
}


static bool includesBoostTest(const CPlusPlus::Document::Ptr &doc,
                              const CPlusPlus::Snapshot &snapshot)
{
    static const QRegularExpression boostTestHpp("^.*/boost/test/.*\\.hpp$");
    for (const CPlusPlus::Document::Include &inc : doc->resolvedIncludes()) {
        if (boostTestHpp.match(inc.resolvedFileName().path()).hasMatch())
            return true;
    }

    for (const FilePath &include : snapshot.allIncludesForDocument(doc->filePath())) {
        if (boostTestHpp.match(include.path()).hasMatch())
            return true;
    }

    return CppParser::precompiledHeaderContains(snapshot, doc->filePath(), boostTestHpp);
}

static bool hasBoostTestMacros(const CPlusPlus::Document::Ptr &doc)
{
    for (const CPlusPlus::Document::MacroUse &macro : doc->macroUses()) {
        if (!macro.isFunctionLike())
            continue;
        if (BoostTestUtils::isBoostTestMacro(QLatin1String(macro.macro().name())))
            return true;
    }
    return false;
}

static BoostTestParseResult *createParseResult(const QString &name, const FilePath &filePath,
                                               const FilePath &projectFile,
                                               ITestFramework *framework,
                                               TestTreeItem::Type type, const BoostTestInfo &info)
{
    BoostTestParseResult *partialSuite = new BoostTestParseResult(framework);
    partialSuite->itemType = type;
    partialSuite->fileName = filePath;
    partialSuite->name = info.fullName;
    partialSuite->displayName = name;
    partialSuite->line = info.line;
    partialSuite->column = 0;
    partialSuite->proFile = projectFile;
    partialSuite->state = info.state;
    return partialSuite;

}

bool BoostTestParser::processDocument(QPromise<TestParseResultPtr> &promise,
                                      const FilePath &fileName)
{
    CPlusPlus::Document::Ptr doc = document(fileName);
    if (doc.isNull() || !includesBoostTest(doc, m_cppSnapshot) || !hasBoostTestMacros(doc))
        return false;

    const QList<CppEditor::ProjectPart::ConstPtr> projectParts
            = CppEditor::CppModelManager::projectPart(fileName);
    if (projectParts.isEmpty()) // happens if shutting down while parsing
        return false;
    const CppEditor::ProjectPart::ConstPtr projectPart = projectParts.first();
    const auto projectFile = FilePath::fromString(projectPart->projectFile);
    const QByteArray &fileContent = getFileContent(fileName);

    BoostCodeParser codeParser(fileContent, projectPart->languageFeatures, doc, m_cppSnapshot);
    const BoostTestCodeLocationList foundTests = codeParser.findTests();
    if (foundTests.isEmpty())
        return false;

    for (const BoostTestCodeLocationAndType &locationAndType : foundTests) {
        BoostTestInfoList suitesStates = locationAndType.m_suitesState;
        BoostTestInfo firstSuite = suitesStates.first();
        QStringList suites = firstSuite.fullName.split('/');
        BoostTestParseResult *topLevelSuite = createParseResult(suites.first(), fileName,
                                                                projectFile, framework(),
                                                                TestTreeItem::TestSuite,
                                                                firstSuite);
        BoostTestParseResult *currentSuite = topLevelSuite;
        suitesStates.removeFirst();
        while (!suitesStates.isEmpty()) {
            firstSuite = suitesStates.first();
            suites = firstSuite.fullName.split('/');
            BoostTestParseResult *suiteResult = createParseResult(suites.last(), fileName,
                                                                  projectFile, framework(),
                                                                  TestTreeItem::TestSuite,
                                                                  firstSuite);
            currentSuite->children.append(suiteResult);
            suitesStates.removeFirst();
            currentSuite = suiteResult;
        }

        if (currentSuite) {
            BoostTestInfo tmpInfo{
                locationAndType.m_suitesState.last().fullName + "::" + locationAndType.m_name,
                        locationAndType.m_state, locationAndType.m_line};
            BoostTestParseResult *funcResult = createParseResult(locationAndType.m_name, fileName,
                                                                 projectFile, framework(),
                                                                 locationAndType.m_type,
                                                                 tmpInfo);
            currentSuite->children.append(funcResult);
            promise.addResult(TestParseResultPtr(topLevelSuite));
        }
    }
    return true;
}

} // namespace Internal
} // namespace Autotest
