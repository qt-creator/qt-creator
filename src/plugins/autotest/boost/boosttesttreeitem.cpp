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

#include "boosttesttreeitem.h"
#include "boosttestconstants.h"
#include "boosttestconfiguration.h"
#include "boosttestparser.h"
#include "../testframeworkmanager.h"

#include <projectexplorer/session.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QRegularExpression>

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
            const QFileInfo fileInfo(bResult->fileName);
            const QFileInfo base(fileInfo.absolutePath());
            for (int row = 0; row < childCount(); ++row) {
                BoostTestTreeItem *group = static_cast<BoostTestTreeItem *>(childAt(row));
                if (group->filePath() != base.absoluteFilePath())
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
    const QFileInfo fileInfo(filePath());
    const QFileInfo base(fileInfo.absolutePath());
    return new BoostTestTreeItem(framework(), base.baseName(), fileInfo.absolutePath(), TestTreeItem::GroupNode);
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

QList<TestConfiguration *> BoostTestTreeItem::getAllTestConfigurations() const
{
    QList<TestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    struct BoostTestCases {
        int testCases;
        QSet<QString> internalTargets;
    };

    // we only need the unique project files (and number of test cases for the progress indicator)
    QHash<QString, BoostTestCases> testsPerProjectfile;
    forAllChildren([&testsPerProjectfile](TreeItem *it){
        auto item = static_cast<BoostTestTreeItem *>(it);
        if (item->type() != TestSuite)
            return;
        int funcChildren = 0;
        item->forAllChildren([&funcChildren](TreeItem *child){
            if (static_cast<BoostTestTreeItem *>(child)->type() == TestCase)
                ++funcChildren;
        });
        if (funcChildren) {
            testsPerProjectfile[item->proFile()].testCases += funcChildren;
            testsPerProjectfile[item->proFile()].internalTargets.unite(item->internalTargets());
        }
    });

    for (auto it = testsPerProjectfile.begin(), end = testsPerProjectfile.end(); it != end; ++it) {
        for (const QString &target : qAsConst(it.value().internalTargets)) {
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

QList<TestConfiguration *> BoostTestTreeItem::getSelectedTestConfigurations() const
{
    QList<TestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    struct BoostTestCases {
        QStringList testCases;
        QSet<QString> internalTargets;
    };

    QHash<QString, BoostTestCases> testCasesForProjectFile;
    forAllChildren([&testCasesForProjectFile](TreeItem *it){
        auto item = static_cast<BoostTestTreeItem *>(it);
        if (item->type() != TestCase)
            return;
        if (!item->enabled()) // ignore child tests known to be disabled when using run selected
            return;
        if (item->checked() == Qt::Checked) {
            QString tcName = item->name();
            if (item->state().testFlag(BoostTestTreeItem::Templated))
                tcName.append("<*");
            else if (item->state().testFlag(BoostTestTreeItem::Parameterized))
                tcName.append('*');
            tcName = handleSpecialFunctionNames(tcName);
            testCasesForProjectFile[item->proFile()].testCases.append(
                        item->prependWithParentsSuitePaths(tcName));
            testCasesForProjectFile[item->proFile()].internalTargets.unite(item->internalTargets());
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

TestConfiguration *BoostTestTreeItem::testConfiguration() const
{
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    QTC_ASSERT(project, return nullptr);

    const Type itemType = type();
    if (itemType == TestSuite || itemType == TestCase) {
        QStringList testCases;
        if (itemType == TestSuite) {
            forFirstLevelChildren([&testCases](TestTreeItem *child) {
                QTC_ASSERT(child, return);
                if (auto boostItem = static_cast<BoostTestTreeItem *>(child)) {
                    if (boostItem->enabled()) {
                        QString tcName = handleSpecialFunctionNames(boostItem->name());
                        if (boostItem->type() == TestSuite) // execute everything below a suite
                            tcName.append("/*");
                        else if (boostItem->state().testFlag(BoostTestTreeItem::Parameterized))
                            tcName.append('*');
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
                tcName.append('*');
            testCases.append(prependWithParentsSuitePaths(handleSpecialFunctionNames(tcName)));
        }

        BoostTestConfiguration *config = new BoostTestConfiguration(framework());
        config->setProjectFile(proFile());
        config->setProject(project);
        config->setTestCases(testCases);
        config->setInternalTargets(internalTargets());
        return config;
    }
    return nullptr;
}

TestConfiguration *BoostTestTreeItem::debugConfiguration() const
{
    BoostTestConfiguration *config = static_cast<BoostTestConfiguration *>(testConfiguration());
    if (config)
        config->setRunMode(TestRunMode::Debug);
    return config;
}

QString BoostTestTreeItem::nameSuffix() const
{
    static QString markups[] = {QCoreApplication::translate("BoostTestTreeItem", "parameterized"),
                                QCoreApplication::translate("BoostTestTreeItem", "fixture"),
                                QCoreApplication::translate("BoostTestTreeItem", "templated")};
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
                                                             const QString &proFile) const
{
    return static_cast<TestTreeItem *>(
                findAnyChild([name, state, proFile](const Utils::TreeItem *other){
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
