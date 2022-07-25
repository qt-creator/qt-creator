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

#include "aspects.h"
#include "qtcassert.h"

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
    Constructs a layout item proxy for \a layout.
 */
LayoutBuilder::LayoutItem::LayoutItem(QLayout *layout)
    : layout(layout)
{}

/*!
    Constructs a layout item proxy for \a widget.
 */
LayoutBuilder::LayoutItem::LayoutItem(QWidget *widget)
    : widget(widget)
{}

/*!
    Constructs a layout item representing a \c BaseAspect.

    This ultimately uses the \a aspect's \c addToLayout(LayoutBuilder &) function,
    which in turn can add one or more layout items to the target layout.

    \sa BaseAspect::addToLayout()
 */
LayoutBuilder::LayoutItem::LayoutItem(BaseAspect &aspect)
    : aspect(&aspect)
{}

LayoutBuilder::LayoutItem::LayoutItem(BaseAspect *aspect)
    : aspect(aspect)
{}

/*!
    Constructs a layout item containing some static \a text.
 */
LayoutBuilder::LayoutItem::LayoutItem(const QString &text)
    : text(text)
{}

QLayout *LayoutBuilder::createLayout() const
{
    QLayout *layout = nullptr;
    switch (m_layoutType) {
    case LayoutBuilder::FormLayout: {
        auto formLayout = new QFormLayout;
        formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        layout = formLayout;
        break;
    }
    case LayoutBuilder::GridLayout: {
        auto gridLayout = new QGridLayout;
        layout = gridLayout;
        break;
    }
    case LayoutBuilder::HBoxLayout: {
        auto hboxLayout = new QHBoxLayout;
        layout = hboxLayout;
        break;
    }
    case LayoutBuilder::VBoxLayout: {
        auto vboxLayout = new QVBoxLayout;
        layout = vboxLayout;
        break;
    }
    }
    QTC_ASSERT(layout, return nullptr);
    if (m_spacing)
        layout->setSpacing(*m_spacing);
    return layout;
}

static void setMargins(bool on, QLayout *layout)
{
    if (!on)
        layout->setContentsMargins(0, 0, 0, 0);
}

static QWidget *widgetForItem(QLayoutItem *item)
{
    if (QWidget *w = item->widget())
        return w;
    if (item->spacerItem())
        return nullptr;
    QLayout *l = item->layout();
    if (!l)
        return nullptr;
    for (int i = 0, n = l->count(); i < n; ++i) {
        if (QWidget *w = widgetForItem(l->itemAt(i)))
            return w;
    }
    return nullptr;
}

static void addItemToBoxLayout(QBoxLayout *layout, const LayoutBuilder::LayoutItem &item)
{
    if (QWidget *w = item.widget) {
        layout->addWidget(w);
    } else if (QLayout *l = item.layout) {
        layout->addLayout(l);
    } else if (item.specialType == LayoutBuilder::SpecialType::Stretch) {
        layout->addStretch(item.specialValue.toInt());
    } else if (item.specialType == LayoutBuilder::SpecialType::Space) {
        layout->addSpacing(item.specialValue.toInt());
    } else if (!item.text.isEmpty()) {
        layout->addWidget(new QLabel(item.text));
    } else {
        QTC_CHECK(false);
    }
}

static void flushPendingFormItems(QFormLayout *formLayout,
                                  LayoutBuilder::LayoutItems &pendingFormItems)
{
    QTC_ASSERT(formLayout, return);

    if (pendingFormItems.empty())
        return;

    // If there are more than two items, we cram the last ones in one hbox.
    if (pendingFormItems.size() > 2) {
        auto hbox = new QHBoxLayout;
        hbox->setContentsMargins(0, 0, 0, 0);
        for (int i = 1; i < pendingFormItems.size(); ++i)
            addItemToBoxLayout(hbox, pendingFormItems.at(i));
        while (pendingFormItems.size() >= 2)
            pendingFormItems.pop_back();
        pendingFormItems.append(LayoutBuilder::LayoutItem(hbox));
    }

    if (pendingFormItems.size() == 1) { // One one item given, so this spans both columns.
        if (auto layout = pendingFormItems.at(0).layout)
            formLayout->addRow(layout);
        else if (auto widget = pendingFormItems.at(0).widget)
            formLayout->addRow(widget);
    } else if (pendingFormItems.size() == 2) { // Normal case, both columns used.
        if (auto label = pendingFormItems.at(0).widget) {
            if (auto layout = pendingFormItems.at(1).layout)
                formLayout->addRow(label, layout);
            else if (auto widget = pendingFormItems.at(1).widget)
                formLayout->addRow(label, widget);
        } else  {
            if (auto layout = pendingFormItems.at(1).layout)
                formLayout->addRow(pendingFormItems.at(0).text, layout);
            else if (auto widget = pendingFormItems.at(1).widget)
                formLayout->addRow(pendingFormItems.at(0).text, widget);
        }
    } else {
        QTC_CHECK(false);
    }

    // Set up label as buddy if possible.
    const int lastRow = formLayout->rowCount() - 1;
    QLayoutItem *l = formLayout->itemAt(lastRow, QFormLayout::LabelRole);
    QLayoutItem *f = formLayout->itemAt(lastRow, QFormLayout::FieldRole);
    if (l && f) {
        if (QLabel *label = qobject_cast<QLabel *>(l->widget())) {
            if (QWidget *widget = widgetForItem(f))
                label->setBuddy(widget);
        }
    }

    pendingFormItems.clear();
}

static void doLayoutHelper(QLayout *layout,
                           const LayoutBuilder::LayoutItems &items,
                           int currentGridRow = 0)
{
    int currentGridColumn = 0;
    LayoutBuilder::LayoutItems pendingFormItems;

    auto formLayout = qobject_cast<QFormLayout *>(layout);
    auto gridLayout = qobject_cast<QGridLayout *>(layout);
    auto boxLayout = qobject_cast<QBoxLayout *>(layout);

    for (const LayoutBuilder::LayoutItem &item : items) {
        if (item.specialType == LayoutBuilder::SpecialType::Break) {
            if (formLayout)
                flushPendingFormItems(formLayout, pendingFormItems);
            else if (gridLayout) {
                if (currentGridColumn != 0) {
                    ++currentGridRow;
                    currentGridColumn = 0;
                }
            }
            continue;
        }

        QWidget *widget = item.widget;

        if (gridLayout) {
            Qt::Alignment align;
            if (item.align == LayoutBuilder::AlignmentType::AlignAsFormLabel)
                align = Qt::Alignment(widget->style()->styleHint(QStyle::SH_FormLayoutLabelAlignment));
            if (widget)
                gridLayout->addWidget(widget, currentGridRow, currentGridColumn, 1, item.span, align);
            else if (item.layout)
                gridLayout->addLayout(item.layout, currentGridRow, currentGridColumn, 1, item.span, align);
            else if (!item.text.isEmpty())
                gridLayout->addWidget(new QLabel(item.text));
            currentGridColumn += item.span;
        } else if (boxLayout) {
            addItemToBoxLayout(boxLayout, item);
        } else {
            pendingFormItems.append(item);
        }
    }

    if (formLayout)
        flushPendingFormItems(formLayout, pendingFormItems);
}


/*!
    Constructs a layout item from the contents of another LayoutBuilder
 */
LayoutBuilder::LayoutItem::LayoutItem(const LayoutBuilder &builder)
{
    layout = builder.createLayout();
    doLayoutHelper(layout, builder.m_items);
}

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

LayoutBuilder::LayoutBuilder(LayoutType layoutType, const LayoutItems &items)
    : m_layoutType(layoutType)
{
    m_items.reserve(items.size() * 2);
    for (const LayoutItem &item : items)
        addItem(item);
}

LayoutBuilder &LayoutBuilder::setSpacing(int spacing)
{
    m_spacing = spacing;
    return *this;
}

LayoutBuilder::LayoutBuilder() = default;

/*!
    Destructs a layout builder.
 */
LayoutBuilder::~LayoutBuilder() = default;

/*!
    Instructs a layout builder to finish the current row.
    This is implicitly called by LayoutBuilder's destructor.
 */
LayoutBuilder &LayoutBuilder::finishRow()
{
    addItem(Break());
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
    Adds the layout item \a item to the current row.
 */
LayoutBuilder &LayoutBuilder::addItem(const LayoutItem &item)
{
    if (item.aspect) {
        item.aspect->addToLayout(*this);
        if (m_layoutType == FormLayout || m_layoutType == VBoxLayout)
            finishRow();
    } else {
        m_items.push_back(item);
    }
    return *this;
}

void LayoutBuilder::doLayout(QWidget *parent, bool withMargins) const
{
    QLayout *layout = createLayout();
    parent->setLayout(layout);

    doLayoutHelper(layout, m_items);
    setMargins(withMargins, layout);
}

/*!
    Adds the layout item \a items to the current row.
 */
LayoutBuilder &LayoutBuilder::addItems(const LayoutItems &items)
{
    for (const LayoutItem &item : items)
        addItem(item);
    return *this;
}

/*!
    Attach the constructed layout to the provided \c QWidget \a parent.

    This operation can only be performed once per LayoutBuilder instance.
 */
void LayoutBuilder::attachTo(QWidget *w, bool withMargins) const
{
    doLayout(w, withMargins);
}

QWidget *LayoutBuilder::emerge(bool withMargins)
{
    auto w = new QWidget;
    doLayout(w, withMargins);
    return w;
}

/*!
    Constructs a layout extender to extend an existing \a layout.

    This constructor can be used to continue the work of previous layout building.
    The type of the underlying layout and previous contents will be retained,
    new items will be added below existing ones.
 */

LayoutExtender::LayoutExtender(QLayout *layout)
    : m_layout(layout)
{}

LayoutExtender::~LayoutExtender()
{
    QTC_ASSERT(m_layout, return);
    int currentGridRow = 0;
    if (auto gridLayout = qobject_cast<QGridLayout *>(m_layout))
        currentGridRow = gridLayout->rowCount();
    doLayoutHelper(m_layout, m_items, currentGridRow);
}

// Special items

LayoutBuilder::Break::Break()
{
    specialType = SpecialType::Break;
}

LayoutBuilder::Stretch::Stretch(int stretch)
{
    specialType = SpecialType::Stretch;
    specialValue = stretch;
}

LayoutBuilder::Space::Space(int space)
{
    specialType = SpecialType::Space;
    specialValue = space;
}

LayoutBuilder::Span::Span(int span_, const LayoutItem &item)
{
    LayoutBuilder::LayoutItem::operator=(item);
    span = span_;
}

LayoutBuilder::AlignAsFormLabel::AlignAsFormLabel(const LayoutItem &item)
{
    LayoutBuilder::LayoutItem::operator=(item);
    align = AlignmentType::AlignAsFormLabel;
}

namespace Layouting {

Group::Group(const LayoutBuilder &innerLayout)
    : Group({}, innerLayout)
{}

Group::Group(const LayoutBuilder::Setters &setters, const LayoutBuilder &innerLayout)
{
    auto box = new QGroupBox;
    innerLayout.attachTo(box, true);
    for (const LayoutBuilder::Setter &func : setters)
        func(box);
    widget = box;
}

LayoutBuilder::Setter Title(const QString &title, BoolAspect *checker)
{
    return Layouting::title(title, checker);
}

LayoutBuilder::Setter title(const QString &title, BoolAspect *checker)
{
    return [title, checker](QObject *target) {
        if (auto groupBox = qobject_cast<QGroupBox *>(target)) {
            groupBox->setTitle(title);
            groupBox->setObjectName(title);
            if (checker) {
                groupBox->setCheckable(true);
                groupBox->setChecked(checker->value());
                checker->setHandlesGroup(groupBox);
            }
        } else {
            QTC_CHECK(false);
        }
    };
}

LayoutBuilder::Break br;
LayoutBuilder::Stretch st;
LayoutBuilder::Space empty(0);

} // Layouting
} // Utils
