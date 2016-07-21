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

#include "qttesttreeitem.h"
#include "qttestconfiguration.h"
#include "qttestparser.h"
#include "../autotest_utils.h"

#include <projectexplorer/session.h>
#include <utils/qtcassert.h>

namespace Autotest {
namespace Internal {

QtTestTreeItem *QtTestTreeItem::createTestItem(const TestParseResult *result)
{
    QtTestTreeItem *item = new QtTestTreeItem(result->displayName, result->fileName,
                                              result->itemType);
    item->setProFile(result->proFile);
    item->setLine(result->line);
    item->setColumn(result->column);

    foreach (const TestParseResult *funcParseResult, result->children)
        item->appendChild(createTestItem(funcParseResult));
    return item;
}

QVariant QtTestTreeItem::data(int column, int role) const
{
    switch (role) {
    case Qt::CheckStateRole:
        switch (type()) {
        case Root:
        case TestDataFunction:
        case TestSpecialFunction:
        case TestDataTag:
            return QVariant();
        default:
            return checked();
        }
    case ItalicRole:
        switch (type()) {
        case TestDataFunction:
        case TestSpecialFunction:
            return true;
        default:
            return false;
        }
    }
    return TestTreeItem::data(column, role);
}

bool QtTestTreeItem::canProvideTestConfiguration() const
{
    switch (type()) {
    case TestCase:
    case TestFunctionOrSet:
    case TestDataTag:
        return true;
    default:
        return false;
    }
}

bool QtTestTreeItem::canProvideDebugConfiguration() const
{
    return canProvideTestConfiguration();
}

TestConfiguration *QtTestTreeItem::testConfiguration() const
{
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    QTC_ASSERT(project, return 0);

    QtTestConfiguration *config = 0;
    switch (type()) {
    case TestCase:
        config = new QtTestConfiguration;
        config->setTestCaseCount(childCount());
        config->setProFile(proFile());
        config->setProject(project);
        config->setDisplayName(TestUtils::getCMakeDisplayNameIfNecessary(filePath(), proFile()));
        break;
    case TestFunctionOrSet: {
        TestTreeItem *parent = parentItem();
        config = new QtTestConfiguration();
        config->setTestCases(QStringList(name()));
        config->setProFile(parent->proFile());
        config->setProject(project);
        config->setDisplayName(
                TestUtils::getCMakeDisplayNameIfNecessary(filePath(), parent->proFile()));
        break;
    }
    case TestDataTag: {
        const TestTreeItem *function = parentItem();
        const TestTreeItem *parent = function ? function->parentItem() : 0;
        if (!parent)
            return 0;
        const QString functionWithTag = function->name() + QLatin1Char(':') + name();
        config = new QtTestConfiguration();
        config->setTestCases(QStringList(functionWithTag));
        config->setProFile(parent->proFile());
        config->setProject(project);
        config->setDisplayName(TestUtils::getCMakeDisplayNameIfNecessary(filePath(),
                                                                         parent->proFile()));
        break;
    }
    default:
        return 0;
    }
    return config;
}

TestConfiguration *QtTestTreeItem::debugConfiguration() const
{
    QtTestConfiguration *config = static_cast<QtTestConfiguration *>(testConfiguration());
    return config;
}

QList<TestConfiguration *> QtTestTreeItem::getAllTestConfigurations() const
{
    QList<TestConfiguration *> result;

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    for (int row = 0, count = childCount(); row < count; ++row) {
        const TestTreeItem *child = childItem(row);

        TestConfiguration *tc = new QtTestConfiguration();
        tc->setTestCaseCount(child->childCount());
        tc->setProFile(child->proFile());
        tc->setProject(project);
        tc->setDisplayName(TestUtils::getCMakeDisplayNameIfNecessary(child->filePath(),
                                                                     child->proFile()));
        result << tc;
    }
    return result;
}

QList<TestConfiguration *> QtTestTreeItem::getSelectedTestConfigurations() const
{
    QList<TestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    QtTestConfiguration *testConfiguration = 0;

    for (int row = 0, count = childCount(); row < count; ++row) {
        const TestTreeItem *child = childItem(row);

        switch (child->checked()) {
        case Qt::Unchecked:
            continue;
        case Qt::Checked:
            testConfiguration = new QtTestConfiguration();
            testConfiguration->setTestCaseCount(child->childCount());
            testConfiguration->setProFile(child->proFile());
            testConfiguration->setProject(project);
            testConfiguration->setDisplayName(
                    TestUtils::getCMakeDisplayNameIfNecessary(child->filePath(), child->proFile()));
            result << testConfiguration;
            continue;
        case Qt::PartiallyChecked:
        default:
            int grandChildCount = child->childCount();
            QStringList testCases;
            for (int grandChildRow = 0; grandChildRow < grandChildCount; ++grandChildRow) {
                const TestTreeItem *grandChild = child->childItem(grandChildRow);
                if (grandChild->checked() == Qt::Checked)
                    testCases << grandChild->name();
            }

            testConfiguration = new QtTestConfiguration();
            testConfiguration->setTestCases(testCases);
            testConfiguration->setProFile(child->proFile());
            testConfiguration->setProject(project);
            testConfiguration->setDisplayName(
                    TestUtils::getCMakeDisplayNameIfNecessary(child->filePath(), child->proFile()));
            result << testConfiguration;
        }
    }

    return result;
}

TestTreeItem *QtTestTreeItem::find(const TestParseResult *result)
{
    QTC_ASSERT(result, return 0);

    switch (type()) {
    case Root:
        return findChildByFile(result->fileName);
    case TestCase:
        return findChildByName(result->displayName);
    case TestFunctionOrSet:
    case TestDataFunction:
    case TestSpecialFunction:
        return findChildByName(result->name);
    default:
        return 0;
    }
}

bool QtTestTreeItem::modify(const TestParseResult *result)
{
    QTC_ASSERT(result, return false);

    switch (type()) {
    case TestCase:
        return modifyTestCaseContent(result->name, result->line, result->column);
    case TestFunctionOrSet:
    case TestDataFunction:
    case TestSpecialFunction:
        return modifyTestFunctionContent(result);
    case TestDataTag:
        return modifyDataTagContent(result->name, result->fileName, result->line, result->line);
    default:
        return false;
    }
}

} // namespace Internal
} // namespace Autotest
