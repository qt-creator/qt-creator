/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "layoutbuilder.h"

#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/qtcassert.h>

#include <QFormLayout>
#include <QGridLayout>
#include <QStyle>
#include <QWidget>

namespace Utils {

/*!
    \enum Utils::LayoutBuilder::LayoutType
    \inmodule QtCreator

    The LayoutType enum describes the type of \c QLayout a layout builder
    operates on.

    \value FormLayout
    \value GridLayout
*/

/*!
    \class Utils::LayoutBuilder::LayoutItem
    \inmodule QtCreator

    \brief The LayoutItem class represents widgets, layouts, and aggregate
    items for use in conjunction with layout builders.

    Layout items are typically implicitly constructed when adding items to a
    \c LayoutBuilder instance using \c LayoutBuilder::addItem() or
    \c LayoutBuilder::addItems() and never stored in user code.
*/

/*!
    Constructs a layout item instance representing an empty cell.
 */
LayoutBuilder::LayoutItem::LayoutItem()
{}


/*!
    Constructs a layout item proxy for \a layout, spanning the number
    of cells specified by \a span in the target layout, with alignment \a align.
 */
LayoutBuilder::LayoutItem::LayoutItem(QLayout *layout, int span, Alignment align)
    : layout(layout), span(span), align(align)
{}

/*!
    Constructs a layout item proxy for \a widget, spanning the number
    of cell specified by \a span in the target layout, with alignment \a align.
 */
LayoutBuilder::LayoutItem::LayoutItem(QWidget *widget, int span, Alignment align)
    : widget(widget), span(span), align(align)
{}

/*!
    Constructs a layout item representing a \c BaseAspect.

    This ultimately uses the \a aspect's \c addToLayout(LayoutBuilder &) function,
    which in turn can add one or more layout items to the target layout.

    \sa BaseAspect::addToLayout()
 */
LayoutBuilder::LayoutItem::LayoutItem(BaseAspect *aspect)
    : aspect(aspect)
{}

/*!
    Constructs a layout item containing some static \a text.
 */
LayoutBuilder::LayoutItem::LayoutItem(const QString &text) : text(text) {}

/*!
    \class Utils::LayoutBuilder
    \inmodule QtCreator

    \brief The LayoutBuilder class provides a convenient way to fill \c QFormLayout
    and \c QGridLayouts with contents.

    Filling a layout with items happens item-by-item, row-by-row.

    A LayoutBuilder instance is typically used locally within a function and never stored.

    \sa addItem(), addItems(), addRow(), finishRow()
*/


/*!
    Constructs a new layout builder with the specified \a layoutType.

    The constructed layout will be attached to the provided \c QWidget \a parent.
 */
LayoutBuilder::LayoutBuilder(QWidget *parent, LayoutType layoutType)
{
    if (layoutType == FormLayout) {
        m_formLayout = new QFormLayout(parent);
        m_formLayout->setContentsMargins(0, 0, 0, 0);
        m_formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    } else {
        m_gridLayout = new QGridLayout(parent);
        m_gridLayout->setContentsMargins(0, 0, 0, 0);
    }
}

/*!
    Constructs a new layout builder to extend an existing \a layout.

    This constructor can be used to continue the work of previous layout building.
    The type of the underlying layout and previous contents will be retained,
    new items will be added below existing ones.
 */
LayoutBuilder::LayoutBuilder(QLayout *layout)
{
    if (auto fl = qobject_cast<QFormLayout *>(layout)) {
        m_formLayout = fl;
    } else if (auto grid = qobject_cast<QGridLayout *>(layout)) {
        m_gridLayout = grid;
        m_currentGridRow = grid->rowCount();
        m_currentGridColumn = 0;
    }
}

/*!
    Destructs a layout builder.
 */
LayoutBuilder::~LayoutBuilder()
{
    if (m_formLayout)
        flushPendingFormItems();
}

/*!
    Instructs a layout builder to finish the current row.
    This is implicitly called by LayoutBuilder's destructor.
 */
LayoutBuilder &LayoutBuilder::finishRow()
{
    if (m_formLayout)
        flushPendingFormItems();
    if (m_gridLayout) {
        if (m_currentGridColumn != 0) {
            ++m_currentGridRow;
            m_currentGridColumn = 0;
        }
    }
    return *this;
}

/*!
    This starts a new row containing the \a item. The row can be further extended by
    other items using \c addItem() or \c addItems().

    \sa finishRow(), addItem(), addItems()
 */
LayoutBuilder &LayoutBuilder::addRow(const LayoutItem &item)
{
    return finishRow().addItem(item);
}

/*!
    This starts a new row containing \a items. The row can be further extended by
    other items using \c addItem() or \c addItems().

    \sa finishRow(), addItem(), addItems()
 */
LayoutBuilder &LayoutBuilder::addRow(const QList<LayoutBuilder::LayoutItem> &items)
{
    return finishRow().addItems(items);
}

/*!
    \internal
 */
void LayoutBuilder::flushPendingFormItems()
{
    QTC_ASSERT(m_formLayout, return);

    if (m_pendingFormItems.isEmpty())
        return;

    // If there are more than two items, we cram the last ones in one hbox.
    if (m_pendingFormItems.size() > 2) {
        auto hbox = new QHBoxLayout;
        hbox->setContentsMargins(0, 0, 0, 0);
        for (int i = 1; i < m_pendingFormItems.size(); ++i) {
            if (QWidget *w = m_pendingFormItems.at(i).widget)
                hbox->addWidget(w);
            else if (QLayout *l = m_pendingFormItems.at(i).layout)
                hbox->addItem(l);
            else
                QTC_CHECK(false);
        }
        while (m_pendingFormItems.size() >= 2)
            m_pendingFormItems.takeLast();
        m_pendingFormItems.append(LayoutItem(hbox));
    }

    if (m_pendingFormItems.size() == 1) { // One one item given, so this spans both columns.
        if (auto layout = m_pendingFormItems.at(0).layout)
            m_formLayout->addRow(layout);
        else if (auto widget = m_pendingFormItems.at(0).widget)
            m_formLayout->addRow(widget);
    } else if (m_pendingFormItems.size() == 2) { // Normal case, both columns used.
        if (auto label = m_pendingFormItems.at(0).widget) {
            if (auto layout = m_pendingFormItems.at(1).layout)
                m_formLayout->addRow(label, layout);
            else if (auto widget = m_pendingFormItems.at(1).widget)
                m_formLayout->addRow(label, widget);
        } else  {
            if (auto layout = m_pendingFormItems.at(1).layout)
                m_formLayout->addRow(m_pendingFormItems.at(0).text, layout);
            else if (auto widget = m_pendingFormItems.at(1).widget)
                m_formLayout->addRow(m_pendingFormItems.at(0).text, widget);
        }
    } else {
        QTC_CHECK(false);
    }

    m_pendingFormItems.clear();
}

/*!
    Returns the layout this layout builder operates on.
 */
QLayout *LayoutBuilder::layout() const
{
    if (m_formLayout)
        return m_formLayout;
    return m_gridLayout;
}

/*!
    Adds the layout item \a item to the current row.
 */
LayoutBuilder &LayoutBuilder::addItem(const LayoutItem &item)
{
    if (item.widget && !item.widget->parent())
        item.widget->setParent(layout()->parentWidget());

    if (item.aspect) {
        item.aspect->addToLayout(*this);
    } else {
        if (m_gridLayout) {
            if (auto widget = item.widget) {
                Qt::Alignment align;
                if (item.align == AlignAsFormLabel)
                    align = Qt::Alignment(widget->style()->styleHint(QStyle::SH_FormLayoutLabelAlignment));
                m_gridLayout->addWidget(widget, m_currentGridRow, m_currentGridColumn, 1, item.span, align);
            }
            m_currentGridColumn += item.span;
        } else {
            m_pendingFormItems.append(item);
        }
    }
    return *this;
}

/*!
    Adds the layout item \a items to the current row.
 */
LayoutBuilder &LayoutBuilder::addItems(const QList<LayoutBuilder::LayoutItem> &items)
{
    for (const LayoutItem &item : items)
        addItem(item);
    return *this;
}

} // Utils
