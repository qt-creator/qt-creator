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
#include "../testframeworkmanager.h"

#include <projectexplorer/session.h>
#include <utils/qtcassert.h>

namespace Autotest {
namespace Internal {

QtTestTreeItem::QtTestTreeItem(const QString &name, const QString &filePath, TestTreeItem::Type type)
    : TestTreeItem(name, filePath, type)
{
    if (type == TestDataTag)
        setData(0, Qt::Checked, Qt::CheckStateRole);
}

TestTreeItem *QtTestTreeItem::copyWithoutChildren()
{
    QtTestTreeItem *copied = new QtTestTreeItem;
    copied->copyBasicDataFrom(this);
    copied->m_inherited = m_inherited;
    return copied;
}

QVariant QtTestTreeItem::data(int column, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (type() == Root)
            break;
        return QVariant(name() + nameSuffix());
    case Qt::CheckStateRole:
        switch (type()) {
        case TestDataFunction:
        case TestSpecialFunction:
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

Qt::ItemFlags QtTestTreeItem::flags(int column) const
{
    static const Qt::ItemFlags defaultFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    switch (type()) {
    case TestDataTag:
        return defaultFlags | Qt::ItemIsUserCheckable;
    case TestFunctionOrSet:
        return defaultFlags | Qt::ItemIsAutoTristate | Qt::ItemIsUserCheckable;
    default:
        return TestTreeItem::flags(column);
    }
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
    QTC_ASSERT(project, return nullptr);

    QtTestConfiguration *config = nullptr;
    switch (type()) {
    case TestCase:
        config = new QtTestConfiguration;
        config->setTestCaseCount(childCount());
        config->setProjectFile(proFile());
        config->setProject(project);
        break;
    case TestFunctionOrSet: {
        TestTreeItem *parent = parentItem();
        config = new QtTestConfiguration();
        config->setTestCases(QStringList(name()));
        config->setProjectFile(parent->proFile());
        config->setProject(project);
        break;
    }
    case TestDataTag: {
        const TestTreeItem *function = parentItem();
        const TestTreeItem *parent = function ? function->parentItem() : nullptr;
        if (!parent)
            return nullptr;
        const QString functionWithTag = function->name() + ':' + name();
        config = new QtTestConfiguration();
        config->setTestCases(QStringList(functionWithTag));
        config->setProjectFile(parent->proFile());
        config->setProject(project);
        break;
    }
    default:
        return nullptr;
    }
    if (config)
        config->setInternalTargets(internalTargets());
    return config;
}

static void fillTestConfigurationsFromCheckState(const TestTreeItem *item,
                                                 QList<TestConfiguration *> &testConfigurations)
{
    QTC_ASSERT(item, return);
    if (item->type() == TestTreeItem::GroupNode) {
        for (int row = 0, count = item->childCount(); row < count; ++row)
            fillTestConfigurationsFromCheckState(item->childAt(row), testConfigurations);
        return;
    }
    QTC_ASSERT(item->type() == TestTreeItem::TestCase, return);
    QtTestConfiguration *testConfig = nullptr;
    switch (item->checked()) {
    case Qt::Unchecked:
        return;
    case Qt::Checked:
        testConfig = static_cast<QtTestConfiguration *>(item->testConfiguration());
        QTC_ASSERT(testConfig, return);
        testConfigurations << testConfig;
        return;
    case Qt::PartiallyChecked:
        QStringList testCases;
        item->forFirstLevelChildren([&testCases](TestTreeItem *grandChild) {
            if (grandChild->checked() == Qt::Checked) {
                testCases << grandChild->name();
            } else if (grandChild->checked() == Qt::PartiallyChecked) {
                const QString funcName = grandChild->name();
                grandChild->forFirstLevelChildren([&testCases, &funcName](TestTreeItem *dataTag) {
                    if (dataTag->checked() == Qt::Checked)
                        testCases << funcName + ':' + dataTag->name();
                });
            }
        });

        testConfig = new QtTestConfiguration();
        testConfig->setTestCases(testCases);
        testConfig->setProjectFile(item->proFile());
        testConfig->setProject(ProjectExplorer::SessionManager::startupProject());
        testConfig->setInternalTargets(item->internalTargets());
        testConfigurations << testConfig;
    }
}

TestConfiguration *QtTestTreeItem::debugConfiguration() const
{
    QtTestConfiguration *config = static_cast<QtTestConfiguration *>(testConfiguration());
    if (config)
        config->setRunMode(TestRunMode::Debug);
    return config;
}

QList<TestConfiguration *> QtTestTreeItem::getAllTestConfigurations() const
{
    QList<TestConfiguration *> result;

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    forFirstLevelChildren([&result](TestTreeItem *child) {
        if (child->type() == TestCase) {
            TestConfiguration *tc = child->testConfiguration();
            QTC_ASSERT(tc, return);
            result << tc;
        } else if (child->type() == GroupNode) {
            child->forFirstLevelChildren([&result](TestTreeItem *groupChild) {
                TestConfiguration *tc = groupChild->testConfiguration();
                QTC_ASSERT(tc, return);
                result << tc;
            });
        }
    });
    return result;
}

QList<TestConfiguration *> QtTestTreeItem::getSelectedTestConfigurations() const
{
    QList<TestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    for (int row = 0, count = childCount(); row < count; ++row)
        fillTestConfigurationsFromCheckState(childAt(row), result);

    return result;
}

QList<TestConfiguration *> QtTestTreeItem::getTestConfigurationsForFile(const Utils::FileName &fileName) const
{
    QList<TestConfiguration *> result;

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    QHash<TestTreeItem *, QStringList> testFunctions;
    const QString &file = fileName.toString();
    forAllChildren([&testFunctions, &file](TestTreeItem *node) {
        if (node->type() == Type::TestFunctionOrSet && node->filePath() == file) {
            QTC_ASSERT(node->parentItem(), return);
            TestTreeItem *testCase = node->parentItem();
            QTC_ASSERT(testCase->type() == Type::TestCase, return);
            testFunctions[testCase] << node->name();
        }
    });

    for (auto it = testFunctions.cbegin(), end = testFunctions.cend(); it != end; ++it) {
        TestConfiguration *tc = it.key()->testConfiguration();
        QTC_ASSERT(tc, continue);
        tc->setTestCases(it.value());
        result << tc;
    }

    return result;
}

TestTreeItem *QtTestTreeItem::find(const TestParseResult *result)
{
    QTC_ASSERT(result, return nullptr);

    switch (type()) {
    case Root:
        if (TestFrameworkManager::instance()->groupingEnabled(result->frameworkId)) {
            const QString path = QFileInfo(result->fileName).absolutePath();
            for (int row = 0; row < childCount(); ++row) {
                TestTreeItem *group = childAt(row);
                if (group->filePath() != path)
                    continue;
                if (auto groupChild = group->findChildByFile(result->fileName))
                    return groupChild;
            }
            return nullptr;
        }
        return findChildByFile(result->fileName);
    case GroupNode:
        return findChildByFile(result->fileName);
    case TestCase: {
        const QtTestParseResult *qtResult = static_cast<const QtTestParseResult *>(result);
        return findChildByNameAndInheritance(qtResult->displayName, qtResult->inherited());
    }
    case TestFunctionOrSet:
    case TestDataFunction:
    case TestSpecialFunction:
        return findChildByName(result->name);
    default:
        return nullptr;
    }
}

TestTreeItem *QtTestTreeItem::findChild(const TestTreeItem *other)
{
    QTC_ASSERT(other, return nullptr);
    const Type otherType = other->type();
    switch (type()) {
    case Root:
        return findChildByFileAndType(other->filePath(), otherType);
    case GroupNode:
        return otherType == TestCase ? findChildByFile(other->filePath()) : nullptr;
    case TestCase: {
        if (otherType != TestFunctionOrSet && otherType != TestDataFunction && otherType != TestSpecialFunction)
            return nullptr;
        auto qtOther = static_cast<const QtTestTreeItem *>(other);
        return findChildByNameAndInheritance(other->filePath(), qtOther->inherited());
    }
    case TestFunctionOrSet:
    case TestDataFunction:
    case TestSpecialFunction:
        return otherType == TestDataTag ? findChildByName(other->name()) : nullptr;
    default:
        return nullptr;
    }
}

bool QtTestTreeItem::modify(const TestParseResult *result)
{
    QTC_ASSERT(result, return false);

    switch (type()) {
    case TestCase:
        return modifyTestCaseContent(result);
    case TestFunctionOrSet:
    case TestDataFunction:
    case TestSpecialFunction:
        return modifyTestFunctionContent(result);
    case TestDataTag:
        return modifyDataTagContent(result);
    default:
        return false;
    }
}

TestTreeItem *QtTestTreeItem::createParentGroupNode() const
{
    const QFileInfo fileInfo(filePath());
    const QFileInfo base(fileInfo.absolutePath());
    return new QtTestTreeItem(base.baseName(), fileInfo.absolutePath(), TestTreeItem::GroupNode);
}

bool QtTestTreeItem::isGroupable() const
{
    return type() == TestCase;
}

TestTreeItem *QtTestTreeItem::findChildByNameAndInheritance(const QString &name, bool inherited) const
{
    return findFirstLevelChild([name, inherited](const TestTreeItem *other) {
        const QtTestTreeItem *qtOther = static_cast<const QtTestTreeItem *>(other);
        return qtOther->inherited() == inherited && qtOther->name() == name;
    });
}

QString QtTestTreeItem::nameSuffix() const
{
    static QString inheritedSuffix = QString(" [")
                + QCoreApplication::translate("QtTestTreeItem", "inherited")
                + QString("]");
    return m_inherited ? inheritedSuffix : QString();
}

} // namespace Internal
} // namespace Autotest
