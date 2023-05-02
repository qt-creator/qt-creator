// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QString>
#include <QtGlobal>

#include <optional>

#if defined(UTILS_LIBRARY)
#  define QTCREATOR_UTILS_EXPORT Q_DECL_EXPORT
#else
#  define QTCREATOR_UTILS_EXPORT Q_DECL_IMPORT
#endif

QT_BEGIN_NAMESPACE
class QLayout;
class QObject;
class QWidget;
template <class T> T qobject_cast(QObject *object);
QT_END_NAMESPACE

namespace Layouting {

class LayoutBuilder;
class LayoutItem;
class Span;

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


// LayoutItem

using LayoutItems = QList<LayoutItem>;

void QTCREATOR_UTILS_EXPORT createItem(LayoutItem *item, const std::function<void(QObject *target)> &t);
void QTCREATOR_UTILS_EXPORT createItem(LayoutItem *item, QWidget *t);
void QTCREATOR_UTILS_EXPORT createItem(LayoutItem *item, QLayout *t);
void QTCREATOR_UTILS_EXPORT createItem(LayoutItem *item, LayoutItem(*t)());
void QTCREATOR_UTILS_EXPORT createItem(LayoutItem *item, const QString &t);
void QTCREATOR_UTILS_EXPORT createItem(LayoutItem *item, const Span &t);
void QTCREATOR_UTILS_EXPORT createItem(LayoutItem *item, const Space &t);
void QTCREATOR_UTILS_EXPORT createItem(LayoutItem *item, const Stretch &t);

class QTCREATOR_UTILS_EXPORT LayoutItem
{
public:
    enum class AlignmentType {
        DefaultAlignment,
        AlignAsFormLabel,
    };

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
    void finishRow();

    std::function<void(LayoutBuilder &)> onAdd;
    std::function<void(LayoutBuilder &)> onExit;
    std::function<void(QObject *target)> setter;
    LayoutItems subItems;
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

class QTCREATOR_UTILS_EXPORT Application : public LayoutItem
{
public:
    Application(std::initializer_list<LayoutItem> items);

    int exec(int &argc, char *argv[]);
};

// "Singletons"

QTCREATOR_UTILS_EXPORT LayoutItem br();
QTCREATOR_UTILS_EXPORT LayoutItem st();
QTCREATOR_UTILS_EXPORT LayoutItem empty();
QTCREATOR_UTILS_EXPORT LayoutItem hr();
QTCREATOR_UTILS_EXPORT LayoutItem noMargin();
QTCREATOR_UTILS_EXPORT LayoutItem normalMargin();
QTCREATOR_UTILS_EXPORT LayoutItem withFormAlignment();

// "Properties"

QTCREATOR_UTILS_EXPORT LayoutItem title(const QString &title);
QTCREATOR_UTILS_EXPORT LayoutItem text(const QString &text);
QTCREATOR_UTILS_EXPORT LayoutItem tooltip(const QString &toolTip);
QTCREATOR_UTILS_EXPORT LayoutItem resize(int, int);
QTCREATOR_UTILS_EXPORT LayoutItem spacing(int);
QTCREATOR_UTILS_EXPORT LayoutItem windowTitle(const QString &windowTitle);
QTCREATOR_UTILS_EXPORT LayoutItem onClicked(const std::function<void()> &,
                                            QObject *guard = nullptr);

// Convenience

QTCREATOR_UTILS_EXPORT QWidget *createHr(QWidget *parent = nullptr);

template <class T>
LayoutItem bindTo(T **out)
{
    return [out](QObject *target) { *out = qobject_cast<T *>(target); };
}

// LayoutBuilder

class QTCREATOR_UTILS_EXPORT LayoutBuilder
{
    Q_DISABLE_COPY_MOVE(LayoutBuilder)

public:
    LayoutBuilder();
    ~LayoutBuilder();

    void addItem(const LayoutItem &item);
    void addItems(const LayoutItems &items);
    void addRow(const LayoutItems &items);

    bool isForm() const;

    struct Slice;
    QList<Slice> stack;
};

class QTCREATOR_UTILS_EXPORT LayoutExtender : public LayoutBuilder
{
public:
    explicit LayoutExtender(QLayout *layout);
    ~LayoutExtender();
};

} // Layouting
