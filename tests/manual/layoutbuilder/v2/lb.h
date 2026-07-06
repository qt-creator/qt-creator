// Copyright (C) 2023 André Pönitz
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#pragma once

#include "builderutils.h"

#include <QMargins>
#include <QObject>
#include <QPointer>
#include <QProperty>
#include <QString>

#include <functional>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <variant>

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
class QMargins;
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

// A shared value channel backed by a QProperty. A source (onValueChanged)
// writes it from a widget signal, readers (e.g. text()) observe it, and
// transformed() derives a new one via a QProperty binding. Because QProperty
// bindings track dependencies and evaluate lazily, the order in which the
// expressions referring to a bindable appear does not matter.
template <typename T>
class Bindable
{
public:
    Bindable() : prop(std::make_shared<QProperty<T>>()) {}
    Bindable(const std::shared_ptr<QProperty<T>> &prop) : prop(prop) {}

    template <class U>
    Bindable<U> transformed(const std::function<U(const T &)> &transformer) const
    {
        auto out = std::make_shared<QProperty<U>>();
        out->setBinding([source = prop, transformer] { return transformer(source->value()); });
        return Bindable<U>{out};
    }

    std::shared_ptr<QProperty<T>> prop;
};

// A destination for the values a source widget produces on change. It unifies
// the two ways to consume those values - writing them into a Bindable (so other
// builder expressions observe them), or invoking a plain callback - so a widget
// needs only a single onValueChanged(const ValueSink<T> &) instead of one
// overload per kind.
template <typename T>
class ValueSink
{
public:
    // Feed a Bindable: seed and update its shared property, so readers such as
    // text() and transformed() observe the value.
    ValueSink(Bindable<T> &bindable)
        : m_push([p = bindable.prop](const T &value) { *p = value; })
        , m_seedInitial(true)
    {}

    // Feed a plain callback, invoked on each change. The value type is fixed by
    // the widget's ValueSink<T>, so the lambda parameter can be spelled directly:
    // onValueChanged([](int value) { ... }).
    template <typename F>
        requires std::is_invocable_v<F, T>
    ValueSink(F &&callback)
        : m_push(std::forward<F>(callback))
    {}

    // Drive the sink from \a sender's \a signal, reading the value via \a producer.
    template <typename Sender, typename Signal>
    void setup(Sender *sender, Signal signal, const std::function<T()> &producer) const
    {
        if (m_seedInitial)
            m_push(producer());
        QObject::connect(sender, signal, sender, [push = m_push, producer] { push(producer()); });
    }

private:
    std::function<void(const T &)> m_push;
    bool m_seedInitial = false;
};

template <class T> using SetterArg = std::variant<T, Bindable<T>>;

//
// Basic
//

class QTCREATOR_UTILS_EXPORT Object
{
public:
    using Implementation = QObject;
    using I = Building::BuilderItem<Object>;

    Object() = default;
    Object(std::initializer_list<I> ps);

    QObject *product() const { return ptr; }

protected:
    QPointer<QObject> ptr; // The product.
};

//
// Layouts
//

class FlowLayout;
class Layout;
using LayoutModifier = std::function<void(Layout *)>;
// using LayoutModifier = void(*)(Layout *);

class QTCREATOR_UTILS_EXPORT Layout : public Object
{
public:
    using Implementation = QLayout;
    using I = Building::BuilderItem<Layout>;

    Layout() = default;
    Layout(Implementation *w);

    class LayoutItem
    {
    public:
        ~LayoutItem();
        LayoutItem();
        LayoutItem(QLayout *l) : layout(l) {}
        LayoutItem(QWidget *w) : widget(w) {}
        LayoutItem(const QString &t) : text(t) {}
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

    void span(int cols, int rows);
    void noMargin();
    void normalMargin();
    void customMargin(const QMargins &margin);
    void setColumnStretch(int cols, int rows);
    void setSpacing(int space);

    void attachTo(QWidget *);
    void addItemHelper(const LayoutItem &item);
    void addItem(I item);
    void addItems(std::initializer_list<I> items);
    void addRow(std::initializer_list<I> items);

    void flush();
    void flush_() const;
    void fieldGrowthPolicy(int policy);

    QWidget *emerge() const;

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
    Span(int n, const Layout::I &item);

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
    Widget(Implementation *w);

    QWidget *emerge() const;

    void show();
    void setSize(int, int);
    void setLayout(const Layout &layout);
    void setWindowTitle(const QString &);
    void setToolTip(const QString &);
    void noMargin(int = 0);
    void normalMargin(int = 0);
    void customMargin(const QMargins &margin);
};

class QTCREATOR_UTILS_EXPORT Label : public Widget
{
public:
    using Implementation = class LabelImpl;
    using I = Building::BuilderItem<Label>;

    Label(std::initializer_list<I> ps);
    Label(const SetterArg<QString> &text);

    void setText(const SetterArg<QString> &);
    // Label cannot act as a source: QLabel has no text-changed signal.
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
    using Implementation = class SpinBoxImpl;
    using I = Building::BuilderItem<SpinBox>;

    SpinBox(std::initializer_list<I> ps);

    void setValue(const SetterArg<int> &);

    void onValueChanged(const ValueSink<int> &sink);
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
    using Implementation = class TextEditImpl;
    using I = Building::BuilderItem<TextEdit>;

    TextEdit(std::initializer_list<I> ps);

    void setText(const SetterArg<QString> &);

    void onValueChanged(const ValueSink<QString> &sink);
};

class QTCREATOR_UTILS_EXPORT Splitter : public Widget
{
public:
    using Implementation = QSplitter;
    using I = Building::BuilderItem<Splitter>;

    Splitter(std::initializer_list<I> items);
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

// The parameter type of a unary callable (e.g. a lambda), stripped of the
// reference, so withBindable() can take the Bindable type from the builder.
template <typename F> struct FirstArg;
template <typename C, typename R, typename A>
struct FirstArg<R (C::*)(A) const> { using type = std::remove_reference_t<A>; };
template <typename C, typename R, typename A>
struct FirstArg<R (C::*)(A)> { using type = std::remove_reference_t<A>; };

// Scopes a bindable to a sub-layout, so a reader and a writer can share it
// without a separate local variable: the bindable is handed to \a builder,
// which returns the layout using it. Its type is taken from the builder's
// parameter, so the caller spells it only once. The shared bindable data
// outlives this call because the created widgets keep it alive.
template <typename Builder>
auto withBindable(const Builder &builder)
{
    typename FirstArg<decltype(&Builder::operator())>::type bindable;
    return builder(bindable);
}

// Binding to a raw pointer

class BindToPtr {};

template <typename T>
auto bindTo(T **p)
{
    return Building::IdAndArg{BindToPtr{}, p};
}

template <typename Interface>
void doit(Interface *x, BindToPtr, auto p)
{
    *p = static_cast<Interface::Implementation *>(x->product());
}


// Binding to another LayoutBuilder element

class ForwardValueId {};

// Accepts anything a ValueSink<T> can be built from - a Bindable<T> or a
// callback invocable with T. The concrete T is pinned by the widget's
// onValueChanged(const ValueSink<T> &), which the source converts to.
template <typename Source>
auto onValueChanged(Source &&source)
{
    return Building::IdAndArg{ForwardValueId{}, std::forward<Source>(source)};
}

template <typename Interface>
void doit(Interface *x, ForwardValueId, auto p)
{
    x->onValueChanged(p);
}


// Setter dispatchers


inline constexpr auto columnStretch = Building::setter(
    [](auto &x, auto &&...a) { x.setColumnStretch(a...); });
inline constexpr auto customMargins = Building::setter(
    [](auto &x, auto &&...a) { x.setContentsMargins(a...); });
inline constexpr auto fieldGrowthPolicy = Building::setter(
    [](auto &x, auto &&...a) { x.setFieldGrowthPolicy(a...); });
inline constexpr auto groupChecker = Building::setter(
    [](auto &x, auto &&...a) { x.setGroupChecker(a...); });
inline constexpr auto onClicked = Building::setter([](auto &x, auto &&...a) { x.onClicked(a...); });
inline constexpr auto onLinkHovered = Building::setter(
    [](auto &x, auto &&...a) { x.onLinkHovered(a...); });
inline constexpr auto onTextChanged = Building::setter(
    [](auto &x, auto &&...a) { x.onTextChanged(a...); });
inline constexpr auto openExternalLinks = Building::setter(
    [](auto &x, auto &&...a) { x.setOpenExternalLinks(a...); });
inline constexpr auto orientation = Building::setter(
    [](auto &x, auto &&...a) { x.setOrientation(a...); });
inline constexpr auto size = Building::setter([](auto &x, auto &&...a) { x.setSize(a...); });
inline constexpr auto text = Building::setter([](auto &x, auto &&...a) { x.setText(a...); });
inline constexpr auto textFormat = Building::setter(
    [](auto &x, auto &&...a) { x.setTextFormat(a...); });
inline constexpr auto textInteractionFlags = Building::setter(
    [](auto &x, auto &&...a) { x.setTextInteractionFlags(a...); });
inline constexpr auto title = Building::setter([](auto &x, auto &&...a) { x.setTitle(a...); });
inline constexpr auto toolTip = Building::setter([](auto &x, auto &&...a) { x.setToolTip(a...); });
inline constexpr auto value = Building::setter([](auto &x, auto &&...a) { x.setValue(a...); });
inline constexpr auto windowTitle = Building::setter(
    [](auto &x, auto &&...a) { x.setWindowTitle(a...); });
inline constexpr auto wordWrap = Building::setter([](auto &x, auto &&...a) { x.setWordWrap(a...); });

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
