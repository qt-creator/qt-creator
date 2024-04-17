// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qttesttreeitem.h"

#include "qttestconfiguration.h"
#include "qttestparser.h"
#include "../autotesttr.h"
#include "../itestframework.h"

#include <cplusplus/Symbol.h>
#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/symbolfinder.h>

#include <projectexplorer/projectmanager.h>

#include <utils/qtcassert.h>

using namespace Utils;

namespace Autotest::Internal {

QtTestTreeItem::QtTestTreeItem(ITestFramework *testFramework, const QString &name,
                               const FilePath &filePath, TestTreeItem::Type type)
    : TestTreeItem(testFramework, name, filePath, type)
{
    if (type == TestDataTag)
        setData(0, Qt::Checked, Qt::CheckStateRole);
}

TestTreeItem *QtTestTreeItem::copyWithoutChildren()
{
    QtTestTreeItem *copied = new QtTestTreeItem(framework());
    copied->copyBasicDataFrom(this);
    copied->m_inherited = m_inherited;
    copied->m_multiTest = m_multiTest;
    return copied;
}

QVariant QtTestTreeItem::data(int column, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (type() == Root)
            break;
        return QVariant(name() + nameSuffix());
    case Qt::ToolTipRole: {
        QString toolTip = TestTreeItem::data(column, role).toString();
        if (m_multiTest && type() == TestCase) {
            toolTip.append(
                "<p>"
                + Tr::tr("Multiple testcases inside a single executable are not officially "
                         "supported. Depending on the implementation they might get executed "
                         "or not, but never will be explicitly selectable.")
                + "</p>");
        } else  if (type() == TestFunction) {
            // avoid confusion (displaying header file, but ending up inside source)
            toolTip = parentItem()->name() + "::" + name();
        }
        return toolTip;
    }
    case Qt::CheckStateRole:
        switch (type()) {
        case TestDataFunction:
        case TestSpecialFunction:
            return QVariant();
        default:
            return m_multiTest ? QVariant() : checked();
        }
    case ItalicRole:
        switch (type()) {
        case TestDataFunction:
        case TestSpecialFunction:
            return true;
        default:
            return m_multiTest;
        }
    case LinkRole:
        if (type() == GroupNode || type() == Root)
            return QVariant();
        if (type() == TestDataFunction || type() == TestDataTag)
            return TestTreeItem::data(column, role);
        // other functions would end up inside declaration - so, find its definition
        return linkForTreeItem();
    }
    return TestTreeItem::data(column, role);
}

Qt::ItemFlags QtTestTreeItem::flags(int column) const
{
    static const Qt::ItemFlags defaultFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    switch (type()) {
    case TestDataTag:
        return defaultFlags | Qt::ItemIsUserCheckable;
    case TestFunction:
        return defaultFlags | Qt::ItemIsAutoTristate | Qt::ItemIsUserCheckable;
    default:
        return m_multiTest ? Qt::ItemIsEnabled | Qt::ItemIsSelectable
                           : TestTreeItem::flags(column);
    }
}

Qt::CheckState QtTestTreeItem::checked() const
{
    switch (type()) {
    case TestDataFunction:
    case TestSpecialFunction:
        return Qt::Unchecked;
    default:
        break;
    }
    return m_multiTest ? Qt::Unchecked : TestTreeItem::checked();
}

bool QtTestTreeItem::canProvideTestConfiguration() const
{
    switch (type()) {
    case TestCase:
    case TestFunction:
    case TestDataTag:
        return !m_multiTest;
    default:
        return false;
    }
}

bool QtTestTreeItem::canProvideDebugConfiguration() const
{
    return canProvideTestConfiguration();
}

ITestConfiguration *QtTestTreeItem::testConfiguration() const
{
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    QTC_ASSERT(project, return nullptr);

    QtTestConfiguration *config = nullptr;
    switch (type()) {
    case TestCase:
        config = new QtTestConfiguration(framework());
        config->setTestCaseCount(childCount());
        config->setProjectFile(proFile());
        config->setProject(project);
        break;
    case TestFunction: {
        TestTreeItem *parent = parentItem();
        config = new QtTestConfiguration(framework());
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
        config = new QtTestConfiguration(framework());
        config->setTestCases(QStringList(functionWithTag));
        config->setProjectFile(parent->proFile());
        config->setProject(project);
        break;
    }
    default:
        return nullptr;
    }
    if (config)
        config->setInternalTargets(CppEditor::CppModelManager::internalTargets(filePath()));
    return config;
}

struct FunctionLocation {
    FunctionLocation(const QString &n, const Link &l, std::optional<Link> r = std::nullopt)
        : name(n), declaration(l), registration(r) {}
    QString name;
    Link declaration;
    std::optional<Link> registration;
};

static QStringList orderedTestCases(const QList<FunctionLocation> &original)
{
    auto compare = [](const Link &lhs, const Link &rhs) {
        if (lhs.targetLine == rhs.targetLine)
            return lhs.targetColumn - rhs.targetColumn;
        return lhs.targetLine - rhs.targetLine;
    };

    QList<FunctionLocation> locations = original;
    Utils::sort(locations, [compare](const FunctionLocation &lhs, const FunctionLocation &rhs) {
        if (lhs.declaration.targetFilePath != rhs.declaration.targetFilePath)
            return lhs.declaration.targetFilePath < rhs.declaration.targetFilePath;
        if (auto diff = compare(lhs.declaration, rhs.declaration))
            return diff < 0;
        return compare(lhs.registration.value_or(Link{{}, INT_MAX, INT_MAX}),
                       rhs.registration.value_or(Link{{}, INT_MAX, INT_MAX})) < 0;
    });
    return Utils::transform(locations, [](const FunctionLocation &loc) { return loc.name; });
}

static void fillTestConfigurationsFromCheckState(const TestTreeItem *item,
                                                 QList<ITestConfiguration *> &testConfigurations)
{
    QTC_ASSERT(item, return);
    if (item->type() == TestTreeItem::GroupNode) {
        for (int row = 0, count = item->childCount(); row < count; ++row)
            fillTestConfigurationsFromCheckState(item->childItem(row), testConfigurations);
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
        // we need the location for keeping the order while running
        // normal test function: declaration, data tag: location of registration
        QList<FunctionLocation> testCases;
        item->forFirstLevelChildren([&testCases](ITestTreeItem *grandChild) {
            if (grandChild->checked() == Qt::Checked) {
                TestTreeItem *funcItem = static_cast<TestTreeItem *>(grandChild);
                const Link link(funcItem->filePath(), funcItem->line(), funcItem->column());
                testCases << FunctionLocation(funcItem->name(), link);
            } else if (grandChild->checked() == Qt::PartiallyChecked) {
                TestTreeItem *funcItem = static_cast<TestTreeItem *>(grandChild);
                grandChild->forFirstLevelChildren([&testCases, funcItem](ITestTreeItem *dataTag) {
                    if (dataTag->checked() == Qt::Checked) {
                        TestTreeItem *tagItem = static_cast<TestTreeItem *>(dataTag);
                        const Link decl(funcItem->filePath(), funcItem->line(), funcItem->column());
                        const Link reg(tagItem->filePath(), tagItem->line(), tagItem->column());
                        testCases << FunctionLocation(funcItem->name() + ':' + tagItem->name(),
                                                      decl, reg);
                    }
                });
            }
        });

        testConfig = new QtTestConfiguration(item->framework());
        testConfig->setTestCases(orderedTestCases(testCases));
        testConfig->setProjectFile(item->proFile());
        testConfig->setProject(ProjectExplorer::ProjectManager::startupProject());
        testConfig->setInternalTargets(
            CppEditor::CppModelManager::internalTargets(item->filePath()));
        testConfigurations << testConfig;
    }
}

static void collectFailedTestInfo(TestTreeItem *item, QList<ITestConfiguration *> &testConfigs)
{
    QTC_ASSERT(item, return);
    if (item->type() == TestTreeItem::GroupNode) {
        for (int row = 0, count = item->childCount(); row < count; ++row)
            collectFailedTestInfo(item->childItem(row), testConfigs);
        return;
    }
    QTC_ASSERT(item->type() == TestTreeItem::TestCase, return);
    // we need the location for keeping the order while running
    // normal test function: declaration, data tag: location of registration
    QList<FunctionLocation> testCases;
    item->forFirstLevelChildren([&testCases](ITestTreeItem *func) {
        if (func->type() == TestTreeItem::TestFunction && func->data(0, FailedRole).toBool()) {
            TestTreeItem *funcItem = static_cast<TestTreeItem *>(func);
            const Link link(funcItem->filePath(), funcItem->line(), funcItem->column());
            testCases << FunctionLocation(func->name(), link);
        } else {
            TestTreeItem *funcItem = static_cast<TestTreeItem *>(func);
            func->forFirstLevelChildren([&testCases, funcItem](ITestTreeItem *dataTag) {
                if (dataTag->data(0, FailedRole).toBool()) {
                    TestTreeItem *tagItem = static_cast<TestTreeItem *>(dataTag);
                    const Link decl(funcItem->filePath(), funcItem->line(), funcItem->column());
                    const Link reg(tagItem->filePath(), tagItem->line(), tagItem->column());
                    testCases << FunctionLocation(funcItem->name() + ':' + dataTag->name(),
                                                  decl, reg);
                }
            });
        }
    });
    if (testCases.isEmpty())
        return;

    QtTestConfiguration *testConfig = new QtTestConfiguration(item->framework());
    testConfig->setTestCases(orderedTestCases(testCases));
    testConfig->setProjectFile(item->proFile());
    testConfig->setProject(ProjectExplorer::ProjectManager::startupProject());
    testConfig->setInternalTargets(
        CppEditor::CppModelManager::internalTargets(item->filePath()));
    testConfigs << testConfig;
}

ITestConfiguration *QtTestTreeItem::debugConfiguration() const
{
    QtTestConfiguration *config = static_cast<QtTestConfiguration *>(testConfiguration());
    if (config)
        config->setRunMode(TestRunMode::Debug);
    return config;
}

QList<ITestConfiguration *> QtTestTreeItem::getAllTestConfigurations() const
{
    QList<ITestConfiguration *> result;

    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (!project || type() != Root)
        return result;

    forFirstLevelChildren([&result](ITestTreeItem *child) {
        if (child->type() == TestCase) {
            ITestConfiguration *tc = child->testConfiguration();
            QTC_ASSERT(tc, return);
            result << tc;
        } else if (child->type() == GroupNode) {
            child->forFirstLevelChildren([&result](ITestTreeItem *groupChild) {
                ITestConfiguration *tc = groupChild->testConfiguration();
                QTC_ASSERT(tc, return);
                result << tc;
            });
        }
    });
    return result;
}

QList<ITestConfiguration *> QtTestTreeItem::getSelectedTestConfigurations() const
{
    QList<ITestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (!project || type() != Root)
        return result;

    for (int row = 0, count = childCount(); row < count; ++row)
        fillTestConfigurationsFromCheckState(childItem(row), result);

    return result;
}

QList<ITestConfiguration *> QtTestTreeItem::getFailedTestConfigurations() const
{
    QList<ITestConfiguration *> result;
    QTC_ASSERT(type() == TestTreeItem::Root, return result);
    for (int row = 0, end = childCount(); row < end; ++row)
        collectFailedTestInfo(childItem(row), result);
    return result;
}

QList<ITestConfiguration *> QtTestTreeItem::getTestConfigurationsForFile(const FilePath &fileName) const
{
    QList<ITestConfiguration *> result;

    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (!project || type() != Root)
        return result;

    QHash<TestTreeItem *, QList<FunctionLocation>> testFunctions;
    forAllChildItems([&testFunctions, &fileName](TestTreeItem *node) {
        if (node->type() == Type::TestFunction && node->filePath() == fileName) {
            QTC_ASSERT(node->parentItem(), return);
            TestTreeItem *testCase = node->parentItem();
            QTC_ASSERT(testCase->type() == Type::TestCase, return);

            // we need the declaration location for keeping the order while running
            const Link link(node->filePath(), node->line(), node->column());
            testFunctions[testCase] << FunctionLocation(node->name(), link);
        }
    });

    for (auto it = testFunctions.cbegin(), end = testFunctions.cend(); it != end; ++it) {
        TestConfiguration *tc = static_cast<TestConfiguration *>(it.key()->testConfiguration());
        QTC_ASSERT(tc, continue);
        tc->setTestCases(orderedTestCases(it.value()));
        result << tc;
    }

    return result;
}

TestTreeItem *QtTestTreeItem::find(const TestParseResult *result)
{
    QTC_ASSERT(result, return nullptr);

    switch (type()) {
    case Root:
        if (result->framework->grouping()) {
            const FilePath path = result->fileName.absolutePath();
            for (int row = 0; row < childCount(); ++row) {
                TestTreeItem *group = childItem(row);
                if (group->filePath() != path)
                    continue;
                if (auto groupChild = group->findChildByFile(result->fileName))
                    return groupChild;
            }
            return nullptr;
        }
        return findChildByNameAndFile(result->name, result->fileName);
    case GroupNode:
        return findChildByNameAndFile(result->name, result->fileName);
    case TestCase: {
        const QtTestParseResult *qtResult = static_cast<const QtTestParseResult *>(result);
        return findChildByNameAndInheritanceAndMultiTest(qtResult->displayName,
                                                         qtResult->inherited(),
                                                         qtResult->runsMultipleTestcases());
    }
    case TestFunction:
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
        return findChildByFileNameAndType(other->filePath(), other->name(), otherType);
    case GroupNode:
        return otherType == TestCase ? findChildByNameAndFile(other->name(), other->filePath()) : nullptr;
    case TestCase: {
        if (otherType != TestFunction && otherType != TestDataFunction && otherType != TestSpecialFunction)
            return nullptr;
        auto qtOther = static_cast<const QtTestTreeItem *>(other);
        return findChildByNameAndInheritanceAndMultiTest(other->name(), qtOther->inherited(),
                                                         qtOther->runsMultipleTestcases());
    }
    case TestFunction:
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
        return modifyTestCaseOrSuiteContent(result);
    case TestFunction:
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
    const FilePath &absPath = filePath().absolutePath();
    return new QtTestTreeItem(framework(), absPath.baseName(), absPath, TestTreeItem::GroupNode);
}

bool QtTestTreeItem::isGroupable() const
{
    return type() == TestCase;
}

QVariant QtTestTreeItem::linkForTreeItem() const
{
    QVariant itemLink;
    using namespace CPlusPlus;
    const Snapshot snapshot = CppEditor::CppModelManager::instance()->snapshot();
    const Document::Ptr doc = snapshot.document(filePath());
    Symbol *symbol = doc->lastVisibleSymbolAt(line(), this->column() + 1);
    if (auto decl = symbol->asDeclaration()) {
        static CppEditor::SymbolFinder symbolFinder;
        if (Symbol *definition = symbolFinder.findMatchingDefinition(decl, snapshot, true);
                definition && definition->fileId()) {
            itemLink.setValue(Link(FilePath::fromUtf8(definition->fileName()),
                                   definition->line(), definition->column() - 1));
        }
    }
    if (!itemLink.isValid()) // fallback in case we failed to find the definition
        itemLink.setValue(Link(filePath(), line(), this->column()));
    return itemLink;
}

TestTreeItem *QtTestTreeItem::findChildByFileNameAndType(const FilePath &file,
                                                         const QString &name, Type type) const
{
    return findFirstLevelChildItem([file, name, type](const TestTreeItem *other) {
        return other->type() == type && other->filePath() == file && other->name() == name;
    });
}

TestTreeItem *QtTestTreeItem::findChildByNameAndInheritanceAndMultiTest(const QString &name,
                                                                        bool inherited,
                                                                        bool multiTest) const
{
    return findFirstLevelChildItem([name, inherited, multiTest](const TestTreeItem *other) {
        const QtTestTreeItem *qtOther = static_cast<const QtTestTreeItem *>(other);
        return qtOther->inherited() == inherited && qtOther->runsMultipleTestcases() == multiTest
                && qtOther->name() == name;
    });
}

QString QtTestTreeItem::nameSuffix() const
{
    static const QString inherited = Tr::tr("inherited");
    static const QString multi = Tr::tr("multiple testcases");

    QString suffix;
    if (m_inherited)
        suffix.append(inherited);
    if (m_multiTest && type() == TestCase) {
        if (m_inherited)
            suffix.append(", ");
        suffix.append(multi);
    }
    return suffix.isEmpty() ? suffix : QString{" [" + suffix + "]"};
}

} // namespace Autotest::Internal
