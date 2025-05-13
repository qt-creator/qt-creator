// Copyright (C) 2023 Tasuku Suzuki <tasuku.suzuki@signal-slot.co.jp>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "fancylineedit.h"
#include "layoutbuilder.h"
#include "stylehelper.h"
#include "theme/theme.h"

#include <QAbstractButton>
#include <QComboBox>
#include <QLabel>

namespace Utils {

class QTCREATOR_UTILS_EXPORT TextFormat
{
public:
    QColor color() const;
    QFont font(bool underlined = false) const;
    int lineHeight() const;

    const Theme::Color themeColor;
    const StyleHelper::UiElement uiElement;
    const int drawTextFlags = Qt::AlignLeft | Qt::AlignBottom | Qt::TextDontClip
                              | Qt::TextShowMnemonic;
};

QTCREATOR_UTILS_EXPORT void applyTf(QLabel *label, const TextFormat &tf, bool singleLine = true);

class QTCREATOR_UTILS_EXPORT QtcButton : public QAbstractButton
{
    Q_OBJECT // Needed for the Q_ENUM(Role) to work

public:
    enum Role {
        LargePrimary,
        LargeSecondary,
        LargeTertiary,
        SmallPrimary,
        SmallSecondary,
        SmallTertiary,
        SmallList,
        SmallLink,
        Tag,
    };
    Q_ENUM(Role)

    explicit QtcButton(const QString &text, Role role, QWidget *parent = nullptr);
    QSize minimumSizeHint() const override;
    void setPixmap(const QPixmap &newPixmap);
    void setRole(Role role);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void updateMargins();

    Role m_role = LargePrimary;
    QPixmap m_pixmap;
};

class QTCREATOR_UTILS_EXPORT QtcLabel : public QLabel
{
    Q_OBJECT // Needed for the Q_ENUM(Role) to work
public:
    enum Role {
        Primary,
        Secondary,
    };
    Q_ENUM(Role)

    explicit QtcLabel(const QString &text, Role role, QWidget *parent = nullptr);

    void setRole(Role role);
};

class QTCREATOR_UTILS_EXPORT QtcSearchBox : public Utils::FancyLineEdit
{
public:
    explicit QtcSearchBox(QWidget *parent = nullptr);
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
};

class QTCREATOR_UTILS_EXPORT QtcComboBox : public QComboBox
{
public:
    explicit QtcComboBox(QWidget *parent = nullptr);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
};

class QTCREATOR_UTILS_EXPORT QtcSwitch : public QAbstractButton
{
public:
    explicit QtcSwitch(const QString &text, QWidget *parent = nullptr);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
};

class QTCREATOR_UTILS_EXPORT QtcIconButton : public QAbstractButton
{
public:
    QtcIconButton(QWidget *parent = nullptr);

    void paintEvent(QPaintEvent *e) override;
    void enterEvent(QEnterEvent *e) override;
    void leaveEvent(QEvent *e) override;

    QSize sizeHint() const override;

private:
    bool m_containsMouse{false};
};

class QTCREATOR_UTILS_EXPORT QtcRectangleWidget : public QWidget
{
public:
    QtcRectangleWidget(QWidget *parent = nullptr);

    QSize sizeHint() const override;

    int radius() const;
    void setRadius(int radius);

    QBrush fillBrush() const;
    void setFillBrush(const QBrush &brush);

    void setStrokePen(QPen pen);
    QPen strokePen() const;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_radius{10};
    QBrush m_fillBrush{Qt::black};
    QPen m_strokePen{Qt::NoPen};
};

namespace QtcWidgets {

class QTCREATOR_UTILS_EXPORT Label : public Layouting::Widget
{
public:
    using Implementation = QtcLabel;
    using I = Building::BuilderItem<Label>;
    Label();
    Label(std::initializer_list<I> ps);
    void setText(const QString &text);
    void setRole(QtcLabel::Role role);
};

class QTCREATOR_UTILS_EXPORT Button : public Layouting::Widget
{
public:
    using Implementation = QtcButton;
    using I = Building::BuilderItem<Button>;
    Button();
    Button(std::initializer_list<I> ps);
    void setText(const QString &text);
    void setIcon(const Utils::Icon &icon);
    void setRole(QtcButton::Role role);
    void onClicked(QObject *guard, const std::function<void()> &);
};

class QTCREATOR_UTILS_EXPORT IconButton : public Layouting::Widget
{
public:
    using Implementation = QtcIconButton;
    using I = Building::BuilderItem<IconButton>;
    IconButton();
    IconButton(std::initializer_list<I> ps);
    void setIcon(const Utils::Icon &icon);
    void onClicked(QObject *guard, const std::function<void()> &);
};

class QTCREATOR_UTILS_EXPORT Switch : public Layouting::Widget
{
public:
    using Implementation = QtcSwitch;
    using I = Building::BuilderItem<Switch>;
    Switch();
    Switch(std::initializer_list<I> ps);
    void setText(const QString &text);
    void setChecked(bool checked);
    void onClicked(QObject *guard, const std::function<void()> &);
};

class QTCREATOR_UTILS_EXPORT SearchBox : public Layouting::Widget
{
public:
    using Implementation = QtcSearchBox;
    using I = Building::BuilderItem<SearchBox>;

    SearchBox();
    SearchBox(std::initializer_list<I> ps);

    void setPlaceholderText(const QString &text);
    void setText(const QString &text);
    void onTextChanged(QObject *guard, const std::function<void(QString)> &);
};

class QTCREATOR_UTILS_EXPORT Rectangle : public Layouting::Widget
{
public:
    using Implementation = QtcRectangleWidget;
    using I = Building::BuilderItem<Rectangle>;

    Rectangle(std::initializer_list<I> ps);

    void setRadius(int radius);
    void setFillBrush(const QBrush &brush);
    void setStrokePen(const QPen &pen);
};

} // namespace QtcWidgets

QTC_DEFINE_BUILDER_SETTER(role, setRole);
QTC_DEFINE_BUILDER_SETTER(fillBrush, setFillBrush);
QTC_DEFINE_BUILDER_SETTER(strokePen, setStrokePen);
QTC_DEFINE_BUILDER_SETTER(radius, setRadius);

} // namespace Utils
