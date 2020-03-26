/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "boosttestparser.h"
#include "boostcodeparser.h"
#include "boosttesttreeitem.h"

#include <cpptools/cppmodelmanager.h>

#include <QMap>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

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
        if (boostTestHpp.match(inc.resolvedFileName()).hasMatch())
            return true;
    }

    for (const QString &include : snapshot.allIncludesForDocument(doc->fileName())) {
        if (boostTestHpp.match(include).hasMatch())
            return true;
    }

    return false;
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

static BoostTestParseResult *createParseResult(const QString &name, const QString &filePath,
                                               const QString &projectFile, ITestFramework *framework,
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

static bool handleBoostTest(QFutureInterface<TestParseResultPtr> futureInterface,
                            const CPlusPlus::Document::Ptr &doc,
                            const CPlusPlus::Snapshot &snapshot,
                            ITestFramework *framework)
{
    const CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    const QString &filePath = doc->fileName();

    const QList<CppTools::ProjectPart::Ptr> projectParts = modelManager->projectPart(filePath);
    if (projectParts.isEmpty()) // happens if shutting down while parsing
        return false;
    const CppTools::ProjectPart::Ptr projectPart = projectParts.first();
    const auto projectFile = projectPart->projectFile;
    const QByteArray &fileContent = CppParser::getFileContent(filePath);

    BoostCodeParser codeParser(fileContent, projectPart->languageFeatures, doc, snapshot);
    const BoostTestCodeLocationList foundTests = codeParser.findTests();
    if (foundTests.isEmpty())
        return false;

    for (const BoostTestCodeLocationAndType &locationAndType : foundTests) {
        BoostTestInfoList suitesStates = locationAndType.m_suitesState;
        BoostTestInfo firstSuite = suitesStates.first();
        QStringList suites = firstSuite.fullName.split('/');
        BoostTestParseResult *topLevelSuite = createParseResult(suites.first(), filePath,
                                                                projectFile, framework,
                                                                TestTreeItem::TestSuite,
                                                                firstSuite);
        BoostTestParseResult *currentSuite = topLevelSuite;
        suitesStates.removeFirst();
        while (!suitesStates.isEmpty()) {
            firstSuite = suitesStates.first();
            suites = firstSuite.fullName.split('/');
            BoostTestParseResult *suiteResult = createParseResult(suites.last(), filePath,
                                                                  projectFile, framework,
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
            BoostTestParseResult *funcResult = createParseResult(locationAndType.m_name, filePath,
                                                                 projectFile, framework,
                                                                 locationAndType.m_type,
                                                                 tmpInfo);
            currentSuite->children.append(funcResult);
            futureInterface.reportResult(TestParseResultPtr(topLevelSuite));
        }
    }
    return true;
}

bool BoostTestParser::processDocument(QFutureInterface<TestParseResultPtr> futureInterface,
                                      const QString &fileName)
{
    CPlusPlus::Document::Ptr doc = document(fileName);
    if (doc.isNull() || !includesBoostTest(doc, m_cppSnapshot) || !hasBoostTestMacros(doc))
        return false;
    return handleBoostTest(futureInterface, doc, m_cppSnapshot, framework());
}

} // namespace Internal
} // namespace Autotest
