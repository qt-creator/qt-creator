// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "builderutils.h"

#include <QString>

#include <initializer_list>
#include <vector>

#if defined(UTILS_LIBRARY)
#  define QTCREATOR_UTILS_EXPORT Q_DECL_EXPORT
#elif defined(UTILS_STATIC_LIBRARY)
#  define QTCREATOR_UTILS_EXPORT
#else
#  define QTCREATOR_UTILS_EXPORT Q_DECL_IMPORT
#endif

QT_BEGIN_NAMESPACE
class QBoxLayout;
class QFormLayout;
class QGridLayout;
class QGroupBox;
class QHBoxLayout;
class QLabel;
class QLayout;
class QObject;
class QPushButton;
class QSpinBox;
class QSplitter;
class QStackedWidget;
class QTabWidget;
class QTextEdit;
class QToolBar;
class QVBoxLayout;
class QWidget;
QT_END_NAMESPACE

namespace Layouting {

//////////////////////////////////////////////

//
// Basic
//

class QTCREATOR_UTILS_EXPORT Thing
{
public:
    void *ptr; // The product.
};

class QTCREATOR_UTILS_EXPORT Object : public Thing
{
public:
    using Implementation = QObject;
    using I = Building::BuilderItem<Object>;

    Object() = default;
    Object(std::initializer_list<I> ps);
};

//
// Layouts
//

class FlowLayout;
class Layout;
using LayoutModifier = std::function<void(Layout *)>;

class QTCREATOR_UTILS_EXPORT LayoutItem
{
public:
    ~LayoutItem();
    LayoutItem();
    LayoutItem(QLayout *l);
    LayoutItem(QWidget *w);
    LayoutItem(const QString &t);

    QString text;
    QLayout *layout = nullptr;
    QWidget *widget = nullptr;
    int stretch = -1;
    int spanCols = 1;
    int spanRows = 1;
    bool empty = false;
};

class QTCREATOR_UTILS_EXPORT Layout : public Object
{
public:
    using Implementation = QLayout;
    using I = Building::BuilderItem<Layout>;

    Layout() = default;
    Layout(Implementation *w) { ptr = w; }

    void span(int cols, int rows);

    void setNoMargins();
    void setNormalMargins();
    void setContentsMargins(int left, int top, int right, int bottom);
    void setColumnStretch(int cols, int rows);
    void setSpacing(int space);
    void setFieldGrowthPolicy(int policy);

    void attachTo(QWidget *);

    void addItem(I item);
    void addItems(std::initializer_list<I> items);
    void addRow(std::initializer_list<I> items);
    void addLayoutItem(const LayoutItem &item);

    void flush();
    void flush_() const;

    QWidget *emerge() const;
    void show() const;

    QFormLayout *asForm();
    QGridLayout *asGrid();
    QBoxLayout *asBox();
    FlowLayout *asFlow();

    // Grid-only
    int currentGridColumn = 0;
    int currentGridRow = 0;
    //Qt::Alignment align = {};
    bool useFormAlignment = false;

    std::vector<LayoutItem> pendingItems;
};

class QTCREATOR_UTILS_EXPORT Column : public Layout
{
public:
    using Implementation = QVBoxLayout;
    using I = Building::BuilderItem<Column>;

    Column(std::initializer_list<I> ps);
};

class QTCREATOR_UTILS_EXPORT Row : public Layout
{
public:
    using Implementation = QHBoxLayout;
    using I = Building::BuilderItem<Row>;

    Row(std::initializer_list<I> ps);
};

class QTCREATOR_UTILS_EXPORT Form : public Layout
{
public:
    using Implementation = QFormLayout;
    using I = Building::BuilderItem<Form>;

    Form();
    Form(std::initializer_list<I> ps);
};

class QTCREATOR_UTILS_EXPORT Grid : public Layout
{
public:
    using Implementation = QGridLayout;
    using I = Building::BuilderItem<Grid>;

    Grid();
    Grid(std::initializer_list<I> ps);
};

class QTCREATOR_UTILS_EXPORT Flow : public Layout
{
public:
    Flow(std::initializer_list<I> ps);
};

class QTCREATOR_UTILS_EXPORT Stretch
{
public:
    explicit Stretch(int stretch) : stretch(stretch) {}

    int stretch;
};

class QTCREATOR_UTILS_EXPORT Space
{
public:
    explicit Space(int space) : space(space) {}

    int space;
};

class QTCREATOR_UTILS_EXPORT Span
{
public:
    Span(int cols, const Layout::I &item);
    Span(int cols, int rows, const Layout::I &item);

    Layout::I item;
    int spanCols = 1;
    int spanRows = 1;
};

//
// Widgets
//

class QTCREATOR_UTILS_EXPORT Widget : public Object
{
public:
    using Implementation = QWidget;
    using I = Building::BuilderItem<Widget>;

    Widget() = default;
    Widget(std::initializer_list<I> ps);
    Widget(Implementation *w) { ptr = w; }

    QWidget *emerge() const;
    void show();

    void setLayout(const Layout &layout);
    void setSize(int, int);
    void setWindowTitle(const QString &);
    void setWindowFlags(Qt::WindowFlags);
    void setToolTip(const QString &);
    void setNoMargins(int = 0);
    void setNormalMargins(int = 0);
    void setContentsMargins(int left, int top, int right, int bottom);
    void activateWindow();
};

class QTCREATOR_UTILS_EXPORT Label : public Widget
{
public:
    using Implementation = QLabel;
    using I = Building::BuilderItem<Label>;

    Label(std::initializer_list<I> ps);
    Label(const QString &text);

    void setText(const QString &);
    void setTextFormat(Qt::TextFormat);
    void setWordWrap(bool);
    void setTextInteractionFlags(Qt::TextInteractionFlags);
    void setOpenExternalLinks(bool);
    void onLinkHovered(const std::function<void(const QString &)> &, QObject *guard);
};

class QTCREATOR_UTILS_EXPORT Group : public Widget
{
public:
    using Implementation = QGroupBox;
    using I = Building::BuilderItem<Group>;

    Group(std::initializer_list<I> ps);

    void setTitle(const QString &);
    void setGroupChecker(const std::function<void(QObject *)> &);
};

class QTCREATOR_UTILS_EXPORT SpinBox : public Widget
{
public:
    using Implementation = QSpinBox;
    using I = Building::BuilderItem<SpinBox>;

    SpinBox(std::initializer_list<I> ps);

    void setValue(int);
    void onTextChanged(const std::function<void(QString)> &);
};

class QTCREATOR_UTILS_EXPORT PushButton : public Widget
{
public:
    using Implementation = QPushButton;
    using I = Building::BuilderItem<PushButton>;

    PushButton(std::initializer_list<I> ps);

    void setText(const QString &);
    void onClicked(const std::function<void()> &, QObject *guard);
};

class QTCREATOR_UTILS_EXPORT TextEdit : public Widget
{
public:
    using Implementation = QTextEdit;
    using I = Building::BuilderItem<TextEdit>;
    using Id = Implementation *;

    TextEdit(std::initializer_list<I> ps);

    void setText(const QString &);
};

class QTCREATOR_UTILS_EXPORT Splitter : public Widget
{
public:
    using Implementation = QSplitter;
    using I = Building::BuilderItem<Splitter>;

    Splitter(std::initializer_list<I> items);
    void setOrientation(Qt::Orientation);
    void setStretchFactor(int index, int stretch);
    void setChildrenCollapsible(bool collapsible);
};

class QTCREATOR_UTILS_EXPORT Stack : public Widget
{
public:
    using Implementation = QStackedWidget;
    using I = Building::BuilderItem<Stack>;

    Stack() : Stack({}) {}
    Stack(std::initializer_list<I> items);
};

class QTCREATOR_UTILS_EXPORT Tab : public Widget
{
public:
    using Implementation = QWidget;

    Tab(const QString &tabName, const Layout &inner);

    const QString tabName;
    const Layout inner;
};

class QTCREATOR_UTILS_EXPORT TabWidget : public Widget
{
public:
    using Implementation = QTabWidget;
    using I = Building::BuilderItem<TabWidget>;

    TabWidget(std::initializer_list<I> items);
};

class QTCREATOR_UTILS_EXPORT ToolBar : public Widget
{
public:
    using Implementation = QToolBar;
    using I = Building::BuilderItem<ToolBar>;

    ToolBar(std::initializer_list<I> items);
};

// Special

class QTCREATOR_UTILS_EXPORT If
{
public:
    If(bool condition,
       const std::initializer_list<Layout::I> ifcase,
       const std::initializer_list<Layout::I> thencase = {});

    const std::initializer_list<Layout::I> used;
};

//
// Dispatchers
//

// We need one 'Id' (and a corresponding function wrapping arguments into a
// tuple marked by this id) per 'name' of "backend" setter member function,
// i.e. one 'text' is sufficient for QLabel::setText, QLineEdit::setText.
// The name of the Id does not have to match the backend names as it
// is mapped per-backend-type in the respective setter implementation
// but we assume that it generally makes sense to stay close to the
// wrapped API name-wise.

// These are free functions overloaded on the type of builder object
// and setter id. The function implementations are independent, but
// the base expectation is that they will forwards to the backend
// type's setter.

// Special dispatchers


class BindToId {};

template <typename T>
auto bindTo(T **p)
{
    return Building::IdAndArg{BindToId{}, p};
}

template <typename Interface>
void doit(Interface *x, BindToId, auto p)
{
    *p = static_cast<typename Interface::Implementation *>(x->ptr);
}

class IdId {};
auto id(auto p) { return Building::IdAndArg{IdId{}, p}; }

template <typename Interface>
void doit(Interface *x, IdId, auto p)
{
    **p = static_cast<typename Interface::Implementation *>(x->ptr);
}

// Setter dispatchers

QTC_DEFINE_BUILDER_SETTER(childrenCollapsible, setChildrenCollapsible)
QTC_DEFINE_BUILDER_SETTER(columnStretch, setColumnStretch)
QTC_DEFINE_BUILDER_SETTER(customMargins, setContentsMargins)
QTC_DEFINE_BUILDER_SETTER(fieldGrowthPolicy, setFieldGrowthPolicy)
QTC_DEFINE_BUILDER_SETTER(groupChecker, setGroupChecker)
QTC_DEFINE_BUILDER_SETTER(onClicked, onClicked)
QTC_DEFINE_BUILDER_SETTER(onLinkHovered, onLinkHovered)
QTC_DEFINE_BUILDER_SETTER(onTextChanged, onTextChanged)
QTC_DEFINE_BUILDER_SETTER(openExternalLinks, setOpenExternalLinks)
QTC_DEFINE_BUILDER_SETTER(orientation, setOrientation);
QTC_DEFINE_BUILDER_SETTER(size, setSize)
QTC_DEFINE_BUILDER_SETTER(text, setText)
QTC_DEFINE_BUILDER_SETTER(textFormat, setTextFormat)
QTC_DEFINE_BUILDER_SETTER(textInteractionFlags, setTextInteractionFlags)
QTC_DEFINE_BUILDER_SETTER(title, setTitle)
QTC_DEFINE_BUILDER_SETTER(toolTip, setToolTip)
QTC_DEFINE_BUILDER_SETTER(windowTitle, setWindowTitle)
QTC_DEFINE_BUILDER_SETTER(wordWrap, setWordWrap);
QTC_DEFINE_BUILDER_SETTER(windowFlags, setWindowFlags);

// Nesting dispatchers

QTCREATOR_UTILS_EXPORT void addToLayout(Layout *layout, const Layout &inner);
QTCREATOR_UTILS_EXPORT void addToLayout(Layout *layout, const Widget &inner);
QTCREATOR_UTILS_EXPORT void addToLayout(Layout *layout, QWidget *inner);
QTCREATOR_UTILS_EXPORT void addToLayout(Layout *layout, QLayout *inner);
QTCREATOR_UTILS_EXPORT void addToLayout(Layout *layout, const LayoutModifier &inner);
QTCREATOR_UTILS_EXPORT void addToLayout(Layout *layout, const QString &inner);
QTCREATOR_UTILS_EXPORT void addToLayout(Layout *layout, const Space &inner);
QTCREATOR_UTILS_EXPORT void addToLayout(Layout *layout, const Stretch &inner);
QTCREATOR_UTILS_EXPORT void addToLayout(Layout *layout, const If &inner);
QTCREATOR_UTILS_EXPORT void addToLayout(Layout *layout, const Span &inner);
// ... can be added to anywhere later to support "user types"

QTCREATOR_UTILS_EXPORT void addToWidget(Widget *widget, const Layout &layout);

QTCREATOR_UTILS_EXPORT void addToTabWidget(TabWidget *tabWidget, const Tab &inner);

QTCREATOR_UTILS_EXPORT void addToSplitter(Splitter *splitter, QWidget *inner);
QTCREATOR_UTILS_EXPORT void addToSplitter(Splitter *splitter, const Widget &inner);
QTCREATOR_UTILS_EXPORT void addToSplitter(Splitter *splitter, const Layout &inner);

QTCREATOR_UTILS_EXPORT void addToStack(Stack *stack, QWidget *inner);
QTCREATOR_UTILS_EXPORT void addToStack(Stack *stack, const Widget &inner);
QTCREATOR_UTILS_EXPORT void addToStack(Stack *stack, const Layout &inner);

template <class Inner>
void doit_nested(Layout *outer, Inner && inner)
{
    addToLayout(outer, std::forward<Inner>(inner));
}

void doit_nested(Widget *outer, auto inner)
{
    addToWidget(outer, inner);
}

void doit_nested(TabWidget *outer, auto inner)
{
    addToTabWidget(outer, inner);
}

void doit_nested(Stack *outer, auto inner)
{
    addToStack(outer, inner);
}

void doit_nested(Splitter *outer, auto inner)
{
    addToSplitter(outer, inner);
}

template <class Inner>
void doit(auto outer, Building::NestId, Inner && inner)
{
    doit_nested(outer, std::forward<Inner>(inner));
}

// Special layout items

QTCREATOR_UTILS_EXPORT void empty(Layout *);
QTCREATOR_UTILS_EXPORT void br(Layout *);
QTCREATOR_UTILS_EXPORT void st(Layout *);
QTCREATOR_UTILS_EXPORT void noMargin(Layout *);
QTCREATOR_UTILS_EXPORT void normalMargin(Layout *);
QTCREATOR_UTILS_EXPORT void withFormAlignment(Layout *);
QTCREATOR_UTILS_EXPORT void hr(Layout *);

QTCREATOR_UTILS_EXPORT LayoutModifier spacing(int space);

// Convenience

QTCREATOR_UTILS_EXPORT QWidget *createHr(QWidget *parent = nullptr);

} // Layouting
