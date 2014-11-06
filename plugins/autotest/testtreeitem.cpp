/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
**
****************************************************************************/

#include "testtreeitem.h"

namespace Autotest {
namespace Internal {

TestTreeItem::TestTreeItem(const QString &name, const QString &filePath, Type type, TestTreeItem *parent)
    : m_name(name),
      m_filePath(filePath),
      m_checked(type == ROOT ? Qt::Unchecked : Qt::Checked),
      m_type(type),
      m_line(0),
      m_parent(parent)
{
}

TestTreeItem::~TestTreeItem()
{
    removeChildren();
}

TestTreeItem *TestTreeItem::child(int row)
{
    return m_children.at(row);
}

TestTreeItem *TestTreeItem::parent() const
{
    return m_parent;
}

void TestTreeItem::appendChild(TestTreeItem *child)
{
    m_children.append(child);
}

int TestTreeItem::row() const
{
    if (m_parent)
        return m_parent->m_children.indexOf(const_cast<TestTreeItem *>(this));
    return 0;
}

int TestTreeItem::childCount() const
{
    return m_children.size();
}

void TestTreeItem::removeChildren()
{
    qDeleteAll(m_children);
    m_children.clear();
}

bool TestTreeItem::removeChild(int row)
{
    if (row < 0 || row >= m_children.size())
        return false;
    TestTreeItem *child = m_children.at(row);
    m_children.removeAt(row);
    delete child;
    return true;
}

bool TestTreeItem::modifyContent(const TestTreeItem *modified)
{
    bool hasBeenModified = false;
    if (m_filePath != modified->m_filePath) {
        m_filePath = modified->m_filePath;
        hasBeenModified = true;
    }
    if (m_name != modified->m_name) {
        m_name = modified->m_name;
        hasBeenModified = true;
    }
    if (m_line != modified->m_line) {
        m_line = modified->m_line;
        hasBeenModified = true;
    }
    if (m_mainFile != modified->m_mainFile) {
        m_mainFile = modified->m_mainFile;
        hasBeenModified = true;
    }
    return hasBeenModified;
}

void TestTreeItem::setChecked(const Qt::CheckState checkState)
{
    switch (m_type) {
    case ROOT:
        return;
    case TEST_FUNCTION:
        m_checked = (checkState == Qt::Unchecked ? Qt::Unchecked : Qt::Checked);
        m_parent->revalidateCheckState();
        break;
    case TEST_CLASS:
        Qt::CheckState usedState = (checkState == Qt::Unchecked ? Qt::Unchecked : Qt::Checked);
        foreach (TestTreeItem *child, m_children) {
            child->setChecked(usedState);
        }
        m_checked = usedState;
    }
}

void TestTreeItem::revalidateCheckState()
{
    if (m_children.size() == 0)
        return;
    bool foundChecked = false;
    bool foundUnchecked = false;
    foreach (const TestTreeItem *child, m_children) {
        foundChecked |= (child->m_checked != Qt::Unchecked);
        foundUnchecked |= (child->m_checked == Qt::Unchecked);
        if (foundChecked && foundUnchecked) {
            m_checked = Qt::PartiallyChecked;
            return;
        }
    }
    m_checked = (foundUnchecked ? Qt::Unchecked : Qt::Checked);
}

} // namespace Internal
} // namespace Autotest
