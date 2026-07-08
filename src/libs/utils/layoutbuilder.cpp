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
#include <QPointer>
#include <QListWidget>
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

#if QT_VERSION < QT_VERSION_CHECK(6, 12, 0)
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformtheme.h>
#endif

#include <memory>

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
    creator = [] { return new Implementation; };
    apply(this, ps);
}

Build::Build(QObject *product)
    : product(product)
{}

Build::~Build() = default;

std::shared_ptr<Build> Object::newBuild(QObject *product) const
{
    return std::make_shared<Build>(product);
}

QObject *Object::materialize() const
{
    QTC_ASSERT(creator, return nullptr);
    QObject *product = creator();
    applyTo(product);
    return product;
}

/*!
    Applies the recorded operations to the existing \a product.
*/
void Object::applyTo(QObject *product) const
{
    const std::shared_ptr<Build> build = newBuild(product);
    replay(*build);
    finish(*build);
}

void Object::replay(Build &build) const
{
    for (const Op &op : ops)
        op(build);
}

void Object::record(Op op)
{
    ops.push_back(std::move(op));
}

void Object::setObjectName(const QString &objectName)
{
    modify(this, [objectName](QObject *o) { o->setObjectName(objectName); });
}

void Object::onDestroyed(QObject *guard, const std::function<void()> &func)
{
    modify(this, [guard, func](QObject *o) {
        QObject::connect(o, &QObject::destroyed, guard, func);
    });
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

class LayoutBuild final : public Build
{
public:
    using Build::Build;

    QLayout *layout() const { return static_cast<QLayout *>(product.data()); }
    QFormLayout *asForm() const { return qobject_cast<QFormLayout *>(layout()); }
    QGridLayout *asGrid() const { return qobject_cast<QGridLayout *>(layout()); }
    QBoxLayout *asBox() const { return qobject_cast<QBoxLayout *>(layout()); }
    FlowLayout *asFlow() const { return dynamic_cast<FlowLayout *>(layout()); }

    void addItem(const LayoutItem &item);
    void flush();

    size_t popMark()
    {
        QTC_ASSERT(!marks.empty(), return pendingItems.size());
        const size_t mark = marks.back();
        marks.pop_back();
        return mark;
    }

    std::vector<LayoutItem> pendingItems;
    // Start indices into pendingItems for SpanAll and Align.
    std::vector<size_t> marks;

    // Grid-only
    int currentGridColumn = 0;
    int currentGridRow = 0;
    bool useFormAlignment = false;
};

static void modifyLayout(Layout *layout, std::function<void(LayoutBuild &)> op)
{
    layout->record([op = std::move(op)](Build &build) { op(static_cast<LayoutBuild &>(build)); });
}

static void markPending(Layout *layout)
{
    modifyLayout(layout, [](LayoutBuild &b) { b.marks.push_back(b.pendingItems.size()); });
}

std::shared_ptr<Build> Layout::newBuild(QObject *product) const
{
    return std::make_shared<LayoutBuild>(product);
}

void Layout::finish(Build &build) const
{
    static_cast<LayoutBuild &>(build).flush();
}

/*!
    Sets the column span to \a cols and the row span to \a rows on the last item
    that was added to this layout. Only meaningful for Grid layouts.

    \sa Layouting::Span, Layouting::SpanAll
*/
void Layout::span(int cols, int rows)
{
    modifyLayout(this, [cols, rows](LayoutBuild &b) {
        QTC_ASSERT(!b.pendingItems.empty(), return);
        b.pendingItems.back().spanCols = cols;
        b.pendingItems.back().spanRows = rows;
    });
}

/*!
    Sets the alignment to \a alignment on the last item that was added to this layout.
    Only meaningful for Grid and Form layouts.
*/
void Layout::align(Qt::Alignment alignment)
{
    modifyLayout(this, [alignment](LayoutBuild &b) {
        QTC_ASSERT(!b.pendingItems.empty(), return);
        b.pendingItems.back().alignment = alignment;
    });
}

void Layout::setAlignment(Qt::Alignment alignment)
{
    modify(this, [alignment](QLayout *l) { l->setAlignment(alignment); });
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
    modify(this, [left, top, right, bottom](QLayout *l) {
        l->setContentsMargins(left, top, right, bottom);
    });
}

/*!
    Builds the layout and sets it on \a widget.

    Each call builds a new layout. Widgets and layouts that were added as
    existing objects are not copied and can be placed into one product only.
 */
void Layout::attachTo(QWidget *widget)
{
    widget->setLayout(emergeLayout());
}

/*!
    Adds the layout item \a item as a sub item.
 */
void Layout::addItem(I item)
{
    item.apply(this);
}

void LayoutBuild::addItem(const LayoutItem &item)
{
    if (QBoxLayout *lt = asBox())
        addItemToBoxLayout(lt, item);
    else if (FlowLayout *lt = asFlow())
        addItemToFlowLayout(lt, item);
    else
        pendingItems.push_back(item);
}

void Layout::addLayoutItem(const LayoutItem &item)
{
    if (item.empty && skipEmptyItems)
        return;
    modifyLayout(this, [item](LayoutBuild &b) { b.addItem(item); });
}

/*!
    Adds the item returned by \a make, which is called each time the layout
    is built. No item is added when \a make returns \c std::nullopt.
 */
void Layout::addLazyLayoutItem(std::function<std::optional<LayoutItem>()> make)
{
    modifyLayout(this, [make = std::move(make), skip = skipEmptyItems](LayoutBuild &b) {
        const std::optional<LayoutItem> item = make();
        if (item && !(item->empty && skip))
            b.addItem(*item);
    });
}

/*!
    Calls \a fill with an empty layout each time this layout is built, and adds
    the items that \a fill adds to it at this position.
 */
void Layout::addLazyItems(std::function<void(Layout &)> fill)
{
    modifyLayout(this, [fill = std::move(fill)](LayoutBuild &b) {
        Layout items;
        fill(items);
        items.replay(b);
    });
}

/*!
    Adds the layout items \a items as sub items.
 */
void Layout::addItems(std::initializer_list<I> items, bool addEmpty)
{
    const bool prevSkip = skipEmptyItems;
    skipEmptyItems = !addEmpty;
    for (const I &item : items)
        item.apply(this);
    skipEmptyItems = prevSkip;
}

/*!
    Starts a new row containing \a items. The row can be further extended by
    other items using \c addItem() or \c addItems().

    \sa addItem(), addItems()
 */

void Layout::addRow(std::initializer_list<I> items)
{
    for (const I &item : items)
        addItem(item);
    flush();
}

void Layout::setSpacing(int spacing)
{
    modify(this, [spacing](QLayout *l) { l->setSpacing(spacing); });
}

void Layout::setColumnStretch(int column, int stretch)
{
    modifyLayout(this, [column, stretch](LayoutBuild &b) {
        if (auto grid = b.asGrid()) {
            grid->setColumnStretch(column, stretch);
        } else {
            QTC_CHECK(false);
        }
    });
}

void Layout::setRowStretch(int row, int stretch)
{
    modifyLayout(this, [row, stretch](LayoutBuild &b) {
        if (auto grid = b.asGrid()) {
            grid->setRowStretch(row, stretch);
        } else {
            QTC_CHECK(false);
        }
    });
}

template<class T>
class SingleUse
{
public:
    explicit SingleUse(T *object) : m_object(object) {}

    T *take() const
    {
        QTC_ASSERT(!*m_used, return nullptr);
        *m_used = true;
        return m_object;
    }

private:
    QPointer<T> m_object;
    std::shared_ptr<bool> m_used = std::make_shared<bool>(false);
};

void addToWidget(Widget *widget, const Layout &layout)
{
    modify(widget, [layout](QWidget *w) { w->setLayout(layout.emergeLayout()); });
}

void addToWidget(Widget *widget, const WidgetModifier &inner)
{
    inner(widget);
}

void addToLayout(Layout *layout, const Widget &inner)
{
    layout->addLazyLayoutItem([inner] { return LayoutItem(inner.emerge()); });
}

void addToLayout(Layout *layout, QWidget *inner)
{
    layout->addLazyLayoutItem(
        [once = SingleUse(inner)] { return LayoutItem(once.take()); });
}

void addToLayout(Layout *layout, QWidget &inner)
{
    addToLayout(layout, &inner);
}

void addToLayout(Layout *layout, QLayout *inner)
{
    layout->addLazyLayoutItem(
        [once = SingleUse(inner)] { return LayoutItem(once.take()); });
}

void addToLayout(Layout *layout, const Layout &inner)
{
    layout->addLazyLayoutItem([inner] { return LayoutItem(inner.emergeLayout()); });
}

void addToLayout(Layout *layout, const LayoutModifier &inner)
{
    inner(layout);
}

void addToLayout(Layout *layout, const QString &inner)
{
    layout->addLayoutItem(inner);
}

void addToLayout(Layout *layout, const char *inner)
{
    layout->addLayoutItem(QString::fromUtf8(inner));
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
    layout->addLazyLayoutItem([] { return LayoutItem(createHr()); });
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
    modifyLayout(this, [](LayoutBuild &b) { b.flush(); });
}

void LayoutBuild::flush()
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

QLayout *Layout::emergeLayout() const
{
    return static_cast<QLayout *>(materialize());
}

/*!
    \fn void Layouting::withFormAlignment(Layout *layout)
    Intended to enable form-style label alignment for \a layout. Not yet implemented.
*/
void withFormAlignment(Layout *layout)
{
    modifyLayout(layout, [](LayoutBuild &b) { b.useFormAlignment = true; });
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
    creator = [] { return new FlowLayout; };
}

Flow::Flow(std::initializer_list<I> ps)
{
    creator = [] { return new FlowLayout; };
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
    creator = [] { return new QHBoxLayout; };
}

Row::Row(std::initializer_list<I> ps)
{
    creator = [] { return new QHBoxLayout; };
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
    creator = [] { return new QVBoxLayout; };
}

Column::Column(std::initializer_list<I> ps)
{
    creator = [] { return new QVBoxLayout; };
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
    creator = [] { return new QGridLayout; };
}

Grid::Grid(std::initializer_list<I> ps)
{
    creator = [] { return new QGridLayout; };
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
    creator = [] { return new QFormLayout; };
}

Form::Form(std::initializer_list<I> ps)
{
    creator = [] {
        auto lt = new QFormLayout;
        lt->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        return lt;
    };
    apply(this, ps);
    flush();
}

void Layout::setFieldGrowthPolicy(int policy)
{
    modifyLayout(this, [policy](LayoutBuild &b) {
        if (auto lt = b.asForm())
            lt->setFieldGrowthPolicy(QFormLayout::FieldGrowthPolicy(policy));
    });
}

void Layout::setStretch(int index, int stretch)
{
    modifyLayout(this, [index, stretch](LayoutBuild &b) {
        if (auto lt = b.asBox())
            lt->setStretch(index, stretch);
    });
}

/*!
    Finalizes the layout and returns it wrapped in a new QWidget.

    This is useful when a layout needs to be embedded as a widget (for example inside
    a QScrollArea or a QStackedWidget) without an explicit outer Widget builder.
    The caller takes ownership of the returned widget.

    Widgets and layouts that were added as existing objects are not copied and
    can be placed into one product only.

    \sa Layout::attachTo()
*/
QWidget *Layout::emerge() const
{
    QWidget *widget = new QWidget;
    widget->setLayout(emergeLayout());
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
    creator = [] { return new Implementation; };
    apply(this, ps);
}

void Widget::setSize(int w, int h)
{
    modify(this, [w, h](Implementation *widget) { widget->resize(w, h); });
}

void Widget::setFixedSize(const QSize &size)
{
    modify(this, [size](Implementation *w) { w->setFixedSize(size); });
}

void Widget::setAutoFillBackground(bool on)
{
    modify(this, [on](Implementation *w) { w->setAutoFillBackground(on); });
}

/*!
    Sets \a layout as the layout of this widget, finalizing it first.
*/
void Widget::setLayout(const Layout &layout)
{
    modify(this, [layout](Implementation *w) { w->setLayout(layout.emergeLayout()); });
}

void Widget::setWindowTitle(const QString &title)
{
    modify(this, [title](Implementation *w) { w->setWindowTitle(title); });
}

void Widget::setWindowFlags(Qt::WindowFlags flags)
{
    modify(this, [flags](Implementation *w) { w->setWindowFlags(flags); });
}

void Widget::setWidgetAttribute(Qt::WidgetAttribute attr, bool on)
{
    modify(this, [attr, on](Implementation *w) { w->setAttribute(attr, on); });
}

void Widget::setToolTip(const QString &title)
{
    modify(this, [title](Implementation *w) { w->setToolTip(title); });
}

void Widget::show() const
{
    QWidget *w = emerge();
    w->setAttribute(Qt::WA_DeleteOnClose, true);
    w->show();
}

void Widget::setVisible(bool visible)
{
    modify(this, [visible](Implementation *w) { w-> setVisible(visible); });
}

void Widget::setEnabled(bool enabled)
{
    modify(this, [enabled](Implementation *w) { w->setEnabled(enabled); });
}

void Widget::setVisibleController(const std::function<void (QObject *)> &controller)
{
    modify(this, [controller](Implementation *w) { controller(w); });
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
    modify(this, [left, top, right, bottom](Implementation *w) {
        w->setContentsMargins(left, top, right, bottom);
    });
}

void Widget::setCursor(Qt::CursorShape shape)
{
    modify(this, [shape](Implementation *w) { w->setCursor(shape); });
}

void Widget::setMinimumWidth(int minw)
{
    modify(this, [minw](Implementation *w) { w->setMinimumWidth(minw); });
}

void Widget::setMinimumHeight(int height)
{
    modify(this, [height](Implementation *w) { w->setMinimumHeight(height); });
}

void Widget::setMaximumWidth(int maxWidth)
{
    modify(this, [maxWidth](Implementation *w) { w->setMaximumWidth(maxWidth); });
}

void Widget::setMaximumHeight(int maxHeight)
{
    modify(this, [maxHeight](Implementation *w) { w->setMaximumHeight(maxHeight); });
}

void Widget::setSizePolicy(const QSizePolicy &policy)
{
    modify(this, [policy](Implementation *w) { w->setSizePolicy(policy); });
}

/*!
    Returns the widget this builder describes.

    Each call creates a new widget without a parent, and the caller takes
    ownership of it. Widgets and layouts that were added as existing
    objects are not copied and can be placed into one product only.
*/
QWidget *Widget::emerge() const
{
    return static_cast<QWidget *>(materialize());
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
    creator = [] { return new Implementation; };
    apply(this, ps);
}

Label::Label(const QString &text)
{
    creator = [text] {
        auto label = new Implementation;
        label->setText(text);
        return label;
    };
}

void Label::setText(const QString &text)
{
    modify(this, [text](Implementation *w) { w->setText(text); });
}

void Label::setTextFormat(Qt::TextFormat format)
{
    modify(this, [format](Implementation *w) { w->setTextFormat(format); });
}

void Label::setWordWrap(bool on)
{
    modify(this, [on](Implementation *w) { w->setWordWrap(on); });
}

void Label::setTextInteractionFlags(Qt::TextInteractionFlags flags)
{
    modify(this, [flags](Implementation *w) { w->setTextInteractionFlags(flags); });
}

void Label::setOpenExternalLinks(bool on)
{
    modify(this, [on](Implementation *w) { w->setOpenExternalLinks(on); });
}

/*!
    Connects \a func to the QLabel::linkHovered signal.

    The connection is automatically removed when \a guard is destroyed, which prevents
    dangling connections. Pass the object whose lifetime determines when the connection
    should be active, typically \c this of the enclosing class.
*/
void Label::onLinkHovered(QObject *guard, const std::function<void (const QString &)> &func)
{
    modify(this, [guard, func](Implementation *w) {
        QObject::connect(w, &QLabel::linkHovered, guard, func);
    });
}

/*!
    Connects \a func to the QLabel::linkActivated signal.
    The connection is automatically removed when \a guard is destroyed.
*/
void Label::onLinkActivated(QObject *guard, const std::function<void(const QString &)> &func)
{
    modify(this, [guard, func](Implementation *w) {
        QObject::connect(w, &QLabel::linkActivated, guard, func);
    });
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
    creator = [] { return new Implementation; };
    apply(this, ps);
}

void Group::setTitle(const QString &title)
{
    modify(this, [title](Implementation *w) {
        w->setTitle(title);
        w->setObjectName(title);
    });
}

/*!
    Calls \a checker with the underlying QGroupBox, allowing external code to
    configure the group box, for example to make it checkable and bind its
    checked state to a settings key.
*/
void Group::setGroupChecker(const std::function<void (QObject *)> &checker)
{
    modify(this, [checker](Implementation *w) { checker(w); });
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
    creator = [] { return new Implementation; };
    apply(this, ps);
}

void SpinBox::setValue(int val)
{
    modify(this, [val](Implementation *w) { w->setValue(val); });
}

/*!
    Connects \a func to the QSpinBox::textChanged signal.
    The connection is automatically removed when \a guard is destroyed.
*/
void SpinBox::onTextChanged(QObject *guard, const std::function<void(QString)> &func)
{
    modify(this, [guard, func](Implementation *w) {
        QObject::connect(w, &QSpinBox::textChanged, guard, func);
    });
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
    creator = [] { return new Implementation; };
    apply(this, ps);
}

void TextEdit::setText(const QString &text)
{
    modify(this, [text](Implementation *w) { w->setText(text); });
}

void TextEdit::setMarkdown(const QString &markdown)
{
    modify(this, [markdown](Implementation *w) { w->setMarkdown(markdown); });
}

void TextEdit::setReadOnly(bool on)
{
    modify(this, [on](Implementation *w) { w->setReadOnly(on); });
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
    creator = [] { return new Implementation; };
    apply(this, ps);
}

void CompletingTextEdit::setText(const QString &text)
{
    modify(this, [text](Implementation *w) { w->setPlainText(text); });
}

void CompletingTextEdit::setMarkdown(const QString &markdown)
{
    modify(this, [markdown](Implementation *w) { w->setMarkdown(markdown); });
}

void CompletingTextEdit::setReadOnly(bool on)
{
    modify(this, [on](Implementation *w) { w->setReadOnly(on); });
}

void CompletingTextEdit::setPlaceHolderText(const QString &text)
{
    modify(this, [text](Implementation *w) { w->setPlaceholderText(text); });
}

void CompletingTextEdit::setRightSideIconPath(const Utils::FilePath &path)
{
    modify(this, [path](Implementation *w) { w->setRightSideIconPath(path); });
}

void CompletingTextEdit::setCompleter(QCompleter *completer)
{
    modify(this, [completer](Implementation *w) { w->setCompleter(completer); });
}

/*!
    Connects \a func to the textChanged signal, passing the current plain text.
    The connection is automatically removed when \a guard is destroyed.
*/
void CompletingTextEdit::onTextChanged(QObject *guard, const std::function<void(QString)> &func)
{
    modify(this, [guard, func](Implementation *w) {
        QObject::connect(w, &Implementation::textChanged, guard, [func, w] {
            func(w->toPlainText());
        });
    });
}

/*!
    Connects \a func to the CompletingTextEdit::returnPressed signal.
    The connection is automatically removed when \a guard is destroyed.
*/
void CompletingTextEdit::onReturnPressed(
    QObject *guard, const std::function<void(Implementation &)> &func)
{
    modify(this, [guard, func](Implementation *w) {
        QObject::connect(w, &Utils::CompletingTextEdit::returnPressed, guard, [func, w] {
            func(*w);
        });
    });
}

/*!
    Connects \a func to the CompletingTextEdit::rightSideIconClicked signal.
    The connection is automatically removed when \a guard is destroyed.
*/
void CompletingTextEdit::onRightSideIconClicked(QObject *guard, const std::function<void()> &func)
{
    modify(this, [guard, func](Implementation *w) {
        QObject::connect(w, &Utils::CompletingTextEdit::rightSideIconClicked, guard, func);
    });
}

void CompletingTextEdit::setCompletionBehavior(Utils::CompletingTextEdit::CompletionBehavior behavior)
{
    modify(this, [behavior](Implementation *w) { w->setCompletionBehavior(behavior); });
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
    creator = [] { return new Implementation; };
    apply(this, ps);
}

void PushButton::setText(const QString &text)
{
    modify(this, [text](Implementation *w) { w->setText(text); });
}

void PushButton::setIconPath(const Utils::FilePath &iconPath)
{
    modify(this, [iconPath](Implementation *w) {
        if (!iconPath.exists()) {
            w->setIcon({});
            return;
        }
        Utils::Icon icon{iconPath};
        w->setIcon(icon.icon());
    });
}

void PushButton::setIconSize(const QSize &size)
{
    modify(this, [size](Implementation *w) { w->setIconSize(size); });
}

void PushButton::setFlat(bool flat)
{
    modify(this, [flat](Implementation *w) { w->setFlat(flat); });
}

/*!
    Connects \a func to the QAbstractButton::clicked signal.
    The connection is automatically removed when \a guard is destroyed.
*/
void PushButton::onClicked(QObject *guard, const std::function<void ()> &func)
{
    modify(this, [guard, func](Implementation *w) {
        QObject::connect(w, &QAbstractButton::clicked, guard, func);
    });
}

ListWidget::ListWidget(std::initializer_list<I> ps)
{
    creator = [] { return new Implementation; };
    apply(this, ps);
}

void ListWidget::setItems(const QStringList &items)
{
    modify(this, [items](Implementation *w) {
        w->clear();
        w->addItems(items);
    });
}

void ListWidget::addItem(const QString &item)
{
    modify(this, [item](Implementation *w) { w->addItem(item); });
}

void ListWidget::clear()
{
    modify(this, [](Implementation *w) { w->clear(); });
}

void ListWidget::setCurrentRow(int row)
{
    modify(this, [row](Implementation *w) { w->setCurrentRow(row); });
}

void ListWidget::onItemActivated(QObject *guard, const std::function<void(int)> &func)
{
    modify(this, [guard, func](Implementation *w) {
        QObject::connect(w, &QListWidget::itemActivated, guard,
                         [w, func](QListWidgetItem *item) { func(w->row(item)); });
    });
}

void ListWidget::onCurrentRowChanged(QObject *guard, const std::function<void(int)> &func)
{
    modify(this, [guard, func](Implementation *w) {
        QObject::connect(w, &QListWidget::currentRowChanged, guard, func);
    });
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
    creator = [] { return new Implementation; };
    apply(this, ps);
}

void addToStack(Stack *stack, const Widget &inner)
{
    modify(stack, [inner](QStackedWidget *w) { w->addWidget(inner.emerge()); });
}

void addToStack(Stack *stack, const Layout &inner)
{
    modify(stack, [inner](QStackedWidget *w) { w->addWidget(inner.emerge()); });
}

void addToStack(Stack *stack, QWidget *inner)
{
    modify(stack, [once = SingleUse(inner)](QStackedWidget *w) {
        if (QWidget *inner = once.take())
            w->addWidget(inner);
    });
}

void addToScrollArea(ScrollArea *scrollArea, QWidget *inner)
{
    modify(scrollArea, [once = SingleUse(inner)](QScrollArea *w) {
        if (QWidget *inner = once.take())
            w->setWidget(inner);
    });
}

void addToScrollArea(ScrollArea *scrollArea, const Layout &layout)
{
    modify(scrollArea, [layout](QScrollArea *w) { w->setWidget(layout.emerge()); });
}

void addToScrollArea(ScrollArea *scrollArea, const Widget &inner)
{
    modify(scrollArea, [inner](QScrollArea *w) { w->setWidget(inner.emerge()); });
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
    creator = [] {
        auto scrollArea = new FixedScrollArea;
        scrollArea->setWidgetResizable(true);
        return scrollArea;
    };
    apply(this, items);
}

ScrollArea::ScrollArea(const Layout &inner)
{
    creator = [] {
        auto scrollArea = new FixedScrollArea;
        scrollArea->setWidgetResizable(true);
        return scrollArea;
    };
    setLayout(inner);
}

void ScrollArea::setLayout(const Layout &inner)
{
    modify(this, [inner](Implementation *w) { w->setWidget(inner.emerge()); });
}

void ScrollArea::setFrameShape(QFrame::Shape shape)
{
    modify(this, [shape](Implementation *w) { w->setFrameShape(shape); });
}

/*!
    When \a fixBug is \c true, disables the default sizeHint cap of 36x24 em-widths
    that works around QTBUG-136762. Enable this when the scroll area must report its
    full preferred size rather than capping it.
*/
void ScrollArea::setFixSizeHintBug(bool fixBug)
{
    modify(this, [fixBug](Implementation *w) {
        static_cast<FixedScrollArea *>(w)->setFixSizeHintBug(fixBug);
    });
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
    creator = [] {
        auto splitter = new Implementation;
        splitter->setOrientation(Qt::Vertical);
        return splitter;
    };
    apply(this, ps);
}

void Splitter::setOrientation(Qt::Orientation orientation)
{
    modify(this, [orientation](Implementation *w) { w->setOrientation(orientation); });
}

void Splitter::setStretchFactor(int index, int stretch)
{
    modify(this, [index, stretch](Implementation *w) { w->setStretchFactor(index, stretch); });
}

void Splitter::setChildrenCollapsible(bool collapsible)
{
    modify(this, [collapsible](Implementation *w) { w->setChildrenCollapsible(collapsible); });
}

void addToSplitter(Splitter *splitter, QWidget *inner)
{
    modify(splitter, [once = SingleUse(inner)](QSplitter *w) {
        if (QWidget *inner = once.take())
            w->addWidget(inner);
    });
}

void addToSplitter(Splitter *splitter, const Widget &inner)
{
    modify(splitter, [inner](QSplitter *w) { w->addWidget(inner.emerge()); });
}

void addToSplitter(Splitter *splitter, const Layout &inner)
{
    modify(splitter, [inner](QSplitter *w) { w->addWidget(inner.emerge()); });
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
    creator = [] {
        auto toolBar = new Implementation;
        toolBar->setOrientation(Qt::Horizontal);
        return toolBar;
    };
    apply(this, ps);
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
    creator = [] { return new Implementation; };
    apply(this, ps);
}

void ToolButton::setDefaultAction(QAction *action)
{
    modify(this, [action](Implementation *w) { w->setDefaultAction(action); });
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
    creator = [] { return new Implementation; };
    apply(this, ps);
}

Tab::Tab(const QString &tabName, const Layout &inner)
    : tabName(tabName)
    , inner(inner)
{}

void addToTabWidget(TabWidget *tabWidget, const Tab &tab)
{
    modify(tabWidget, [tab](QTabWidget *w) { w->addTab(tab.inner.emerge(), tab.tabName); });
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
    creator = [] { return new Implementation; };
    apply(this, ps);
}

void MarkdownBrowser::setMarkdown(const QString &markdown)
{
    modify(this, [markdown](Implementation *w) { w->setMarkdown(markdown); });
}

void MarkdownBrowser::setBasePath(const Utils::FilePath &path)
{
    modify(this, [path](Implementation *w) { w->setBasePath(path); });
}

void MarkdownBrowser::setEnableCodeCopyButton(bool enable)
{
    modify(this, [enable](Implementation *w) { w->setEnableCodeCopyButton(enable); });
}

void MarkdownBrowser::setViewportMargins(int left, int top, int right, int bottom)
{
    modify(this, [left, top, right, bottom](Implementation *w) {
        w->setMargins(QMargins(left, top, right, bottom));
    });
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
    creator = [] { return new Implementation; };
    apply(this, ps);
}

void Canvas::setPaintFunction(const CanvasWidget::PaintFunction &paintFunction)
{
    modify(this, [paintFunction](Implementation *w) { w->setPaintFunction(paintFunction); });
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
    QLayout *layout = emergeLayout();

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
    modifyLayout(layout, [cols = inner.spanCols, rows = inner.spanRows](LayoutBuild &b) {
        if (b.pendingItems.empty()) {
            QTC_CHECK(cols == 1 && rows == 1);
            return;
        }
        b.pendingItems.back().spanCols = cols;
        b.pendingItems.back().spanRows = rows;
    });
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
    markPending(layout);
    layout->addItem(inner.item);
    modifyLayout(layout, [cols = inner.spanCols, rows = inner.spanRows](LayoutBuild &b) {
        const size_t start = b.popMark();
        if (b.pendingItems.empty()) {
            QTC_CHECK(cols == 1 && rows == 1);
            return;
        }
        for (size_t i = start; i < b.pendingItems.size(); ++i) {
            b.pendingItems.at(i).spanCols = cols;
            b.pendingItems.at(i).spanRows = rows;
        }
    });
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
    markPending(layout);
    layout->addItem(inner.item);
    modifyLayout(layout, [alignment = inner.alignment](LayoutBuild &b) {
        const size_t start = b.popMark();
        if (b.pendingItems.empty()) {
            QTC_CHECK(alignment == Qt::Alignment());
            return;
        }
        for (size_t i = start; i < b.pendingItems.size(); ++i)
            b.pendingItems.at(i).alignment = alignment;
    });
}

void addToLayout(Layout *layout, const GridCell &inner)
{
    for (auto i : inner.items) {
        i.apply(layout);
        modifyLayout(layout, [](LayoutBuild &b) {
            if (!b.pendingItems.empty())
                b.pendingItems.back().advancesCell = false;
        });
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
    modifyLayout(layout, [space = inner.space](LayoutBuild &b) {
        if (auto lt = b.asBox())
            lt->addSpacing(space);
    });
}

void addToLayout(Layout *layout, const Stretch &inner)
{
    modifyLayout(layout, [stretch = inner.stretch](LayoutBuild &b) {
        if (auto lt = b.asBox())
            lt->addStretch(stretch);
    });
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
    modify(widget, [](QWidget *w) { Utils::setIgnoreForDirtyHook(w); });
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
    creator = [] { return new LineEditImpl; };
    apply(this, ps);
}

void LineEdit::setText(const QString &text)
{
    modify(this, [text](Implementation *w) { w->setText(text); });
}

void LineEdit::setRightSideIconPath(const Utils::FilePath &path)
{
    if (!path.isEmpty()) {
        QIcon icon(path.toFSPathString());
        QTC_CHECK(!icon.isNull());
        modify(this, [icon](Implementation *w) {
            w->setButtonIcon(Utils::FancyLineEdit::Right, icon);
            w->setButtonVisible(Utils::FancyLineEdit::Right, !icon.isNull());
        });
    }
}

void LineEdit::setPlaceholderText(const QString &text)
{
    modify(this, [text](Implementation *w) { w->setPlaceholderText(text); });
}

void LineEdit::setCompleter(QCompleter *completer)
{
    modify(this, [completer](Implementation *w) { w->setSpecialCompleter(completer); });
}

/*!
    Connects \a func to the FancyLineEdit::returnPressed signal and enables
    acceptance of Return/Enter key events so they are not propagated further.
    The connection is automatically removed when \a guard is destroyed.
*/
void LineEdit::onReturnPressed(QObject *guard, const std::function<void(Implementation &)> &func)
{
    modify(this, [guard, func](Implementation *w) {
        static_cast<LineEditImpl *>(w)->acceptReturnKeys = true;
        QObject::connect(w, &Utils::FancyLineEdit::returnPressed, guard, [func, w] { func(*w); });
    });
}

/*!
    Connects \a func to the FancyLineEdit::rightButtonClicked signal.
    The connection is automatically removed when \a guard is destroyed.
*/
void LineEdit::onRightSideIconClicked(QObject *guard, const std::function<void()> &func)
{
    modify(this, [guard, func](Implementation *w) {
        QObject::connect(w, &Utils::FancyLineEdit::rightButtonClicked, guard, func);
    });
}

/*!
    Connects \a func to the FancyLineEdit::textChanged signal.
    The connection is automatically removed when \a guard is destroyed.
*/
void LineEdit::onTextChanged(QObject *guard, const std::function<void(QString)> &func)
{
    modify(this, [guard, func](Implementation *w) {
        QObject::connect(w, &Utils::FancyLineEdit::textChanged, guard, func);
    });
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
    creator = [] { return new Implementation; };
    apply(this, ps);
}

void Spinner::setRunning(bool running)
{
    using State = SpinnerSolution::SpinnerState;
    modify(this, [running](Implementation *w) {
        w->setState(running ? State::Running : State::NotRunning);
    });
}

void Spinner::setDecorated(bool on)
{
    modify(this, [on](Implementation *w) { w->setDecorated(on); });
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

DialogButtonBox::DialogButtonBox(std::initializer_list<I> ps)
{
    creator = [] { return new Implementation; };
    Layouting::Tools::apply(this, ps);
}

void DialogButtonBox::addButton(QDialogButtonBox::ButtonRole role, QAbstractButton *button)
{
    modify(this, [role, once = SingleUse(button)](Implementation *w) {
        if (QAbstractButton *button = once.take())
            w->addButton(button, role);
    });
}

QString standardButtonText(QDialogButtonBox::StandardButton stdButton)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 12, 0)
    QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme();
    QTC_ASSERT(theme, return {});
    return theme->standardButtonText(stdButton);
#else
    return QDialogButtonBox::standardButtonText(stdButton);
#endif
}
} // namespace Layouting
