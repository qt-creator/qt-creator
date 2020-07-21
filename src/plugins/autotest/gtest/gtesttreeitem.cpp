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

#include "gtesttreeitem.h"
#include "gtestconfiguration.h"
#include "gtestconstants.h"
#include "gtestframework.h"
#include "gtestparser.h"
#include "../testframeworkmanager.h"

#include <cpptools/cppmodelmanager.h>
#include <projectexplorer/session.h>

#include <utils/algorithm.h>
#include <utils/icon.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>

#include <QRegularExpression>

namespace Autotest {
namespace Internal {

static QString matchingString()
{
    return QCoreApplication::translate("GTestTreeItem", "<matching>");
}

static QString notMatchingString()
{
    return QCoreApplication::translate("GTestTreeItem", "<not matching>");
}

static QString gtestFilter(GTestTreeItem::TestStates states)
{
    if ((states & GTestTreeItem::Parameterized) && (states & GTestTreeItem::Typed))
        return QString("*/%1/*.%2");
    if (states & GTestTreeItem::Parameterized)
        return QString("*/%1.%2/*");
    if (states & GTestTreeItem::Typed)
        return QString("%1/*.%2");
    return QString("%1.%2");
}

TestTreeItem *GTestTreeItem::copyWithoutChildren()
{
    GTestTreeItem *copied = new GTestTreeItem(framework());
    copied->copyBasicDataFrom(this);
    copied->m_state = m_state;
    return copied;
}

static QString wildCardPattern(const QString &original)
{
    QString pattern = original;
    pattern.replace('.', "\\.");
    pattern.replace('$', "\\$");
    pattern.replace('(', "\\(").replace(')', "\\)");
    pattern.replace('[', "\\[").replace(']', "\\]");
    pattern.replace('{', "\\{").replace('}', "\\}");
    pattern.replace('+', "\\+");
    pattern.replace('*', ".*");
    pattern.replace('?', '.');
    return pattern;
}

static bool matchesFilter(const QString &filter, const QString &fullTestName)
{
    QStringList positive;
    QStringList negative;
    int startOfNegative = filter.indexOf('-');
    if (startOfNegative == -1) {
        positive.append(filter.split(':', Qt::SkipEmptyParts));
    } else {
        positive.append(filter.left(startOfNegative).split(':', Qt::SkipEmptyParts));
        negative.append(filter.mid(startOfNegative + 1).split(':', Qt::SkipEmptyParts));
    }

    QString testName = fullTestName;
    if (!testName.contains('.'))
        testName.append('.');

    for (const QString &curr : negative) {
        QRegularExpression regex(wildCardPattern(curr));
        if (regex.match(testName).hasMatch())
            return false;
    }
    for (const QString &curr : positive) {
        QRegularExpression regex(wildCardPattern(curr));
        if (regex.match(testName).hasMatch())
            return true;
    }
    return positive.isEmpty();
}

QVariant GTestTreeItem::data(int column, int role) const
{
    switch (role) {
    case Qt::DisplayRole: {
        if (type() == TestTreeItem::Root)
            break;

        const QString &displayName = (m_state & Disabled) ? name().mid(9) : name();
        return QVariant(displayName + nameSuffix());
    }
    case Qt::DecorationRole:
        if (type() == GroupNode
                && GTestFramework::groupMode() == GTest::Constants::GTestFilter) {
            static const QIcon filterIcon = Utils::Icon({{":/utils/images/filtericon.png",
                                                          Utils::Theme::PanelTextColorMid}}).icon();
            return filterIcon;
        }
        break;
    case Qt::ToolTipRole:
        if (type() == GroupNode
                && GTestFramework::groupMode() == GTest::Constants::GTestFilter) {
            const auto tpl = QString("<p>%1</p><p>%2</p>").arg(filePath());
            return tpl.arg(QCoreApplication::translate(
                               "GTestTreeItem", "Change GTest filter in use inside the settings."));
        }
        break;
    case Qt::CheckStateRole:
        switch (type()) {
        case Root:
        case GroupNode:
        case TestSuite:
        case TestCase:
            return checked();
        default:
            return QVariant();
        }
    case ItalicRole:
        return false;
    case EnabledRole:
        return !(m_state & Disabled);
    default:
        break;
    }
    return TestTreeItem::data(column, role);
}

TestConfiguration *GTestTreeItem::testConfiguration() const
{
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    QTC_ASSERT(project, return nullptr);

    GTestConfiguration *config = nullptr;
    switch (type()) {
    case TestSuite: {
        const QString &testSpecifier = gtestFilter(state()).arg(name()).arg('*');
        if (int count = childCount()) {
            config = new GTestConfiguration(framework());
            config->setTestCases(QStringList(testSpecifier));
            config->setTestCaseCount(count);
            config->setProjectFile(proFile());
            config->setProject(project);
        }
        break;
    }
    case TestCase: {
        GTestTreeItem *parent = static_cast<GTestTreeItem *>(parentItem());
        if (!parent)
            return nullptr;
        const QString &testSpecifier = gtestFilter(parent->state()).arg(parent->name()).arg(name());
        config = new GTestConfiguration(framework());
        config->setTestCases(QStringList(testSpecifier));
        config->setProjectFile(proFile());
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

TestConfiguration *GTestTreeItem::debugConfiguration() const
{
    GTestConfiguration *config = static_cast<GTestConfiguration *>(testConfiguration());
    if (config)
        config->setRunMode(TestRunMode::Debug);
    return config;
}

struct GTestCases
{
    QStringList filters;
    int testSetCount = 0;
    QSet<QString> internalTargets;
};

static void collectTestInfo(const GTestTreeItem *item,
                            QHash<QString, GTestCases> &testCasesForProFile,
                            bool ignoreCheckState)
{
    QTC_ASSERT(item, return);
    if (item->type() == TestTreeItem::GroupNode) {
        for (int row = 0, count = item->childCount(); row < count; ++row) {
            auto child = static_cast<const GTestTreeItem *>(item->childAt(row));
            collectTestInfo(child, testCasesForProFile, ignoreCheckState);
        }
        return;
    }
    const int childCount = item->childCount();
    QTC_ASSERT(childCount != 0, return);
    QTC_ASSERT(item->type() == TestTreeItem::TestSuite, return);
    if (ignoreCheckState || item->checked() == Qt::Checked) {
        const QString &projectFile = item->childAt(0)->proFile();
        testCasesForProFile[projectFile].filters.append(
                    gtestFilter(item->state()).arg(item->name()).arg('*'));
        testCasesForProFile[projectFile].testSetCount += childCount - 1;
        testCasesForProFile[projectFile].internalTargets.unite(item->internalTargets());
    } else if (item->checked() == Qt::PartiallyChecked) {
        item->forFirstLevelChildren([&testCasesForProFile, item](TestTreeItem *child){
            QTC_ASSERT(child->type() == TestTreeItem::TestCase, return);
            if (child->checked() == Qt::Checked) {
                testCasesForProFile[child->proFile()].filters.append(
                            gtestFilter(item->state()).arg(item->name()).arg(child->name()));
                testCasesForProFile[child->proFile()].internalTargets.unite(
                            child->internalTargets());
            }
        });
    }
}

QList<TestConfiguration *> GTestTreeItem::getTestConfigurations(bool ignoreCheckState) const
{
    QList<TestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    QHash<QString, GTestCases> testCasesForProFile;
    for (int row = 0, count = childCount(); row < count; ++row) {
        auto child = static_cast<const GTestTreeItem *>(childAt(row));
        collectTestInfo(child, testCasesForProFile, ignoreCheckState);
    }

    for (auto it = testCasesForProFile.begin(), end = testCasesForProFile.end(); it != end; ++it) {
        for (const QString &target : qAsConst(it.value().internalTargets)) {
            GTestConfiguration *tc = new GTestConfiguration(framework());
            if (!ignoreCheckState)
                tc->setTestCases(it.value().filters);
            tc->setTestCaseCount(tc->testCaseCount() + it.value().testSetCount);
            tc->setProjectFile(it.key());
            tc->setProject(project);
            tc->setInternalTarget(target);
            result << tc;
        }
    }

    return result;
}

QList<TestConfiguration *> GTestTreeItem::getAllTestConfigurations() const
{
    return getTestConfigurations(true);
}

QList<TestConfiguration *> GTestTreeItem::getSelectedTestConfigurations() const
{
    return getTestConfigurations(false);
}

QList<TestConfiguration *> GTestTreeItem::getTestConfigurationsForFile(const Utils::FilePath &fileName) const
{
    QList<TestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    QHash<QString, GTestCases> testCases;
    const QString &file = fileName.toString();
    forAllChildren([&testCases, &file](TestTreeItem *node) {
        if (node->type() == Type::TestCase && node->filePath() == file) {
            QTC_ASSERT(node->parentItem(), return);
            const GTestTreeItem *testCase = static_cast<GTestTreeItem *>(node->parentItem());
            QTC_ASSERT(testCase->type() == Type::TestSuite, return);
            GTestCases &cases = testCases[testCase->proFile()];
            cases.filters.append(
                        gtestFilter(testCase->state()).arg(testCase->name(), node->name()));
            cases.internalTargets.unite(node->internalTargets());
        }
    });
    for (auto it = testCases.begin(), end = testCases.end(); it != end; ++it) {
        for (const QString &target : qAsConst(it.value().internalTargets)) {
            GTestConfiguration *tc = new GTestConfiguration(framework());
            tc->setTestCases(it.value().filters);
            tc->setProjectFile(it.key());
            tc->setProject(project);
            tc->setInternalTarget(target);
            result << tc;
        }
    }
    return result;
}

TestTreeItem *GTestTreeItem::find(const TestParseResult *result)
{
    QTC_ASSERT(result, return nullptr);

    const GTestParseResult *parseResult = static_cast<const GTestParseResult *>(result);
    GTestTreeItem::TestStates states = parseResult->disabled ? GTestTreeItem::Disabled
                                                             : GTestTreeItem::Enabled;
    if (parseResult->parameterized)
        states |= GTestTreeItem::Parameterized;
    if (parseResult->typed)
        states |= GTestTreeItem::Typed;
    switch (type()) {
    case Root:
        if (result->framework->grouping()) {
            if (GTestFramework::groupMode() == GTest::Constants::Directory) {
                const QFileInfo fileInfo(parseResult->fileName);
                const QFileInfo base(fileInfo.absolutePath());
                for (int row = 0; row < childCount(); ++row) {
                    GTestTreeItem *group = static_cast<GTestTreeItem *>(childAt(row));
                    if (group->filePath() != base.absoluteFilePath())
                        continue;
                    if (auto groupChild = group->findChildByNameStateAndFile(
                                parseResult->name, states,parseResult->proFile)) {
                        return groupChild;
                    }
                }
                return nullptr;
            } else { // GTestFilter
                QTC_ASSERT(parseResult->children.size(), return nullptr);
                auto fstChild = static_cast<const GTestParseResult *>(parseResult->children.at(0));
                bool matching = matchesFilter(GTestFramework::currentGTestFilter(),
                                              parseResult->name + '.' + fstChild->name);
                for (int row = 0; row < childCount(); ++row) {
                    GTestTreeItem *group = static_cast<GTestTreeItem *>(childAt(row));
                    if ((matching && group->name() == matchingString())
                            || (!matching && group->name() == notMatchingString())) {
                        if (auto groupChild = group->findChildByNameStateAndFile(
                                    parseResult->name, states, parseResult->proFile))
                            return groupChild;
                    }
                }
                return nullptr;
            }
        }
        return findChildByNameStateAndFile(parseResult->name, states, parseResult->proFile);
    case GroupNode:
        return findChildByNameStateAndFile(parseResult->name, states, parseResult->proFile);
    case TestSuite:
        return findChildByNameAndFile(result->name, result->fileName);
    default:
        return nullptr;
    }
}

TestTreeItem *GTestTreeItem::findChild(const TestTreeItem *other)
{
    QTC_ASSERT(other, return nullptr);
    const Type otherType = other->type();
    switch (type()) {
    case Root: {
        TestTreeItem *result = nullptr;
        if (otherType == GroupNode) {
            result = findChildByNameAndFile(other->name(), other->filePath());
        } else if (otherType == TestSuite) {
            auto gtOther = static_cast<const GTestTreeItem *>(other);
            result = findChildByNameStateAndFile(gtOther->name(), gtOther->state(),
                                                 gtOther->proFile());
        }
        return (result && result->type() == otherType) ? result : nullptr;
    }
    case GroupNode: {
        auto gtOther = static_cast<const GTestTreeItem *>(other);
        return otherType == TestSuite
                ? findChildByNameStateAndFile(gtOther->name(), gtOther->state(), gtOther->proFile())
                : nullptr;
    }
    case TestSuite:
        return otherType == TestCase
                ? findChildByNameAndFile(other->name(), other->filePath())
                : nullptr;
    default:
        return nullptr;
    }
}

bool GTestTreeItem::modify(const TestParseResult *result)
{
    QTC_ASSERT(result, return false);

    switch (type()) {
    case TestCase:
        return modifyTestSetContent(static_cast<const GTestParseResult *>(result));
    default:
        return false;
    }
}

TestTreeItem *GTestTreeItem::createParentGroupNode() const
{
    if (GTestFramework::groupMode() == GTest::Constants::Directory) {
        const QFileInfo fileInfo(filePath());
        const QFileInfo base(fileInfo.absolutePath());
        return new GTestTreeItem(framework(), base.baseName(), fileInfo.absolutePath(), TestTreeItem::GroupNode);
    } else { // GTestFilter
        QTC_ASSERT(childCount(), return nullptr); // paranoia
        const TestTreeItem *firstChild = childAt(0);
        const QString activeFilter = GTestFramework::currentGTestFilter();
        const QString fullTestName = name() + '.' + firstChild->name();
        const QString groupNodeName =
                matchesFilter(activeFilter, fullTestName) ? matchingString() : notMatchingString();
        auto groupNode = new GTestTreeItem(framework(), groupNodeName, activeFilter, TestTreeItem::GroupNode);
        if (groupNodeName == notMatchingString())
            groupNode->setData(0, Qt::Unchecked, Qt::CheckStateRole);
        return groupNode;
    }
}

bool GTestTreeItem::modifyTestSetContent(const GTestParseResult *result)
{
    bool hasBeenModified = modifyLineAndColumn(result);
    GTestTreeItem::TestStates states = result->disabled ? GTestTreeItem::Disabled
                                                        : GTestTreeItem::Enabled;
    if (m_state != states) {
        m_state = states;
        hasBeenModified = true;
    }
    return hasBeenModified;
}

TestTreeItem *GTestTreeItem::findChildByNameStateAndFile(const QString &name,
                                                         GTestTreeItem::TestStates state,
                                                         const QString &proFile) const
{
    return findFirstLevelChild([name, state, proFile](const TestTreeItem *other) {
        const GTestTreeItem *gtestItem = static_cast<const GTestTreeItem *>(other);
        return other->proFile() == proFile && other->name() == name && gtestItem->state() == state;
    });
}

QString GTestTreeItem::nameSuffix() const
{
    static QString markups[] = {QCoreApplication::translate("GTestTreeItem", "parameterized"),
                                QCoreApplication::translate("GTestTreeItem", "typed")};
    QString suffix;
    if (m_state & Parameterized)
        suffix =  QString(" [") + markups[0];
    if (m_state & Typed)
        suffix += (suffix.isEmpty() ? QString(" [") : QString(", ")) + markups[1];
    if (!suffix.isEmpty())
        suffix += ']';
    return suffix;
}

QSet<QString> GTestTreeItem::internalTargets() const
{
    QSet<QString> result;
    const auto cppMM = CppTools::CppModelManager::instance();
    const auto projectInfo = cppMM->projectInfo(ProjectExplorer::SessionManager::startupProject());
    const QString file = filePath();
    const QVector<CppTools::ProjectPart::Ptr> projectParts = projectInfo.projectParts();
    if (projectParts.isEmpty())
        return TestTreeItem::dependingInternalTargets(cppMM, file);
    for (const CppTools::ProjectPart::Ptr &projectPart : projectParts) {
        if (projectPart->projectFile == proFile()
                && Utils::anyOf(projectPart->files, [&file] (const CppTools::ProjectFile &pf) {
                                return pf.path == file;
        })) {
            result.insert(projectPart->buildSystemTarget);
            if (projectPart->buildTargetType != ProjectExplorer::BuildTargetType::Executable)
                result.unite(TestTreeItem::dependingInternalTargets(cppMM, file));
        }
    }
    return result;
}

bool GTestTreeItem::isGroupNodeFor(const TestTreeItem *other) const
{
    QTC_ASSERT(other, return false);
    if (type() != TestTreeItem::GroupNode)
        return false;

    if (GTestFramework::groupMode() == GTest::Constants::Directory) {
        return QFileInfo(other->filePath()).absolutePath() == filePath();
    } else { // GTestFilter
        QString fullName;
        if (other->type() == TestSuite) {
            fullName = other->name();
            if (other->childCount())
                fullName += '.' + other->childAt(0)->name();
        } else if (other->type() == TestCase) {
            QTC_ASSERT(other->parentItem(), return false);
            fullName = other->parentItem()->name() + '.' + other->name();
        } else if (other->type() == GroupNode) { // can happen on a rebuild if only filter changes
            return false;
        } else {
            QTC_ASSERT(false, return false);
        }
        if (GTestFramework::currentGTestFilter() != filePath()) // filter has changed in settings
            return false;
        bool matches = matchesFilter(filePath(), fullName);
        return (matches && name() == matchingString())
                || (!matches && name() == notMatchingString());
    }
}

bool GTestTreeItem::isGroupable() const
{
    return type() == TestSuite;
}

TestTreeItem *GTestTreeItem::applyFilters()
{
    if (type() != TestSuite)
        return nullptr;

    if (GTestFramework::groupMode() != GTest::Constants::GTestFilter)
        return nullptr;

    const QString gtestFilter = GTestFramework::currentGTestFilter();
    TestTreeItem *filtered = nullptr;
    for (int row = childCount() - 1; row >= 0; --row) {
        GTestTreeItem *child = static_cast<GTestTreeItem *>(childAt(row));
        if (!matchesFilter(gtestFilter, name() + '.' + child->name())) {
            if (!filtered) {
                filtered = copyWithoutChildren();
                filtered->setData(0, Qt::Unchecked, Qt::CheckStateRole);
            }
            auto childCopy = child->copyWithoutChildren();
            childCopy->setData(0, Qt::Unchecked, Qt::CheckStateRole);
            filtered->appendChild(childCopy);
            removeChildAt(row);
        }
    }
    return filtered;
}

bool GTestTreeItem::shouldBeAddedAfterFiltering() const
{
    return type() == TestTreeItem::TestCase || childCount();
}

} // namespace Internal
} // namespace Autotest
