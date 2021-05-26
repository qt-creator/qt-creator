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

