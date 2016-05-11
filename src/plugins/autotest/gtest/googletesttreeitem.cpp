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

#include "googletesttreeitem.h"
#include "googletestconfiguration.h"
#include "googletestparser.h"
#include "../autotest_utils.h"

#include <projectexplorer/session.h>
#include <utils/qtcassert.h>

namespace Autotest {
namespace Internal {

static QString gtestFilter(GoogleTestTreeItem::TestStates states)
{
    if ((states & GoogleTestTreeItem::Parameterized) && (states & GoogleTestTreeItem::Typed))
        return QLatin1String("*/%1/*.%2");
    if (states & GoogleTestTreeItem::Parameterized)
        return QLatin1String("*/%1.%2/*");
    if (states & GoogleTestTreeItem::Typed)
        return QLatin1String("%1/*.%2");
    return QLatin1String("%1.%2");
}

GoogleTestTreeItem *GoogleTestTreeItem::createTestItem(const TestParseResult *result)
{
    const GoogleTestParseResult *parseResult = static_cast<const GoogleTestParseResult *>(result);
    GoogleTestTreeItem *item = new GoogleTestTreeItem(parseResult->name, parseResult->fileName,
                                                      parseResult->itemType);
    item->setProFile(parseResult->proFile);
    item->setLine(parseResult->line);
    item->setColumn(parseResult->column);

    if (parseResult->parameterized)
        item->setState(Parameterized);
    if (parseResult->typed)
        item->setState(Typed);
    if (parseResult->disabled)
        item->setState(Disabled);
    foreach (const TestParseResult *testSet, parseResult->children)
        item->appendChild(createTestItem(testSet));
    return item;
}

QVariant GoogleTestTreeItem::data(int column, int role) const
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

TestConfiguration *GoogleTestTreeItem::testConfiguration() const
{
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    QTC_ASSERT(project, return 0);

    GoogleTestConfiguration *config = 0;
    switch (type()) {
    case TestCase: {
        const QString &testSpecifier = gtestFilter(state()).arg(name()).arg(QLatin1Char('*'));
        if (int count = childCount()) {
            config = new GoogleTestConfiguration;
            config->setTestCases(QStringList(testSpecifier));
            config->setTestCaseCount(count);
            config->setProFile(proFile());
            config->setProject(project);
            // item has no filePath set - so take it of the first children
            config->setDisplayName(
                    TestUtils::getCMakeDisplayNameIfNecessary(childItem(0)->filePath(), proFile()));
        }
        break;
    }
    case TestFunctionOrSet: {
        GoogleTestTreeItem *parent = static_cast<GoogleTestTreeItem *>(parentItem());
        if (!parent)
            return 0;
        const QString &testSpecifier = gtestFilter(parent->state()).arg(parent->name()).arg(name());
        config = new GoogleTestConfiguration;
        config->setTestCases(QStringList(testSpecifier));
        config->setProFile(proFile());
        config->setProject(project);
        config->setDisplayName(
                    TestUtils::getCMakeDisplayNameIfNecessary(filePath(), parent->proFile()));
        break;
    }
    default:
        return 0;
    }
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

QList<TestConfiguration *> GoogleTestTreeItem::getAllTestConfigurations() const
{
    QList<TestConfiguration *> result;

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    QHash<ProFileWithDisplayName, int> proFilesWithTestSets;
    for (int row = 0, count = childCount(); row < count; ++row) {
        const GoogleTestTreeItem *child = static_cast<const GoogleTestTreeItem *>(childItem(row));

        const int grandChildCount = child->childCount();
        for (int grandChildRow = 0; grandChildRow < grandChildCount; ++grandChildRow) {
            const TestTreeItem *grandChild = child->childItem(grandChildRow);
            if (grandChild->checked() == Qt::Checked) {
                ProFileWithDisplayName key(grandChild->proFile(),
                    TestUtils::getCMakeDisplayNameIfNecessary(grandChild->filePath(),
                                                              grandChild->proFile()));

                proFilesWithTestSets.insert(key, proFilesWithTestSets[key] + 1);
            }
        }
    }

    QHash<ProFileWithDisplayName, int>::ConstIterator it = proFilesWithTestSets.begin();
    QHash<ProFileWithDisplayName, int>::ConstIterator end = proFilesWithTestSets.end();
    for ( ; it != end; ++it) {
        const ProFileWithDisplayName &key = it.key();
        GoogleTestConfiguration *tc = new GoogleTestConfiguration;
        tc->setTestCaseCount(it.value());
        tc->setProFile(key.proFile);
        tc->setDisplayName(key.displayName);
        tc->setProject(project);
        result << tc;
    }

    return result;
}

QList<TestConfiguration *> GoogleTestTreeItem::getSelectedTestConfigurations() const
{
    QList<TestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    QHash<ProFileWithDisplayName, QStringList> proFilesWithCheckedTestSets;
    for (int row = 0, count = childCount(); row < count; ++row) {
        const GoogleTestTreeItem *child = static_cast<const GoogleTestTreeItem *>(childItem(row));
        if (child->checked() == Qt::Unchecked)
            continue;

        int grandChildCount = child->childCount();
        for (int grandChildRow = 0; grandChildRow < grandChildCount; ++grandChildRow) {
            const TestTreeItem *grandChild = child->childItem(grandChildRow);
            if (grandChild->checked() == Qt::Checked) {
                ProFileWithDisplayName key(grandChild->proFile(),
                    TestUtils::getCMakeDisplayNameIfNecessary(grandChild->filePath(),
                                                              grandChild->proFile()));

                proFilesWithCheckedTestSets[key].append(
                            gtestFilter(child->state()).arg(child->name()).arg(grandChild->name()));
            }
        }
    }

    QHash<ProFileWithDisplayName, QStringList>::ConstIterator it = proFilesWithCheckedTestSets.begin();
    QHash<ProFileWithDisplayName, QStringList>::ConstIterator end = proFilesWithCheckedTestSets.end();
    for ( ; it != end; ++it) {
        const ProFileWithDisplayName &key = it.key();
        GoogleTestConfiguration *tc = new GoogleTestConfiguration;
        tc->setTestCases(it.value());
        tc->setProFile(key.proFile);
        tc->setDisplayName(key.displayName);
        tc->setProject(project);
        result << tc;
    }

    return result;
}

TestTreeItem *GoogleTestTreeItem::find(const TestParseResult *result)
{
    QTC_ASSERT(result, return 0);

    const GoogleTestParseResult *parseResult = static_cast<const GoogleTestParseResult *>(result);
    GoogleTestTreeItem::TestStates states = parseResult->disabled ? GoogleTestTreeItem::Disabled
                                                                  : GoogleTestTreeItem::Enabled;
    if (parseResult->parameterized)
        states |= GoogleTestTreeItem::Parameterized;
    if (parseResult->typed)
        states |= GoogleTestTreeItem::Typed;
    switch (type()) {
    case Root:
        return findChildByNameStateAndFile(parseResult->name, states, parseResult->proFile);
    case TestCase:
        return findChildByNameAndFile(result->name, result->fileName);
    default:
        return 0;
    }
}

bool GoogleTestTreeItem::modify(const TestParseResult *result)
{
    QTC_ASSERT(result, return false);

    switch (type()) {
    case TestFunctionOrSet:
        return modifyTestSetContent(static_cast<const GoogleTestParseResult *>(result));
    default:
        return false;
    }
}

bool GoogleTestTreeItem::modifyTestSetContent(const GoogleTestParseResult *result)
{
    bool hasBeenModified = modifyLineAndColumn(result->line, result->column);
    GoogleTestTreeItem::TestStates states = result->disabled ? GoogleTestTreeItem::Disabled
                                                             : GoogleTestTreeItem::Enabled;
    if (m_state != states) {
        m_state = states;
        hasBeenModified = true;
    }
    return hasBeenModified;
}

TestTreeItem *GoogleTestTreeItem::findChildByNameStateAndFile(const QString &name,
                                                              GoogleTestTreeItem::TestStates state,
                                                              const QString &proFile) const
{
    return findChildBy([name, state, proFile](const TestTreeItem *other) -> bool {
        const GoogleTestTreeItem *gtestItem = static_cast<const GoogleTestTreeItem *>(other);
        return other->proFile() == proFile
                && other->name() == name
                && gtestItem->state() == state;
    });
}

QString GoogleTestTreeItem::nameSuffix() const
{
    static QString markups[] = { QCoreApplication::translate("GoogleTestTreeItem", "parameterized"),
                                 QCoreApplication::translate("GoogleTestTreeItem", "typed") };
    QString suffix;
    if (m_state & Parameterized)
        suffix =  QLatin1String(" [") + markups[0];
    if (m_state & Typed)
        suffix += (suffix.isEmpty() ? QLatin1String(" [") : QLatin1String(", ")) + markups[1];
    if (!suffix.isEmpty())
        suffix += QLatin1Char(']');
    return suffix;
}

} // namespace Internal
} // namespace Autotest
