// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "layoutbuilder.h"

#include "algorithm.h"
#include "completingtextedit.h"
#include "fancylineedit.h"
#include "filepath.h"
#include "guiutils.h"
#include "hostosinfo.h"
#include "icon.h"
#include "markdownbrowser.h"
#include "qtcassert.h"
#include "spinner/spinner.h"

#include <QCloseEvent>
#include <QDebug>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QShortcut>
#include <QSize>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedLayout>
#include <QStackedWidget>
#include <QStyle>
#include <QTabWidget>
#include <QTextEdit>
#include <QToolBar>
#include <QToolButton>

using namespace Layouting::Tools;

/*!
    \namespace Layouting
    \inmodule QtCreator

    \brief The Layouting namespace contains classes and functions to conveniently
    create layouts in code.

    Classes in the namespace help to create QLayout or QWidget derived classes.
    Instances should be used locally within a function and never stored.

    \sa Layouting::Widget, Layouting::Layout
*/

namespace Layouting {

// FlowLayout

class FlowLayout : public QLayout
{
public:
    explicit FlowLayout(QWidget *parent, int margin = -1, int vSpacing = -1)
        : QLayout(parent)
        , m_vSpace(vSpacing)
    {
        setContentsMargins(margin, margin, margin, margin);
    }

    FlowLayout(int margin = -1, int vSpacing = -1)
        : m_vSpace(vSpacing)
    {
        setContentsMargins(margin, margin, margin, margin);
    }

    ~FlowLayout() override
    {
        qDeleteAll(itemList);
        itemList.clear();
    }

    void addItem(QLayoutItem *item) override { itemList.append(item); }

    int horizontalSpacing() const { return spacing(); }

    int verticalSpacing() const
    {
        if (m_vSpace >= 0)
            return m_vSpace;
        else
            return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
    }

    Qt::Orientations expandingDirections() const override { return {}; }

    bool hasHeightForWidth() const override { return true; }

    int heightForWidth(int width) const override
    {
        int height = doLayout(QRect(0, 0, width, 0), true);
        return height;
    }

    int count() const override { return itemList.size(); }

    QLayoutItem *itemAt(int index) const override { return itemList.value(index); }

    QSize minimumSize() const override
    {
        QSize size;
        for (QLayoutItem *item : itemList)
            size = size.expandedTo(item->minimumSize());

        int left, top, right, bottom;
        getContentsMargins(&left, &top, &right, &bottom);
        size += QSize(left + right, top + bottom);
        return size;
    }

    QSize preferredSize() const
    {
        QSize size(0, 0);
        for (int i = 0; i < itemList.size(); ++i) {
            QLayoutItem *item = itemList.at(i);
            QWidget *wid = item->widget();
            // Last item doesn't have spacing to the right
            int spaceX = i < itemList.size() - 1 ? horizontalSpacing() : 0;
            if (spaceX < 0) {
                if (i < itemList.size() - 1) {
                    spaceX = wid->style()->combinedLayoutSpacing(
                        item->controlTypes(), itemList.at(i + 1)->controlTypes(), Qt::Horizontal);
                }
                if (spaceX < 0)
                    spaceX = 0;
            }
            size.setWidth(size.width() + item->sizeHint().width() + spaceX);
            size.setHeight(std::max(size.height(), item->sizeHint().height()));
        }

        int left, top, right, bottom;
        getContentsMargins(&left, &top, &right, &bottom);
        size += QSize(left + right, top + bottom);
        return size;
    }

    void setGeometry(const QRect &rect) override
    {
        QLayout::setGeometry(rect);
        doLayout(rect, false);
    }

    QSize sizeHint() const override { return preferredSize(); }

    QLayoutItem *takeAt(int index) override
    {
        if (index >= 0 && index < itemList.size())
            return itemList.takeAt(index);
        else
            return nullptr;
    }

private:
    int doLayout(const QRect &rect, bool testOnly) const
    {
        int left, top, right, bottom;
        getContentsMargins(&left, &top, &right, &bottom);
        if (Utils::HostOsInfo::isMacHost() && Utils::anyOf(itemList, [](QLayoutItem *i) {
                return qobject_cast<QPushButton *>(i->widget());
            })) {
            // TODO Find a more generic way, maybe through QBoxLayout::effectiveMargins?
            // This prevents push buttons to be downgraded to square button style.
            // QLayoutItem::setGeometry modifies the rect by
            //   QWidgetPrivate::(left|right|top|bottom)LayoutItemMargin.
            // That is set by QPushButton::resetLayoutItemMargins() via the style's
            // subElementRect for QStyle::SE_PushButtonLayoutItem.
            // That is -4 from the top and +8 to the bottom for normal (SizeLarge) QPushButton,
            // which results in sizeHint 20 + 12 = 32, which is the value that is checked against
            // for the decision if the QPushButton can remain a normal QPushButton or is changed
            // to square button style.
            // If the modified rect's y-position is < 0, then the rect is is moved down and cut
            // smaller by that amount, which then results in a size != 32 and a square button.
            top = std::max(top, 4);
            bottom = std::max(bottom, 4); // no need to enforce the full 8, just go symmetric
        }
        const QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
        int x = effectiveRect.x();
        int y = effectiveRect.y();
        int lineHeight = 0;

        QList<QRect> geometries;
        geometries.resize(itemList.size());
        const auto setItemGeometries =
            [this, &geometries, effectiveRect](int count, int beforeIndex, int lineHeight) {
                QTC_ASSERT(count > 0, return);
                QTC_ASSERT(beforeIndex - count >= 0, return);
                // Move to the right edge if alignment is Right.
                const int xd = alignment() & Qt::AlignRight
                                   ? (effectiveRect.right() - geometries.at(count - 1).right())
                                   : 0;
                for (int j = 0; j < count; ++j) {
                    // Vertically center the items.
                    QLayoutItem *currentItem = itemList.at(beforeIndex - count + j);
                    const QRect geom = geometries.at(j);
                    currentItem->setGeometry(QRect(
                        QPoint(geom.x() + xd, geom.y() + (lineHeight - geom.height()) / 2),
                        geom.size()));
                }
            };
        int indexInRow = 0;
        for (int i = 0; i < itemList.size(); ++i) {
            QLayoutItem *item = itemList.at(i);
            QWidget *wid = item->widget();
            // Last item doesn't have spacing to the right
            int spaceX = i < itemList.size() - 1 ? horizontalSpacing() : 0;
            if (spaceX < 0) {
                if (i < itemList.size() - 1) {
                    spaceX = wid->style()->combinedLayoutSpacing(
                        item->controlTypes(), itemList.at(i + 1)->controlTypes(), Qt::Horizontal);
                }
                if (spaceX < 0)
                    spaceX = 0;
            }
            const QSize itemSizeHint = item->sizeHint();
            int nextX = x + itemSizeHint.width() + spaceX;
            // The "right-most x-coordinate" of item is x + width - 1
            if (nextX - spaceX - 1 > effectiveRect.right() && lineHeight > 0) {
                // The item doesn't fit and it isn't the only one, finish the row.
                if (!testOnly)
                    setItemGeometries(/*count=*/indexInRow, /*beforeIndex=*/i, lineHeight);

                int spaceY = verticalSpacing();
                if (spaceY < 0) {
                    spaceY = wid->style()->layoutSpacing(
                        QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);
                }
                if (spaceY < 0)
                    spaceY = 0;
                x = effectiveRect.x();
                y = y + lineHeight + spaceY;
                nextX = x + itemSizeHint.width() + spaceX;
                lineHeight = 0;
                indexInRow = 0;
            }
            geometries[indexInRow] = QRect(QPoint(x, y), itemSizeHint);

            x = nextX;
            lineHeight = qMax(lineHeight, itemSizeHint.height());
            ++indexInRow;
        }
        // Finish the last row.
        if (!testOnly && !itemList.isEmpty())
            setItemGeometries(/*count=*/indexInRow, /*beforeIndex=*/itemList.size(), lineHeight);

        return y + lineHeight - rect.y() + bottom;
    }

    // see qboxlayout.cpp qSmartSpacing
    int smartSpacing(QStyle::PixelMetric pm) const
    {
        QObject *parent = this->parent();
        if (!parent) {
            return -1;
        } else if (parent->isWidgetType()) {
            auto pw = static_cast<QWidget *>(parent);
            return pw->style()->pixelMetric(pm, nullptr, pw);
        } else {
            return static_cast<QLayout *>(parent)->spacing();
        }
    }

    QList<QLayoutItem *> itemList;
    int m_vSpace;
};

/*!
    \class Layouting::Layout
    \inmodule QtCreator
    \brief Base class for layout builder classes.

    Wraps a QLayout-derived object. Instances are ephemeral: construct, configure,
    and call attachTo() or emerge(). Do not store them beyond the enclosing function.

    Subclasses accumulate items via addItem(), addItems(), and addRow(), which are
    flushed into the underlying QLayout when the builder is finalized. For layouts
    that support rows and spans (Form, Grid), items are staged and committed to the
    layout by flush().

    \sa Layouting::Widget, Layouting::Column, Layouting::Row, Layouting::Form, Layouting::Grid
*/

/*!
    \class Layouting::Widget
    \inmodule QtCreator
    \brief Base class for widget builder classes.

    Wraps a QWidget-derived object. Instances are ephemeral: construct and configure.
    They call emerge() to retrieve the underlying QWidget, or nest the builder inside
    a Layout to embed it directly.

    \sa Layouting::Layout
*/

/*!
    \class Layouting::LayoutItem
    \inmodule QtCreator

    The LayoutItem class is used for intermediate results
    while creating layouts with a concept of rows and spans, such
    as Form and Grid.
*/

LayoutItem::LayoutItem() = default;

LayoutItem::~LayoutItem() = default;

LayoutItem::LayoutItem(QLayout *l)
    : layout(l)
    , empty(!l)
{}

LayoutItem::LayoutItem(QWidget *w)
    : widget(w)
    , empty(!w)
{}

LayoutItem::LayoutItem(const QString &t)
    : text(t)
    , empty(t.isEmpty())
{}

/*!
    \fn  template <class T> LayoutItem(const T &t)
    \internal

    Constructs a layout item proxy for \a t.

    T could be
    \list
    \li  \c {QString}
    \li  \c {QWidget *}
    \li  \c {QLayout *}
    \endlist
*/

// Object

Object::Object(std::initializer_list<I> ps)
{
    ptr = new Implementation;
    apply(this, ps);
}

void Object::setObjectName(const QString &objectName)
{
    access(this)->setObjectName(objectName);
}

void Object::onDestroyed(QObject *guard, const std::function<void()> &func)
{
    QObject::connect(access(this), &QObject::destroyed, guard, func);
}

static QWidget *widgetForItem(QLayoutItem *item)
{
    if (QWidget *w = item->widget())
        return w;
    if (item->spacerItem())
        return nullptr;
    if (QLayout *l = item->layout()) {
        for (int i = 0, n = l->count(); i < n; ++i) {
            if (QWidget *w = widgetForItem(l->itemAt(i)))
                return w;
        }
    }
    return nullptr;
}

static QLabel *createLabel(const QString &text)
{
    auto label = new QLabel(text);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
}

static void addItemToBoxLayout(QBoxLayout *layout, const LayoutItem &item)
{
    if (QWidget *w = item.widget) {
        layout->addWidget(w);
    } else if (QLayout *l = item.layout) {
        layout->addLayout(l);
    } else if (item.stretch != -1) {
        layout->addStretch(item.stretch);
    } else if (!item.text.isEmpty()) {
        layout->addWidget(createLabel(item.text));
    } else if (item.empty) {
        // Nothing to do, but no reason to warn, either.
    } else {
        QTC_CHECK(false);
    }
}

static void addItemToFlowLayout(FlowLayout *layout, const LayoutItem &item)
{
    if (QWidget *w = item.widget) {
        layout->addWidget(w);
    } else if (QLayout *l = item.layout) {
        layout->addItem(l);
//    } else if (item.stretch != -1) {
//        layout->addStretch(item.stretch);
    } else if (item.empty) {
        // Nothing to do, but no reason to warn, either
    } else if (!item.text.isEmpty()) {
        layout->addWidget(createLabel(item.text));
    } else {
        QTC_CHECK(false);
    }
}

/*!
    \class Layouting::Space
    \inmodule QtCreator

    \brief The Space class represents some empty space in a layout.
 */

/*!
    \class Layouting::Stretch
    \inmodule QtCreator

    \brief The Stretch class represents some stretch in a layout.
 */

// Layout
Layout::Layout(Implementation *w)
{
    ptr = w;
}

/*!
    Sets the column span to \a cols and the row span to \a rows on the last item
    that was added to this layout. Only meaningful for Grid layouts.

    \sa Layouting::Span, Layouting::SpanAll
*/
void Layout::span(int cols, int rows)
{
    QTC_ASSERT(!pendingItems.empty(), return);
    pendingItems.back().spanCols = cols;
    pendingItems.back().spanRows = rows;
}

/*!
    Sets the alignment to \a alignment on the last item that was added to this layout.
    Only meaningful for Grid and Form layouts.
*/
void Layout::align(Qt::Alignment alignment)
{
    QTC_ASSERT(!pendingItems.empty(), return);
    pendingItems.back().alignment = alignment;
}

void Layout::setAlignment(Qt::Alignment alignment)
{
    access(this)->setAlignment(alignment);
}

/*!
    Sets all four content margins to zero.

    \sa Layout::setNormalMargins(), Layouting::noMargin
*/
void Layout::setNoMargins()
{
    setContentsMargins(0, 0, 0, 0);
}

/*!
    Sets all four content margins to 9 pixels.

    \sa Layout::setNoMargins(), Layouting::normalMargin
*/
void Layout::setNormalMargins()
{
    setContentsMargins(9, 9, 9, 9);
}

void Layout::setContentsMargins(int left, int top, int right, int bottom)
{
    access(this)->setContentsMargins(left, top, right, bottom);
}

/*!
    Attaches the constructed layout to the provided QWidget \a widget.

    This operation can only be performed once per LayoutBuilder instance.
 */
void Layout::attachTo(QWidget *widget)
{
    flush();
    widget->setLayout(access(this));
}

/*!
    Adds the layout item \a item as a sub item.
 */
void Layout::addItem(I item)
{
    item.apply(this);
}

void Layout::addLayoutItem(const LayoutItem &item)
{
    if (QBoxLayout *lt = asBox())
        addItemToBoxLayout(lt, item);
    else if (FlowLayout *lt = asFlow())
        addItemToFlowLayout(lt, item);
    else
        pendingItems.push_back(item);
}

/*!
    Adds the layout items \a items as sub items.
 */
void Layout::addItems(std::initializer_list<I> items)
{
    for (const I &item : items)
        item.apply(this);
}

/*!
    Starts a new row containing \a items. The row can be further extended by
    other items using \c addItem() or \c addItems().

    \sa addItem(), addItems()
 */

void Layout::addRow(std::initializer_list<I> items)
{
    for (const I &item : items)
        item.apply(this);
    flush();
}

void Layout::setSpacing(int spacing)
{
    access(this)->setSpacing(spacing);
}

void Layout::setColumnStretch(int column, int stretch)
{
    if (auto grid = qobject_cast<QGridLayout *>(access(this))) {
        grid->setColumnStretch(column, stretch);
    } else {
        QTC_CHECK(false);
    }
}

void Layout::setRowStretch(int row, int stretch)
{
    if (auto grid = qobject_cast<QGridLayout *>(access(this))) {
        grid->setRowStretch(row, stretch);
    } else {
        QTC_CHECK(false);
    }
}

void addToWidget(Widget *widget, const Layout &layout)
{
    layout.flush_();
    access(widget)->setLayout(access(&layout));
}

void addToWidget(Widget *widget, const WidgetModifier &inner)
{
    inner(widget);
}

void addToLayout(Layout *layout, const Widget &inner)
{
    layout->addLayoutItem(access(&inner));
}

void addToLayout(Layout *layout, QWidget *inner)
{
    layout->addLayoutItem(inner);
}

void addToLayout(Layout *layout, QWidget &inner)
{
    layout->addLayoutItem(&inner);
}

void addToLayout(Layout *layout, QLayout *inner)
{
    layout->addLayoutItem(inner);
}

void addToLayout(Layout *layout, const Layout &inner)
{
    inner.flush_();
    layout->addLayoutItem(access(&inner));
}

void addToLayout(Layout *layout, const LayoutModifier &inner)
{
    inner(layout);
}

void addToLayout(Layout *layout, const QString &inner)
{
    layout->addLayoutItem(inner);
}

/*!
    \fn void Layouting::empty(Layout *layout)
    Adds an empty (no-op) placeholder item to \a layout. Useful as a filler in
    Grid or Form rows where a cell should be left blank.
*/
void empty(Layout *layout)
{
    LayoutItem item;
    item.empty = true;
    layout->addLayoutItem(item);
}

/*!
    \fn void Layouting::hr(Layout *layout)
    Adds a horizontal rule (a sunken QFrame) to \a layout.

    \sa Layouting::createHr()
*/
void hr(Layout *layout)
{
    layout->addLayoutItem(createHr());
}

/*!
    \fn void Layouting::br(Layout *layout)
    Starts a new row in \a layout by calling flush(). Use in Grid and Form
    initializer lists to end the current row.

    \sa Layout::flush(), Layout::addRow()
*/
void br(Layout *layout)
{
    layout->flush();
}

/*!
    \fn void Layouting::st(Layout *layout)
    Adds a stretch factor of 1 to \a layout. Only effective in box (Column/Row) layouts.

    \sa Layouting::Stretch, Layouting::spacing()
*/
void st(Layout *layout)
{
    LayoutItem item;
    item.stretch = 1;
    layout->addLayoutItem(item);
}

/*!
    \fn void Layouting::noMargin(Layout *layout)
    Sets all content margins of \a layout to zero.

    \sa Layout::setNoMargins(), Layouting::normalMargin(), Layouting::tight()
*/
void noMargin(Layout *layout)
{
    layout->setNoMargins();
}

/*!
    \fn void Layouting::normalMargin(Layout *layout)
    Sets all content margins of \a layout to 9 pixels.

    \sa Layout::setNormalMargins(), Layouting::noMargin()
*/
void normalMargin(Layout *layout)
{
    layout->setNormalMargins();
}

QFormLayout *Layout::asForm()
{
    return qobject_cast<QFormLayout *>(access(this));
}

QGridLayout *Layout::asGrid()
{
    return qobject_cast<QGridLayout *>(access(this));
}

QBoxLayout *Layout::asBox()
{
    return qobject_cast<QBoxLayout *>(access(this));
}

FlowLayout *Layout::asFlow()
{
    return dynamic_cast<FlowLayout *>(access(this));
}

/*!
    Commits all accumulated pending items into the underlying QLayout.

    For Grid layouts each call to flush() advances to the next row. For Form layouts
    each flush() call adds one label-field row (or a single full-width row). For box
    and flow layouts items are added directly and flush() is a no-op.

    flush() is called automatically at the end of a constructor that accepts an
    initializer list, and by attachTo() and emerge(). Call it manually only when
    building a layout step by step with addItem() or addRow().

    \sa Layout::attachTo(), Layout::emerge()
*/
void Layout::flush()
{
    if (pendingItems.empty())
        return;

    if (QGridLayout *lt = asGrid()) {
        int maxSpanCols = 0;
        bool previousItemDidAdvanceCell = false;

        for (const LayoutItem &item : std::as_const(pendingItems)) {
            if (!previousItemDidAdvanceCell && item.advancesCell) {
                currentGridColumn += maxSpanCols;
                maxSpanCols = 0;
            }

            Qt::Alignment a = item.alignment;
            // FIXME: Check whether this is needed
            if (currentGridColumn == 0 && useFormAlignment) {
                // if (auto widget = builder.stack.at(builder.stack.size() - 2).widget) {
                //     a = widget->style()->styleHint(QStyle::SH_FormLayoutLabelAlignment);
            }

            if (item.widget) {
                lt->addWidget(
                    item.widget, currentGridRow, currentGridColumn, item.spanRows, item.spanCols, a);
            } else if (item.layout) {
                lt->addLayout(
                    item.layout, currentGridRow, currentGridColumn, item.spanRows, item.spanCols, a);
            } else if (!item.text.isEmpty()) {
                lt->addWidget(
                    createLabel(item.text),
                    currentGridRow,
                    currentGridColumn,
                    item.spanRows,
                    item.spanCols,
                    a);
            }
            maxSpanCols = std::max(maxSpanCols, item.spanCols);
            if (item.advancesCell) {
                currentGridColumn += maxSpanCols;
                maxSpanCols = 0;
            }

            previousItemDidAdvanceCell = item.advancesCell;
        }
        ++currentGridRow;
        currentGridColumn = 0;
        pendingItems.clear();
        return;
    }

    if (QFormLayout *fl = asForm()) {
        if (pendingItems.size() > 2) {
            auto hbox = new QHBoxLayout;
            hbox->setContentsMargins(0, 0, 0, 0);
            for (size_t i = 1; i < pendingItems.size(); ++i)
                addItemToBoxLayout(hbox, pendingItems.at(i));
            while (pendingItems.size() > 1)
                pendingItems.pop_back();
            pendingItems.push_back(hbox);
        }

        if (pendingItems.size() == 1) { // Only one item given, so this spans both columns.
            const LayoutItem &f0 = pendingItems.at(0);
            if (auto layout = f0.layout)
                fl->addRow(layout);
            else if (auto widget = f0.widget)
                fl->addRow(widget);
        } else if (pendingItems.size() == 2) { // Normal case, both columns used.
            LayoutItem &f1 = pendingItems[1];
            const LayoutItem &f0 = pendingItems.at(0);
            if (!f1.widget && !f1.layout && !f1.text.isEmpty())
                f1.widget = createLabel(f1.text);

            // QFormLayout accepts only widgets or text in the first column.
            // FIXME: Should we be more generous?
            if (f0.widget) {
                if (f1.layout)
                    fl->addRow(f0.widget, f1.layout);
                else if (f1.widget)
                    fl->addRow(f0.widget, f1.widget);
            } else  {
                if (f1.layout)
                    fl->addRow(createLabel(f0.text), f1.layout);
                else if (f1.widget)
                    fl->addRow(createLabel(f0.text), f1.widget);
            }
        } else {
            QTC_CHECK(false);
        }

        // Set up label as buddy if possible.
        const int lastRow = fl->rowCount() - 1;
        QLayoutItem *l = fl->itemAt(lastRow, QFormLayout::LabelRole);
        QLayoutItem *f = fl->itemAt(lastRow, QFormLayout::FieldRole);
        if (l && f) {
            if (QLabel *label = qobject_cast<QLabel *>(l->widget())) {
                if (QWidget *widget = widgetForItem(f))
                    label->setBuddy(widget);
            }
        }

        pendingItems.clear();
        return;
    }

    QTC_CHECK(false); // The other layouts shouldn't use flush()
}

void Layout::flush_() const
{
    const_cast<Layout *>(this)->flush();
}

/*!
    \fn void Layouting::withFormAlignment(Layout *layout)
    Intended to enable form-style label alignment for \a layout. Not yet implemented.
*/
void withFormAlignment(Layout *layout)
{
    layout->useFormAlignment = true;
}

/*!
    \class Layouting::Flow
    \inmodule QtCreator
    \brief Arranges items in a left-to-right flow that wraps to a new line when the
    available width is exceeded.

    Backed by a custom \c FlowLayout. Margins and spacing can be adjusted via the
    \c noMargin, \c normalMargin, and \c spacing LayoutModifiers.

    \sa Layouting::Row, Layouting::Column, Layouting::Layout
*/

// Flow

Flow::Flow()
{
    ptr = new FlowLayout;
}

Flow::Flow(std::initializer_list<I> ps)
{
    ptr = new FlowLayout;
    apply(this, ps);
    flush();
}

/*!
    \class Layouting::Row
    \inmodule QtCreator
    \brief Arranges items horizontally (QHBoxLayout).

    \sa Layouting::Column, Layouting::Layout
*/

// Row

Row::Row()
{
    ptr = new QHBoxLayout;
}

Row::Row(std::initializer_list<I> ps)
{
    ptr = new QHBoxLayout;
    apply(this, ps);
    flush();
}

/*!
    \class Layouting::Column
    \inmodule QtCreator
    \brief Arranges items vertically (QVBoxLayout).

    \sa Layouting::Row, Layouting::Layout
*/

// Column

Column::Column()
{
    ptr = new QVBoxLayout;
}

Column::Column(std::initializer_list<I> ps)
{
    ptr = new QVBoxLayout;
    apply(this, ps);
    flush();
}

/*!
    \class Layouting::Grid
    \inmodule QtCreator
    \brief Arranges items in a two-dimensional grid (QGridLayout).

    Items are added row by row. Call addRow() or use the \c br free function to start
    a new row. Individual items can span multiple columns or rows via Span or SpanAll
    wrappers, or by calling span() on the layout after adding the item. Multiple items
    that share a single grid cell without advancing the column counter can be grouped
    with GridCell.

    \sa Layouting::Form, Layouting::Span, Layouting::SpanAll, Layouting::GridCell, Layouting::Layout
*/

// Grid

Grid::Grid()
{
    ptr = new QGridLayout;
}

Grid::Grid(std::initializer_list<I> ps)
{
    ptr = new QGridLayout;
    apply(this, ps);
    flush();
}

/*!
    \class Layouting::Form
    \inmodule QtCreator
    \brief Arranges label-field pairs in a two-column form layout (QFormLayout).

    Each call to addRow() defines one row. If a row contains a single item it spans
    both columns (full-width). If it contains two items, the first becomes the label
    and the second the field. A plain \c QString is automatically wrapped in a QLabel.
    Rows with more than two items pack the extra items into an auxiliary QHBoxLayout
    in the field column. QLabel buddies are set automatically when possible.

    By default the field growth policy is \c AllNonFixedFieldsGrow.

    \sa Layouting::Grid, Layouting::Layout
*/

// Form

Form::Form()
{
    ptr = new QFormLayout;
}

Form::Form(std::initializer_list<I> ps)
{
    auto lt = new QFormLayout;
    ptr = lt;
    lt->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    apply(this, ps);
    flush();
}

void Layout::setFieldGrowthPolicy(int policy)
{
    if (auto lt = asForm())
        lt->setFieldGrowthPolicy(QFormLayout::FieldGrowthPolicy(policy));
}

void Layout::setStretch(int index, int stretch)
{
    if (auto lt = asBox())
        lt->setStretch(index, stretch);
}

/*!
    Finalizes the layout and returns it wrapped in a new QWidget.

    This is useful when a layout needs to be embedded as a widget (for example inside
    a QScrollArea or a QStackedWidget) without an explicit outer Widget builder.
    The caller takes ownership of the returned widget.

    \sa Layout::attachTo()
*/
QWidget *Layout::emerge() const
{
    const_cast<Layout *>(this)->flush();
    QWidget *widget = new QWidget;
    widget->setLayout(access(this));
    return widget;
}

void Layout::show() const
{
    auto wdgt = emerge();
    wdgt->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);
    wdgt->show();
}

// "Widgets"

Widget::Widget(std::initializer_list<I> ps)
{
    ptr = new Implementation;
    apply(this, ps);
}

Widget::Widget(Implementation *w)
{
    ptr = w;
}

void Widget::setSize(int w, int h)
{
    access(this)->resize(w, h);
}

void Widget::setFixedSize(const QSize &size)
{
    access(this)->setFixedSize(size);
}

void Widget::setAutoFillBackground(bool on)
{
    access(this)->setAutoFillBackground(on);
}

/*!
    Sets \a layout as the layout of this widget, finalizing it first.
*/
void Widget::setLayout(const Layout &layout)
{
    access(this)->setLayout(access(&layout));
}

void Widget::setWindowTitle(const QString &title)
{
    access(this)->setWindowTitle(title);
}

void Widget::setWindowFlags(Qt::WindowFlags flags)
{
    access(this)->setWindowFlags(flags);
}

void Widget::setWidgetAttribute(Qt::WidgetAttribute attr, bool on)
{
    access(this)->setAttribute(attr, on);
}

void Widget::setToolTip(const QString &title)
{
    access(this)->setToolTip(title);
}

void Widget::show()
{
    access(this)->show();
}

bool Widget::isVisible() const
{
    return access(this)->isVisible();
}

bool Widget::isEnabled() const
{
    return access(this)->isEnabled();
}

void Widget::setVisible(bool visible)
{
    access(this)-> setVisible(visible);
}

void Widget::setEnabled(bool enabled)
{
    access(this)->setEnabled(enabled);
}

void Widget::setNoMargins(int)
{
    setContentsMargins(0, 0, 0, 0);
}

void Widget::setNormalMargins(int)
{
    setContentsMargins(9, 9, 9, 9);
}

void Widget::setContentsMargins(int left, int top, int right, int bottom)
{
    access(this)->setContentsMargins(left, top, right, bottom);
}

void Widget::setCursor(Qt::CursorShape shape)
{
    access(this)->setCursor(shape);
}

void Widget::setMinimumWidth(int minw)
{
    access(this)->setMinimumWidth(minw);
}

void Widget::setMinimumHeight(int height)
{
    access(this)->setMinimumHeight(height);
}

void Widget::setMaximumWidth(int maxWidth)
{
    access(this)->setMaximumWidth(maxWidth);
}

void Widget::setMaximumHeight(int maxHeight)
{
    access(this)->setMaximumHeight(maxHeight);
}

void Widget::setSizePolicy(const QSizePolicy &policy)
{
    access(this)->setSizePolicy(policy);
}

void Widget::activateWindow()
{
    access(this)->activateWindow();
}

void Widget::close()
{
    access(this)->close();
}

/*!
    Returns the underlying QWidget.

    The caller does not take ownership. The widget is owned by the builder's
    parent widget or layout.
*/
QWidget *Widget::emerge() const
{
    return access(this);
}

/*!
    \class Layouting::Label
    \inmodule QtCreator
    \brief Wraps QLabel for use in a Layouting builder.

    \sa Layouting::Widget
*/

// Label

Label::Label(std::initializer_list<I> ps)
{
    ptr = new Implementation;
    apply(this, ps);
}

Label::Label(const QString &text)
{
    ptr = new Implementation;
    setText(text);
}

QString Label::text() const
{
    return access(this)->text();
}

void Label::setText(const QString &text)
{
    access(this)->setText(text);
}

void Label::setTextFormat(Qt::TextFormat format)
{
    access(this)->setTextFormat(format);
}

void Label::setWordWrap(bool on)
{
    access(this)->setWordWrap(on);
}

void Label::setTextInteractionFlags(Qt::TextInteractionFlags flags)
{
    access(this)->setTextInteractionFlags(flags);
}

void Label::setOpenExternalLinks(bool on)
{
    access(this)->setOpenExternalLinks(on);
}

/*!
    Connects \a func to the QLabel::linkHovered signal.

    The connection is automatically removed when \a guard is destroyed, which prevents
    dangling connections. Pass the object whose lifetime determines when the connection
    should be active, typically \c this of the enclosing class.
*/
void Label::onLinkHovered(QObject *guard, const std::function<void (const QString &)> &func)
{
    QObject::connect(access(this), &QLabel::linkHovered, guard, func);
}

/*!
    Connects \a func to the QLabel::linkActivated signal.
    The connection is automatically removed when \a guard is destroyed.
*/
void Label::onLinkActivated(QObject *guard, const std::function<void(const QString &)> &func)
{
    QObject::connect(access(this), &QLabel::linkActivated, guard, func);
}

/*!
    \class Layouting::Group
    \inmodule QtCreator
    \brief Wraps QGroupBox for use in a Layouting builder.

    A title and an optional checkable state can be configured via setTitle() and
    setGroupChecker(). The group's content is provided by nesting a Layout inside it.

    \sa Layouting::Widget
*/

// Group

Group::Group(std::initializer_list<I> ps)
{
    ptr = new Implementation;
    apply(this, ps);
}

void Group::setTitle(const QString &title)
{
    access(this)->setTitle(title);
    access(this)->setObjectName(title);
}

/*!
    Calls \a checker with the underlying QGroupBox, allowing external code to
    configure the group box, for example to make it checkable and bind its
    checked state to a settings key.
*/
void Group::setGroupChecker(const std::function<void (QObject *)> &checker)
{
    checker(access(this));
}

/*!
    \class Layouting::SpinBox
    \inmodule QtCreator
    \brief Wraps QSpinBox for use in a Layouting builder.

    \sa Layouting::Widget
*/

// SpinBox

SpinBox::SpinBox(std::initializer_list<I> ps)
{
    ptr = new Implementation;
    apply(this, ps);
}

void SpinBox::setValue(int val)
{
    access(this)->setValue(val);
}

/*!
    Connects \a func to the QSpinBox::textChanged signal.
    The connection is automatically removed when \a guard is destroyed.
*/
void SpinBox::onTextChanged(QObject *guard, const std::function<void(QString)> &func)
{
    QObject::connect(access(this), &QSpinBox::textChanged, guard, func);
}

/*!
    \class Layouting::TextEdit
    \inmodule QtCreator
    \brief Wraps QTextEdit for use in a Layouting builder.

    Supports both plain text (via setText()) and Markdown content (via setMarkdown()).

    \sa Layouting::CompletingTextEdit, Layouting::LineEdit, Layouting::Widget
*/

// TextEdit

TextEdit::TextEdit(std::initializer_list<I> ps)
{
    ptr = new Implementation;
    apply(this, ps);
}

QString TextEdit::markdown() const
{
    return access(this)->toMarkdown();
}

void TextEdit::setText(const QString &text)
{
    access(this)->setText(text);
}

void TextEdit::setMarkdown(const QString &markdown)
{
    access(this)->setMarkdown(markdown);
}

void TextEdit::setReadOnly(bool on)
{
    access(this)->setReadOnly(on);
}

/*!
    \class Layouting::CompletingTextEdit
    \inmodule QtCreator
    \brief Wraps Utils::CompletingTextEdit for use in a Layouting builder.

    Provides auto-completion, an optional right-side icon, and configurable completion
    behavior in addition to the standard multi-line text editing capabilities of
    TextEdit.

    \sa Layouting::TextEdit, Layouting::LineEdit, Layouting::Widget
*/

// CompletingTextEdit

CompletingTextEdit::CompletingTextEdit(std::initializer_list<I> ps)
{
    ptr = new Implementation;
    apply(this, ps);
}

QString CompletingTextEdit::markdown() const
{
    return access(this)->toMarkdown();
}

QString CompletingTextEdit::text() const
{
    return access(this)->toPlainText();
}

void CompletingTextEdit::setText(const QString &text)
{
    access(this)->setPlainText(text);
}

void CompletingTextEdit::setMarkdown(const QString &markdown)
{
    access(this)->setMarkdown(markdown);
}

void CompletingTextEdit::setReadOnly(bool on)
{
    access(this)->setReadOnly(on);
}

void CompletingTextEdit::setPlaceHolderText(const QString &text)
{
    access(this)->setPlaceholderText(text);
}

void CompletingTextEdit::setRightSideIconPath(const Utils::FilePath &path)
{
    access(this)->setRightSideIconPath(path);
}

void CompletingTextEdit::setCompleter(QCompleter *completer)
{
    access(this)->setCompleter(completer);
}

/*!
    Connects \a func to the textChanged signal, passing the current plain text.
    The connection is automatically removed when \a guard is destroyed.
*/
void CompletingTextEdit::onTextChanged(QObject *guard, const std::function<void(QString)> &func)
{
    QObject::connect(access(this), &Implementation::textChanged, guard, [func, this]() {
        func(access(this)->toPlainText());
    });
}

QCompleter *CompletingTextEdit::completer() const
{
    return access(this)->completer();
}

/*!
    Connects \a func to the CompletingTextEdit::returnPressed signal.
    The connection is automatically removed when \a guard is destroyed.
*/
void CompletingTextEdit::onReturnPressed(
    QObject *guard, const std::function<void(Implementation &)> &func)
{
    QObject::connect(
        access(this),
        &Utils::CompletingTextEdit::returnPressed,
        guard,
        [func, self = access(this)]() { func(*self); });
}

/*!
    Connects \a func to the CompletingTextEdit::rightSideIconClicked signal.
    The connection is automatically removed when \a guard is destroyed.
*/
void CompletingTextEdit::onRightSideIconClicked(QObject *guard, const std::function<void()> &func)
{
    QObject::connect(access(this), &Utils::CompletingTextEdit::rightSideIconClicked, guard, func);
}

Utils::CompletingTextEdit::CompletionBehavior CompletingTextEdit::completionBehavior() const
{
    return access(this)->completionBehavior();
}

void CompletingTextEdit::setCompletionBehavior(Utils::CompletingTextEdit::CompletionBehavior behavior)
{
    access(this)->setCompletionBehavior(behavior);
}

/*!
    \class Layouting::PushButton
    \inmodule QtCreator
    \brief Wraps QPushButton for use in a Layouting builder.

    \sa Layouting::ToolButton, Layouting::Widget
*/

// PushButton

PushButton::PushButton(std::initializer_list<I> ps)
{
    ptr = new Implementation;
    apply(this, ps);
}

void PushButton::setText(const QString &text)
{
    access(this)->setText(text);
}

void PushButton::setIconPath(const Utils::FilePath &iconPath)
{
    if (!iconPath.exists()) {
        access(this)->setIcon({});
        return;
    }

    Utils::Icon icon{iconPath};
    access(this)->setIcon(icon.icon());
}

void PushButton::setIconSize(const QSize &size)
{
    access(this)->setIconSize(size);
}

void PushButton::setFlat(bool flat)
{
    access(this)->setFlat(flat);
}

/*!
    Connects \a func to the QAbstractButton::clicked signal.
    The connection is automatically removed when \a guard is destroyed.
*/
void PushButton::onClicked(QObject *guard, const std::function<void ()> &func)
{
    QObject::connect(access(this), &QAbstractButton::clicked, guard, func);
}

/*!
    \class Layouting::Stack
    \inmodule QtCreator
    \brief Wraps QStackedWidget for use in a Layouting builder.

    Only one child widget is visible at a time. Children are added via the builder
    initializer list. The visible child is controlled through the underlying
    QStackedWidget after construction.

    \sa Layouting::TabWidget, Layouting::Widget
*/

// Stack

// We use a QStackedWidget instead of a QStackedLayout here because the latter will call
// "setVisible()" when a child is added, which can lead to the widget being spawned as a
// top-level widget. This can lead to the focus shifting away from the main application.
Stack::Stack(std::initializer_list<I> ps)
{
    ptr = new Implementation;
    apply(this, ps);
}

void addToStack(Stack *stack, const Widget &inner)
{
    access(stack)->addWidget(inner.emerge());
}

void addToStack(Stack *stack, const Layout &inner)
{
    inner.flush_();
    access(stack)->addWidget(inner.emerge());
}

void addToStack(Stack *stack, QWidget *inner)
{
    access(stack)->addWidget(inner);
}

void addToScrollArea(ScrollArea *scrollArea, QWidget *inner)
{
    access(scrollArea)->setWidget(inner);
}

void addToScrollArea(ScrollArea *scrollArea, const Layout &layout)
{
    access(scrollArea)->setWidget(layout.emerge());
}

void addToScrollArea(ScrollArea *scrollArea, const Widget &inner)
{
    access(scrollArea)->setWidget(inner.emerge());
}

/*!
    \class Layouting::ScrollArea
    \inmodule QtCreator
    \brief Wraps QScrollArea for use in a Layouting builder.

    The scroll area is created with \c widgetResizable set to \c true. The inner
    content can be a Layout, a Widget, or a raw QWidget. By default, sizeHint is
    capped at 36x24 em-widths as a workaround for QTBUG-136762; call
    setFixSizeHintBug(true) to disable the cap.

    \sa Layouting::Widget
*/

// ScrollArea

// See QTBUG-136762
class FixedScrollArea : public QScrollArea
{
public:
    QSize sizeHint() const override
    {
        const int f = 2 * frameWidth();
        QSize sz(f, f);
        const int h = fontMetrics().height();
        if (auto w = widget()) {
            sz += w->sizeHint();
        } else {
            sz += QSize(12 * h, 8 * h);
        }
        if (verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOn)
            sz.setWidth(sz.width() + verticalScrollBar()->sizeHint().width());
        if (horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOn)
            sz.setHeight(sz.height() + horizontalScrollBar()->sizeHint().height());
        if (!m_fixSizeHintBug)
            return sz.boundedTo(QSize(36 * h, 24 * h));
        return sz;
    }

    void setFixSizeHintBug(bool fix) { m_fixSizeHintBug = fix; }

    bool m_fixSizeHintBug{false};
};

ScrollArea::ScrollArea(std::initializer_list<I> items)
{
    ptr = new FixedScrollArea;
    apply(this, items);
    access(this)->setWidgetResizable(true);
}

ScrollArea::ScrollArea(const Layout &inner)
{
    ptr = new FixedScrollArea;
    access(this)->setWidget(inner.emerge());
    access(this)->setWidgetResizable(true);
}

void ScrollArea::setLayout(const Layout &inner)
{
    access(this)->setWidget(inner.emerge());
}

void ScrollArea::setFrameShape(QFrame::Shape shape)
{
    access(this)->setFrameShape(shape);
}

/*!
    When \a fixBug is \c true, disables the default sizeHint cap of 36x24 em-widths
    that works around QTBUG-136762. Enable this when the scroll area must report its
    full preferred size rather than capping it.
*/
void ScrollArea::setFixSizeHintBug(bool fixBug)
{
    auto fixedScrollArea = static_cast<FixedScrollArea *>(access(this));
    fixedScrollArea->setFixSizeHintBug(fixBug);
}

/*!
    \class Layouting::Splitter
    \inmodule QtCreator
    \brief Wraps QSplitter for use in a Layouting builder.

    The default orientation is \c Qt::Vertical. Children can be any combination of
    Layouts, Widgets, or raw QWidgets.

    \sa Layouting::Widget
*/

// Splitter

Splitter::Splitter(std::initializer_list<I> ps)
{
    ptr = new Implementation;
    access(this)->setOrientation(Qt::Vertical);
    apply(this, ps);
}

void Splitter::setOrientation(Qt::Orientation orientation)
{
    access(this)->setOrientation(orientation);
}

void Splitter::setStretchFactor(int index, int stretch)
{
    access(this)->setStretchFactor(index, stretch);
}

void Splitter::setChildrenCollapsible(bool collapsible)
{
    access(this)->setChildrenCollapsible(collapsible);
}

void addToSplitter(Splitter *splitter, QWidget *inner)
{
    access(splitter)->addWidget(inner);
}

void addToSplitter(Splitter *splitter, const Widget &inner)
{
    access(splitter)->addWidget(inner.emerge());
}

void addToSplitter(Splitter *splitter, const Layout &inner)
{
    inner.flush_();
    access(splitter)->addWidget(inner.emerge());
}

/*!
    \class Layouting::ToolBar
    \inmodule QtCreator
    \brief Wraps QToolBar for use in a Layouting builder.

    The toolbar is created with horizontal orientation. Items such as QAction-based
    tool buttons and separators can be added via the initializer list.

    \sa Layouting::ToolButton, Layouting::Widget
*/

// ToolBar

ToolBar::ToolBar(std::initializer_list<I> ps)
{
    ptr = new Implementation;
    apply(this, ps);
    access(this)->setOrientation(Qt::Horizontal);
}

/*!
    \class Layouting::ToolButton
    \inmodule QtCreator
    \brief Wraps QToolButton for use in a Layouting builder.

    \sa Layouting::PushButton, Layouting::Widget
*/

// ToolButton

ToolButton::ToolButton(std::initializer_list<I> ps)
{
    ptr = new Implementation;
    apply(this, ps);
}

void ToolButton::setDefaultAction(QAction *action)
{
    access(this)->setDefaultAction(action);
}

/*!
    \class Layouting::TabWidget
    \inmodule QtCreator
    \brief Wraps QTabWidget for use in a Layouting builder.

    Tabs are added by including Tab items in the initializer list. Each Tab carries
    a tab label and a Layout that defines the tab's content.

    \sa Layouting::Tab, Layouting::Stack, Layouting::Widget
*/

// TabWidget

TabWidget::TabWidget(std::initializer_list<I> ps)
{
    ptr = new Implementation;
    apply(this, ps);
}

Tab::Tab(const QString &tabName, const Layout &inner)
    : tabName(tabName)
    , inner(inner)
{}

void addToTabWidget(TabWidget *tabWidget, const Tab &tab)
{
    access(tabWidget)->addTab(tab.inner.emerge(), tab.tabName);
}

/*!
    \class Layouting::MarkdownBrowser
    \inmodule QtCreator
    \brief Wraps Utils::MarkdownBrowser for use in a Layouting builder.

    Displays rendered Markdown content with optional code copy buttons and configurable
    viewport margins.

    \sa Layouting::TextEdit, Layouting::Widget
*/

// MarkdownBrowser

MarkdownBrowser::MarkdownBrowser(std::initializer_list<I> ps)
{
    ptr = new Implementation;
    apply(this, ps);
}

QString MarkdownBrowser::toMarkdown() const
{
    return access(this)->toMarkdown();
}

void MarkdownBrowser::setMarkdown(const QString &markdown)
{
    access(this)->setMarkdown(markdown);
}

void MarkdownBrowser::setBasePath(const Utils::FilePath &path)
{
    access(this)->setBasePath(path);
}

void MarkdownBrowser::setEnableCodeCopyButton(bool enable)
{
    access(this)->setEnableCodeCopyButton(enable);
}

void MarkdownBrowser::setViewportMargins(int left, int top, int right, int bottom)
{
    access(this)->setMargins(QMargins(left, top, right, bottom));
}

/*!
    \class Layouting::Canvas
    \inmodule QtCreator
    \brief Provides a custom-painted widget surface for use in a Layouting builder.

    The paint function is set via setPaintFunction() and receives a QPainter reference
    on each paint event. The underlying widget is a CanvasWidget.

    \sa Layouting::Widget
*/

/*!
    \class Layouting::CanvasWidget
    \inmodule QtCreator
    \brief A QWidget subclass that delegates painting to a user-supplied function.

    \sa Layouting::Canvas
*/

void CanvasWidget::setPaintFunction(const PaintFunction &paintFunction)
{
    m_paintFunction = std::move(paintFunction);
}

void CanvasWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    if (m_paintFunction) {
        QPainter painter(this);
        m_paintFunction(painter);
    }
}

Canvas::Canvas(std::initializer_list<I> ps)
{
    ptr = new Implementation;
    apply(this, ps);
}

void Canvas::setPaintFunction(const CanvasWidget::PaintFunction &paintFunction)
{
    access(this)->setPaintFunction(paintFunction);
}

// Special Preview

struct PreviewWidget final : public QWidget
{
    void keyPressEvent(QKeyEvent *ev) final
    {
        if (ev->key() == Qt::Key_Escape)
            deleteLater();
        else
            QWidget::keyPressEvent(ev);
    }
};

Preview::Preview(std::initializer_list<I> ps)
    : Column(ps)
{}

void Preview::show() const
{
    auto widget = new PreviewWidget;
    auto layout = access(this);

    auto quit = new QPushButton("Quit Preview", widget);
    QObject::connect(quit, &QAbstractButton::clicked, widget, &QObject::deleteLater);

    auto hbox = new QHBoxLayout;
    hbox->addStretch();
    hbox->addWidget(quit);
    layout->addWidget(createHr());
    layout->addItem(hbox);

    widget->setLayout(layout);
    widget->show();
}

/*!
    \class Layouting::If
    \inmodule QtCreator
    \brief Conditionally includes layout items based on a boolean condition.

    Use the \c >> operator to chain a Then and an optional Else clause:
    \code
        Column {
            If(condition) >> Then { label1, label2 } >> Else { label3 },
        }
    \endcode

    At construction time the condition is evaluated and the matching item list is
    inlined into the enclosing layout.

    \sa Layouting::Then, Layouting::Else
*/

/*!
    \class Layouting::Then
    \inmodule QtCreator
    \brief Holds the list of items to include when the If condition is true.

    \sa Layouting::If, Layouting::Else
*/

/*!
    \class Layouting::Else
    \inmodule QtCreator
    \brief Holds the list of items to include when the If condition is false.

    \sa Layouting::If, Layouting::Then
*/

// Special If

If::If(bool condition)
    : condition(condition)
{}

If::If(bool condition, const Items &list)
    : condition(condition), list(list)
{}

If operator>>(const If &if_, const Then &then_)
{
    return If(if_.condition, if_.condition ? then_.list : if_.list);
}

If operator>>(const If &if_, const Else &else_)
{
    return If(if_.condition, if_.condition ? if_.list : else_.list);
}

void addToLayout(Layout *layout, const If &if_)
{
    for (const Layout::I &item : if_.list)
        item.apply(layout);
}

// Specials

/*!
    Creates and returns a horizontal rule widget (a sunken QFrame with a HLine shape).
    The optional \a parent is passed to the QFrame constructor.

    \sa Layouting::hr()
*/
QWidget *createHr(QWidget *parent)
{
    auto frame = new QFrame(parent);
    frame->setFrameShape(QFrame::HLine);
    frame->setFrameShadow(QFrame::Sunken);
    return frame;
}

/*!
    \class Layouting::Span
    \inmodule QtCreator
    \brief Sets a column and/or row span on a single item in a Grid layout.

    \a cols and \a rows specify how many grid columns and rows the wrapped item
    should occupy. Only the last pending item is affected.

    \sa Layouting::SpanAll, Layouting::Grid
*/

Span::Span(int cols, const Layout::I &item)
    : item(item)
    , spanCols(cols)
{}

Span::Span(int cols, int rows, const Layout::I &item)
    : item(item)
    , spanCols(cols)
    , spanRows(rows)
{}

void addToLayout(Layout *layout, const Span &inner)
{
    layout->addItem(inner.item);
    if (layout->pendingItems.empty()) {
        QTC_CHECK(inner.spanCols == 1 && inner.spanRows == 1);
        return;
    }
    layout->pendingItems.back().spanCols = inner.spanCols;
    layout->pendingItems.back().spanRows = inner.spanRows;
}

/*!
    \class Layouting::SpanAll
    \inmodule QtCreator
    \brief Sets a column and/or row span on all items added by a nested builder.

    Unlike Span, which only affects the last pending item, SpanAll applies the span
    to every item that the wrapped builder produces.

    \sa Layouting::Span, Layouting::Grid
*/

SpanAll::SpanAll(int cols, const Layout::I &item)
    : item(item)
    , spanCols(cols)
{}

SpanAll::SpanAll(int cols, int rows, const Layout::I &item)
    : item(item)
    , spanCols(cols)
    , spanRows(rows)
{}

void addToLayout(Layout *layout, const SpanAll &inner)
{
    size_t nPreviousItems = layout->pendingItems.size();
    layout->addItem(inner.item);
    if (layout->pendingItems.empty()) {
        QTC_CHECK(inner.spanCols == 1 && inner.spanRows == 1);
        return;
    }
    for (size_t i = nPreviousItems; i < layout->pendingItems.size(); ++i) {
        layout->pendingItems.at(i).spanCols = inner.spanCols;
        layout->pendingItems.at(i).spanRows = inner.spanRows;
    }
}

/*!
    \class Layouting::Align
    \inmodule QtCreator
    \brief Sets a Qt::Alignment on all items produced by the wrapped builder.

    \sa Layouting::Grid, Layouting::Form
*/

/*!
    \class Layouting::GridCell
    \inmodule QtCreator
    \brief Groups multiple items into a single grid cell without advancing the column counter.

    Normally each item added to a Grid or Form advances the current column. Use
    GridCell to place several items into the same cell. Only the last item in the
    group advances the column.

    \sa Layouting::Grid
*/

/*!
    \class Layouting::Tab
    \inmodule QtCreator
    \brief Represents a single tab for use inside a TabWidget builder.

    Carries a display name (\c tabName) and a Layout that defines the tab's content.

    \sa Layouting::TabWidget
*/

Align::Align(Qt::Alignment alignment, const Layout::I &item)
    : item(item)
    , alignment(alignment)
{}

void addToLayout(Layout *layout, const Align &inner)
{
    auto nPreviousItems = layout->pendingItems.size();
    layout->addItem(inner.item);
    if (layout->pendingItems.empty()) {
        QTC_CHECK(inner.alignment == Qt::Alignment());
        return;
    }
    for (auto i = nPreviousItems; i < layout->pendingItems.size(); ++i)
        layout->pendingItems.at(i).alignment = inner.alignment;
}

void addToLayout(Layout *layout, const GridCell &inner)
{
    for (auto i : inner.items) {
        i.apply(layout);
        layout->pendingItems.back().advancesCell = false;
    }
}

/*!
    Returns a LayoutModifier that sets the spacing of a layout to \a space pixels.

    \sa Layouting::stretch()
*/
LayoutModifier spacing(int space)
{
    return [space](Layout *layout) { layout->setSpacing(space); };
}

/*!
    Returns a LayoutModifier that sets the stretch factor of item at \a index to \a stretch.

    \sa Layouting::spacing()
*/
LayoutModifier stretch(int index, int stretch)
{
    return [index, stretch](Layout *layout) { layout->setStretch(index, stretch); };
}

void addToLayout(Layout *layout, const Space &inner)
{
    if (auto lt = layout->asBox())
        lt->addSpacing(inner.space);
}

void addToLayout(Layout *layout, const Stretch &inner)
{
    if (auto lt = layout->asBox())
        lt->addStretch(inner.stretch);
}

/*!
    \fn void Layouting::tight(Layout *layout)
    Sets both the content margins and the spacing of \a layout to zero.
    Equivalent to calling noMargin() followed by spacing(0).

    \sa Layouting::noMargin(), Layouting::spacing()
*/
void tight(Layout *layout)
{
    layout->setNoMargins();
    layout->setSpacing(0);
}

/*!
    \fn void Layouting::ignoreDirtyHooks(Widget *widget)
    Marks \a widget so that changes to it do not trigger dirty-state notifications.
    Useful for widgets whose value changes should not mark a settings page or document
    as modified.
*/
void ignoreDirtyHooks(Widget *widget)
{
    Utils::setIgnoreForDirtyHook(widget->emerge());
}


class LineEditImpl : public Utils::FancyLineEdit
{
public:
    using FancyLineEdit::FancyLineEdit;

    void keyPressEvent(QKeyEvent *event) override
    {
        FancyLineEdit::keyPressEvent(event);
        if (acceptReturnKeys && (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return))
            event->accept();
    }

    bool acceptReturnKeys = false;
};

/*!
    \class Layouting::LineEdit
    \inmodule QtCreator
    \brief Wraps Utils::FancyLineEdit for use in a Layouting builder.

    Supports auto-completion, a right-side icon button, a placeholder text, and
    a return-key pressed signal. When onReturnPressed() is connected, the Return
    and Enter keys are accepted (not propagated further).

    \sa Layouting::CompletingTextEdit, Layouting::TextEdit, Layouting::Widget
*/

LineEdit::LineEdit(std::initializer_list<I> ps)
{
    ptr = new LineEditImpl;
    apply(this, ps);
}

QString LineEdit::text() const
{
    return access(this)->text();
}

void LineEdit::setText(const QString &text)
{
    access(this)->setText(text);
}

void LineEdit::setRightSideIconPath(const Utils::FilePath &path)
{
    if (!path.isEmpty()) {
        QIcon icon(path.toFSPathString());
        QTC_CHECK(!icon.isNull());
        access(this)->setButtonIcon(Utils::FancyLineEdit::Right, icon);
        access(this)->setButtonVisible(Utils::FancyLineEdit::Right, !icon.isNull());
    }
}

void LineEdit::setPlaceholderText(const QString &text)
{
    access(this)->setPlaceholderText(text);
}

void LineEdit::setCompleter(QCompleter *completer)
{
    access(this)->setSpecialCompleter(completer);
}

/*!
    Connects \a func to the FancyLineEdit::returnPressed signal and enables
    acceptance of Return/Enter key events so they are not propagated further.
    The connection is automatically removed when \a guard is destroyed.
*/
void LineEdit::onReturnPressed(QObject *guard, const std::function<void(Implementation &)> &func)
{
    static_cast<LineEditImpl *>(access(this))->acceptReturnKeys = true;
    QObject::connect(
        access(this), &Utils::FancyLineEdit::returnPressed, guard, [func, self = access(this)]() {
            func(*self);
        });
}

/*!
    Connects \a func to the FancyLineEdit::rightButtonClicked signal.
    The connection is automatically removed when \a guard is destroyed.
*/
void LineEdit::onRightSideIconClicked(QObject *guard, const std::function<void()> &func)
{
    QObject::connect(access(this), &Utils::FancyLineEdit::rightButtonClicked, guard, func);
}

/*!
    \class Layouting::Spinner
    \inmodule QtCreator
    \brief Wraps SpinnerSolution::SpinnerWidget for use in a Layouting builder.

    Displays an animated spinner that can be started and stopped via setRunning().
    An optional visual decoration can be toggled with setDecorated().

    \sa Layouting::Widget
*/

Spinner::Spinner(std::initializer_list<I> ps)
{
    ptr = new Implementation;
    apply(this, ps);
}

void Spinner::setRunning(bool running)
{
    using State = SpinnerSolution::SpinnerState;
    access(this)->setState(running ? State::Running : State::NotRunning);
}

void Spinner::setDecorated(bool on)
{
    access(this)->setDecorated(on);
}

/*!
    Recursively destroys \a layout and all its child items and nested layouts.
    Widgets owned by the layout items are also deleted.
*/
void destroyLayout(QLayout *layout)
{
    if (layout) {
        while (QLayoutItem *child = layout->takeAt(0)) {
            delete child->widget();
            destroyLayout(child->layout());
        }
        delete layout;
    }
}

// void createItem(LayoutItem *item, QWidget *t)
// {
//     if (auto l = qobject_cast<QLabel *>(t))
//         l->setTextInteractionFlags(l->textInteractionFlags() | Qt::TextSelectableByMouse);

//     item->onAdd = [t](LayoutBuilder &builder) { doAddWidget(builder, t); };
// }

} // namespace Layouting
