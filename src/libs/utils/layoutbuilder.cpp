// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "layoutbuilder.h"

#include "fancylineedit.h"
#include "filepath.h"
#include "icon.h"
#include "icondisplay.h"
#include "markdownbrowser.h"
#include "qtcassert.h"
#include "spinner/spinner.h"

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
        if (!testOnly)
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
    \namespace Layouting
    \inmodule QtCreator

    \brief The Layouting namespace contains classes and functions to conveniently
    create layouts in code.

    Classes in the namespace help to create create QLayout or QWidget derived class,
    instances should be used locally within a function and never stored.

    \sa Layouting::Widget, Layouting::Layout
*/

/*!
    \class Layouting::Layout
    \inmodule QtCreator

    The Layout class is a base class for more specific builder
    classes to create QLayout derived objects.
 */

/*!
    \class Layouting::Widget
    \inmodule QtCreator

    The Widget class is a base class for more specific builder
    classes to create QWidget derived objects.
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

void Layout::span(int cols, int rows)
{
    QTC_ASSERT(!pendingItems.empty(), return);
    pendingItems.back().spanCols = cols;
    pendingItems.back().spanRows = rows;
}

void Layout::align(Qt::Alignment alignment)
{
    QTC_ASSERT(!pendingItems.empty(), return);
    pendingItems.back().alignment = alignment;
}

void Layout::setAlignment(Qt::Alignment alignment)
{
    access(this)->setAlignment(alignment);
}

void Layout::setNoMargins()
{
    setContentsMargins(0, 0, 0, 0);
}

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

void addToLayout(Layout *layout, const Widget &inner)
{
    layout->addLayoutItem(access(&inner));
}

void addToLayout(Layout *layout, QWidget *inner)
{
    layout->addLayoutItem(inner);
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

void empty(Layout *layout)
{
    LayoutItem item;
    item.empty = true;
    layout->addLayoutItem(item);
}

void hr(Layout *layout)
{
    layout->addLayoutItem(createHr());
}

void br(Layout *layout)
{
    layout->flush();
}

void st(Layout *layout)
{
    LayoutItem item;
    item.stretch = 1;
    layout->addLayoutItem(item);
}

void noMargin(Layout *layout)
{
    layout->setNoMargins();
}

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

void Layout::flush()
{
    if (pendingItems.empty())
        return;

    if (QGridLayout *lt = asGrid()) {
        for (const LayoutItem &item : std::as_const(pendingItems)) {
            Qt::Alignment a = item.alignment;
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
            currentGridColumn += item.spanCols;
            // Intentionally not used, use 'br'/'empty' for vertical progress.
            // currentGridRow += item.spanRows;
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

void withFormAlignment(Layout *layout)
{
    layout->useFormAlignment = true;
}

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

QWidget *Layout::emerge() const
{
    const_cast<Layout *>(this)->flush();
    QWidget *widget = new QWidget;
    widget->setLayout(access(this));
    return widget;
}

void Layout::show() const
{
    return emerge()->show();
}

// "Widgets"

Widget::Widget(std::initializer_list<I> ps)
{
    ptr = new Implementation;
    apply(this, ps);
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

QWidget *Widget::emerge() const
{
    return access(this);
}

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

void Label::onLinkHovered(QObject *guard, const std::function<void (const QString &)> &func)
{
    QObject::connect(access(this), &QLabel::linkHovered, guard, func);
}

void Label::onLinkActivated(QObject *guard, const std::function<void(const QString &)> &func)
{
    QObject::connect(access(this), &QLabel::linkActivated, guard, func);
}

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

void Group::setGroupChecker(const std::function<void (QObject *)> &checker)
{
    checker(access(this));
}

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

void SpinBox::onTextChanged(QObject *guard, const std::function<void(QString)> &func)
{
    QObject::connect(access(this), &QSpinBox::textChanged, guard, func);
}

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

void PushButton::onClicked(QObject *guard, const std::function<void ()> &func)
{
    QObject::connect(access(this), &QAbstractButton::clicked, guard, func);
}

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

void ScrollArea::setFixSizeHintBug(bool fixBug)
{
    auto fixedScrollArea = static_cast<FixedScrollArea *>(access(this));
    fixedScrollArea->setFixSizeHintBug(fixBug);
}

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

// ToolBar

ToolBar::ToolBar(std::initializer_list<I> ps)
{
    ptr = new Implementation;
    apply(this, ps);
    access(this)->setOrientation(Qt::Horizontal);
}

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

// Special If

If::If(
    bool condition,
    const std::initializer_list<Layout::I> ifcase,
    const std::initializer_list<Layout::I> elsecase)
    : used(condition ? ifcase : elsecase)
{}

void addToLayout(Layout *layout, const If &inner)
{
    for (const Layout::I &item : inner.used)
        item.apply(layout);
}

// Specials

QWidget *createHr(QWidget *parent)
{
    auto frame = new QFrame(parent);
    frame->setFrameShape(QFrame::HLine);
    frame->setFrameShadow(QFrame::Sunken);
    return frame;
}

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

Align::Align(Qt::Alignment alignment, const Layout::I &item)
    : item(item)
    , alignment(alignment)
{}

void addToLayout(Layout *layout, const Align &inner)
{
    layout->addItem(inner.item);
    if (layout->pendingItems.empty()) {
        QTC_CHECK(inner.alignment == Qt::Alignment());
        return;
    }
    layout->pendingItems.back().alignment = inner.alignment;
}

LayoutModifier spacing(int space)
{
    return [space](Layout *layout) { layout->setSpacing(space); };
}

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

void tight(Layout *layout)
{
    layout->setNoMargins();
    layout->setSpacing(0);
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

void LineEdit::setPlaceHolderText(const QString &text)
{
    access(this)->setPlaceholderText(text);
}

void LineEdit::setCompleter(QCompleter *completer)
{
    access(this)->setSpecialCompleter(completer);
}

void LineEdit::onReturnPressed(QObject *guard, const std::function<void()> &func)
{
    static_cast<LineEditImpl *>(access(this))->acceptReturnKeys = true;
    QObject::connect(access(this), &Utils::FancyLineEdit::returnPressed, guard, func);
}

void LineEdit::onRightSideIconClicked(QObject *guard, const std::function<void()> &func)
{
    QObject::connect(access(this), &Utils::FancyLineEdit::rightButtonClicked, guard, func);
}

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

IconDisplay::IconDisplay(std::initializer_list<I> ps)
{
    ptr = new Implementation;
    apply(this, ps);
}

void IconDisplay::setIcon(const Utils::Icon &icon)
{
    access(this)->setIcon(icon);
}

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
