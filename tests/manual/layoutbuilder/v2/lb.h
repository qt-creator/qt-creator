// Copyright (C) 2023 André Pönitz
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#pragma once

#include <QList>
#include <QString>

#include <array>
#include <cstring>
#include <initializer_list>

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
class QLabel;
class QLayout;
class QMargins;
class QObject;
class QPushButton;
class QSpinBox;
class QSplitter;
class QStackedWidget;
class QTabWidget;
class QTextEdit;
class QToolBar;
class QWidget;
QT_END_NAMESPACE

namespace Layouting {

struct LayoutItem;

// struct NestId {};
struct NestId {};

template <typename T1, typename T2>
struct IdAndArg
{
    IdAndArg(const T1 &id, const T2 &arg) : id(id), arg(arg) {}
    T1 id;
    T2 arg;
};

// The main dispatchers

void doit(auto x, auto id, auto a);

// BuilderItem

template <typename X, typename XInterface>
struct BuilderItem : XInterface
{
public:
    struct I
    {
        // Nested child object
        template <typename Inner>
        I(const Inner &p)
        {
            apply = [p](XInterface *x) { doit(x, NestId{}, p); };
        }

        // Property setter
        template <typename Id, typename Arg1>
        I(const IdAndArg<Id, Arg1> &p)
        {
            apply = [p](XInterface *x) { doit(x, p.id, p.arg); };
        }

        std::function<void(XInterface *)> apply;
    };

    void create()
    {
        XInterface::ptr = new XInterface::Implementation;
    }

    void adopt(XInterface::Implementation *ptr)
    {
        XInterface::ptr = ptr;
    }

    void apply(const I &init)
    {
        init.apply(this);
    }

    using Id = typename XInterface::Implementation *;
};


//////////////////////////////////////////////

struct LayoutInterface;
struct WidgetInterface;

struct QTCREATOR_UTILS_EXPORT LayoutItem
{
    LayoutItem();
    LayoutItem(QLayout *l) : layout(l), empty(!l) {}
    LayoutItem(QWidget *w) : widget(w), empty(!w) {}
    LayoutItem(const QString &t) : text(t) {}
    LayoutItem(const LayoutInterface &inner);
    LayoutItem(const WidgetInterface &inner);
    ~LayoutItem();

    QString text;
    QLayout *layout = nullptr;
    QWidget *widget = nullptr;
    int space = -1;
    int stretch = -1;
    int spanCols = 1;
    int spanRows = 1;
    bool empty = false;
    bool break_ = false;
};

using LayoutItems = QList<LayoutItem>;


// We need two classes for each user visible Builder type.
// The actual Builder classes derive from  BuilderItem with two parameters,
// one is the Builder class itself for CRTP and a one "Interface" type.
// The "Interface" types act (individually, and as a hierarchy) as interface
// members of the real QObject/QWidget/... hierarchy things. This wrapper is not
// strictly needed, the Q* hierarchy could be used directly, at the
// price of #include'ing the definitions of each participating class,
// which does not scale well.

//
// Basic
//

struct QTCREATOR_UTILS_EXPORT ThingInterface
{
    template <typename T>
    T *access_() const { return static_cast<T *>(ptr); }

    void *ptr; // The product.
};

struct QTCREATOR_UTILS_EXPORT ObjectInterface : ThingInterface
{
    using Implementation = QObject;
};

struct QTCREATOR_UTILS_EXPORT Object : BuilderItem<Object, ObjectInterface>
{
    Object(std::initializer_list<I> ps);
};

//
// Layouts
//

struct QTCREATOR_UTILS_EXPORT LayoutInterface : ObjectInterface
{
    using Implementation = QLayout;

    void span(int cols, int rows);
    void noMargin();
    void normalMargin();
    void customMargin(const QMargins &margin);

    void addItem(const LayoutItem &item);

    void flush();
    void fieldGrowthPolicy(int policy);

    QFormLayout *asForm();
    QGridLayout *asGrid();
    QBoxLayout *asBox();

    std::vector<LayoutItem> pendingItems;

    // Grid-only
    int currentGridColumn = 0;
    int currentGridRow = 0;
    Qt::Alignment align = {};
};

struct QTCREATOR_UTILS_EXPORT Layout : BuilderItem<Layout, LayoutInterface>
{
};

struct QTCREATOR_UTILS_EXPORT Column : BuilderItem<Column, LayoutInterface>
{
    Column(std::initializer_list<I> ps);
};

struct QTCREATOR_UTILS_EXPORT Row : BuilderItem<Row, LayoutInterface>
{
    Row(std::initializer_list<I> ps);
};

struct QTCREATOR_UTILS_EXPORT Form : BuilderItem<Form, LayoutInterface>
{
    Form(std::initializer_list<I> ps);
};

struct QTCREATOR_UTILS_EXPORT Grid : BuilderItem<Grid, LayoutInterface>
{
    Grid(std::initializer_list<I> ps);
};

struct QTCREATOR_UTILS_EXPORT Flow : BuilderItem<Flow, LayoutInterface>
{
    Flow(std::initializer_list<I> ps);
};

struct QTCREATOR_UTILS_EXPORT Stretch : LayoutItem
{
    explicit Stretch(int stretch) { this->stretch = stretch; }
};

struct QTCREATOR_UTILS_EXPORT Space : LayoutItem
{
    explicit Space(int space) { this->space = space; }
};

struct QTCREATOR_UTILS_EXPORT Span : LayoutItem
{
    Span(int n, const LayoutItem &item);
};

//
// Widgets
//

struct QTCREATOR_UTILS_EXPORT WidgetInterface : ObjectInterface
{
    using Implementation = QWidget;
    QWidget *emerge();

    void show();
    void resize(int, int);
    void setLayout(const LayoutInterface &layout);
    void setWindowTitle(const QString &);
    void setToolTip(const QString &);
    void noMargin();
    void normalMargin();
    void customMargin(const QMargins &margin);
};

struct QTCREATOR_UTILS_EXPORT Widget : BuilderItem<Widget, WidgetInterface>
{
    Widget(std::initializer_list<I> ps);
};

// Label

struct QTCREATOR_UTILS_EXPORT LabelInterface : WidgetInterface
{
    using Implementation = QLabel;

    void setText(const QString &);
};

struct QTCREATOR_UTILS_EXPORT Label : BuilderItem<Label, LabelInterface>
{
    Label(std::initializer_list<I> ps);
    Label(const QString &text);
};

// Group

struct QTCREATOR_UTILS_EXPORT GroupInterface : WidgetInterface
{
    using Implementation = QGroupBox;

    void setTitle(const QString &);
};

struct QTCREATOR_UTILS_EXPORT Group : BuilderItem<Group, GroupInterface>
{
    Group(std::initializer_list<I> ps);
};

// SpinBox

struct QTCREATOR_UTILS_EXPORT SpinBoxInterface : WidgetInterface
{
    using Implementation = QSpinBox;

    void setValue(int);
    void onTextChanged(const std::function<void(QString)> &);
};

struct QTCREATOR_UTILS_EXPORT SpinBox : BuilderItem<SpinBox, SpinBoxInterface>
{
    SpinBox(std::initializer_list<I> ps);
};

// PushButton

struct QTCREATOR_UTILS_EXPORT PushButtonInterface : WidgetInterface
{
    using Implementation = QPushButton;

    void setText(const QString &);
    void onClicked(const std::function<void()> &);
};

struct QTCREATOR_UTILS_EXPORT PushButton : BuilderItem<PushButton, PushButtonInterface>
{
    PushButton(std::initializer_list<I> ps);
};

// TextEdit

struct QTCREATOR_UTILS_EXPORT TextEditInterface : WidgetInterface
{
    using Implementation = QTextEdit;

    void setText(const QString &);
};

struct QTCREATOR_UTILS_EXPORT TextEdit : BuilderItem<TextEdit, TextEditInterface>
{
    TextEdit(std::initializer_list<I> ps);
};

// Splitter

struct QTCREATOR_UTILS_EXPORT SplitterInterface : WidgetInterface
{
    using Implementation = QSplitter;
};

struct QTCREATOR_UTILS_EXPORT Splitter : BuilderItem<Splitter, SplitterInterface>
{
    Splitter(std::initializer_list<I> items);
};

// Stack

struct QTCREATOR_UTILS_EXPORT StackInterface : WidgetInterface
{
};

struct QTCREATOR_UTILS_EXPORT Stack : BuilderItem<Stack, StackInterface>
{
    Stack() : Stack({}) {}
    Stack(std::initializer_list<I> items);
};

// TabWidget

struct QTCREATOR_UTILS_EXPORT TabInterface : WidgetInterface
{
};

struct QTCREATOR_UTILS_EXPORT TabWidgetInterface : WidgetInterface
{
    using Implementation = QTabWidget;
};

struct QTCREATOR_UTILS_EXPORT Tab :  BuilderItem<Tab, TabInterface>
{
    Tab(const QString &tabName, const LayoutItem &item);
};

struct QTCREATOR_UTILS_EXPORT TabWidget : BuilderItem<TabWidget, TabWidgetInterface>
{
    TabWidget(std::initializer_list<I> items);
};

// ToolBar

struct QTCREATOR_UTILS_EXPORT ToolBarInterface : WidgetInterface
{
    using Implementation = QToolBar;
};

struct QTCREATOR_UTILS_EXPORT ToolBar : BuilderItem<ToolBar, ToolBarInterface>
{
    ToolBar(std::initializer_list<I> items);
};

// Special

struct QTCREATOR_UTILS_EXPORT If : LayoutItem
{
    If(bool condition, const LayoutItems &item, const LayoutItems &other = {});
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

// Special dispatchers :w


struct BindToId {};

template <typename T>
auto bindTo(T **x)
{
    // FIXME: Evil hack to shut up clang-tidy which does not see that the returned tuple will
    // result in an assignment to *x and complains about every use of the bound value later.
    // main.cpp:129:5: Called C++ object pointer is null [clang-analyzer-core.CallAndMessage]
    // 1: Calling 'bindTo<QWidget>' in /data/dev/creator/tests/manual/layoutbuilder/v2/main.cpp:73
    // 2: Null pointer value stored to 'w' in /data/dev/creator/tests/manual/layoutbuilder/v2/lb.h:518
    // 3: Returning from 'bindTo<QWidget>' in /data/dev/creator/tests/manual/layoutbuilder/v2/main.cpp:73
    // 4: Called C++ object pointer is null in /data/dev/creator/tests/manual/layoutbuilder/v2/main.cpp:129
    *x = reinterpret_cast<T*>(1);

    return IdAndArg{BindToId{}, x};
}

template <typename Interface>
void doit(Interface *x, BindToId, auto p)
{
    *p = static_cast<Interface::Implementation *>(x->ptr);
}

struct IdId {};
auto id(auto x) { return IdAndArg{IdId{}, x}; }

template <typename Interface>
void doit(Interface *x, IdId, auto p)
{
    *p = static_cast<Interface::Implementation *>(x->ptr);
}

// Setter dispatchers

struct SizeId {};
auto size(auto w, auto h) { return IdAndArg{SizeId{}, std::pair{w, h}}; }
void doit(auto x, SizeId, auto p) { x->resize(p.first, p.second); }

struct TextId {};
auto text(auto x) { return IdAndArg{TextId{}, x}; }
void doit(auto x, TextId, auto t) { x->setText(t); }

struct TitleId {};
auto title(auto x) { return IdAndArg{TitleId{}, x}; }
void doit(auto x, TitleId, auto t) { x->setTitle(t); }

struct ToolTipId {};
auto toolTip(auto x) { return IdAndArg{ToolTipId{}, x}; }
void doit(auto x, ToolTipId, auto t) { x->setToolTip(t); }

struct WindowTitleId {};
auto windowTitle(auto x) { return IdAndArg{WindowTitleId{}, x}; }
void doit(auto x, WindowTitleId, auto t) { x->setWindowTitle(t); }

struct OnTextChangedId {};
auto onTextChanged(auto x) { return IdAndArg{OnTextChangedId{}, x}; }
void doit(auto x, OnTextChangedId, auto func) { x->onTextChanged(func); }

struct OnClickedId {};
auto onClicked(auto x) { return IdAndArg{OnClickedId{}, x}; }
void doit(auto x, OnClickedId, auto func) { x->onClicked(func); }


// Nesting dispatchers

QTCREATOR_UTILS_EXPORT void addNestedItem(WidgetInterface *widget, const LayoutInterface &layout);
QTCREATOR_UTILS_EXPORT void addNestedItem(LayoutInterface *layout, const LayoutItem &inner);
QTCREATOR_UTILS_EXPORT void addNestedItem(LayoutInterface *layout, const LayoutInterface &inner);
QTCREATOR_UTILS_EXPORT void addNestedItem(LayoutInterface *layout, const WidgetInterface &inner);
QTCREATOR_UTILS_EXPORT void addNestedItem(LayoutInterface *layout, const QString &inner);
QTCREATOR_UTILS_EXPORT void addNestedItem(LayoutInterface *layout, const std::function<LayoutItem()> &inner);
// ... can be added to anywhere later to support "user types"

void doit(auto outer, NestId, auto inner)
{
    addNestedItem(outer, inner);
}


// Special layout items

QTCREATOR_UTILS_EXPORT LayoutItem br();
QTCREATOR_UTILS_EXPORT LayoutItem empty();
QTCREATOR_UTILS_EXPORT LayoutItem hr();
QTCREATOR_UTILS_EXPORT LayoutItem withFormAlignment();
QTCREATOR_UTILS_EXPORT LayoutItem st();


// Convenience

QTCREATOR_UTILS_EXPORT QWidget *createHr(QWidget *parent = nullptr);

} // Layouting
