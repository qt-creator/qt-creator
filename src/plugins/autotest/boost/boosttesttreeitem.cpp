// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "boosttesttreeitem.h"

#include "boosttestconstants.h"
#include "boosttestconfiguration.h"
#include "boosttestparser.h"

#include "../autotesttr.h"
#include "../itestframework.h"

#include <cppeditor/cppmodelmanager.h>

#include <projectexplorer/projectmanager.h>

#include <utils/qtcassert.h>

#include <QRegularExpression>

using namespace Utils;

namespace Autotest {
namespace Internal {

TestTreeItem *BoostTestTreeItem::copyWithoutChildren()
{
    BoostTestTreeItem *copied = new BoostTestTreeItem(framework());
    copied->copyBasicDataFrom(this);
    copied->m_state = m_state;
    copied->m_fullName = m_fullName;
    return copied;
}

QVariant BoostTestTreeItem::data(int column, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (type() == Root)
            break;
        return QString(name() + nameSuffix());
    case Qt::CheckStateRole:
        return checked();
    case ItalicRole:
        return false;
    case EnabledRole:
        return enabled();
    default:
        break;
    }
    return TestTreeItem::data(column, role);
}

TestTreeItem *BoostTestTreeItem::find(const TestParseResult *result)
{
    QTC_ASSERT(result, return nullptr);

    const BoostTestParseResult *bResult = static_cast<const BoostTestParseResult *>(result);

    switch (type()) {
    case Root:
        if (result->framework->grouping()) {
            for (int row = 0; row < childCount(); ++row) {
                BoostTestTreeItem *group = static_cast<BoostTestTreeItem *>(childAt(row));
                if (group->filePath() != bResult->fileName.absoluteFilePath())
                    continue;
                if (auto groupChild = group->findChildByNameStateAndFile(
                            bResult->name, bResult->state, bResult->proFile)) {
                    return groupChild;
                }
            }
        }
        return findChildByNameStateAndFile(bResult->name, bResult->state, bResult->proFile);
    case GroupNode:
    case TestSuite:
        return findChildByNameStateAndFile(bResult->name, bResult->state, bResult->proFile);
    default:
        return nullptr;
    }
}

TestTreeItem *BoostTestTreeItem::findChild(const TestTreeItem *other)
{
    QTC_ASSERT(other, return nullptr);
    const Type otherType = other->type();

    switch (type()) {
    case Root: {
        TestTreeItem *result = nullptr;
        if (otherType == GroupNode) {
            result = findChildByNameAndFile(other->name(), other->filePath());
        } else if (otherType == TestSuite) {
            auto bOther = static_cast<const BoostTestTreeItem *>(other);
            result = findChildByNameStateAndFile(bOther->name(), bOther->state(),
                                                 bOther->proFile());
        }
        return (result && result->type() == otherType) ? result : nullptr;
    }
    case GroupNode: {
        auto bOther = static_cast<const BoostTestTreeItem *>(other);
        return otherType == TestSuite
                ? findChildByNameStateAndFile(bOther->name(), bOther->state(), bOther->proFile())
                : nullptr;
    }
    case TestSuite: {
        if (otherType == TestCase) {
            return findChildByNameAndFile(other->name(), other->filePath());
        } else if (otherType == TestSuite) {
            auto bOther = static_cast<const BoostTestTreeItem *>(other);
            return findChildByNameStateAndFile(other->name(), bOther->state(), other->proFile());
        } else {
            return nullptr;
        }
    }
    default:
        return nullptr;
    }
}

bool BoostTestTreeItem::modify(const TestParseResult *result)
{
    QTC_ASSERT(result, return false);
    return (type() == TestCase || type() == TestSuite)
            ? modifyTestContent(static_cast<const BoostTestParseResult *>(result))
            : false;
}

TestTreeItem *BoostTestTreeItem::createParentGroupNode() const
{
    const FilePath &absPath = filePath().absolutePath();
    return new BoostTestTreeItem(framework(), absPath.baseName(), absPath, TestTreeItem::GroupNode);
}

QString BoostTestTreeItem::prependWithParentsSuitePaths(const QString &testName) const
{
    QString prepend = type() == TestSuite ? m_fullName.left(m_fullName.lastIndexOf('/'))
                                          : m_fullName.left(m_fullName.indexOf("::"));
    if (prepend.startsWith(BoostTest::Constants::BOOST_MASTER_SUITE))
        prepend = prepend.mid(QString(BoostTest::Constants::BOOST_MASTER_SUITE).length());

    return prepend + '/' + testName;
}

static QString handleSpecialFunctionNames(const QString &name)
{
    static const QRegularExpression function(".*\\((.*),.*\\)");
    const QRegularExpressionMatch match = function.match(name);
    if (!match.hasMatch())
        return name;
    QString result = match.captured(1);
    int index = result.lastIndexOf(':');
    if (index != -1)
        result = result.mid(index + 1);
    result.prepend('*').append('*');
    return result;
}

QList<ITestConfiguration *> BoostTestTreeItem::getAllTestConfigurations() const
{
    QList<ITestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (!project || type() != Root)
        return result;

    struct BoostTestCases {
        int testCases;
        QSet<QString> internalTargets;
    };

    // we only need the unique project files (and number of test cases for the progress indicator)
    QHash<FilePath, BoostTestCases> testsPerProjectfile;
    forAllChildItems([&testsPerProjectfile](TestTreeItem *item){
        if (item->type() != TestSuite)
            return;
        int funcChildren = 0;
        item->forAllChildItems([&funcChildren](TestTreeItem *child){
            if (child->type() == TestCase)
                ++funcChildren;
        });
        if (funcChildren) {
            testsPerProjectfile[item->proFile()].testCases += funcChildren;
            testsPerProjectfile[item->proFile()].internalTargets.unite(
                CppEditor::CppModelManager::internalTargets(item->filePath()));
        }
    });

    for (auto it = testsPerProjectfile.begin(), end = testsPerProjectfile.end(); it != end; ++it) {
        for (const QString &target : std::as_const(it.value().internalTargets)) {
            BoostTestConfiguration *config = new BoostTestConfiguration(framework());
            config->setProject(project);
            config->setProjectFile(it.key());
            config->setTestCaseCount(it.value().testCases);
            config->setInternalTarget(target);
            result.append(config);
        }
    }
    return result;
}

QList<ITestConfiguration *> BoostTestTreeItem::getTestConfigurations(
        std::function<bool(BoostTestTreeItem *)> predicate) const
{
    QList<ITestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (!project || type() != Root)
        return result;

    struct BoostTestCases {
        QStringList testCases;
        QSet<QString> internalTargets;
    };

    QHash<FilePath, BoostTestCases> testCasesForProjectFile;
    forAllChildren([&testCasesForProjectFile, &predicate](TreeItem *it){
        auto item = static_cast<BoostTestTreeItem *>(it);
        if (item->type() != TestCase)
            return;
        if (!item->enabled()) // ignore child tests known to be disabled when using run selected
            return;
        if (predicate(item)) {
            QString tcName = item->name();
            if (item->state().testFlag(BoostTestTreeItem::Templated))
                tcName.append("<*");
            else if (item->state().testFlag(BoostTestTreeItem::Parameterized))
                tcName.append("_*");
            tcName = handleSpecialFunctionNames(tcName);
            testCasesForProjectFile[item->proFile()].testCases.append(
                        item->prependWithParentsSuitePaths(tcName));
            testCasesForProjectFile[item->proFile()].internalTargets.unite(
                CppEditor::CppModelManager::internalTargets(item->filePath()));
        }
    });

    auto end = testCasesForProjectFile.cend();
    for (auto it = testCasesForProjectFile.cbegin(); it != end; ++it) {
        for (const QString &target : it.value().internalTargets) {
            BoostTestConfiguration *config = new BoostTestConfiguration(framework());
            config->setProject(project);
            config->setProjectFile(it.key());
            config->setTestCases(it.value().testCases);
            config->setInternalTarget(target);
            result.append(config);
        }
    }
    return result;
}

QList<ITestConfiguration *> BoostTestTreeItem::getSelectedTestConfigurations() const
{
    return getTestConfigurations([](BoostTestTreeItem *it) {
        return it->checked() == Qt::Checked;
    });
}

QList<ITestConfiguration *> BoostTestTreeItem::getFailedTestConfigurations() const
{
    return getTestConfigurations([](BoostTestTreeItem *it) {
        return it->data(0, FailedRole).toBool();
    });
}

ITestConfiguration *BoostTestTreeItem::testConfiguration() const
{
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    QTC_ASSERT(project, return nullptr);

    const Type itemType = type();
    if (itemType == TestSuite || itemType == TestCase) {
        QStringList testCases;
        if (itemType == TestSuite) {
            forFirstLevelChildItems([&testCases](TestTreeItem *child) {
                QTC_ASSERT(child, return);
                if (auto boostItem = static_cast<BoostTestTreeItem *>(child)) {
                    if (boostItem->enabled()) {
                        QString tcName = handleSpecialFunctionNames(boostItem->name());
                        if (boostItem->type() == TestSuite) // execute everything below a suite
                            tcName.append("/*");
                        else if (boostItem->state().testFlag(BoostTestTreeItem::Parameterized))
                            tcName.append("_*");
                        else if (boostItem->state().testFlag(BoostTestTreeItem::Templated))
                            tcName.append("<*");
                        testCases.append(boostItem->prependWithParentsSuitePaths(tcName));
                    }
                }
            });
        } else {
            QString tcName = name();
            if (state().testFlag(BoostTestTreeItem::Templated))
                tcName.append("<*");
            else if (state().testFlag(BoostTestTreeItem::Parameterized))
                tcName.append("_*");
            testCases.append(prependWithParentsSuitePaths(handleSpecialFunctionNames(tcName)));
        }

        BoostTestConfiguration *config = new BoostTestConfiguration(framework());
        config->setProjectFile(proFile());
        config->setProject(project);
        config->setTestCases(testCases);
        config->setInternalTargets(CppEditor::CppModelManager::internalTargets(filePath()));
        return config;
    }
    return nullptr;
}

ITestConfiguration *BoostTestTreeItem::debugConfiguration() const
{
    BoostTestConfiguration *config = static_cast<BoostTestConfiguration *>(testConfiguration());
    if (config)
        config->setRunMode(TestRunMode::Debug);
    return config;
}

QString BoostTestTreeItem::nameSuffix() const
{
    static QString markups[] = {Tr::tr("parameterized"), Tr::tr("fixture"), Tr::tr("templated")};

    QString suffix;
    if (m_state & Parameterized)
        suffix = QString(" [") + markups[0];
    if (m_state & Fixture)
        suffix += (suffix.isEmpty() ? QString(" [") : QString(", ")) + markups[1];
    if (m_state & Templated)
        suffix += (suffix.isEmpty() ? QString(" [") : QString(", ")) + markups[2];
    if (!suffix.isEmpty())
        suffix += ']';
    return suffix;
}

bool BoostTestTreeItem::enabled() const
{
    if (m_state & ExplicitlyEnabled)
        return true;

    if (m_state & Disabled)
        return false;

    if (type() == Root)
        return true;

    const TestTreeItem *parent = parentItem();
    if (parent && parent->type() == TestSuite) // take test suites into account
        return static_cast<const BoostTestTreeItem *>(parent)->enabled();

    return true;
}

TestTreeItem *BoostTestTreeItem::findChildByNameStateAndFile(const QString &name,
                                                             BoostTestTreeItem::TestStates state,
                                                             const FilePath &proFile) const
{
    return static_cast<TestTreeItem *>(
                findAnyChild([name, state, proFile](const TreeItem *other){
        const BoostTestTreeItem *boostItem = static_cast<const BoostTestTreeItem *>(other);
        return boostItem->proFile() == proFile && boostItem->fullName() == name
                && boostItem->state() == state;
    }));
}

bool BoostTestTreeItem::modifyTestContent(const BoostTestParseResult *result)
{
    bool hasBeenModified = modifyLineAndColumn(result);

    if (m_state != result->state) {
        m_state = result->state;
        hasBeenModified = true;
    }
    if (m_fullName != result->name) {
        m_fullName = result->name;
        hasBeenModified = true;
    }
    return hasBeenModified;
}

} // namespace Internal
} // namespace Autotest
