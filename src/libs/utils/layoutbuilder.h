// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QString>
#include <QVariant>
#include <QtGlobal>

#include <optional>

#if defined(UTILS_LIBRARY)
#  define QTCREATOR_UTILS_EXPORT Q_DECL_EXPORT
#else
#  define QTCREATOR_UTILS_EXPORT Q_DECL_IMPORT
#endif

QT_BEGIN_NAMESPACE
class QLayout;
class QSplitter;
class QTabWidget;
class QTextEdit;
class QWidget;
QT_END_NAMESPACE

namespace Layouting {

enum AttachType {
    WithMargins,
    WithoutMargins,
    WithFormAlignment, // Handle Grid similar to QFormLayout, i.e. use special alignment for the first column on Mac
};

class LayoutBuilder;
class LayoutItem;

// Special items

class QTCREATOR_UTILS_EXPORT Space
{
public:
    explicit Space(int space) : space(space) {}
    const int space;
};

class QTCREATOR_UTILS_EXPORT Stretch
{
public:
    explicit Stretch(int stretch = 1) : stretch(stretch) {}
    const int stretch;
};

class QTCREATOR_UTILS_EXPORT Break
{
public:
    Break() {}
};

class QTCREATOR_UTILS_EXPORT Span
{
public:
    Span(int span, const LayoutItem &item) : span(span), item(item) {}
    const int span;
    const LayoutItem &item;
};

class QTCREATOR_UTILS_EXPORT HorizontalRule
{
public:
    HorizontalRule() {}
};

// LayoutItem

class QTCREATOR_UTILS_EXPORT LayoutItem
{
public:
    enum class AlignmentType {
        DefaultAlignment,
        AlignAsFormLabel,
    };

    enum class SpecialType {
        NotSpecial,
        Space,
        Stretch,
        Break,
        HorizontalRule,
        Tab,
    };

    using Setter = std::function<void(QObject *target)>;
    using OnAdder = std::function<void(LayoutBuilder &)>;

    LayoutItem();

    template <class T> LayoutItem(const T &t)
    {
        if constexpr (std::is_same_v<QString, T>) {
            text = t;
        } else if constexpr (std::is_same_v<Space, T>) {
            specialType = LayoutItem::SpecialType::Space;
            specialValue = t.space;
        } else if constexpr (std::is_same_v<Stretch, T>) {
            specialType = LayoutItem::SpecialType::Stretch;
            specialValue = t.stretch;
        } else if constexpr (std::is_same_v<Break, T>) {
            specialType = LayoutItem::SpecialType::Break;
        } else if constexpr (std::is_same_v<Span, T>) {
            LayoutItem::operator=(t.item);
            span = t.span;
        } else if constexpr (std::is_same_v<HorizontalRule, T>) {
            specialType = SpecialType::HorizontalRule;
        } else if constexpr (std::is_base_of_v<LayoutBuilder, T>) {
            setBuilder(t);
        } else if constexpr (std::is_base_of_v<LayoutItem, T>) {
            LayoutItem::operator=(t);
        } else if constexpr (std::is_base_of_v<Setter, T>) {
            setter = t;
        } else if constexpr (std::is_base_of_v<QLayout, std::remove_pointer_t<T>>) {
            layout = t;
        } else if constexpr (std::is_base_of_v<QWidget, std::remove_pointer_t<T>>) {
            widget = t;
        } else if constexpr (std::is_pointer_v<T>) {
            onAdd = [t](LayoutBuilder &builder) { doLayout(*t, builder); };
        } else {
            onAdd = [&t](LayoutBuilder &builder) { doLayout(t, builder); };
        }
    }

    void setBuilder(const LayoutBuilder &builder);

    QLayout *layout = nullptr;
    QWidget *widget = nullptr;
    OnAdder onAdd;

    QString text; // FIXME: Use specialValue for that
    int span = 1;
    AlignmentType align = AlignmentType::DefaultAlignment;
    Setter setter;
    SpecialType specialType = SpecialType::NotSpecial;
    QVariant specialValue;
};

class QTCREATOR_UTILS_EXPORT Tab : public LayoutItem
{
public:
    Tab(const QString &tabName, const LayoutBuilder &item);
};

class QTCREATOR_UTILS_EXPORT Group : public LayoutItem
{
public:
    Group(std::initializer_list<LayoutItem> items);
};

class QTCREATOR_UTILS_EXPORT TextEdit : public LayoutItem
{
public:
    TextEdit(std::initializer_list<LayoutItem> items);
};

class QTCREATOR_UTILS_EXPORT PushButton : public LayoutItem
{
public:
    PushButton(std::initializer_list<LayoutItem> items);
};

class QTCREATOR_UTILS_EXPORT Splitter : public LayoutItem
{
public:
    Splitter(std::initializer_list<LayoutItem> items);
};

class QTCREATOR_UTILS_EXPORT TabWidget : public LayoutItem
{
public:
    TabWidget(std::initializer_list<LayoutItem> items);
};

// Singleton items.

QTCREATOR_UTILS_EXPORT extern Break br;
QTCREATOR_UTILS_EXPORT extern Stretch st;
QTCREATOR_UTILS_EXPORT extern Space empty;
QTCREATOR_UTILS_EXPORT extern HorizontalRule hr;

// "Properties"

QTCREATOR_UTILS_EXPORT LayoutItem bindTo(QTabWidget **);
QTCREATOR_UTILS_EXPORT LayoutItem bindTo(QSplitter **);

QTCREATOR_UTILS_EXPORT LayoutItem title(const QString &title);
QTCREATOR_UTILS_EXPORT LayoutItem text(const QString &text);
QTCREATOR_UTILS_EXPORT LayoutItem tooltip(const QString &toolTip);
QTCREATOR_UTILS_EXPORT LayoutItem onClicked(const std::function<void()> &func,
                                                    QObject *guard = nullptr);


// Convenience

QTCREATOR_UTILS_EXPORT QWidget *createHr(QWidget *parent = nullptr);


// LayoutBuilder

class QTCREATOR_UTILS_EXPORT LayoutBuilder
{
    Q_DISABLE_COPY(LayoutBuilder)

public:
    enum LayoutType {
        HBoxLayout,
        VBoxLayout,
        FormLayout,
        GridLayout,
        StackLayout,
    };

    using LayoutItems = QList<LayoutItem>;

    explicit LayoutBuilder(LayoutType layoutType, const LayoutItems &items = {});

    LayoutBuilder(LayoutBuilder &&) = default;
    LayoutBuilder &operator=(LayoutBuilder &&) = default;

    ~LayoutBuilder();

    LayoutBuilder &setSpacing(int spacing);

    LayoutBuilder &addItem(const LayoutItem &item);
    LayoutBuilder &addItems(const LayoutItems &items);

    LayoutBuilder &finishRow();
    LayoutBuilder &addRow(const LayoutItems &items);

    LayoutType layoutType() const { return m_layoutType; }

    void attachTo(QWidget *w, Layouting::AttachType attachType = Layouting::WithMargins) const;
    QWidget *emerge(Layouting::AttachType attachType = Layouting::WithMargins);

protected:
    friend class LayoutItem;

    explicit LayoutBuilder(); // Adds to existing layout.

    QLayout *createLayout() const;
    void doLayout(QWidget *parent, Layouting::AttachType attachType) const;

    LayoutItems m_items;
    LayoutType m_layoutType;
    std::optional<int> m_spacing;
};

class QTCREATOR_UTILS_EXPORT LayoutExtender : public LayoutBuilder
{
public:
    explicit LayoutExtender(QLayout *layout, Layouting::AttachType attachType);
    ~LayoutExtender();

private:
    QLayout *m_layout = nullptr;
    Layouting::AttachType m_attachType = {};
};

class QTCREATOR_UTILS_EXPORT Column : public LayoutBuilder
{
public:
    Column() : LayoutBuilder(VBoxLayout) {}
    Column(std::initializer_list<LayoutItem> items) : LayoutBuilder(VBoxLayout, items) {}
};

class QTCREATOR_UTILS_EXPORT Row : public LayoutBuilder
{
public:
    Row() : LayoutBuilder(HBoxLayout) {}
    Row(std::initializer_list<LayoutItem> items) : LayoutBuilder(HBoxLayout, items) {}
};

class QTCREATOR_UTILS_EXPORT Grid : public LayoutBuilder
{
public:
    Grid() : LayoutBuilder(GridLayout) {}
    Grid(std::initializer_list<LayoutItem> items) : LayoutBuilder(GridLayout, items) {}
};

class QTCREATOR_UTILS_EXPORT Form : public LayoutBuilder
{
public:
    Form() : LayoutBuilder(FormLayout) {}
    Form(std::initializer_list<LayoutItem> items) : LayoutBuilder(FormLayout, items) {}
};

class QTCREATOR_UTILS_EXPORT Stack : public LayoutBuilder
{
public:
    Stack() : LayoutBuilder(StackLayout) {}
    Stack(std::initializer_list<LayoutItem> items) : LayoutBuilder(StackLayout, items) {}
};

} // Layouting
