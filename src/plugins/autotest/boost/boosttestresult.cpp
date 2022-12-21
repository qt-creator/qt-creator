// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "boosttestresult.h"
#include "boosttestconstants.h"
#include "boosttesttreeitem.h"
#include "../testframeworkmanager.h"

#include <utils/id.h>
#include <utils/qtcassert.h>

#include <QRegularExpression>

namespace Autotest {
namespace Internal {

BoostTestResult::BoostTestResult(const QString &id, const Utils::FilePath &projectFile, const QString &name)
    : TestResult(id, name), m_projectFile(projectFile)
{
}

const QString BoostTestResult::outputString(bool selected) const
{
    const QString &desc = description();
    QString output;
    switch (result()) {
    case ResultType::Pass:
    case ResultType::Fail:
        output = m_testCase;
        if (selected && !desc.isEmpty())
            output.append('\n').append(desc);
        break;
    default:
        output = desc;
        if (!selected)
            output = output.split('\n').first();
    }
    return output;
}

bool BoostTestResult::isDirectParentOf(const TestResult *other, bool *needsIntermediate) const
{
    if (!TestResult::isDirectParentOf(other, needsIntermediate))
        return false;

    if (result() != ResultType::TestStart)
        return false;

    bool weAreModule = (m_testCase.isEmpty() && m_testSuite.isEmpty());
    bool weAreSuite = (m_testCase.isEmpty() && !m_testSuite.isEmpty());
    bool weAreCase = (!m_testCase.isEmpty());

    const BoostTestResult *boostOther = static_cast<const BoostTestResult *>(other);
    bool otherIsSuite = boostOther->m_testCase.isEmpty() && !boostOther->m_testSuite.isEmpty();
    bool otherIsCase = !boostOther->m_testCase.isEmpty();

    if (otherIsSuite)
        return weAreSuite ? boostOther->m_testSuite.startsWith(m_testSuite + '/') : weAreModule;

    if (otherIsCase) {
        if (weAreCase)
            return boostOther->m_testCase == m_testCase && boostOther->m_testSuite == m_testSuite;
        if (weAreSuite)
            return boostOther->m_testSuite == m_testSuite;
        if (weAreModule)
            return boostOther->m_testSuite.isEmpty();
    }
    return false;
}

const ITestTreeItem *BoostTestResult::findTestTreeItem() const
{
    auto id = Utils::Id(Constants::FRAMEWORK_PREFIX).withSuffix(BoostTest::Constants::FRAMEWORK_NAME);
    ITestFramework *framework = TestFrameworkManager::frameworkForId(id);
    QTC_ASSERT(framework, return nullptr);
    const TestTreeItem *rootNode = framework->rootNode();
    if (!rootNode)
        return nullptr;

    return rootNode->findAnyChild([this](const Utils::TreeItem *item) {
        return matches(static_cast<const BoostTestTreeItem *>(item));
    });
}

bool BoostTestResult::matches(const BoostTestTreeItem *item) const
{
    // due to lacking information on the result side and a not fully appropriate tree we
    // might end up here with a differing set of tests, but it's the best we can do
    if (!item)
        return false;
    if (m_testCase.isEmpty()) // a top level module node
        return item->proFile() == m_projectFile;
    if (item->proFile() != m_projectFile)
        return false;
    if (!fileName().isEmpty() && fileName() != item->filePath())
        return false;

    QString fullName = "::" + m_testCase;
    fullName.prepend(m_testSuite.isEmpty() ? QString(BoostTest::Constants::BOOST_MASTER_SUITE)
                                           : m_testSuite);

    BoostTestTreeItem::TestStates states = item->state();
    if (states & BoostTestTreeItem::Templated) {
        const QRegularExpression regex(
            QRegularExpression::wildcardToRegularExpression(item->fullName() + "<*>"));
        return regex.match(fullName).hasMatch();
    } else if (states & BoostTestTreeItem::Parameterized) {
        const QRegularExpression regex(
            QRegularExpression::anchoredPattern(item->fullName() + "_\\d+"));
        return regex.isValid() && regex.match(fullName).hasMatch();
    }
    return item->fullName() == fullName;
}

} // namespace Internal
} // namespace Autotest

