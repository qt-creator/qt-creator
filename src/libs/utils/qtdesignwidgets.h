// Copyright (C) 2023 Tasuku Suzuki <tasuku.suzuki@signal-slot.co.jp>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "fancylineedit.h"
#include "infolabel.h"
#include "layoutbuilder.h"
#include "stylehelper.h"

#include <QAbstractButton>
#include <QComboBox>
#include <QLabel>
#include <QProgressBar>
#include <QTabBar>

QT_FORWARD_DECLARE_CLASS(QVariantAnimation)

namespace Utils {

class QTCREATOR_UTILS_EXPORT QtcButton : public QAbstractButton
{
    Q_OBJECT // Needed for the Q_ENUM(Role) to work

public:
    enum Role {
        LargePrimary,
        LargeSecondary,
        LargeTertiary,
        LargeGhost,
        MediumPrimary,
        MediumSecondary,
        MediumTertiary,
        MediumGhost,
        SmallPrimary,
        SmallSecondary,
        SmallTertiary,
        SmallGhost,
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

    Role m_role = MediumPrimary;
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

class QTCREATOR_UTILS_EXPORT QtcLineEdit : public FancyLineEdit
{
public:
    explicit QtcLineEdit(QWidget *parent = nullptr);

    QSize minimumSizeHint() const override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

protected:
    void paintEvent(QPaintEvent *event) override;
};

class QTCREATOR_UTILS_EXPORT QtcSearchBox : public QtcLineEdit
{
public:
    explicit QtcSearchBox(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};

class QTCREATOR_UTILS_EXPORT QtcComboBox : public QComboBox
{
    Q_OBJECT // Needed for the Q_ENUM(Role) to work

public:
    enum Role {
        LargePrimary,
        SmallPrimary,
    };
    Q_ENUM(Role)

    explicit QtcComboBox(Role role, QWidget *parent = nullptr);
    explicit QtcComboBox(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    void setRole(Role role);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    Role m_role = LargePrimary;
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

class QTCREATOR_UTILS_EXPORT QtcProgressBar : public QProgressBar
{
    Q_OBJECT
public:
    explicit QtcProgressBar(QWidget *parent = nullptr);

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

    QSize sizeHint() const override;
};

class QTCREATOR_UTILS_EXPORT QtcRectangleWidget : public QWidget
{
public:
    QtcRectangleWidget(QWidget *parent = nullptr);

    int radius() const;
    void setRadius(int radius);

    QBrush fillBrush() const;
    void setFillBrush(const QBrush &brush);

    void setStrokePen(QPen pen);
    QPen strokePen() const;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_radius{StyleHelper::SpacingTokens::RadiusM};
    QBrush m_fillBrush{Qt::black};
    QPen m_strokePen{Qt::NoPen};
};

class CachedImage;

class QTCREATOR_UTILS_EXPORT QtcImage : public QWidget
{
public:
    QtcImage(QWidget *parent = nullptr);

    void setUrl(const QString &url);

    void setRadius(int radius);
    int radius() const;

    void paintEvent(QPaintEvent *event) override;

    QSize sizeForWidth(int width) const;

    QSize sizeHint() const override;

    int heightForWidth(int width) const override;

private:
    void setPixmap(const QPixmap &px);

private:
    CachedImage *m_cachedImage = nullptr;
    QPixmap m_pixmap;
    int m_radius = 0;
};

class QTCREATOR_UTILS_EXPORT QtcTabBar : public QTabBar
{
public:
    QtcTabBar(QWidget *parent = nullptr);

protected:
    bool event(QEvent *event) override;
};

class QTCREATOR_UTILS_EXPORT QtcPageIndicator : public QWidget
{
public:
    QtcPageIndicator(QWidget *parent = nullptr);

    QSize minimumSizeHint() const override;

    void setPagesCount(int count);
    int pagesCount() const;
    void setCurrentPage(int current);
    int currentPage() const;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_previousPage = -1;
    int m_currentPage = 0;
    int m_pagesCount = 5;
    QVariantAnimation *m_animation;
};

class Icon;
class IconDisplayPrivate;

class QTCREATOR_UTILS_EXPORT QtcIconDisplay : public QWidget
{
    Q_OBJECT
public:
    QtcIconDisplay(QWidget *parent = nullptr);
    ~QtcIconDisplay() override;

    void setIcon(const Utils::Icon &);

    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;

private:
    friend class IconDisplayPrivates;

    std::unique_ptr<IconDisplayPrivate> d;
};

class QTCREATOR_UTILS_EXPORT QtcBadge : public QWidget
{
    Q_OBJECT // Needed for the Q_ENUM(Role) to work

public:
    enum Role {
        NumberPrimary,
        NumberSecondary,
    };
    Q_ENUM(Role)

    QtcBadge(QWidget *parent = nullptr);

    QSize minimumSizeHint() const override;

    InfoLabelType infoType() const;
    void setInfoType(InfoLabelType infoType);

    QString text() const;
    void setText(const QString &text);

    Role role() const;
    void setRole(Role role);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    InfoLabelType m_infoType{InfoLabelType::Ok};
    Role m_role{NumberPrimary};
    QString m_text;
};

namespace QtDesignWidgets {

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

class QTCREATOR_UTILS_EXPORT ProgressBar : public Layouting::Widget
{
public:
    using Implementation = QtcProgressBar;
    using I = Building::BuilderItem<ProgressBar>;
    ProgressBar();
    ProgressBar(std::initializer_list<I> ps);
    void setMinimum(int minimum);
    void setMaximum(int maximum);
    void setRange(int minimum, int maximum);
    void setValue(int value);
    void onValueChanged(QObject *guard, const std::function<void(int)> &);
};

class QTCREATOR_UTILS_EXPORT LineEdit : public Layouting::Widget
{
public:
    using Implementation = QtcLineEdit;
    using I = Building::BuilderItem<LineEdit>;

    LineEdit();
    LineEdit(std::initializer_list<I> ps);

    void setPlaceholderText(const QString &text);
    void setText(const QString &text);
    void onTextChanged(QObject *guard, const std::function<void(QString)> &);
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

class QTCREATOR_UTILS_EXPORT Image : public Layouting::Widget
{
public:
    using Implementation = QtcImage;
    using I = Building::BuilderItem<Image>;

    Image(std::initializer_list<I> ps);

    void setUrl(const QString &url);
    void setRadius(int radius);
};

class QTCREATOR_UTILS_EXPORT IconDisplay : public Layouting::Widget
{
public:
    using Implementation = QtcIconDisplay;
    using I = Building::BuilderItem<IconDisplay>;

    IconDisplay(std::initializer_list<I> ps);
    void setIcon(const Icon &icon);
};

class QTCREATOR_UTILS_EXPORT Badge : public Layouting::Widget
{
public:
    using Implementation = QtcBadge;
    using I = Building::BuilderItem<Badge>;

    Badge();
    Badge(std::initializer_list<I> ps);

    void setText(const QString &text);
    void setInfoType(InfoLabelType infoType);
    void setRole(QtcBadge::Role role);
};

} // namespace QtDesignWidgets

inline constexpr auto role = Building::setter([](auto &x, auto &&...a) { x.setRole(a...); });
inline constexpr auto fillBrush = Building::setter(
    [](auto &x, auto &&...a) { x.setFillBrush(a...); });
inline constexpr auto strokePen = Building::setter(
    [](auto &x, auto &&...a) { x.setStrokePen(a...); });
inline constexpr auto radius = Building::setter([](auto &x, auto &&...a) { x.setRadius(a...); });
inline constexpr auto url = Building::setter([](auto &x, auto &&...a) { x.setUrl(a...); });
inline constexpr auto value = Building::setter([](auto &x, auto &&...a) { x.setValue(a...); });
inline constexpr auto minimum = Building::setter([](auto &x, auto &&...a) { x.setMinimum(a...); });
inline constexpr auto maximum = Building::setter([](auto &x, auto &&...a) { x.setMaximum(a...); });
inline constexpr auto range = Building::setter([](auto &x, auto &&...a) { x.setRange(a...); });
inline constexpr auto infoType = Building::setter(
    [](auto &x, auto &&...a) { x.setInfoType(a...); });

} // namespace Utils
