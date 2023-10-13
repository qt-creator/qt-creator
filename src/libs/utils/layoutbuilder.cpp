// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "layoutbuilder.h"

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

class FlowLayout final : public QLayout
{
    Q_OBJECT

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

struct ResultItem
{
    ResultItem() = default;
    explicit ResultItem(QLayout *l) : layout(l), empty(!l) {}
    explicit ResultItem(QWidget *w) : widget(w), empty(!w) {}

    QString text;
    QLayout *layout = nullptr;
    QWidget *widget = nullptr;
    int space = -1;
    int stretch = -1;
    int span = 1;
    bool empty = false;
};

struct Slice
{
    Slice() = default;
    Slice(QLayout *l) : layout(l) {}
    Slice(QWidget *w, bool isLayouting=false) : widget(w), isLayouting(isLayouting) {}

    QLayout *layout = nullptr;
    QWidget *widget = nullptr;

    void flush();

    // Grid-specific
    int currentGridColumn = 0;
    int currentGridRow = 0;
    bool isFormAlignment = false;
    bool isLayouting = false;
    Qt::Alignment align = {}; // Can be changed to

    // Grid or Form
    QList<ResultItem> pendingItems;
};

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

static QLabel *createLabel(const QString &text)
{
    auto label = new QLabel(text);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
}

static void addItemToBoxLayout(QBoxLayout *layout, const ResultItem &item)
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

static void addItemToFlowLayout(FlowLayout *layout, const ResultItem &item)
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

void Slice::flush()
{
    if (pendingItems.empty())
        return;

    if (auto formLayout = qobject_cast<QFormLayout *>(layout)) {

        // If there are more than two items, we cram the last ones in one hbox.
        if (pendingItems.size() > 2) {
            auto hbox = new QHBoxLayout;
            hbox->setContentsMargins(0, 0, 0, 0);
            for (int i = 1; i < pendingItems.size(); ++i)
                addItemToBoxLayout(hbox, pendingItems.at(i));
            while (pendingItems.size() > 1)
                pendingItems.pop_back();
            pendingItems.append(ResultItem(hbox));
        }

        if (pendingItems.size() == 1) { // One one item given, so this spans both columns.
            const ResultItem &f0 = pendingItems.at(0);
            if (auto layout = f0.layout)
                formLayout->addRow(layout);
            else if (auto widget = f0.widget)
                formLayout->addRow(widget);
        } else if (pendingItems.size() == 2) { // Normal case, both columns used.
            ResultItem &f1 = pendingItems[1];
            const ResultItem &f0 = pendingItems.at(0);
            if (!f1.widget && !f1.layout && !f1.text.isEmpty())
                f1.widget = createLabel(f1.text);

            if (f0.widget) {
                if (f1.layout)
                    formLayout->addRow(f0.widget, f1.layout);
                else if (f1.widget)
                    formLayout->addRow(f0.widget, f1.widget);
            } else  {
                if (f1.layout)
                    formLayout->addRow(createLabel(f0.text), f1.layout);
                else if (f1.widget)
                    formLayout->addRow(createLabel(f0.text), f1.widget);
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

    } else if (auto gridLayout = qobject_cast<QGridLayout *>(layout)) {

        for (const ResultItem &item : std::as_const(pendingItems)) {
            Qt::Alignment a = currentGridColumn == 0 ? align : Qt::Alignment();
            if (item.widget)
                gridLayout->addWidget(item.widget, currentGridRow, currentGridColumn, 1, item.span, a);
            else if (item.layout)
                gridLayout->addLayout(item.layout, currentGridRow, currentGridColumn, 1, item.span, a);
            else if (!item.text.isEmpty())
                gridLayout->addWidget(createLabel(item.text), currentGridRow, currentGridColumn, 1, 1, a);
            currentGridColumn += item.span;
        }
        ++currentGridRow;
        currentGridColumn = 0;

    } else if (auto boxLayout = qobject_cast<QBoxLayout *>(layout)) {

        for (const ResultItem &item : std::as_const(pendingItems))
            addItemToBoxLayout(boxLayout, item);

    } else if (auto flowLayout = qobject_cast<FlowLayout *>(layout)) {

        for (const ResultItem &item : std::as_const(pendingItems))
            addItemToFlowLayout(flowLayout, item);

    } else {
        QTC_CHECK(false);
    }

    pendingItems.clear();
}

// LayoutBuilder

class LayoutBuilder
{
    Q_DISABLE_COPY_MOVE(LayoutBuilder)

public:
    LayoutBuilder();
    ~LayoutBuilder();

    void addItem(const LayoutItem &item);
    void addItems(const LayoutItems &items);

    QList<Slice> stack;
};

static void addItemHelper(LayoutBuilder &builder, const LayoutItem &item)
{
    if (item.onAdd)
        item.onAdd(builder);

    if (item.setter) {
        if (QWidget *widget = builder.stack.last().widget)
            item.setter(widget);
        else if (QLayout *layout = builder.stack.last().layout)
            item.setter(layout);
        else
            QTC_CHECK(false);
    }

    for (const LayoutItem &subItem : item.subItems)
        addItemHelper(builder, subItem);

    if (item.onExit)
        item.onExit(builder);
}

void doAddText(LayoutBuilder &builder, const QString &text)
{
    ResultItem fi;
    fi.text = text;
    builder.stack.last().pendingItems.append(fi);
}

void doAddSpace(LayoutBuilder &builder, const Space &space)
{
    ResultItem fi;
    fi.space = space.space;
    builder.stack.last().pendingItems.append(fi);
}

void doAddStretch(LayoutBuilder &builder, const Stretch &stretch)
{
    ResultItem fi;
    fi.stretch = stretch.stretch;
    builder.stack.last().pendingItems.append(fi);
}

void doAddLayout(LayoutBuilder &builder, QLayout *layout)
{
    builder.stack.last().pendingItems.append(ResultItem(layout));
}

void doAddWidget(LayoutBuilder &builder, QWidget *widget)
{
    builder.stack.last().pendingItems.append(ResultItem(widget));
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


LayoutBuilder::LayoutBuilder() = default;

/*!
    \internal
    Destructs a layout builder.
 */
LayoutBuilder::~LayoutBuilder() = default;

void LayoutBuilder::addItem(const LayoutItem &item)
{
    addItemHelper(*this, item);
}

void LayoutBuilder::addItems(const LayoutItems &items)
{
    for (const LayoutItem &item : items)
        addItemHelper(*this, item);
}

/*!
    Starts a new row containing \a items. The row can be further extended by
    other items using \c addItem() or \c addItems().

    \sa addItem(), addItems()
 */
void LayoutItem::addRow(const LayoutItems &items)
{
    addItem(br);
    addItems(items);
}

/*!
    Adds the layout item \a item as sub items.
 */
void LayoutItem::addItem(const LayoutItem &item)
{
    subItems.append(item);
}

/*!
    Adds the layout items \a items as sub items.
 */
void LayoutItem::addItems(const LayoutItems &items)
{
    subItems.append(items);
}

/*!
    Attaches the constructed layout to the provided QWidget \a w.

    This operation can only be performed once per LayoutBuilder instance.
 */

void LayoutItem::attachTo(QWidget *w) const
{
    LayoutBuilder builder;

    builder.stack.append(w);
    addItemHelper(builder, *this);
}

QWidget *LayoutItem::emerge()
{
    LayoutBuilder builder;

    builder.stack.append(Slice());
    addItemHelper(builder, *this);

    if (builder.stack.empty())
        return nullptr;

    QTC_ASSERT(builder.stack.last().pendingItems.size() == 1, return nullptr);
    ResultItem ri = builder.stack.last().pendingItems.takeFirst();

    QTC_ASSERT(ri.layout || ri.widget, return nullptr);

    if (ri.layout) {
        auto w = new QWidget;
        w->setLayout(ri.layout);
        return w;
    }

    return ri.widget;
}

static void layoutExit(LayoutBuilder &builder)
{
    builder.stack.last().flush();
    QLayout *layout = builder.stack.last().layout;
    builder.stack.pop_back();

    if (builder.stack.last().isLayouting) {
        builder.stack.last().pendingItems.append(ResultItem(layout));
    } else if (QWidget *widget = builder.stack.last().widget) {
        widget->setLayout(layout);
    } else
        builder.stack.last().pendingItems.append(ResultItem(layout));
}

template<class T>
static void layoutingWidgetExit(LayoutBuilder &builder)
{
    const Slice slice = builder.stack.last();
    T *w = qobject_cast<T *>(slice.widget);
    for (const ResultItem &ri : slice.pendingItems) {
        if (ri.widget) {
            w->addWidget(ri.widget);
        } else if (ri.layout) {
            auto child = new QWidget;
            child->setLayout(ri.layout);
            w->addWidget(child);
        }
    }
    builder.stack.pop_back();
    builder.stack.last().pendingItems.append(ResultItem(w));
}

static void widgetExit(LayoutBuilder &builder)
{
    QWidget *widget = builder.stack.last().widget;
    builder.stack.pop_back();
    builder.stack.last().pendingItems.append(ResultItem(widget));
}

Column::Column(std::initializer_list<LayoutItem> items)
{
    subItems = items;
    onAdd = [](LayoutBuilder &builder) { builder.stack.append(new QVBoxLayout); };
    onExit = layoutExit;
}

Row::Row(std::initializer_list<LayoutItem> items)
{
    subItems = items;
    onAdd = [](LayoutBuilder &builder) { builder.stack.append(new QHBoxLayout); };
    onExit = layoutExit;
}

Flow::Flow(std::initializer_list<LayoutItem> items)
{
    subItems = items;
    onAdd = [](LayoutBuilder &builder) { builder.stack.append(new FlowLayout); };
    onExit = layoutExit;
}

Grid::Grid(std::initializer_list<LayoutItem> items)
{
    subItems = items;
    onAdd = [](LayoutBuilder &builder) { builder.stack.append(new QGridLayout); };
    onExit = layoutExit;
}

static QFormLayout *newFormLayout()
{
    auto formLayout = new QFormLayout;
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    return formLayout;
}

Form::Form(std::initializer_list<LayoutItem> items)
{
    subItems = items;
    onAdd = [](LayoutBuilder &builder) { builder.stack.append(newFormLayout()); };
    onExit = layoutExit;
}

LayoutItem br()
{
    LayoutItem item;
    item.onAdd = [](LayoutBuilder &builder) {
        builder.stack.last().flush();
    };
    return item;
}

LayoutItem empty()
{
    LayoutItem item;
    item.onAdd = [](LayoutBuilder &builder) {
        ResultItem ri;
        ri.empty = true;
        builder.stack.last().pendingItems.append(ri);
    };
    return item;
}

LayoutItem hr()
{
    LayoutItem item;
    item.onAdd = [](LayoutBuilder &builder) { doAddWidget(builder, createHr()); };
    return item;
}

LayoutItem st()
{
    LayoutItem item;
    item.onAdd = [](LayoutBuilder &builder) { doAddStretch(builder, Stretch(1)); };
    return item;
}

LayoutItem noMargin()
{
    return customMargin({});
}

LayoutItem normalMargin()
{
    return customMargin({9, 9, 9, 9});
}

LayoutItem customMargin(const QMargins &margin)
{
    LayoutItem item;
    item.onAdd = [margin](LayoutBuilder &builder) {
        if (auto layout = builder.stack.last().layout)
            layout->setContentsMargins(margin);
        else if (auto widget = builder.stack.last().widget)
            widget->setContentsMargins(margin);
    };
    return item;
}

LayoutItem withFormAlignment()
{
    LayoutItem item;
    item.onAdd = [](LayoutBuilder &builder) {
        if (builder.stack.size() >= 2) {
            if (auto widget = builder.stack.at(builder.stack.size() - 2).widget) {
                const Qt::Alignment align(widget->style()->styleHint(QStyle::SH_FormLayoutLabelAlignment));
                builder.stack.last().align = align;
            }
        }
    };
    return item;
}

// "Widgets"

template <class T>
void setupWidget(LayoutItem *item)
{
    item->onAdd = [](LayoutBuilder &builder) { builder.stack.append(new T); };
    item->onExit = widgetExit;
};

Widget::Widget(std::initializer_list<LayoutItem> items)
{
    this->subItems = items;
    setupWidget<QWidget>(this);
}

Group::Group(std::initializer_list<LayoutItem> items)
{
    this->subItems = items;
    setupWidget<QGroupBox>(this);
}

Stack::Stack(std::initializer_list<LayoutItem> items)
{
    // We use a QStackedWidget instead of a QStackedLayout here because the latter will call
    // "setVisible()" when a child is added, which can lead to the widget being spawned as a
    // top-level widget. This can lead to the focus shifting away from the main application.
    subItems = items;
    onAdd = [](LayoutBuilder &builder) {
        builder.stack.append(Slice(new QStackedWidget, true));
    };
    onExit = layoutingWidgetExit<QStackedWidget>;
}

PushButton::PushButton(std::initializer_list<LayoutItem> items)
{
    this->subItems = items;
    setupWidget<QPushButton>(this);
}

SpinBox::SpinBox(std::initializer_list<LayoutItem> items)
{
    this->subItems = items;
    setupWidget<QSpinBox>(this);
}

TextEdit::TextEdit(std::initializer_list<LayoutItem> items)
{
    this->subItems = items;
    setupWidget<QTextEdit>(this);
}

Splitter::Splitter(std::initializer_list<LayoutItem> items)
{
    subItems = items;
    onAdd = [](LayoutBuilder &builder) {
        auto splitter = new QSplitter;
        splitter->setOrientation(Qt::Vertical);
        builder.stack.append(Slice(splitter, true));
    };
    onExit = layoutingWidgetExit<QSplitter>;
}

ToolBar::ToolBar(std::initializer_list<LayoutItem> items)
{
    subItems = items;
    onAdd = [](LayoutBuilder &builder) {
        auto toolbar = new QToolBar;
        toolbar->setOrientation(Qt::Horizontal);
        builder.stack.append(Slice(toolbar, true));
    };
    onExit = layoutingWidgetExit<QToolBar>;
}

TabWidget::TabWidget(std::initializer_list<LayoutItem> items)
{
    this->subItems = items;
    setupWidget<QTabWidget>(this);
}

// Special Tab

Tab::Tab(const QString &tabName, const LayoutItem &item)
{
    onAdd = [item](LayoutBuilder &builder) {
        auto tab = new QWidget;
        builder.stack.append(tab);
        item.attachTo(tab);
    };
    onExit = [tabName](LayoutBuilder &builder) {
        QWidget *inner = builder.stack.last().widget;
        builder.stack.pop_back();
        auto tabWidget = qobject_cast<QTabWidget *>(builder.stack.last().widget);
        QTC_ASSERT(tabWidget, return);
        tabWidget->addTab(inner, tabName);
    };
}

// Special If

If::If(bool condition, const LayoutItems &items, const LayoutItems &other)
{
    subItems.append(condition ? items : other);
}

// Special Application

Application::Application(std::initializer_list<LayoutItem> items)
{
    subItems = items;
    setupWidget<QWidget>(this);
    onExit = {}; // Hack: Don't dropp the last slice, we need the resulting widget.
}

int Application::exec(int &argc, char *argv[])
{
    QApplication app(argc, argv);
    LayoutBuilder builder;
    addItemHelper(builder, *this);
    if (QWidget *widget = builder.stack.last().widget)
        widget->show();
    return app.exec();
}

// "Properties"

LayoutItem title(const QString &title)
{
    return [title](QObject *target) {
        if (auto groupBox = qobject_cast<QGroupBox *>(target)) {
            groupBox->setTitle(title);
            groupBox->setObjectName(title);
        } else if (auto widget = qobject_cast<QWidget *>(target)) {
            widget->setWindowTitle(title);
        } else {
            QTC_CHECK(false);
        }
    };
}

LayoutItem windowTitle(const QString &windowTitle)
{
    return [windowTitle](QObject *target) {
        if (auto widget = qobject_cast<QWidget *>(target)) {
            widget->setWindowTitle(windowTitle);
        } else {
            QTC_CHECK(false);
        }
    };
}

LayoutItem text(const QString &text)
{
    return [text](QObject *target) {
        if (auto button = qobject_cast<QAbstractButton *>(target)) {
            button->setText(text);
        } else if (auto textEdit = qobject_cast<QTextEdit *>(target)) {
            textEdit->setText(text);
        } else {
            QTC_CHECK(false);
        }
    };
}

LayoutItem tooltip(const QString &toolTip)
{
    return [toolTip](QObject *target) {
        if (auto widget = qobject_cast<QWidget *>(target)) {
            widget->setToolTip(toolTip);
        } else {
            QTC_CHECK(false);
        }
    };
}

LayoutItem spacing(int spacing)
{
    return [spacing](QObject *target) {
        if (auto layout = qobject_cast<QLayout *>(target)) {
            layout->setSpacing(spacing);
        } else {
            QTC_CHECK(false);
        }
    };
}

LayoutItem resize(int w, int h)
{
    return [w, h](QObject *target) {
        if (auto widget = qobject_cast<QWidget *>(target)) {
            widget->resize(w, h);
        } else {
            QTC_CHECK(false);
        }
    };
}

LayoutItem columnStretch(int column, int stretch)
{
    return [column, stretch](QObject *target) {
        if (auto grid = qobject_cast<QGridLayout *>(target)) {
            grid->setColumnStretch(column, stretch);
        } else {
            QTC_CHECK(false);
        }
    };
}

LayoutItem fieldGrowthPolicy(QFormLayout::FieldGrowthPolicy policy)
{
    return [policy](QObject *target) {
        if (auto form = qobject_cast<QFormLayout *>(target)) {
            form->setFieldGrowthPolicy(policy);
        } else {
            QTC_CHECK(false);
        }
    };
}


// Id based setters

LayoutItem id(ID &out)
{
    return [&out](QObject *target) { out.ob = target; };
}

void setText(ID id, const QString &text)
{
    if (auto textEdit = qobject_cast<QTextEdit *>(id.ob))
        textEdit->setText(text);
}

// Signals

LayoutItem onClicked(const std::function<void ()> &func, QObject *guard)
{
    return [func, guard](QObject *target) {
        if (auto button = qobject_cast<QAbstractButton *>(target)) {
            QObject::connect(button, &QAbstractButton::clicked, guard ? guard : target, func);
        } else {
            QTC_CHECK(false);
        }
    };
}

LayoutItem onTextChanged(const std::function<void (const QString &)> &func, QObject *guard)
{
    return [func, guard](QObject *target) {
        if (auto button = qobject_cast<QSpinBox *>(target)) {
            QObject::connect(button, &QSpinBox::textChanged, guard ? guard : target, func);
        } else {
            QTC_CHECK(false);
        }
    };
}

LayoutItem onValueChanged(const std::function<void (int)> &func, QObject *guard)
{
    return [func, guard](QObject *target) {
        if (auto button = qobject_cast<QSpinBox *>(target)) {
            QObject::connect(button, &QSpinBox::valueChanged, guard ? guard : target, func);
        } else {
            QTC_CHECK(false);
        }
    };
}

// Convenience

QWidget *createHr(QWidget *parent)
{
    auto frame = new QFrame(parent);
    frame->setFrameShape(QFrame::HLine);
    frame->setFrameShadow(QFrame::Sunken);
    return frame;
}

// Singletons.

LayoutItem::LayoutItem(const LayoutItem &t)
{
    operator=(t);
}

void createItem(LayoutItem *item,  LayoutItem(*t)())
{
    *item = t();
}

void createItem(LayoutItem *item, const std::function<void(QObject *target)> &t)
{
    item->setter = t;
}

void createItem(LayoutItem *item, QWidget *t)
{
    if (auto l = qobject_cast<QLabel *>(t))
        l->setTextInteractionFlags(l->textInteractionFlags() | Qt::TextSelectableByMouse);

    item->onAdd = [t](LayoutBuilder &builder) { doAddWidget(builder, t); };
}

void createItem(LayoutItem *item, QLayout *t)
{
    item->onAdd = [t](LayoutBuilder &builder) { doAddLayout(builder, t); };
}

void createItem(LayoutItem *item, const QString &t)
{
    item->onAdd = [t](LayoutBuilder &builder) { doAddText(builder, t); };
}

void createItem(LayoutItem *item, const Space &t)
{
    item->onAdd = [t](LayoutBuilder &builder) { doAddSpace(builder, t); };
}

void createItem(LayoutItem *item, const Stretch &t)
{
    item->onAdd = [t](LayoutBuilder &builder) { doAddStretch(builder, t); };
}

void createItem(LayoutItem *item, const Span &t)
{
    item->onAdd = [t](LayoutBuilder &builder) {
        addItemHelper(builder, t.item);
        builder.stack.last().pendingItems.last().span = t.span;
    };
}

} // Layouting

#include "layoutbuilder.moc"
