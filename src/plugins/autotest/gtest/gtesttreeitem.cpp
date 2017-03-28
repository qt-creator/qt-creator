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
#include "gtestparser.h"
#include "../autotest_utils.h"

#include <projectexplorer/session.h>
#include <utils/qtcassert.h>

namespace Autotest {
namespace Internal {

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

QVariant GTestTreeItem::data(int column, int role) const
{
    switch (role) {
    case Qt::DisplayRole: {
        if (type() == TestTreeItem::Root)
            break;

        const QString &displayName = (m_state & Disabled) ? name().mid(9) : name();
        return QVariant(displayName + nameSuffix());
    }
    case Qt::CheckStateRole:
        switch (type()) {
        case TestCase:
        case TestFunctionOrSet:
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
    case TestCase: {
        const QString &testSpecifier = gtestFilter(state()).arg(name()).arg('*');
        if (int count = childCount()) {
            config = new GTestConfiguration;
            config->setTestCases(QStringList(testSpecifier));
            config->setTestCaseCount(count);
            config->setProjectFile(proFile());
            config->setProject(project);
        }
        break;
    }
    case TestFunctionOrSet: {
        GTestTreeItem *parent = static_cast<GTestTreeItem *>(parentItem());
        if (!parent)
            return nullptr;
        const QString &testSpecifier = gtestFilter(parent->state()).arg(parent->name()).arg(name());
        config = new GTestConfiguration;
        config->setTestCases(QStringList(testSpecifier));
        config->setProjectFile(proFile());
        config->setProject(project);
        break;
    }
    default:
        return nullptr;
    }
    return config;
}

TestConfiguration *GTestTreeItem::debugConfiguration() const
{
    GTestConfiguration *config = static_cast<GTestConfiguration *>(testConfiguration());
    if (config)
        config->setRunMode(DebuggableTestConfiguration::Debug);
    return config;
}

// used as key inside getAllTestCases()/getSelectedTestCases() for Google Tests
class ProFileWithDisplayName
{
public:
    ProFileWithDisplayName(const QString &file, const QString &name)
        : proFile(file), displayName(name) {}
    QString proFile;
    QString displayName;

    bool operator==(const ProFileWithDisplayName &rhs) const
    {
        return proFile == rhs.proFile && displayName == rhs.displayName;
    }
};

// needed as ProFileWithDisplayName is used as key inside a QHash
bool operator<(const ProFileWithDisplayName &lhs, const ProFileWithDisplayName &rhs)
{
    return lhs.proFile == rhs.proFile ? lhs.displayName < rhs.displayName
                                      : lhs.proFile < rhs.proFile;
}

// needed as ProFileWithDisplayName is used as a key inside a QHash
uint qHash(const ProFileWithDisplayName &lhs)
{
    return ::qHash(lhs.proFile) ^ ::qHash(lhs.displayName);
}

QList<TestConfiguration *> GTestTreeItem::getAllTestConfigurations() const
{
    QList<TestConfiguration *> result;

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    QHash<ProFileWithDisplayName, int> proFilesWithTestSets;
    for (int row = 0, count = childCount(); row < count; ++row) {
        const GTestTreeItem *child = static_cast<const GTestTreeItem *>(childItem(row));

        const int grandChildCount = child->childCount();
        for (int grandChildRow = 0; grandChildRow < grandChildCount; ++grandChildRow) {
            const TestTreeItem *grandChild = child->childItem(grandChildRow);
            ProFileWithDisplayName key(grandChild->proFile(),
                                       TestUtils::getCMakeDisplayNameIfNecessary(grandChild->filePath(),
                                                                                 grandChild->proFile()));

            proFilesWithTestSets.insert(key, proFilesWithTestSets[key] + 1);
        }
    }

    QHash<ProFileWithDisplayName, int>::ConstIterator it = proFilesWithTestSets.begin();
    QHash<ProFileWithDisplayName, int>::ConstIterator end = proFilesWithTestSets.end();
    for ( ; it != end; ++it) {
        const ProFileWithDisplayName &key = it.key();
        GTestConfiguration *tc = new GTestConfiguration;
        tc->setTestCaseCount(it.value());
        tc->setProjectFile(key.proFile);
        tc->setProject(project);
        result << tc;
    }

    return result;
}

struct TestCases
{
    QStringList filters;
    int additionalTestCaseCount = 0;
};

static const ProFileWithDisplayName createProfile(const TestTreeItem *item)
{
    return ProFileWithDisplayName(
                item->proFile(),
                TestUtils::getCMakeDisplayNameIfNecessary(item->filePath(), item->proFile()));
}

QList<TestConfiguration *> GTestTreeItem::getSelectedTestConfigurations() const
{
    QList<TestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    QHash<ProFileWithDisplayName, TestCases> proFilesWithCheckedTestSets;
    for (int row = 0, count = childCount(); row < count; ++row) {
        const GTestTreeItem *child = static_cast<const GTestTreeItem *>(childItem(row));

        const int grandChildCount = child->childCount();
        QTC_ASSERT(grandChildCount != 0, continue);

        switch (child->checked()) {
        case Qt::Unchecked:
            continue;
        case Qt::Checked: {
            auto &testCases = proFilesWithCheckedTestSets[createProfile(child->childItem(0))];
            testCases.filters.append(gtestFilter(child->state()).arg(child->name()).arg('*'));
            testCases.additionalTestCaseCount += grandChildCount - 1;
            break;
        }
        case Qt::PartiallyChecked: {
            for (int grandChildRow = 0; grandChildRow < grandChildCount; ++grandChildRow) {
                const TestTreeItem *grandChild = child->childItem(grandChildRow);
                if (grandChild->checked() == Qt::Checked) {
                    proFilesWithCheckedTestSets[createProfile(grandChild)].filters.append(
                                gtestFilter(child->state()).arg(child->name()).arg(grandChild->name()));
                }
            }
            break;
        }
        }
    }

    QHash<ProFileWithDisplayName, TestCases>::ConstIterator it = proFilesWithCheckedTestSets.begin();
    QHash<ProFileWithDisplayName, TestCases>::ConstIterator end = proFilesWithCheckedTestSets.end();
    for ( ; it != end; ++it) {
        const ProFileWithDisplayName &proFileWithDisplayName = it.key();
        GTestConfiguration *tc = new GTestConfiguration;
        tc->setTestCases(it.value().filters);
        tc->setTestCaseCount(tc->testCaseCount() + it.value().additionalTestCaseCount);
        tc->setProjectFile(proFileWithDisplayName.proFile);
        tc->setProject(project);
        result << tc;
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
        return findChildByNameStateAndFile(parseResult->name, states, parseResult->proFile);
    case TestCase:
        return findChildByNameAndFile(result->name, result->fileName);
    default:
        return nullptr;
    }
}

bool GTestTreeItem::modify(const TestParseResult *result)
{
    QTC_ASSERT(result, return false);

    switch (type()) {
    case TestFunctionOrSet:
        return modifyTestSetContent(static_cast<const GTestParseResult *>(result));
    default:
        return false;
    }
}

bool GTestTreeItem::modifyTestSetContent(const GTestParseResult *result)
{
    bool hasBeenModified = modifyLineAndColumn(result->line, result->column);
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
    return findChildBy([name, state, proFile](const TestTreeItem *other) -> bool {
        const GTestTreeItem *gtestItem = static_cast<const GTestTreeItem *>(other);
        return other->proFile() == proFile
                && other->name() == name
                && gtestItem->state() == state;
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

} // namespace Internal
} // namespace Autotest
