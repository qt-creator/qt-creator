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
#include "autotest_utils.h"
#include "itestparser.h"
#include "testconfiguration.h"
#include "testtreeitem.h"

#include <cplusplus/Icons.h>
#include <texteditor/texteditor.h>

#include <QIcon>

namespace Autotest {
namespace Internal {

TestTreeItem::TestTreeItem(const QString &name, const QString &filePath, Type type)
    : m_name(name),
      m_filePath(filePath),
      m_type(type)
{
    m_checked = (m_type == TestCase || m_type == TestFunctionOrSet) ? Qt::Checked : Qt::Unchecked;
}

static QIcon testTreeIcon(TestTreeItem::Type type)
{
    static QIcon icons[] = {
        QIcon(),
        CPlusPlus::Icons::iconForType(CPlusPlus::Icons::ClassIconType),
        CPlusPlus::Icons::iconForType(CPlusPlus::Icons::SlotPrivateIconType),
        QIcon(":/images/data.png")
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
            return QCoreApplication::translate("TestTreeItem", "%1 (none)").arg(m_name);
        else
            return m_name;
    case Qt::ToolTipRole:
        return m_filePath;
    case Qt::DecorationRole:
        return testTreeIcon(m_type);
    case Qt::CheckStateRole:
        return QVariant();
    case LinkRole: {
        QVariant itemLink;
        itemLink.setValue(TextEditor::TextEditorWidget::Link(m_filePath, m_line, m_column));
        return itemLink;
    }
    case ItalicRole:
        return false;
    case TypeRole:
        return m_type;
    case EnabledRole:
        return true;
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

Qt::ItemFlags TestTreeItem::flags(int /*column*/) const
{
    static const Qt::ItemFlags defaultFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    switch (m_type) {
    case Root:
        return Qt::ItemIsEnabled;
    case TestCase:
        return defaultFlags | Qt::ItemIsAutoTristate | Qt::ItemIsUserCheckable;
    case TestFunctionOrSet:
        return defaultFlags | Qt::ItemIsUserCheckable;
    case TestDataFunction:
    case TestSpecialFunction:
    case TestDataTag:
    default:
        return defaultFlags;
    }
}

bool TestTreeItem::modifyTestCaseContent(const QString &name, unsigned line, unsigned column)
{
    bool hasBeenModified = modifyName(name);
    hasBeenModified |= modifyLineAndColumn(line, column);
    return hasBeenModified;
}

bool TestTreeItem::modifyTestFunctionContent(const TestParseResult *result)
{
    bool hasBeenModified = modifyFilePath(result->fileName);
    hasBeenModified |= modifyLineAndColumn(result->line, result->column);
    return hasBeenModified;
}

// TODO pass TestParseResult * to all modifyXYZ() OR remove completely if possible
bool TestTreeItem::modifyDataTagContent(const QString &name, const QString &fileName,
                                        unsigned line, unsigned column)
{
    bool hasBeenModified = modifyFilePath(fileName);
    hasBeenModified |= modifyName(name);
    hasBeenModified |= modifyLineAndColumn(line, column);
    return hasBeenModified;
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
    case TestDataTag: {
        m_checked = (checkState == Qt::Unchecked ? Qt::Unchecked : Qt::Checked);
        if (auto parent = parentItem())
            parent->revalidateCheckState();
        break;
    }
    case TestFunctionOrSet:
    case TestCase: {
        Qt::CheckState usedState = (checkState == Qt::Unchecked ? Qt::Unchecked : Qt::Checked);
        for (int row = 0, count = childCount(); row < count; ++row)
            childItem(row)->setChecked(usedState);
        m_checked = usedState;
        if (m_type == TestFunctionOrSet) {
            if (auto parent = parentItem())
                parent->revalidateCheckState();
        }
        break;
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
    case TestDataTag:
        return m_checked;
    default:
        return Qt::Unchecked;
    }
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

void TestTreeItem::markForRemovalRecursively(const QString &filePath)
{
    if (m_filePath == filePath) {
        markForRemovalRecursively(true);
    } else {
        for (int row = 0, count = childCount(); row < count; ++row) {
            TestTreeItem *child = childItem(row);
            child->markForRemovalRecursively(filePath);
        }
    }
}

TestTreeItem *TestTreeItem::parentItem() const
{
    return static_cast<TestTreeItem *>(parent());
}

TestTreeItem *TestTreeItem::childItem(int row) const
{
    return static_cast<TestTreeItem *>(childAt(row));
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

QList<TestConfiguration *> TestTreeItem::getAllTestConfigurations() const
{
    return QList<TestConfiguration *>();
}

QList<TestConfiguration *> TestTreeItem::getSelectedTestConfigurations() const
{
    return QList<TestConfiguration *>();
}

bool TestTreeItem::lessThan(const TestTreeItem *other, SortMode mode) const
{
    const QString &lhs = data(0, Qt::DisplayRole).toString();
    const QString &rhs = other->data(0, Qt::DisplayRole).toString();

    switch (mode) {
    case Alphabetically:
        if (lhs == rhs)
            return index().row() > other->index().row();
        return lhs > rhs;
    case Naturally: {
        const TextEditor::TextEditorWidget::Link &leftLink =
                data(0, LinkRole).value<TextEditor::TextEditorWidget::Link>();
        const TextEditor::TextEditorWidget::Link &rightLink =
                other->data(0, LinkRole).value<TextEditor::TextEditorWidget::Link>();
        if (leftLink.targetFileName == rightLink.targetFileName) {
            return leftLink.targetLine == rightLink.targetLine
                    ? leftLink.targetColumn > rightLink.targetColumn
                    : leftLink.targetLine > rightLink.targetLine;
        }
        return leftLink.targetFileName > rightLink.targetFileName;
    }
    default:
        return true;
    }
}

void TestTreeItem::revalidateCheckState()
{
    const Type ttiType = type();
    if (ttiType != TestCase && ttiType != TestFunctionOrSet)
        return;
    if (childCount() == 0) // can this happen? (we're calling revalidateCS() on parentItem()
        return;
    bool foundChecked = false;
    bool foundUnchecked = false;
    bool foundPartiallyChecked = false;
    for (int row = 0, count = childCount(); row < count; ++row) {
        TestTreeItem *child = childItem(row);
        switch (child->type()) {
        case TestDataFunction:
        case TestSpecialFunction:
            continue;
        default:
            break;
        }

        foundChecked |= (child->checked() == Qt::Checked);
        foundUnchecked |= (child->checked() == Qt::Unchecked);
        foundPartiallyChecked |= (child->checked() == Qt::PartiallyChecked);
        if (foundPartiallyChecked || (foundChecked && foundUnchecked)) {
            m_checked = Qt::PartiallyChecked;
            if (ttiType == TestFunctionOrSet)
                parentItem()->revalidateCheckState();
            return;
        }
    }
    m_checked = (foundUnchecked ? Qt::Unchecked : Qt::Checked);
    if (ttiType == TestFunctionOrSet)
        parentItem()->revalidateCheckState();
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

TestTreeItem *TestTreeItem::findChildBy(CompareFunction compare) const
{
    for (int row = 0, count = childCount(); row < count; ++row) {
        TestTreeItem *child = childItem(row);
        if (compare(child))
            return child;
    }
    return nullptr;
}

} // namespace Internal
} // namespace Autotest
