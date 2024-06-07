// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

#include <functional>
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

class NestId {};

template <typename Id, typename Arg>
class IdAndArg
{
public:
    IdAndArg(Id, const Arg &arg) : arg(arg) {}
    const Arg arg; // FIXME: Could be const &, but this would currently break bindTo().
};

template<typename T1, typename T2>
struct Arg2
{
    Arg2(const T1 &a1, const T2 &a2)
        : p1(a1)
        , p2(a2)
    {}
    const T1 p1;
    const T2 p2;
};

template<typename T1, typename T2, typename T3>
struct Arg3
{
    Arg3(const T1 &a1, const T2 &a2, const T3 &a3)
        : p1(a1)
        , p2(a2)
        , p3(a3)
    {}
    const T1 p1;
    const T2 p2;
    const T3 p3;
};

template<typename T1, typename T2, typename T3, typename T4>
struct Arg4
{
    Arg4(const T1 &a1, const T2 &a2, const T3 &a3, const T4 &a4)
        : p1(a1)
        , p2(a2)
        , p3(a3)
        , p4(a4)
    {}
    const T1 p1;
    const T2 p2;
    const T3 p3;
    const T4 p4;
};

// The main dispatcher

void doit(auto x, auto id, auto p);

template <typename X> class BuilderItem
{
public:
    // Nested child object
    template <typename Inner>
    BuilderItem(Inner && p)
    {
        apply = [&p](X *x) { doit(x, NestId{}, std::forward<Inner>(p)); };
    }

    // Property setter
    template <typename Id, typename Arg>
    BuilderItem(IdAndArg<Id, Arg> && idarg)
    {
        apply = [&idarg](X *x) { doit(x, Id{}, idarg.arg); };
    }

    std::function<void(X *)> apply;
};


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
    using I = BuilderItem<Object>;

    Object() = default;
    Object(std::initializer_list<I> ps);
};

//
// Layouts
//

class FlowLayout;
class Layout;
using LayoutModifier = std::function<void(Layout *)>;
// using LayoutModifier = void(*)(Layout *);

class QTCREATOR_UTILS_EXPORT LayoutItem
{
public:
    ~LayoutItem();
    LayoutItem();
    LayoutItem(QLayout *l);
    LayoutItem(QWidget *w);
    LayoutItem(const QString &t);
    LayoutItem(const LayoutModifier &inner);

    QString text;
    QLayout *layout = nullptr;
    QWidget *widget = nullptr;
    int stretch = -1;
    int spanCols = 1;
    int spanRows = 1;
    bool empty = false;
    LayoutModifier ownerModifier;
    //Qt::Alignment align = {};
};

class QTCREATOR_UTILS_EXPORT Layout : public Object
{
public:
    using Implementation = QLayout;
    using I = BuilderItem<Layout>;

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
    using I = BuilderItem<Column>;

    Column(std::initializer_list<I> ps);
};

class QTCREATOR_UTILS_EXPORT Row : public Layout
{
public:
    using Implementation = QHBoxLayout;
    using I = BuilderItem<Row>;

    Row(std::initializer_list<I> ps);
};

class QTCREATOR_UTILS_EXPORT Form : public Layout
{
public:
    using Implementation = QFormLayout;
    using I = BuilderItem<Form>;

    Form();
    Form(std::initializer_list<I> ps);
};

class QTCREATOR_UTILS_EXPORT Grid : public Layout
{
public:
    using Implementation = QGridLayout;
    using I = BuilderItem<Grid>;

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
    using I = BuilderItem<Widget>;

    Widget() = default;
    Widget(std::initializer_list<I> ps);
    Widget(Implementation *w) { ptr = w; }

    QWidget *emerge() const;
    void show();

    void setLayout(const Layout &layout);
    void setSize(int, int);
    void setWindowTitle(const QString &);
    void setToolTip(const QString &);
    void setNoMargins(int = 0);
    void setNormalMargins(int = 0);
    void setContentsMargins(int left, int top, int right, int bottom);
};

class QTCREATOR_UTILS_EXPORT Label : public Widget
{
public:
    using Implementation = QLabel;
    using I = BuilderItem<Label>;

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
    using I = BuilderItem<Group>;

    Group(std::initializer_list<I> ps);

    void setTitle(const QString &);
    void setGroupChecker(const std::function<void(QObject *)> &);
};

class QTCREATOR_UTILS_EXPORT SpinBox : public Widget
{
public:
    using Implementation = QSpinBox;
    using I = BuilderItem<SpinBox>;

    SpinBox(std::initializer_list<I> ps);

    void setValue(int);
    void onTextChanged(const std::function<void(QString)> &);
};

class QTCREATOR_UTILS_EXPORT PushButton : public Widget
{
public:
    using Implementation = QPushButton;
    using I = BuilderItem<PushButton>;

    PushButton(std::initializer_list<I> ps);

    void setText(const QString &);
    void onClicked(const std::function<void()> &, QObject *guard);
};

class QTCREATOR_UTILS_EXPORT TextEdit : public Widget
{
public:
    using Implementation = QTextEdit;
    using I = BuilderItem<TextEdit>;
    using Id = Implementation *;

    TextEdit(std::initializer_list<I> ps);

    void setText(const QString &);
};

class QTCREATOR_UTILS_EXPORT Splitter : public Widget
{
public:
    using Implementation = QSplitter;
    using I = BuilderItem<Splitter>;

    Splitter(std::initializer_list<I> items);
};

class QTCREATOR_UTILS_EXPORT Stack : public Widget
{
public:
    using Implementation = QStackedWidget;
    using I = BuilderItem<Stack>;

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
    using I = BuilderItem<TabWidget>;

    TabWidget(std::initializer_list<I> items);
};

class QTCREATOR_UTILS_EXPORT ToolBar : public Widget
{
public:
    using Implementation = QToolBar;
    using I = Layouting::BuilderItem<ToolBar>;

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
    return IdAndArg{BindToId{}, p};
}

template <typename Interface>
void doit(Interface *x, BindToId, auto p)
{
    *p = static_cast<typename Interface::Implementation *>(x->ptr);
}

class IdId {};
auto id(auto p) { return IdAndArg{IdId{}, p}; }

template <typename Interface>
void doit(Interface *x, IdId, auto p)
{
    **p = static_cast<typename Interface::Implementation *>(x->ptr);
}

// Setter dispatchers

#define QTCREATOR_SETTER(name, setter) \
    class name##_TAG {}; \
    inline auto name(auto p) { return IdAndArg{name##_TAG{}, p}; } \
    inline void doit(auto x, name##_TAG, auto p) { x->setter(p); }

#define QTCREATOR_SETTER2(name, setter) \
    class name##_TAG {}; \
    inline auto name(auto p1, auto p2) { return IdAndArg{name##_TAG{}, Arg2{p1, p2}}; } \
    inline void doit(auto x, name##_TAG, auto p) { x->setter(p.p1, p.p2); }

#define QTCREATOR_SETTER3(name, setter) \
    class name##_TAG {}; \
    inline auto name(auto p1, auto p2, auto p3) { return IdAndArg{name##_TAG{}, Arg3{p1, p2, p3}}; } \
    inline void doit(auto x, name##_TAG, auto p) { x->setter(p.p1, p.p2, p.p3); }

#define QTCREATOR_SETTER4(name, setter) \
    class name##_TAG {}; \
    inline auto name(auto p1, auto p2, auto p3, auto p4) { return IdAndArg{name##_TAG{}, Arg4{p1, p2, p3, p4}}; } \
    inline void doit(auto x, name##_TAG, auto p) { x->setter(p.p1, p.p2, p.p3, p.p4); }

QTCREATOR_SETTER(fieldGrowthPolicy, setFieldGrowthPolicy);
QTCREATOR_SETTER(groupChecker, setGroupChecker);
QTCREATOR_SETTER(openExternalLinks, setOpenExternalLinks);
QTCREATOR_SETTER2(size, setSize)
QTCREATOR_SETTER(text, setText)
QTCREATOR_SETTER(textFormat, setTextFormat);
QTCREATOR_SETTER(textInteractionFlags, setTextInteractionFlags);
QTCREATOR_SETTER(title, setTitle)
QTCREATOR_SETTER(toolTip, setToolTip);
QTCREATOR_SETTER(windowTitle, setWindowTitle);
QTCREATOR_SETTER(wordWrap, setWordWrap);
QTCREATOR_SETTER2(columnStretch, setColumnStretch);
QTCREATOR_SETTER2(onClicked, onClicked);
QTCREATOR_SETTER2(onLinkHovered, onLinkHovered);
QTCREATOR_SETTER2(onTextChanged, onTextChanged);
QTCREATOR_SETTER4(customMargins, setContentsMargins);

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
void doit(auto outer, NestId, Inner && inner)
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
