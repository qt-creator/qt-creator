// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QFormLayout>
#include <QList>
#include <QString>
#include <QtGlobal>

#include <optional>

#if defined(UTILS_LIBRARY)
#  define QTCREATOR_UTILS_EXPORT Q_DECL_EXPORT
#elif defined(UTILS_STATIC_LIBRARY)
#  define QTCREATOR_UTILS_EXPORT
#else
#  define QTCREATOR_UTILS_EXPORT Q_DECL_IMPORT
#endif

QT_BEGIN_NAMESPACE
class QLayout;
class QMargins;
class QObject;
class QWidget;
template <class T> T qobject_cast(QObject *object);
QT_END_NAMESPACE

namespace Layouting {

// LayoutItem

class LayoutBuilder;
class LayoutItem;
using LayoutItems = QList<LayoutItem>;

class QTCREATOR_UTILS_EXPORT LayoutItem
{
public:
    using Setter = std::function<void(QObject *target)>;

    LayoutItem();
    ~LayoutItem();

    LayoutItem(const LayoutItem &t);
    LayoutItem &operator=(const LayoutItem &t) = default;

    template <class T> LayoutItem(const T &t)
    {
        if constexpr (std::is_base_of_v<LayoutItem, T>)
            LayoutItem::operator=(t);
        else
            createItem(this, t);
    }

    void attachTo(QWidget *w) const;
    QWidget *emerge();

    void addItem(const LayoutItem &item);
    void addItems(const LayoutItems &items);
    void addRow(const LayoutItems &items);

    std::function<void(LayoutBuilder &)> onAdd;
    std::function<void(LayoutBuilder &)> onExit;
    std::function<void(QObject *target)> setter;
    LayoutItems subItems;
};

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

class QTCREATOR_UTILS_EXPORT Span
{
public:
    Span(int span, const LayoutItem &item) : span(span), item(item) {}
    const int span;
    LayoutItem item;
};

class QTCREATOR_UTILS_EXPORT Column : public LayoutItem
{
public:
    Column(std::initializer_list<LayoutItem> items);
};

class QTCREATOR_UTILS_EXPORT Row : public LayoutItem
{
public:
    Row(std::initializer_list<LayoutItem> items);
};

class QTCREATOR_UTILS_EXPORT Flow : public LayoutItem
{
public:
    Flow(std::initializer_list<LayoutItem> items);
};

class QTCREATOR_UTILS_EXPORT Grid : public LayoutItem
{
public:
    Grid() : Grid({}) {}
    Grid(std::initializer_list<LayoutItem> items);
};

class QTCREATOR_UTILS_EXPORT Form : public LayoutItem
{
public:
    Form() : Form({}) {}
    Form(std::initializer_list<LayoutItem> items);
};

class QTCREATOR_UTILS_EXPORT Widget : public LayoutItem
{
public:
    Widget(std::initializer_list<LayoutItem> items);
};

class QTCREATOR_UTILS_EXPORT Stack : public LayoutItem
{
public:
    Stack() : Stack({}) {}
    Stack(std::initializer_list<LayoutItem> items);
};

class QTCREATOR_UTILS_EXPORT Tab : public LayoutItem
{
public:
    Tab(const QString &tabName, const LayoutItem &item);
};

class QTCREATOR_UTILS_EXPORT If : public LayoutItem
{
public:
    If(bool condition, const LayoutItems &item, const LayoutItems &other = {});
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

class QTCREATOR_UTILS_EXPORT SpinBox : public LayoutItem
{
public:
    SpinBox(std::initializer_list<LayoutItem> items);
};

class QTCREATOR_UTILS_EXPORT Splitter : public LayoutItem
{
public:
    Splitter(std::initializer_list<LayoutItem> items);
};

class QTCREATOR_UTILS_EXPORT ToolBar : public LayoutItem
{
public:
    ToolBar(std::initializer_list<LayoutItem> items);
};

class QTCREATOR_UTILS_EXPORT TabWidget : public LayoutItem
{
public:
    TabWidget(std::initializer_list<LayoutItem> items);
};

class QTCREATOR_UTILS_EXPORT Application : public LayoutItem
{
public:
    Application(std::initializer_list<LayoutItem> items);

    int exec(int &argc, char *argv[]);
};


void QTCREATOR_UTILS_EXPORT createItem(LayoutItem *item, const std::function<void(QObject *target)> &t);
void QTCREATOR_UTILS_EXPORT createItem(LayoutItem *item, QWidget *t);
void QTCREATOR_UTILS_EXPORT createItem(LayoutItem *item, QLayout *t);
void QTCREATOR_UTILS_EXPORT createItem(LayoutItem *item, LayoutItem(*t)());
void QTCREATOR_UTILS_EXPORT createItem(LayoutItem *item, const QString &t);
void QTCREATOR_UTILS_EXPORT createItem(LayoutItem *item, const Span &t);
void QTCREATOR_UTILS_EXPORT createItem(LayoutItem *item, const Space &t);
void QTCREATOR_UTILS_EXPORT createItem(LayoutItem *item, const Stretch &t);


// "Singletons"

QTCREATOR_UTILS_EXPORT LayoutItem br();
QTCREATOR_UTILS_EXPORT LayoutItem st();
QTCREATOR_UTILS_EXPORT LayoutItem empty();
QTCREATOR_UTILS_EXPORT LayoutItem hr();
QTCREATOR_UTILS_EXPORT LayoutItem noMargin();
QTCREATOR_UTILS_EXPORT LayoutItem normalMargin();
QTCREATOR_UTILS_EXPORT LayoutItem customMargin(const QMargins &margin);
QTCREATOR_UTILS_EXPORT LayoutItem withFormAlignment();

// "Setters"

QTCREATOR_UTILS_EXPORT LayoutItem title(const QString &title);
QTCREATOR_UTILS_EXPORT LayoutItem text(const QString &text);
QTCREATOR_UTILS_EXPORT LayoutItem tooltip(const QString &toolTip);
QTCREATOR_UTILS_EXPORT LayoutItem resize(int, int);
QTCREATOR_UTILS_EXPORT LayoutItem columnStretch(int column, int stretch);
QTCREATOR_UTILS_EXPORT LayoutItem spacing(int);
QTCREATOR_UTILS_EXPORT LayoutItem windowTitle(const QString &windowTitle);
QTCREATOR_UTILS_EXPORT LayoutItem fieldGrowthPolicy(QFormLayout::FieldGrowthPolicy policy);


// "Getters"

class ID
{
public:
    QObject *ob = nullptr;
};

QTCREATOR_UTILS_EXPORT LayoutItem id(ID &out);

QTCREATOR_UTILS_EXPORT void setText(ID id, const QString &text);


// "Signals"

QTCREATOR_UTILS_EXPORT LayoutItem onClicked(const std::function<void()> &,
                                            QObject *guard = nullptr);
QTCREATOR_UTILS_EXPORT LayoutItem onTextChanged(const std::function<void(const QString &)> &,
                                                QObject *guard = nullptr);
QTCREATOR_UTILS_EXPORT LayoutItem onValueChanged(const std::function<void(int)> &,
                                                QObject *guard = nullptr);

QTCREATOR_UTILS_EXPORT LayoutItem onTextChanged(ID &id, QVariant(*sig)(QObject *));

// Convenience

QTCREATOR_UTILS_EXPORT QWidget *createHr(QWidget *parent = nullptr);

template <class T>
LayoutItem bindTo(T **out)
{
    return [out](QObject *target) { *out = qobject_cast<T *>(target); };
}


} // Layouting
