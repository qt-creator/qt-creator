// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "builderutils.h"

#include <QAction>
#include <QFrame>
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
class QCompleter;
class QFormLayout;
class QGridLayout;
class QGroupBox;
class QHBoxLayout;
class QLabel;
class QLayout;
class QObject;
class QPushButton;
class QScrollArea;
class QSize;
class QSizePolicy;
class QSpinBox;
class QSplitter;
class QStackedWidget;
class QTabWidget;
class QTextEdit;
class QToolBar;
class QToolButton;
class QVBoxLayout;
class QWidget;
QT_END_NAMESPACE

namespace SpinnerSolution {
class SpinnerWidget;
enum class SpinnerState;
} // namespace SpinnerSolution

namespace Utils {
class FancyLineEdit;
class FilePath;
class MarkdownBrowser;
class Icon;
class IconDisplay;
} // namespace Utils

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
    Qt::Alignment alignment = {};
    bool empty = false;
    bool advancesCell = true;
};

class QTCREATOR_UTILS_EXPORT Layout : public Object
{
public:
    using Implementation = QLayout;
    using I = Building::BuilderItem<Layout>;

    Layout() = default;
    Layout(Implementation *w) { ptr = w; }

    void span(int cols, int rows);
    void align(Qt::Alignment alignment);

    void setAlignment(Qt::Alignment alignment);
    void setNoMargins();
    void setNormalMargins();
    void setContentsMargins(int left, int top, int right, int bottom);
    void setColumnStretch(int column, int stretch);
    void setRowStretch(int row, int stretch);
    void setSpacing(int space);
    void setFieldGrowthPolicy(int policy);
    void setStretch(int index, int stretch);

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

    Column();
    Column(std::initializer_list<I> ps);
};

class QTCREATOR_UTILS_EXPORT Row : public Layout
{
public:
    using Implementation = QHBoxLayout;
    using I = Building::BuilderItem<Row>;

    Row();
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
    Flow();
    Flow(std::initializer_list<I> ps);
};

class QTCREATOR_UTILS_EXPORT Stretch
{
public:
    explicit Stretch(int stretch)
        : stretch(stretch)
    {}

    int stretch;
};

class QTCREATOR_UTILS_EXPORT Space
{
public:
    explicit Space(int space)
        : space(space)
    {}

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

// Unlike Span, this class sets the span on all of its items.
class QTCREATOR_UTILS_EXPORT SpanAll
{
public:
    SpanAll(int cols, const Layout::I &item);
    SpanAll(int cols, int rows, const Layout::I &item);

    Layout::I item;
    int spanCols = 1;
    int spanRows = 1;
};

class QTCREATOR_UTILS_EXPORT Align
{
public:
    Align(Qt::Alignment alignment, const Layout::I &item);

    Layout::I item;
    Qt::Alignment alignment = {};
};

class QTCREATOR_UTILS_EXPORT GridCell
{
public:
    GridCell(const std::initializer_list<Layout::I> &items)
        : items(items)
    {}

    std::initializer_list<Layout::I> items;
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

    bool isVisible() const;
    bool isEnabled() const;

    void setVisible(bool);
    void setEnabled(bool);

    void setAutoFillBackground(bool);
    void setLayout(const Layout &layout);
    void setSize(int, int);
    void setSizePolicy(const QSizePolicy &policy);
    void setFixedSize(const QSize &);
    void setWindowTitle(const QString &);
    void setWindowFlags(Qt::WindowFlags);
    void setWidgetAttribute(Qt::WidgetAttribute, bool on);
    void setToolTip(const QString &);
    void setNoMargins(int = 0);
    void setNormalMargins(int = 0);
    void setContentsMargins(int left, int top, int right, int bottom);
    void setCursor(Qt::CursorShape shape);
    void setMinimumWidth(int);
    void setMinimumHeight(int height);
    void setMaximumWidth(int maxWidth);
    void setMaximumHeight(int maxHeight);

    void activateWindow();
    void close();
};

class QTCREATOR_UTILS_EXPORT Label : public Widget
{
public:
    using Implementation = QLabel;
    using I = Building::BuilderItem<Label>;

    Label(std::initializer_list<I> ps);
    Label(const QString &text);

    QString text() const;
    void setText(const QString &);
    void setTextFormat(Qt::TextFormat);
    void setWordWrap(bool);
    void setTextInteractionFlags(Qt::TextInteractionFlags);
    void setOpenExternalLinks(bool);
    void onLinkHovered(QObject *guard, const std::function<void(const QString &)> &);
    void onLinkActivated(QObject *guard, const std::function<void(const QString &)> &);
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
    void onTextChanged(QObject *guard, const std::function<void(QString)> &);
};

class QTCREATOR_UTILS_EXPORT PushButton : public Widget
{
public:
    using Implementation = QPushButton;
    using I = Building::BuilderItem<PushButton>;

    PushButton(std::initializer_list<I> ps);

    void setText(const QString &);
    void setIconPath(const Utils::FilePath &);
    void setIconSize(const QSize &);
    void setFlat(bool);
    void onClicked(QObject *guard, const std::function<void()> &);
};

class QTCREATOR_UTILS_EXPORT TextEdit : public Widget
{
public:
    using Implementation = QTextEdit;
    using I = Building::BuilderItem<TextEdit>;
    using Id = Implementation *;

    TextEdit(std::initializer_list<I> ps);

    QString markdown() const;
    void setText(const QString &);
    void setMarkdown(const QString &);
    void setReadOnly(bool);
};

class QTCREATOR_UTILS_EXPORT LineEdit : public Widget
{
public:
    using Implementation = Utils::FancyLineEdit;
    using I = Building::BuilderItem<LineEdit>;
    using Id = Implementation *;

    LineEdit(std::initializer_list<I> ps);

    QString text() const;
    void setText(const QString &);
    void setRightSideIconPath(const Utils::FilePath &path);
    void setPlaceHolderText(const QString &text);
    void setCompleter(QCompleter *completer);
    void onReturnPressed(QObject *guard, const std::function<void()> &);
    void onRightSideIconClicked(QObject *guard, const std::function<void()> &);
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

class QTCREATOR_UTILS_EXPORT IconDisplay : public Widget
{
public:
    using Implementation = Utils::IconDisplay;
    using I = Building::BuilderItem<IconDisplay>;

    IconDisplay(std::initializer_list<I> ps);
    void setIcon(const Utils::Icon &icon);
};

class QTCREATOR_UTILS_EXPORT ScrollArea : public Widget
{
public:
    using Implementation = QScrollArea;
    using I = Building::BuilderItem<ScrollArea>;

    ScrollArea(std::initializer_list<I> items);
    ScrollArea(const Layout &inner);

    void setLayout(const Layout &inner);
    void setFrameShape(QFrame::Shape shape);
    void setFixSizeHintBug(bool fix);
};

class QTCREATOR_UTILS_EXPORT Stack : public Widget
{
public:
    using Implementation = QStackedWidget;
    using I = Building::BuilderItem<Stack>;

    Stack()
        : Stack({})
    {}
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

class QTCREATOR_UTILS_EXPORT ToolButton : public Widget
{
public:
    using Implementation = QToolButton;
    using I = Building::BuilderItem<ToolButton>;

    ToolButton(std::initializer_list<I> items);

    void setDefaultAction(QAction *action);
};

class QTCREATOR_UTILS_EXPORT Spinner : public Widget
{
public:
    using Implementation = SpinnerSolution::SpinnerWidget;
    using I = Building::BuilderItem<Spinner>;
    using Id = Implementation *;

    Spinner(std::initializer_list<I> ps);

    void setRunning(bool running);
    void setDecorated(bool on);
};

class QTCREATOR_UTILS_EXPORT MarkdownBrowser : public Widget
{
public:
    using Implementation = Utils::MarkdownBrowser;
    using I = Building::BuilderItem<MarkdownBrowser>;

    MarkdownBrowser(std::initializer_list<I> items);

    QString toMarkdown() const;
    void setMarkdown(const QString &);
    void setBasePath(const Utils::FilePath &);
    void setEnableCodeCopyButton(bool enable);
    void setViewportMargins(int left, int top, int right, int bottom);
};

class QTCREATOR_UTILS_EXPORT CanvasWidget : public QWidget
{
public:
    using PaintFunction = std::function<void(QPainter &painter)>;

    void setPaintFunction(const PaintFunction &paintFunction);
    void paintEvent(QPaintEvent *event) override;

private:
    PaintFunction m_paintFunction;
};

class QTCREATOR_UTILS_EXPORT Canvas : public Layouting::Widget
{
public:
    using Implementation = CanvasWidget;
    using I = Building::BuilderItem<Canvas>;

    Canvas() = default;
    Canvas(std::initializer_list<I> ps);

    void setPaintFunction(const CanvasWidget::PaintFunction &paintFunction);
};

// Special

class QTCREATOR_UTILS_EXPORT Then
{
public:
    Then(std::initializer_list<Layout::I> list) : list(list) {}

    const QList<Layout::I> list;
};

class QTCREATOR_UTILS_EXPORT Else
{
public:
    Else(std::initializer_list<Layout::I> list) : list(list) {}

    const QList<Layout::I> list;
};

class QTCREATOR_UTILS_EXPORT If
{
public:
    explicit If(bool condition);

private:
    friend class Then;
    friend class Else;
    friend QTCREATOR_UTILS_EXPORT void addToLayout(Layout *layout, const If &if_);

    using Items = QList<Layout::I>;
    If(bool condition, const Items &list);

    friend QTCREATOR_UTILS_EXPORT If operator>>(const If &if_, const Then &then_);
    friend QTCREATOR_UTILS_EXPORT If operator>>(const If &if_, const Else &else_);

    const bool condition;
    const QList<Layout::I> list;
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

class BindToId
{};

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

class IdId
{};

auto id(auto p)
{
    return Building::IdAndArg{IdId{}, p};
}

template <typename Interface>
void doit(Interface *x, IdId, auto p)
{
    **p = static_cast<typename Interface::Implementation *>(x->ptr);
}

// Setter dispatchers

QTC_DEFINE_BUILDER_SETTER(alignment, setAlignment)
QTC_DEFINE_BUILDER_SETTER(childrenCollapsible, setChildrenCollapsible)
QTC_DEFINE_BUILDER_SETTER(columnStretch, setColumnStretch)
QTC_DEFINE_BUILDER_SETTER(rowStretch, setRowStretch)
QTC_DEFINE_BUILDER_SETTER(customMargins, setContentsMargins)
QTC_DEFINE_BUILDER_SETTER(fieldGrowthPolicy, setFieldGrowthPolicy)
QTC_DEFINE_BUILDER_SETTER(groupChecker, setGroupChecker)
QTC_DEFINE_BUILDER_SETTER(icon, setIcon)
QTC_DEFINE_BUILDER_SETTER(onClicked, onClicked)
QTC_DEFINE_BUILDER_SETTER(onLinkHovered, onLinkHovered)
QTC_DEFINE_BUILDER_SETTER(onLinkActivated, onLinkActivated)
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
QTC_DEFINE_BUILDER_SETTER(widgetAttribute, setWidgetAttribute);
QTC_DEFINE_BUILDER_SETTER(autoFillBackground, setAutoFillBackground);
QTC_DEFINE_BUILDER_SETTER(readOnly, setReadOnly);
QTC_DEFINE_BUILDER_SETTER(markdown, setMarkdown);
QTC_DEFINE_BUILDER_SETTER(sizePolicy, setSizePolicy);
QTC_DEFINE_BUILDER_SETTER(basePath, setBasePath);
QTC_DEFINE_BUILDER_SETTER(fixedSize, setFixedSize);
QTC_DEFINE_BUILDER_SETTER(placeholderText, setPlaceholderText);
QTC_DEFINE_BUILDER_SETTER(frameShape, setFrameShape);
QTC_DEFINE_BUILDER_SETTER(paint, setPaintFunction);
QTC_DEFINE_BUILDER_SETTER(fixSizeHintBug, setFixSizeHintBug);
QTC_DEFINE_BUILDER_SETTER(maximumWidth, setMaximumWidth)
QTC_DEFINE_BUILDER_SETTER(maximumHeight, setMaximumHeight)
QTC_DEFINE_BUILDER_SETTER(minimumWidth, setMinimumWidth)
QTC_DEFINE_BUILDER_SETTER(minimumHeight, setMinimumHeight)

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
QTCREATOR_UTILS_EXPORT void addToLayout(Layout *layout, const SpanAll &inner);
QTCREATOR_UTILS_EXPORT void addToLayout(Layout *layout, const Align &inner);
QTCREATOR_UTILS_EXPORT void addToLayout(Layout *layout, const GridCell &inner);

template<class T>
void addToLayout(Layout *layout, const QList<T> &inner)
{
    for (const auto &i : inner)
        addToLayout(layout, i);
}

template<std::ranges::view T>
void addToLayout(Layout *layout, T inner)
{
    for (const auto &i : inner)
        addToLayout(layout, i);
}

// ... can be added to anywhere later to support "user types"

QTCREATOR_UTILS_EXPORT void addToWidget(Widget *widget, const Layout &layout);

QTCREATOR_UTILS_EXPORT void addToTabWidget(TabWidget *tabWidget, const Tab &inner);

QTCREATOR_UTILS_EXPORT void addToSplitter(Splitter *splitter, QWidget *inner);
QTCREATOR_UTILS_EXPORT void addToSplitter(Splitter *splitter, const Widget &inner);
QTCREATOR_UTILS_EXPORT void addToSplitter(Splitter *splitter, const Layout &inner);

QTCREATOR_UTILS_EXPORT void addToStack(Stack *stack, QWidget *inner);
QTCREATOR_UTILS_EXPORT void addToStack(Stack *stack, const Widget &inner);
QTCREATOR_UTILS_EXPORT void addToStack(Stack *stack, const Layout &inner);

QTCREATOR_UTILS_EXPORT void addToScrollArea(ScrollArea *scrollArea, QWidget *inner);
QTCREATOR_UTILS_EXPORT void addToScrollArea(ScrollArea *scrollArea, const Widget &inner);
QTCREATOR_UTILS_EXPORT void addToScrollArea(ScrollArea *scrollArea, const Layout &inner);

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

void doit_nested(ScrollArea *outer, auto inner)
{
    addToScrollArea(outer, inner);
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
QTCREATOR_UTILS_EXPORT void tight(Layout *); // noMargin + spacing(0)

QTCREATOR_UTILS_EXPORT LayoutModifier spacing(int space);
QTCREATOR_UTILS_EXPORT LayoutModifier stretch(int index, int stretch);

// Convenience

QTCREATOR_UTILS_EXPORT QWidget *createHr(QWidget *parent = nullptr);

// Dynamic layout replacement

QTCREATOR_UTILS_EXPORT void destroyLayout(QLayout *layout);

template<typename Sender, typename Signal, typename Widget, typename LayoutCreateFunc>
void replaceLayoutOn(Sender *sender, Signal signal, Widget *widget, const LayoutCreateFunc &func)
{
    func().attachTo(widget);

    QObject::connect(sender, signal, widget, [widget, func] {
        destroyLayout(widget->layout());
        func().attachTo(widget);
    });
}

class ReplaceLayoutOnId {};

template<typename Type>
concept DerivedFromWidget = std::derived_from<Type, Layouting::Widget>;

template<typename Interface, typename Sender, typename Signal, typename LayoutCreateFunc>
    requires DerivedFromWidget<Interface>
void doit(Interface *x, ReplaceLayoutOnId, const std::tuple<Sender *, Signal, LayoutCreateFunc> &arg)
{
    replaceLayoutOn(std::get<0>(arg), std::get<1>(arg), x->emerge(), std::get<2>(arg));
}

template<class FUNC>
concept ReturnsLayout = std::derived_from<typename std::invoke_result<FUNC>::type, Layout>;

template<typename Sender, typename Signal, typename LayoutCreateFunc>
    requires ReturnsLayout<LayoutCreateFunc>
auto replaceLayoutOn(Sender *sender, Signal signal, const LayoutCreateFunc &func)
{
    return Building::IdAndArg{ReplaceLayoutOnId{}, std::make_tuple(sender, signal, func)};
}

namespace Tools {

template<typename X>
typename X::Implementation *access(const X *x)
{
    return static_cast<typename X::Implementation *>(x->ptr);
}

template<typename X>
void apply(X *x, std::initializer_list<typename X::I> ps)
{
    for (auto &&p : ps)
        p.apply(x);
}

} // Tools

} // namespace Layouting
