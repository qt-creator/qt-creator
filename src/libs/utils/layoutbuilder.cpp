// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "layoutbuilder.h"

#include "filepath.h"
#include "icon.h"
#include "qtcassert.h"

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

template <typename X>
typename X::Implementation *access(const X *x)
{
    return static_cast<typename X::Implementation *>(x->ptr);
}

template <typename X>
void apply(X *x, std::initializer_list<typename X::I> ps)
{
    for (auto && p : ps)
        p.apply(x);
}

// FlowLayout

class FlowLayout : public QLayout
{
public:
    explicit FlowLayout(QWidget *parent, int margin = -1, int hSpacing = -1, int vSpacing = -1)
        : QLayout(parent)
        , m_hSpace(hSpacing)
        , m_vSpace(vSpacing)
    {
        setContentsMargins(margin, margin, margin, margin);
    }

    FlowLayout(int margin = -1, int hSpacing = -1, int vSpacing = -1)
        : m_hSpace(hSpacing)
        , m_vSpace(vSpacing)
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

    void setGeometry(const QRect &rect) override
    {
        QLayout::setGeometry(rect);
        doLayout(rect, false);
    }

    QSize sizeHint() const override { return minimumSize(); }

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
            if (spaceX == -1) {
                spaceX = wid->style()->layoutSpacing(
                    QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
            }
            int spaceY = verticalSpacing();
            if (spaceY == -1) {
                spaceY = wid->style()->layoutSpacing(
                    QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);
            }
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
            Qt::Alignment a;
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

Flow::Flow(std::initializer_list<I> ps)
{
    ptr = new FlowLayout;
    apply(this, ps);
    flush();
}

// Row & Column

Row::Row(std::initializer_list<I> ps)
{
    ptr = new QHBoxLayout;
    apply(this, ps);
    flush();
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

void Label::onLinkHovered(const std::function<void (const QString &)> &func, QObject *guard)
{
    QObject::connect(access(this), &QLabel::linkHovered, guard, func);
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

void SpinBox::onTextChanged(const std::function<void (QString)> &func)
{
    QObject::connect(access(this), &QSpinBox::textChanged, func);
}

// TextEdit

TextEdit::TextEdit(std::initializer_list<I> ps)
{
    ptr = new Implementation;
    apply(this, ps);
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

void PushButton::setFlat(bool flat)
{
    access(this)->setFlat(flat);
}

void PushButton::onClicked(const std::function<void ()> &func, QObject *guard)
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

// Special If

If::If(
    bool condition,
    const std::initializer_list<Layout::I> ifcase,
    const std::initializer_list<Layout::I> thencase)
    : used(condition ? ifcase : thencase)
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

LayoutModifier spacing(int space)
{
    return [space](Layout *layout) { layout->setSpacing(space); };
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

// void createItem(LayoutItem *item, QWidget *t)
// {
//     if (auto l = qobject_cast<QLabel *>(t))
//         l->setTextInteractionFlags(l->textInteractionFlags() | Qt::TextSelectableByMouse);

//     item->onAdd = [t](LayoutBuilder &builder) { doAddWidget(builder, t); };
// }

} // namespace Layouting
