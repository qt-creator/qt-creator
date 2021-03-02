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
#include <QGroupBox>
#include <QStyle>
#include <QWidget>

namespace Utils {

/*!
    \enum Utils::LayoutBuilder::LayoutType
    \inmodule QtCreator

    The LayoutType enum describes the type of \c QLayout a layout builder
    operates on.

    \value Form
    \value Grid
    \value HBox
    \value VBox
    \value HBoxWithMargins
    \value VBoxWithMargins
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
LayoutBuilder::LayoutItem::LayoutItem(BaseAspect &aspect, int span, Alignment align)
    : aspect(&aspect), span(span), align(align)
{}

LayoutBuilder::LayoutItem::LayoutItem(BaseAspect *aspect, int span, Alignment align)
    : aspect(aspect), span(span), align(align)
{}

/*!
    Constructs a layout item containing some static \a text.
 */
LayoutBuilder::LayoutItem::LayoutItem(const QString &text, int span, Alignment align)
    : text(text), span(span), align(align)
{}

/*!
    Constructs a layout item from the contents of another LayoutBuilder
 */
LayoutBuilder::LayoutItem::LayoutItem(const LayoutBuilder &builder, int span, Alignment align)
    : widget(builder.parentWidget()), span(span), align(align)
{}

/*!
    \class Utils::LayoutBuilder::Space
    \inmodule QtCreator

    \brief The LayoutBuilder::Space class represents some empty space in a layout.
 */

/*!
    \class Utils::LayoutBuilder::Stretch
    \inmodule QtCreator

    \brief The LayoutBuilder::Stretch class represents some stretch in a layout.
 */

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
    init(parent, layoutType);
}

LayoutBuilder::LayoutBuilder(LayoutType layoutType, const LayoutItems &items)
{
    init(new QWidget, layoutType);
    addItems(items);
}

void LayoutBuilder::init(QWidget *parent, LayoutType layoutType)
{
    switch (layoutType) {
    case Form:
        m_formLayout = new QFormLayout(parent);
        m_formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        break;
    case Grid:
        m_gridLayout = new QGridLayout(parent);
        break;
    case HBox:
    case HBoxWithMargins:
        m_boxLayout = new QHBoxLayout(parent);
        break;
    case VBox:
    case VBoxWithMargins:
        m_boxLayout = new QVBoxLayout(parent);
        break;
    }

    switch (layoutType) {
    case Form:
    case Grid:
    case HBox:
    case VBox:
        layout()->setContentsMargins(0, 0, 0, 0);
        break;
    default:
        break;
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
LayoutBuilder &LayoutBuilder::addRow(const LayoutItems &items)
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
    if (m_boxLayout)
        return m_boxLayout;
    return m_gridLayout;
}

QWidget *LayoutBuilder::parentWidget() const
{
    QLayout *l = layout();
    return l ? l->parentWidget() : nullptr;
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
            if (item.specialType == SpecialType::Break)
                finishRow();
        } else if (m_boxLayout) {
            if (auto widget = item.widget) {
                m_boxLayout->addWidget(widget);
            } else if (item.specialType == SpecialType::Stretch) {
                m_boxLayout->addStretch(item.specialValue.toInt());
            } else if (item.specialType == SpecialType::Space) {
                m_boxLayout->addSpacing(item.specialValue.toInt());
            }
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

void LayoutBuilder::attachTo(QWidget *w, bool stretchAtBottom)
{
    LayoutBuilder builder(w, VBoxWithMargins);
    builder.addItem(*this);
    if (stretchAtBottom)
        builder.addItem(Stretch());
}

LayoutBuilder::Break::Break()
{
    specialType = LayoutBuilder::SpecialType::Break;
}

LayoutBuilder::Stretch::Stretch(int stretch)
{
    specialType = LayoutBuilder::SpecialType::Stretch;
    specialValue = stretch;
}

LayoutBuilder::Space::Space(int space)
{
    specialType = LayoutBuilder::SpecialType::Space;
    specialValue = space;
}

LayoutBuilder::Title::Title(const QString &title)
{
    specialType = LayoutBuilder::SpecialType::Title;
    specialValue = title;
}

// FIXME: Decide on which style to use:
//    Group { Title(...), child1, child2, ...};   or
//    Group { child1, child2, ... }.withTitle(...);

Layouting::Group &Layouting::Group::withTitle(const QString &title)
{
    if (auto box = qobject_cast<QGroupBox *>(parentWidget()))
        box->setTitle(title);
    return *this;
}

namespace Layouting {

Group::Group(std::initializer_list<LayoutItem> items)
    : LayoutBuilder(new QGroupBox, VBoxWithMargins)
{
    for (const LayoutItem &item : items) {
        if (item.specialType == LayoutBuilder::SpecialType::Title) {
            auto box = qobject_cast<QGroupBox *>(parentWidget());
            QTC_ASSERT(box, continue);
            box->setTitle(item.specialValue.toString());
        } else {
            addItem(item);
        }
    }
}

Box::Box(LayoutType type, const LayoutItems &items)
    : LayoutBuilder(type, items)
{}

} // Layouting
} // Utils
