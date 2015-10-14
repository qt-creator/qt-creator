/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#include "autotestconstants.h"
#include "testtreeitem.h"

#include <utils/qtcassert.h>

#include <QIcon>

#include <texteditor/texteditor.h>

namespace Autotest {
namespace Internal {

TestTreeItem::TestTreeItem(const QString &name, const QString &filePath, Type type)
    : TreeItem( { name } ),
      m_name(name),
      m_filePath(filePath),
      m_type(type),
      m_line(0)
{
    switch (m_type) {
    case TEST_CLASS:
    case TEST_FUNCTION:
        m_checked = Qt::Checked;
        break;
    default:
        m_checked = Qt::Unchecked;
    }
}

TestTreeItem::~TestTreeItem()
{
    removeChildren();
}

TestTreeItem::TestTreeItem(const TestTreeItem &other)
    : TreeItem( { other.m_name } ),
      m_name(other.m_name),
      m_filePath(other.m_filePath),
      m_checked(other.m_checked),
      m_type(other.m_type),
      m_line(other.m_line),
      m_column(other.m_column),
      m_mainFile(other.m_mainFile)
{
    for (int row = 0, count = other.childCount(); row < count; ++row)
        appendChild(new TestTreeItem(*childItem(row)));
}

static QIcon testTreeIcon(TestTreeItem::Type type)
{
    static QIcon icons[] = {
        QIcon(),
        QIcon(QLatin1String(":/images/class.png")),
        QIcon(QLatin1String(":/images/func.png")),
        QIcon(QLatin1String(":/images/data.png"))
    };
    if (static_cast<unsigned long>(type) >= sizeof(icons))
        return icons[2];
    return icons[type];
}

QVariant TestTreeItem::data(int /*column*/, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (m_type == ROOT && childCount() == 0)
            return QString(m_name + QObject::tr(" (none)"));
        else if (m_name.isEmpty())
            return QObject::tr(Constants::UNNAMED_QUICKTESTS);
        else
            return m_name;
    case Qt::ToolTipRole:
        if (m_type == TEST_CLASS && m_name.isEmpty()) {
            return QObject::tr("<p>Give all test cases a name to ensure correct behavior "
                               "when running test cases and to be able to select them.</p>");
        }
        return m_filePath;
    case Qt::DecorationRole:
        return testTreeIcon(m_type);
    case Qt::CheckStateRole:
        switch (m_type) {
        case ROOT:
        case TEST_DATAFUNCTION:
        case TEST_SPECIALFUNCTION:
            return QVariant();
        case TEST_CLASS:
            return m_name.isEmpty() ? QVariant() : checked();
        case TEST_FUNCTION:
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
        case TEST_DATAFUNCTION:
        case TEST_SPECIALFUNCTION:
            return true;
        case TEST_CLASS:
            return m_name.isEmpty();
        case TEST_FUNCTION:
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
    if (m_type != modified->m_type) {
        m_type = modified->m_type;
        hasBeenModified = true;
    }
    return hasBeenModified;
}

void TestTreeItem::setChecked(const Qt::CheckState checkState)
{
    switch (m_type) {
    case TEST_FUNCTION: {
        m_checked = (checkState == Qt::Unchecked ? Qt::Unchecked : Qt::Checked);
        parentItem()->revalidateCheckState();
        break;
    }
    case TEST_CLASS: {
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
    case TEST_CLASS:
    case TEST_FUNCTION:
        return m_checked;
    case TEST_DATAFUNCTION:
    case TEST_SPECIALFUNCTION:
        return Qt::Unchecked;
    default:
        if (parent())
            return parentItem()->m_checked;
    }
    return Qt::Unchecked;
}

QList<QString> TestTreeItem::getChildNames() const
{
    QList<QString> names;
    for (int row = 0, count = childCount(); row < count; ++row)
        names << childItem(row)->name();
    return names;
}

TestTreeItem *TestTreeItem::parentItem() const
{
    return static_cast<TestTreeItem *>(parent());
}

TestTreeItem *TestTreeItem::childItem(int row) const
{
    return static_cast<TestTreeItem *>(child(row));
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
        case TEST_DATAFUNCTION:
        case TEST_SPECIALFUNCTION:
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

} // namespace Internal
} // namespace Autotest
