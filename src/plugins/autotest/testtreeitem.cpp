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

#include "autotestconstants.h"
#include "testtreeitem.h"
#include "testtreemodel.h"

#include <utils/qtcassert.h>

#include <QIcon>

#include <texteditor/texteditor.h>

#include <cplusplus/Icons.h>

namespace Autotest {
namespace Internal {

TestTreeItem::TestTreeItem(const QString &name, const QString &filePath, Type type)
    : TreeItem( { name } ),
      m_name(name),
      m_filePath(filePath),
      m_type(type),
      m_line(0),
      m_status(NewlyAdded)
{
    m_checked = (m_type == TestCase || m_type == TestFunctionOrSet) ? Qt::Checked : Qt::Unchecked;
}

static QIcon testTreeIcon(TestTreeItem::Type type)
{
    static QIcon icons[] = {
        QIcon(),
        CPlusPlus::Icons::iconForType(CPlusPlus::Icons::ClassIconType),
        CPlusPlus::Icons::iconForType(CPlusPlus::Icons::SlotPrivateIconType),
        QIcon(QLatin1String(":/images/data.png"))
    };

    if (int(type) >= int(sizeof icons / sizeof *icons))
        return icons[2];
    return icons[type];
}

QVariant TestTreeItem::data(int /*column*/, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (m_type == Root && childCount() == 0)
            return QString(m_name + QObject::tr(" (none)"));
        else if (m_name.isEmpty())
            return QObject::tr(Constants::UNNAMED_QUICKTESTS);
        else
            return m_name;
    case Qt::ToolTipRole:
        if (m_type == TestCase && m_name.isEmpty()) {
            return QObject::tr("<p>Give all test cases a name to ensure correct behavior "
                               "when running test cases and to be able to select them.</p>");
        }
        return m_filePath;
    case Qt::DecorationRole:
        return testTreeIcon(m_type);
    case Qt::CheckStateRole:
        switch (m_type) {
        case Root:
        case TestDataFunction:
        case TestSpecialFunction:
        case TestDataTag:
            return QVariant();
        case TestCase:
            return m_name.isEmpty() ? QVariant() : checked();
        case TestFunctionOrSet:
            if (parentItem() && parentItem()->name().isEmpty())
                return QVariant();
            return checked();
        default:
            return checked();
        }
    case LinkRole: {
        QVariant itemLink;
        itemLink.setValue(TextEditor::TextEditorWidget::Link(m_filePath, m_line, m_column));
        return itemLink;
    }
    case ItalicRole:
        switch (m_type) {
        case TestDataFunction:
        case TestSpecialFunction:
            return true;
        case TestCase:
            return m_name.isEmpty();
        case TestFunctionOrSet:
            return parentItem() ? parentItem()->name().isEmpty() : false;
        default:
            return false;
        }
    case TypeRole:
        return m_type;
    }
    return QVariant();
}

bool TestTreeItem::setData(int /*column*/, const QVariant &data, int role)
{
    if (role == Qt::CheckStateRole) {
        Qt::CheckState old = checked();
        setChecked((Qt::CheckState)data.toInt());
        return checked() != old;
    }
    return false;
}

bool TestTreeItem::modifyTestCaseContent(const QString &name, unsigned line, unsigned column)
{
    bool hasBeenModified = modifyName(name);
    hasBeenModified |= modifyLineAndColumn(line, column);
    return hasBeenModified;
}

bool TestTreeItem::modifyTestFunctionContent(const TestCodeLocationAndType &location)
{
    bool hasBeenModified = modifyFilePath(location.m_name);
    hasBeenModified |= modifyLineAndColumn(location);
    return hasBeenModified;
}

bool TestTreeItem::modifyDataTagContent(const QString &fileName,
                                        const TestCodeLocationAndType &location)
{
    bool hasBeenModified = modifyFilePath(fileName);
    hasBeenModified |= modifyName(location.m_name);
    hasBeenModified |= modifyLineAndColumn(location);
    return hasBeenModified;
}

bool TestTreeItem::modifyLineAndColumn(const TestCodeLocationAndType &location)
{
    return modifyLineAndColumn(location.m_line, location.m_column);
}

bool TestTreeItem::modifyLineAndColumn(unsigned line, unsigned column)
{
    bool hasBeenModified = false;
    if (m_line != line) {
        m_line = line;
        hasBeenModified = true;
    }
    if (m_column != column) {
        m_column = column;
        hasBeenModified = true;
    }
    return hasBeenModified;
}

void TestTreeItem::setChecked(const Qt::CheckState checkState)
{
    switch (m_type) {
    case TestFunctionOrSet: {
        m_checked = (checkState == Qt::Unchecked ? Qt::Unchecked : Qt::Checked);
        parentItem()->revalidateCheckState();
        break;
    }
    case TestCase: {
        Qt::CheckState usedState = (checkState == Qt::Unchecked ? Qt::Unchecked : Qt::Checked);
        for (int row = 0, count = childCount(); row < count; ++row)
            childItem(row)->setChecked(usedState);
        m_checked = usedState;
    }
    default:
        return;
    }
}

Qt::CheckState TestTreeItem::checked() const
{
    switch (m_type) {
    case TestCase:
    case TestFunctionOrSet:
        return m_checked;
    case TestDataFunction:
    case TestSpecialFunction:
        return Qt::Unchecked;
    default:
        if (parent())
            return parentItem()->m_checked;
    }
    return Qt::Unchecked;
}

void TestTreeItem::markForRemoval(bool mark)
{
    m_status = mark ? MarkedForRemoval : Cleared;
}

void TestTreeItem::markForRemovalRecursively(bool mark)
{
    markForRemoval(mark);
    for (int row = 0, count = childCount(); row < count; ++row)
        childItem(row)->markForRemovalRecursively(mark);
}

TestTreeItem *TestTreeItem::parentItem() const
{
    return static_cast<TestTreeItem *>(parent());
}

TestTreeItem *TestTreeItem::childItem(int row) const
{
    return static_cast<TestTreeItem *>(child(row));
}

TestTreeItem *TestTreeItem::findChildByName(const QString &name)
{
    return findChildBy([name](const TestTreeItem *other) -> bool {
        return other->name() == name;
    });
}

TestTreeItem *TestTreeItem::findChildByFile(const QString &filePath)
{
    return findChildBy([filePath](const TestTreeItem *other) -> bool {
        return other->filePath() == filePath;
    });
}

TestTreeItem *TestTreeItem::findChildByNameAndFile(const QString &name, const QString &filePath)
{
    return findChildBy([name, filePath](const TestTreeItem *other) -> bool {
        return other->filePath() == filePath && other->name() == name;
    });
}

void TestTreeItem::revalidateCheckState()
{
    if (childCount() == 0)
        return;
    bool foundChecked = false;
    bool foundUnchecked = false;
    for (int row = 0, count = childCount(); row < count; ++row) {
        TestTreeItem *child = childItem(row);
        switch (child->type()) {
        case TestDataFunction:
        case TestSpecialFunction:
            continue;
        default:
            break;
        }

        foundChecked |= (child->checked() != Qt::Unchecked);
        foundUnchecked |= (child->checked() == Qt::Unchecked);
        if (foundChecked && foundUnchecked) {
            m_checked = Qt::PartiallyChecked;
            return;
        }
    }
    m_checked = (foundUnchecked ? Qt::Unchecked : Qt::Checked);
}

inline bool TestTreeItem::modifyFilePath(const QString &filePath)
{
    if (m_filePath != filePath) {
        m_filePath = filePath;
        return true;
    }
    return false;
}

inline bool TestTreeItem::modifyName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        return true;
    }
    return false;
}

TestTreeItem *TestTreeItem::findChildBy(CompareFunction compare)
{
    for (int row = 0, count = childCount(); row < count; ++row) {
        TestTreeItem *child = childItem(row);
        if (compare(child))
            return child;
    }
    return 0;
}

AutoTestTreeItem *AutoTestTreeItem::createTestItem(const TestParseResult &result)
{
    AutoTestTreeItem *item = new AutoTestTreeItem(result.testCaseName, result.fileName, TestCase);
    item->setProFile(result.proFile);
    item->setLine(result.line);
    item->setColumn(result.column);

    foreach (const QString &functionName, result.functions.keys()) {
        const TestCodeLocationAndType &locationAndType = result.functions.value(functionName);
        const QString qualifiedName = result.testCaseName + QLatin1String("::") + functionName;
        item->appendChild(createFunctionItem(functionName, locationAndType,
                                             result.dataTagsOrTestSets.value(qualifiedName)));
    }
    return item;
}

AutoTestTreeItem *AutoTestTreeItem::createFunctionItem(const QString &functionName,
                                                       const TestCodeLocationAndType &location,
                                                       const TestCodeLocationList &dataTags)
{
    AutoTestTreeItem *item = new AutoTestTreeItem(functionName, location.m_name, location.m_type);
    item->setLine(location.m_line);
    item->setColumn(location.m_column);

    // if there are any data tags for this function add them
    foreach (const TestCodeLocationAndType &tagLocation, dataTags)
        item->appendChild(createDataTagItem(location.m_name, tagLocation));
    return item;
}

AutoTestTreeItem *AutoTestTreeItem::createDataTagItem(const QString &fileName,
                                                      const TestCodeLocationAndType &location)
{
    AutoTestTreeItem *tagItem = new AutoTestTreeItem(location.m_name, fileName, location.m_type);
    tagItem->setLine(location.m_line);
    tagItem->setColumn(location.m_column);
    return tagItem;
}

QuickTestTreeItem *QuickTestTreeItem::createTestItem(const TestParseResult &result)
{
    QuickTestTreeItem *item = new QuickTestTreeItem(result.testCaseName, result.fileName, TestCase);
    item->setProFile(result.proFile);
    item->setLine(result.line);
    item->setColumn(result.column);
    foreach (const QString &functionName, result.functions.keys())
        item->appendChild(createFunctionItem(functionName, result.functions.value(functionName)));
    return item;
}

QuickTestTreeItem *QuickTestTreeItem::createFunctionItem(const QString &functionName,
                                                         const TestCodeLocationAndType &location)
{
    QuickTestTreeItem *item = new QuickTestTreeItem(functionName, location.m_name, location.m_type);
    item->setLine(location.m_line);
    item->setColumn(location.m_column);
    return item;
}

QuickTestTreeItem *QuickTestTreeItem::createUnnamedQuickTestItem(const TestParseResult &result)
{
    QuickTestTreeItem *item = new QuickTestTreeItem(QString(), QString(), TestCase);
    foreach (const QString &functionName, result.functions.keys())
        item->appendChild(createUnnamedQuickFunctionItem(functionName, result));
    return item;
}

QuickTestTreeItem *QuickTestTreeItem::createUnnamedQuickFunctionItem(const QString &functionName,
                                                                     const TestParseResult &result)
{
    const TestCodeLocationAndType &location = result.functions.value(functionName);
    QuickTestTreeItem *item = new QuickTestTreeItem(functionName, location.m_name, location.m_type);
    item->setLine(location.m_line);
    item->setColumn(location.m_column);
    item->setProFile(result.proFile);
    return item;
}

GoogleTestTreeItem *GoogleTestTreeItem::createTestItem(const TestParseResult &result)
{
    GoogleTestTreeItem *item = new GoogleTestTreeItem(result.testCaseName, QString(), TestCase);
    item->setProFile(result.proFile);
    if (result.parameterized)
        item->setState(Parameterized);
    if (result.typed)
        item->setState(Typed);
    if (result.disabled)
        item->setState(Disabled);
    foreach (const TestCodeLocationAndType &location, result.dataTagsOrTestSets.first())
        item->appendChild(createTestSetItem(result, location));
    return item;
}

GoogleTestTreeItem *GoogleTestTreeItem::createTestSetItem(const TestParseResult &result,
                                                          const TestCodeLocationAndType &location)
{
    GoogleTestTreeItem *item = new GoogleTestTreeItem(location.m_name, result.fileName,
                                                      location.m_type);
    item->setStates(location.m_state);
    item->setLine(location.m_line);
    item->setColumn(location.m_column);
    item->setProFile(result.proFile);
    return item;
}

QVariant GoogleTestTreeItem::data(int column, int role) const
{
    switch (role) {
    case Qt::DisplayRole: {
        if (type() == TestTreeItem::Root)
            return TestTreeItem::data(column, role);

        const QString &displayName = (m_state & GoogleTestTreeItem::Disabled)
                ? name().mid(9) : name();
        return QVariant(displayName + nameSuffix());
    }
    case StateRole:
        return (int)m_state;
    default:
        break;
    }
    return TestTreeItem::data(column, role);
}

bool GoogleTestTreeItem::modifyTestSetContent(const QString &fileName,
                                              const TestCodeLocationAndType &location)
{
    bool hasBeenModified = modifyFilePath(fileName);
    hasBeenModified |= modifyLineAndColumn(location);
    if (m_state != location.m_state) {
        m_state = location.m_state;
        hasBeenModified = true;
    }
    return hasBeenModified;
}

TestTreeItem *GoogleTestTreeItem::findChildByNameStateAndFile(const QString &name,
                                                              GoogleTestTreeItem::TestStates state,
                                                              const QString &proFile)
{
    return findChildBy([name, state, proFile](const TestTreeItem *other) -> bool {
        GoogleTestTreeItem *gtestItem = const_cast<TestTreeItem *>(other)->asGoogleTestTreeItem();
        return other->proFile() == proFile
                && other->name() == name
                && gtestItem->state() == state;
    });
}

QString GoogleTestTreeItem::nameSuffix() const
{
    static QString markups[] = { QObject::tr("parameterized"), QObject::tr("typed") };
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
