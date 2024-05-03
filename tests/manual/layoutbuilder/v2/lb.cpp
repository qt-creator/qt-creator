// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "lb.h"

#include <QApplication>
#include <QDebug>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedLayout>
#include <QStackedWidget>
#include <QStyle>
#include <QTabWidget>
#include <QTextEdit>
#include <QToolBar>

namespace Layouting {

// That's cut down qtcassert.{c,h} to avoid the dependency.
#define QTC_STRINGIFY_HELPER(x) #x
#define QTC_STRINGIFY(x) QTC_STRINGIFY_HELPER(x)
#define QTC_STRING(cond) qDebug("SOFT ASSERT: \"%s\" in %s: %s", cond,  __FILE__, QTC_STRINGIFY(__LINE__))
#define QTC_ASSERT(cond, action) if (Q_LIKELY(cond)) {} else { QTC_STRING(#cond); action; } do {} while (0)
#define QTC_CHECK(cond) if (cond) {} else { QTC_STRING(#cond); } do {} while (0)


template <typename XInterface>
XInterface::Implementation *access(const XInterface *x)
{
    return static_cast<XInterface::Implementation *>(x->ptr);
}

// Setter implementation

// These are free functions overloaded on the type of builder object
// and setter id. The function implementations are independent, but
// the base expectation is that they will forwards to the backend
// type's setter.

class FlowLayout : public QLayout
{
public:
    explicit FlowLayout(QWidget *parent, int margin = -1, int hSpacing = -1, int vSpacing = -1)
        : QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing)
    {
        setContentsMargins(margin, margin, margin, margin);
    }

    FlowLayout(int margin = -1, int hSpacing = -1, int vSpacing = -1)
        : m_hSpace(hSpacing), m_vSpace(vSpacing)
    {
        setContentsMargins(margin, margin, margin, margin);
    }

    ~FlowLayout() override
    {
        QLayoutItem *item;
        while ((item = takeAt(0)))
            delete item;
    }

    void addItem(QLayoutItem *item) override { itemList.append(item); }

    int horizontalSpacing() const
    {
        if (m_hSpace >= 0)
            return m_hSpace;
        else
            return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
    }

    int verticalSpacing() const
    {
        if (m_vSpace >= 0)
            return m_vSpace;
        else
            return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
    }

    Qt::Orientations expandingDirections() const override
    {
        return {};
    }

    bool hasHeightForWidth() const override { return true; }

    int heightForWidth(int width) const override
    {
        int height = doLayout(QRect(0, 0, width, 0), true);
        return height;
    }

    int count() const override { return itemList.size(); }

    QLayoutItem *itemAt(int index) const override
    {
        return itemList.value(index);
    }

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

    void setGeometry(const QRect &rect) override
    {
        QLayout::setGeometry(rect);
        doLayout(rect, false);
    }

    QSize sizeHint() const override
    {
        return minimumSize();
    }

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
        QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
        int x = effectiveRect.x();
        int y = effectiveRect.y();
        int lineHeight = 0;

        for (QLayoutItem *item : itemList) {
            QWidget *wid = item->widget();
            int spaceX = horizontalSpacing();
            if (spaceX == -1)
                spaceX = wid->style()->layoutSpacing(
                            QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
            int spaceY = verticalSpacing();
            if (spaceY == -1)
                spaceY = wid->style()->layoutSpacing(
                            QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);
            int nextX = x + item->sizeHint().width() + spaceX;
            if (nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
                x = effectiveRect.x();
                y = y + lineHeight + spaceY;
                nextX = x + item->sizeHint().width() + spaceX;
                lineHeight = 0;
            }

            if (!testOnly)
                item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));

            x = nextX;
            lineHeight = qMax(lineHeight, item->sizeHint().height());
        }
        return y + lineHeight - rect.y() + bottom;
    }

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
    int m_hSpace;
    int m_vSpace;
};

/*!
    \namespace Layouting
    \inmodule QtCreator

    \brief The Layouting namespace contains classes for use with layout builders.
*/


/*!
    \class Layouting::LayoutItem
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
LayoutItem::LayoutItem() = default;

LayoutItem::~LayoutItem() = default;

LayoutItem::LayoutItem(const LayoutInterface &inner)
    : LayoutItem(access(&inner))
{}

LayoutItem::LayoutItem(const WidgetInterface &inner)
    : LayoutItem(access(&inner))
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

// Helpers


// Object

Object::Object(std::initializer_list<I> ps)
{
    create();
    for (auto && p : ps)
        apply(p);
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
    } else if (item.space != -1) {
        layout->addSpacing(item.space);
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
//    } else if (item.space != -1) {
//        layout->addSpacing(item.space);
    } else if (item.empty) {
        // Nothing to do, but no reason to warn, either
    } else if (!item.text.isEmpty()) {
        layout->addWidget(createLabel(item.text));
    } else {
        QTC_CHECK(false);
    }
}

// void doAddSpace(LayoutBuilder &builder, const Space &space)
// {
//     ResultItem fi;
//     fi.space = space.space;
//     builder.stack.last().pendingItems.append(fi);
// }

// void doAddStretch(LayoutBuilder &builder, const Stretch &stretch)
// {
//     Item fi;
//     fi.stretch = stretch.stretch;
//     builder.stack.last().pendingItems.append(fi);
// }

// void doAddLayout(LayoutBuilder &builder, QLayout *layout)
// {
//     builder.stack.last().pendingItems.append(Item(layout));
// }

// void doAddWidget(LayoutBuilder &builder, QWidget *widget)
// {
//     builder.stack.last().pendingItems.append(Item(widget));
// }


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

/*!
    \class Layouting::LayoutBuilder
    \internal
    \inmodule QtCreator

    \brief The LayoutBuilder class provides a convenient way to fill \c QFormLayout
    and \c QGridLayouts with contents.

    Filling a layout with items happens item-by-item, row-by-row.

    A LayoutBuilder instance is typically used locally within a function and never stored.

    \sa addItem(), addItems()
*/


/*!
    \internal
    Destructs a layout builder.
 */

/*!
    Starts a new row containing \a items. The row can be further extended by
    other items using \c addItem() or \c addItems().

    \sa addItem(), addItems()
 */
// void LayoutItem::addRow(const LayoutItems &items)
// {
//     addItem(br);
//     addItems(items);
// }

// /*!
//     Adds the layout item \a item as sub items.
//  */
// void LayoutItem::addItem(const LayoutItem &item)
// {
//     subItems.append(item);
// }

// /*!
//     Adds the layout items \a items as sub items.
//  */
// void LayoutItem::addItems(const LayoutItems &items)
// {
//     subItems.append(items);
// }

// /*!
//     Attaches the constructed layout to the provided QWidget \a w.

//     This operation can only be performed once per LayoutBuilder instance.
//  */

// void LayoutItem::attachTo(QWidget *w) const
// {
//     LayoutBuilder builder;

//     builder.stack.append(w);
//     addItemHelper(builder, *this);
// }


// Layout

void LayoutInterface::span(int cols, int rows)
{
    QTC_ASSERT(!pendingItems.empty(), return);
    pendingItems.back().spanCols = cols;
    pendingItems.back().spanRows = rows;
}

void LayoutInterface::noMargin()
{
    customMargin({});
}

void LayoutInterface::normalMargin()
{
    customMargin({9, 9, 9, 9});
}

void LayoutInterface::customMargin(const QMargins &margin)
{
    access(this)->setContentsMargins(margin);
}

void LayoutInterface::addItem(const LayoutItem &item)
{
    if (item.break_)
        flush();
    else
        pendingItems.push_back(item);
}

void addNestedItem(WidgetInterface *widget, const LayoutInterface &layout)
{
    widget->setLayout(layout);
}

void addNestedItem(LayoutInterface *layout, const WidgetInterface &inner)
{
    LayoutItem item;
    item.widget = access(&inner);
    layout->addItem(item);
}

void addNestedItem(LayoutInterface *layout, const LayoutItem &inner)
{
    layout->addItem(inner);
}

void addNestedItem(LayoutInterface *layout, const LayoutInterface &inner)
{
    LayoutItem item;
    item.layout = access(&inner);
    layout->addItem(item);
}

void addNestedItem(LayoutInterface *layout, const std::function<LayoutItem()> &inner)
{
    LayoutItem item = inner();
    layout->addItem(item);
}

void addNestedItem(LayoutInterface *layout, const QString &inner)
{
    LayoutItem item;
    item.text = inner;
    layout->addItem(item);
}

LayoutItem empty()
{
    LayoutItem item;
    item.empty = true;
    return item;
}

LayoutItem hr()
{
    LayoutItem item;
    item.widget = createHr();
    return item;
}

LayoutItem br()
{
    LayoutItem item;
    item.break_ = true;
    return item;
}

LayoutItem st()
{
    LayoutItem item;
    item.stretch = 1;
    return item;
}

QFormLayout *LayoutInterface::asForm()
{
    return qobject_cast<QFormLayout *>(access(this));
}

QGridLayout *LayoutInterface::asGrid()
{
    return qobject_cast<QGridLayout *>(access(this));
}

QBoxLayout *LayoutInterface::asBox()
{
    return qobject_cast<QBoxLayout *>(access(this));
}

void LayoutInterface::flush()
{
    if (QGridLayout *lt = asGrid()) {
        for (const LayoutItem &item : std::as_const(pendingItems)) {
            Qt::Alignment a = currentGridColumn == 0 ? align : Qt::Alignment();
            if (item.widget)
                lt->addWidget(item.widget, currentGridRow, currentGridColumn, item.spanRows, item.spanCols, a);
            else if (item.layout)
                lt->addLayout(item.layout, currentGridRow, currentGridColumn, item.spanRows, item.spanCols, a);
            else if (!item.text.isEmpty())
                lt->addWidget(createLabel(item.text), currentGridRow, currentGridColumn, item.spanRows, item.spanCols, a);
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
            for (int i = 1; i < pendingItems.size(); ++i)
                addItemToBoxLayout(hbox, pendingItems.at(i));
            while (pendingItems.size() > 1)
                pendingItems.pop_back();
            pendingItems.push_back(LayoutItem(hbox));
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
}

// LayoutItem withFormAlignment()
// {
//     LayoutItem item;
//     item.onAdd = [](LayoutBuilder &builder) {
//         if (builder.stack.size() >= 2) {
//             if (auto widget = builder.stack.at(builder.stack.size() - 2).widget) {
//                 const Qt::Alignment align(widget->style()->styleHint(QStyle::SH_FormLayoutLabelAlignment));
//                 builder.stack.last().align = align;
//             }
//         }
//     };
//     return item;
// }

// Flow

Flow::Flow(std::initializer_list<I> ps)
{
    adopt(new FlowLayout);
    for (auto && p : ps)
        apply(p);
        // for (const LayoutItem &item : std::as_const(pendingItems))
        //     addItemToFlowLayout(flowLayout, item);

}

// Row & Column

Row::Row(std::initializer_list<I> ps)
{
    adopt(new QHBoxLayout);
    for (auto && p : ps)
        apply(p);
    auto self = asBox();
    for (const LayoutItem &item : pendingItems)
        addItemToBoxLayout(self, item);
}

Column::Column(std::initializer_list<I> ps)
{
    adopt(new QVBoxLayout);
    for (auto && p : ps)
        apply(p);
    auto self = asBox();
    for (const LayoutItem &item : pendingItems)
        addItemToBoxLayout(self, item);
}

// Grid

Grid::Grid(std::initializer_list<I> ps)
{
    adopt(new QGridLayout);
    for (auto && p : ps)
        apply(p);
    flush();
}

// Form

Form::Form(std::initializer_list<I> ps)
{
    adopt(new QFormLayout);
    fieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    for (auto && p : ps)
        apply(p);
    flush();
}

void LayoutInterface::fieldGrowthPolicy(int policy)
{
    if (auto lt = asForm())
        lt->setFieldGrowthPolicy(QFormLayout::FieldGrowthPolicy(policy));
}

// "Widgets"

Widget::Widget(std::initializer_list<I> ps)
{
    create();
    for (auto && p : ps)
        apply(p);
}

void WidgetInterface::resize(int w, int h)
{
    access(this)->resize(w, h);
}

void WidgetInterface::setLayout(const LayoutInterface &layout)
{
    access(this)->setLayout(access(&layout));
}

void WidgetInterface::setWindowTitle(const QString &title)
{
    access(this)->setWindowTitle(title);
}

void WidgetInterface::setToolTip(const QString &title)
{
    access(this)->setToolTip(title);
}

void WidgetInterface::show()
{
    access(this)->show();
}

void WidgetInterface::noMargin()
{
    customMargin({});
}

void WidgetInterface::normalMargin()
{
    customMargin({9, 9, 9, 9});
}

void WidgetInterface::customMargin(const QMargins &margin)
{
    access(this)->setContentsMargins(margin);
}

QWidget *WidgetInterface::emerge()
{
    return access(this);
}

// Label

Label::Label(std::initializer_list<I> ps)
{
    create();
    for (auto && p : ps)
        apply(p);
}

Label::Label(const QString &text)
{
    create();
    setText(text);
}

void LabelInterface::setText(const QString &text)
{
    access(this)->setText(text);
}

// Group

Group::Group(std::initializer_list<I> ps)
{
    create();
    for (auto && p : ps)
        apply(p);
}

void GroupInterface::setTitle(const QString &title)
{
    access(this)->setTitle(title);
    access(this)->setObjectName(title);
}

// SpinBox

SpinBox::SpinBox(std::initializer_list<I> ps)
{
    create();
    for (auto && p : ps)
        apply(p);
}

void SpinBoxInterface::setValue(int val)
{
    access(this)->setValue(val);
}

void SpinBoxInterface::onTextChanged(const std::function<void (QString)> &func)
{
    QObject::connect(access(this), &QSpinBox::textChanged, func);
}

// TextEdit

TextEdit::TextEdit(std::initializer_list<I> ps)
{
    create();
    for (auto && p : ps)
        apply(p);
}

void TextEditInterface::setText(const QString &text)
{
    access(this)->setText(text);
}

// PushButton

PushButton::PushButton(std::initializer_list<I> ps)
{
    create();
    for (auto && p : ps)
        apply(p);
}

void PushButtonInterface::setText(const QString &text)
{
    access(this)->setText(text);
}

void PushButtonInterface::onClicked(const std::function<void ()> &func)
{
    QObject::connect(access(this), &QAbstractButton::clicked, func);
}

// Stack

// We use a QStackedWidget instead of a QStackedLayout here because the latter will call
// "setVisible()" when a child is added, which can lead to the widget being spawned as a
// top-level widget. This can lead to the focus shifting away from the main application.
Stack::Stack(std::initializer_list<I> ps)
{
    create();
    for (auto && p : ps)
        apply(p);
}

// Splitter

Splitter::Splitter(std::initializer_list<I> ps)
{
    create();
    access(this)->setOrientation(Qt::Vertical);
    for (auto && p : ps)
        apply(p);
}

// ToolBar

ToolBar::ToolBar(std::initializer_list<I> ps)
{
    create();
    access(this)->setOrientation(Qt::Horizontal);
    for (auto && p : ps)
        apply(p);
}

// TabWidget

TabWidget::TabWidget(std::initializer_list<I> ps)
{
    create();
    for (auto && p : ps)
        apply(p);
}

// // Special Tab

// Tab::Tab(const QString &tabName, const LayoutItem &item)
// {
//     onAdd = [item](LayoutBuilder &builder) {
//         auto tab = new QWidget;
//         builder.stack.append(tab);
//         item.attachTo(tab);
//     };
//     onExit = [tabName](LayoutBuilder &builder) {
//         QWidget *inner = builder.stack.last().widget;
//         builder.stack.pop_back();
//         auto tabWidget = qobject_cast<QTabWidget *>(builder.stack.last().widget);
//         QTC_ASSERT(tabWidget, return);
//         tabWidget->addTab(inner, tabName);
//     };
// }

// // Special If

// If::If(bool condition, const LayoutItems &items, const LayoutItems &other)
// {
//     subItems.append(condition ? items : other);
// }

// "Properties"

// LayoutItem spacing(int spacing)
// {
//     return [spacing](QObject *target) {
//         if (auto layout = qobject_cast<QLayout *>(target)) {
//             layout->setSpacing(spacing);
//         } else {
//             QTC_CHECK(false);
//         }
//     };
// }

// LayoutItem columnStretch(int column, int stretch)
// {
//     return [column, stretch](QObject *target) {
//         if (auto grid = qobject_cast<QGridLayout *>(target)) {
//             grid->setColumnStretch(column, stretch);
//         } else {
//             QTC_CHECK(false);
//         }
//     };
// }

QWidget *createHr(QWidget *parent)
{
    auto frame = new QFrame(parent);
    frame->setFrameShape(QFrame::HLine);
    frame->setFrameShadow(QFrame::Sunken);
    return frame;
}

Span::Span(int n, const LayoutItem &item)
    : LayoutItem(item)
{
    spanCols = n;
}

// void createItem(LayoutItem *item, QWidget *t)
// {
//     if (auto l = qobject_cast<QLabel *>(t))
//         l->setTextInteractionFlags(l->textInteractionFlags() | Qt::TextSelectableByMouse);

//     item->onAdd = [t](LayoutBuilder &builder) { doAddWidget(builder, t); };
// }


} // Layouting
