// Copyright (C) 2023 Tasuku Suzuki <tasuku.suzuki@signal-slot.co.jp>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtcwidgets.h"

#include "hostosinfo.h"
#include "icon.h"

#include <QEvent>
#include <QGuiApplication>
#include <QPaintEvent>
#include <QPainter>
#include <QWidget>

namespace Utils {

using namespace StyleHelper::SpacingTokens;

const qreal disabledIconOpacity = 0.3;

QColor TextFormat::color() const
{
    return Utils::creatorColor(themeColor);
}

QFont TextFormat::font(bool underlined) const
{
    QFont result = Utils::StyleHelper::uiFont(uiElement);
    result.setUnderline(underlined);
    return result;
}

int TextFormat::lineHeight() const
{
    return Utils::StyleHelper::uiFontLineHeight(uiElement);
}

void applyTf(QLabel *label, const TextFormat &tf, bool singleLine)
{
    if (singleLine)
        label->setFixedHeight(tf.lineHeight());
    label->setFont(tf.font());
    label->setAlignment(Qt::Alignment(tf.drawTextFlags));
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);

    QPalette pal = label->palette();
    pal.setColor(QPalette::WindowText, tf.color());
    label->setPalette(pal);
}

enum WidgetState {
    WidgetStateDefault,
    WidgetStateChecked,
    WidgetStateHovered,
};

static const TextFormat &buttonTF(QtcButton::Role role, WidgetState state)
{
    static const TextFormat largePrimaryTF
        {Theme::Token_Basic_White, StyleHelper::UiElement::UiElementButtonMedium,
         Qt::AlignCenter | Qt::TextDontClip | Qt::TextShowMnemonic};
    static const TextFormat largeSecondaryTF
        {Theme::Token_Text_Default, largePrimaryTF.uiElement, largePrimaryTF.drawTextFlags};
    static const TextFormat smallPrimaryTF
        {largePrimaryTF.themeColor, StyleHelper::UiElement::UiElementButtonSmall,
         largePrimaryTF.drawTextFlags};
    static const TextFormat smallSecondaryTF
        {largeSecondaryTF.themeColor, smallPrimaryTF.uiElement, smallPrimaryTF.drawTextFlags};
    static const TextFormat smallListDefaultTF
        {Theme::Token_Text_Default, StyleHelper::UiElement::UiElementIconStandard,
         Qt::AlignLeft | Qt::AlignVCenter | Qt::TextDontClip | Qt::TextShowMnemonic};
    static const TextFormat smallListCheckedTF = smallListDefaultTF;
    static const TextFormat smallLinkDefaultTF
        {Theme::Token_Text_Default, StyleHelper::UiElement::UiElementIconStandard,
         smallListDefaultTF.drawTextFlags};
    static const TextFormat smallLinkHoveredTF
        {Theme::Token_Text_Accent, smallLinkDefaultTF.uiElement,
         smallLinkDefaultTF.drawTextFlags};
    static const TextFormat tagDefaultTF
        {Theme::Token_Text_Muted, StyleHelper::UiElement::UiElementLabelMedium};
    static const TextFormat tagHoverTF
        {Theme::Token_Text_Default, tagDefaultTF.uiElement};

    switch (role) {
    case QtcButton::LargePrimary: return largePrimaryTF;
    case QtcButton::LargeSecondary:
    case QtcButton::LargeTertiary:
        return largeSecondaryTF;
    case QtcButton::SmallPrimary: return smallPrimaryTF;
    case QtcButton::SmallSecondary:
    case QtcButton::SmallTertiary:
        return smallSecondaryTF;
    case QtcButton::SmallList: return (state == WidgetStateDefault) ? smallListDefaultTF
                                             : smallListCheckedTF;
    case QtcButton::SmallLink: return (state == WidgetStateDefault) ? smallLinkDefaultTF
                                             : smallLinkHoveredTF;
    case QtcButton::Tag: return (state == WidgetStateDefault) ? tagDefaultTF
                                             : tagHoverTF;
    }
    return largePrimaryTF;
}

QtcButton::QtcButton(const QString &text, Role role, QWidget *parent)
    : QAbstractButton(parent)
    , m_role(role)
{
    setText(text);
    setAttribute(Qt::WA_Hover);

    updateMargins();
    if (m_role == SmallList)
        setCheckable(true);
    else if (m_role == SmallLink)
        setCursor(Qt::PointingHandCursor);
}

QSize QtcButton::minimumSizeHint() const
{
    int maxTextWidth = 0;
    for (WidgetState state : {WidgetStateDefault, WidgetStateChecked, WidgetStateHovered} ) {
        const TextFormat &tf = buttonTF(m_role, state);
        const QFontMetrics fm(tf.font());
        const QSize textS = fm.size(Qt::TextShowMnemonic, text());
        maxTextWidth = qMax(maxTextWidth, textS.width());
    }
    const TextFormat &tf = buttonTF(m_role, WidgetStateDefault);
    const QMargins margins = contentsMargins();
    return {margins.left() + maxTextWidth + margins.right(),
            margins.top() + tf.lineHeight() + margins.bottom()};
}

void QtcButton::paintEvent(QPaintEvent *event)
{
    // Without pixmap
    // +----------------+----------------+----------------+
    // |                |(VPadding[S|XS])|                |
    // |                +----------------+----------------+
    // |(HPadding[S|XS])|     <label>    |(HPadding[S|XS])|
    // |                +----------------+----------------+
    // |                |(VPadding[S|XS])|                |
    // +----------------+---------------------------------+
    //
    // With pixmap
    // +-------+--------+-------------------------+---------------------------------+
    // |       |        |                         |(VPadding[S|XS])|                |
    // |       |        |                         +----------------+                |
    // |(HGapS)|<pixmap>|([ExPaddingGapM|HGapXxs])|     <label>    |(HPadding[S|XS])|
    // |       |        |                         +----------------+                |
    // |       |        |                         |(VPadding[S|XS])|                |
    // +-------+--------+-------------------------+----------------+----------------+
    //
    // SmallLink with pixmap
    // +--------+---------------+---------------------------------+
    // |        |               |(VPadding[S|XS])|                |
    // |        |               +----------------+                |
    // |<pixmap>|(ExPaddingGapM)|     <label>    |(HPadding[S|XS])|
    // |        |               +----------------+                |
    // |        |               |(VPadding[S|XS])|                |
    // +--------+---------------+----------------+----------------+

    const bool hovered = underMouse();
    const WidgetState state = isChecked() ? WidgetStateChecked : hovered ? WidgetStateHovered
                                                                         : WidgetStateDefault;
    const TextFormat &tf = buttonTF(m_role, state);
    const QMargins margins = contentsMargins();
    const QRect bgR = rect();

    QPainter p(this);

    const qreal brRectRounding = 3.75;
    switch (m_role) {
    case LargePrimary:
    case SmallPrimary: {
        const Theme::Color color = isEnabled() ? (isDown()
                                                  ? Theme::Token_Accent_Subtle
                                                  : (hovered ? Theme::Token_Accent_Muted
                                                             : Theme::Token_Accent_Default))
                                               : Theme::Token_Foreground_Subtle;
        const QBrush fill(creatorColor(color));
        StyleHelper::drawCardBg(&p, bgR, fill, QPen(Qt::NoPen), brRectRounding);
        break;
    }
    case LargeSecondary:
    case SmallSecondary: {
        const Theme::Color color = isEnabled() ? Theme::Token_Stroke_Strong
                                               : Theme::Token_Stroke_Subtle;
        const qreal width = hovered ? 2.0 : 1.0;
        const QPen outline(creatorColor(color), width);
        StyleHelper::drawCardBg(&p, bgR, QBrush(Qt::NoBrush), outline, brRectRounding);
        break;
    }
    case LargeTertiary:
    case SmallTertiary: {
        const Theme::Color border = isDown() ? Theme::Token_Stroke_Muted
                                             : Theme::Token_Stroke_Subtle;
        const Theme::Color bg = isEnabled() ? (isDown() ? Theme::Token_Foreground_Default
                                                        : (hovered ? Theme::Token_Foreground_Muted
                                                                   : Theme::Token_Foreground_Subtle))
                                            : Theme::Token_Foreground_Subtle;
        StyleHelper::drawCardBg(&p, bgR, creatorColor(bg), creatorColor(border), brRectRounding);
        break;
    }
    case SmallList: {
        if (isChecked() || hovered) {
            const QBrush fill(creatorColor(isChecked() ? Theme::Token_Foreground_Muted
                                                       : Theme::Token_Foreground_Subtle));
            StyleHelper::drawCardBg(&p, bgR, fill, QPen(Qt::NoPen), brRectRounding);
        }
        break;
    }
    case SmallLink:
        break;
    case Tag: {
        const QBrush fill(hovered ? creatorColor(Theme::Token_Foreground_Subtle)
                                  : QBrush(Qt::NoBrush));
        const QPen outline(hovered ? QPen(Qt::NoPen) : creatorColor(Theme::Token_Stroke_Subtle));
        StyleHelper::drawCardBg(&p, bgR, fill, outline, brRectRounding);
        break;
    }
    }

    if (!m_pixmap.isNull()) {
        const QSizeF pixmapS = m_pixmap.deviceIndependentSize();
        const int pixmapX = m_role == SmallLink ? 0 : HGapS;
        const int pixmapY = (bgR.height() - pixmapS.height()) / 2;
        p.drawPixmap(pixmapX, pixmapY, m_pixmap);
    }

    const int availableLabelWidth = event->rect().width() - margins.left() - margins.right();
    const QFont font = tf.font();
    const QFontMetrics fm(font);
    const QString elidedLabelText = fm.elidedText(text(), Qt::ElideRight, availableLabelWidth);
    const QRect labelR(margins.left(), margins.top(), availableLabelWidth, tf.lineHeight());
    p.setFont(font);
    const QColor textColor = isEnabled() ? tf.color() : creatorColor(Theme::Token_Text_Subtle);
    p.setPen(textColor);
    p.drawText(labelR, tf.drawTextFlags, elidedLabelText);
}

void QtcButton::setPixmap(const QPixmap &pixmap)
{
    m_pixmap = pixmap;
    updateMargins();
}

void QtcButton::setRole(Role role)
{
    m_role = role;
    updateMargins();
}

void QtcButton::updateMargins()
{
    if (m_role == Tag) {
        setContentsMargins(HPaddingXs, VPaddingXxs, HPaddingXs, VPaddingXxs);
        return;
    }
    const bool tokenSizeS = m_role == LargePrimary || m_role == LargeSecondary
                            || m_role == SmallList || m_role == SmallLink;
    const int hPaddingR = tokenSizeS ? HPaddingS : HPaddingXs;
    const int hPaddingL = m_pixmap.isNull() ? hPaddingR
                                            : (m_role == SmallLink ? 0 : HGapS)
                                                  + int(m_pixmap.deviceIndependentSize().width())
                                                  + (tokenSizeS ? ExPaddingGapM : HGapXxs);
    const int vPadding = tokenSizeS ? VPaddingS : VPaddingXs;
    setContentsMargins(hPaddingL, vPadding, hPaddingR, vPadding);
}

QtcLabel::QtcLabel(const QString &text, Role role, QWidget *parent)
    : QLabel(text, parent)
{
    setRole(role);
}

void QtcLabel::setRole(Role role)
{
    static const TextFormat primaryTF{Theme::Token_Text_Muted, StyleHelper::UiElement::UiElementH3};
    static const TextFormat
        secondaryTF{primaryTF.themeColor, StyleHelper::UiElement::UiElementH6Capital};

    const TextFormat &tF = role == Primary ? primaryTF : secondaryTF;
    const int vPadding = role == Primary ? ExPaddingGapM : VPaddingS;

    setFixedHeight(vPadding + tF.lineHeight() + vPadding);
    setFont(tF.font());
    QPalette pal = palette();
    pal.setColor(QPalette::Active, QPalette::WindowText, tF.color());
    pal.setColor(QPalette::Disabled, QPalette::WindowText, creatorColor(Theme::Token_Text_Subtle));
    setPalette(pal);

    update();
}

constexpr TextFormat searchBoxTextTF
    {Theme::Token_Text_Default, StyleHelper::UiElement::UiElementBody2};
constexpr TextFormat searchBoxPlaceholderTF
    {Theme::Token_Text_Muted, searchBoxTextTF.uiElement};

static const QPixmap &searchBoxIcon()
{
    static const QPixmap icon = Icon({{FilePath::fromString(":/core/images/search.png"),
                                       Theme::Token_Text_Muted}}, Icon::Tint).pixmap();
    return icon;
}

QtcSearchBox::QtcSearchBox(QWidget *parent)
    : Utils::FancyLineEdit(parent)
{
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setAutoFillBackground(false);
    setFont(searchBoxTextTF.font());
    setFrame(false);
    setMouseTracking(true);

    QPalette pal = palette();
    pal.setColor(QPalette::Base, Qt::transparent);
    pal.setColor(QPalette::Active, QPalette::PlaceholderText, searchBoxPlaceholderTF.color());
    pal.setColor(QPalette::Disabled, QPalette::PlaceholderText,
                 creatorColor(Theme::Token_Text_Subtle));
    pal.setColor(QPalette::Text, searchBoxTextTF.color());
    setPalette(pal);

    setContentsMargins({HPaddingXs, ExPaddingGapM, 0, ExPaddingGapM});
    setFixedHeight(ExPaddingGapM + searchBoxTextTF.lineHeight() + ExPaddingGapM);
    setFiltering(true);
}

QSize QtcSearchBox::minimumSizeHint() const
{
    const QFontMetrics fm(searchBoxTextTF.font());
    const QSize textS = fm.size(Qt::TextSingleLine, text());
    const QMargins margins = contentsMargins();
    return {margins.left() + textS.width() + margins.right(),
            margins.top() + searchBoxTextTF.lineHeight() + margins.bottom()};
}

void QtcSearchBox::enterEvent(QEnterEvent *event)
{
    QLineEdit::enterEvent(event);
    update();
}

void QtcSearchBox::leaveEvent(QEvent *event)
{
    QLineEdit::leaveEvent(event);
    update();
}

static void paintCommonBackground(QPainter *p, const QRectF &rect, const QWidget *w)
{
    const QBrush fill(creatorColor(Theme::Token_Background_Muted));
    const Theme::Color c =
        w->isEnabled() ? (w->hasFocus() ? Theme::Token_Stroke_Strong
                                        : (w->underMouse() ? Theme::Token_Stroke_Muted
                                                           : Theme::Token_Stroke_Subtle))
                       : Theme::Token_Foreground_Subtle;
    const QPen pen(creatorColor(c));
    StyleHelper::drawCardBg(p, rect, fill, pen);
}

void QtcSearchBox::paintEvent(QPaintEvent *event)
{
    // +------------+---------------+------------+------+------------+
    // |            |(ExPaddingGapM)|            |      |            |
    // |            +---------------+            |      |            |
    // |(HPaddingXs)|  <lineEdit>   |(HPaddingXs)|<icon>|(HPaddingXs)|
    // |            +---------------+            |      |            |
    // |            |(ExPaddingGapM)|            |      |            |
    // +------------+---------------+------------+------+------------+

    QPainter p(this);

    paintCommonBackground(&p, rect(), this);
    if (text().isEmpty()) {
        const QPixmap icon = searchBoxIcon();
        const QSize iconS = icon.deviceIndependentSize().toSize();
        const QPoint iconPos(width() - HPaddingXs - iconS.width(), (height() - iconS.height()) / 2);
        if (!isEnabled())
            p.setOpacity(disabledIconOpacity);
        p.drawPixmap(iconPos, icon);
    }

    QLineEdit::paintEvent(event);
}

constexpr TextFormat ComboBoxTf
    {Theme::Token_Text_Muted, StyleHelper::UiElementIconActive,
     Qt::AlignLeft | Qt::AlignVCenter | Qt::TextDontClip};

static const QPixmap &comboBoxIcon()
{
    static const QPixmap icon = Icon({{FilePath::fromString(":/core/images/expandarrow.png"),
                                       ComboBoxTf.themeColor}}, Icon::Tint).pixmap();
    return icon;
}

QtcComboBox::QtcComboBox(QWidget *parent)
    : QComboBox(parent)
{
    setFont(ComboBoxTf.font());
    setMouseTracking(true);

    const QSize iconSize = comboBoxIcon().deviceIndependentSize().toSize();
    setContentsMargins({HPaddingXs, VPaddingXs,
                        HGapXxs + iconSize.width() + HPaddingXs, VPaddingXs});
}

QSize QtcComboBox::sizeHint() const
{
    const QSize parentS = QComboBox::sizeHint();
    const QMargins margins = contentsMargins();
    return {margins.left() + parentS.width() + margins.right(),
            margins.top() + ComboBoxTf.lineHeight() + margins.bottom()};
}

void QtcComboBox::enterEvent(QEnterEvent *event)
{
    QComboBox::enterEvent(event);
    update();
}

void QtcComboBox::leaveEvent(QEvent *event)
{
    QComboBox::leaveEvent(event);
    update();
}

void QtcComboBox::paintEvent(QPaintEvent *)
{
    // +------------+-------------+---------+-------+------------+
    // |            | (VPaddingXs)|         |       |            |
    // |            +-------------+         |       |            |
    // |(HPaddingXs)|<currentItem>|(HGapXxs)|<arrow>|(HPaddingXs)|
    // |            +-------------+         |       |            |
    // |            | (VPaddingXs)|         |       |            |
    // +------------+-------------+---------+-------+------------+

    QPainter p(this);
    paintCommonBackground(&p, rect(), this);

    const QMargins margins = contentsMargins();
    const QRect textR(margins.left(), margins.top(),
                      width() - margins.right(), ComboBoxTf.lineHeight());
    p.setFont(ComboBoxTf.font());
    const QColor color = isEnabled() ? ComboBoxTf.color() : creatorColor(Theme::Token_Text_Subtle);
    p.setPen(color);
    p.drawText(textR, ComboBoxTf.drawTextFlags, currentText());

    const QPixmap icon = comboBoxIcon();
    const QSize iconS = icon.deviceIndependentSize().toSize();
    const QPoint iconPos(width() - HPaddingXs - iconS.width(), (height() - iconS.height()) / 2);
    if (!isEnabled())
        p.setOpacity(disabledIconOpacity);
    p.drawPixmap(iconPos, icon);
}

constexpr TextFormat SwitchLabelTf
    {Theme::Token_Text_Default, StyleHelper::UiElementLabelMedium};
constexpr QSize switchTrackS(32, 16);

QtcSwitch::QtcSwitch(const QString &text, QWidget *parent)
    : QAbstractButton(parent)
{
    setText(text);
    setCheckable(true);
    setAttribute(Qt::WA_Hover);
    setLayoutDirection(Qt::RightToLeft); // Switch right, label left
}

QSize QtcSwitch::sizeHint() const
{
    const QFontMetrics fm(SwitchLabelTf.font());
    const int textWidth = fm.size(Qt::TextShowMnemonic, text()).width();
    const int width = switchTrackS.width() + HGapS + textWidth;
    return {width, ExPaddingGapM + SwitchLabelTf.lineHeight() + ExPaddingGapM};
}

QSize QtcSwitch::minimumSizeHint() const
{
    return switchTrackS;
}

void QtcSwitch::paintEvent([[maybe_unused]] QPaintEvent *event)
{
    const bool ltr = layoutDirection() == Qt::LeftToRight;
    const int trackX = ltr ? 0 : width() - switchTrackS.width();
    const int trackY = (height() - switchTrackS.height()) / 2;
    const QRect trackR(QPoint(trackX, trackY), switchTrackS);
    const int trackRounding = trackR.height() / 2;
    const bool checkedEnabled = isChecked() && isEnabled();
    QPainter p(this);

    { // track
        const bool hovered = underMouse();
        const QBrush fill = creatorColor(checkedEnabled ? (hovered ? Theme::Token_Accent_Subtle
                                                                   : Theme::Token_Accent_Default)
                                                        : Theme::Token_Foreground_Subtle);
        const QPen outline = checkedEnabled ? QPen(Qt::NoPen)
                                            : creatorColor(hovered ? Theme::Token_Stroke_Muted
                                                                   : Theme::Token_Stroke_Subtle);
        StyleHelper::drawCardBg(&p, trackR, fill, outline, trackRounding);
    }
    { // track label
        const QColor color = creatorColor(isEnabled() ? (isChecked() ? Theme::Token_Basic_White
                                                                     : Theme::Token_Text_Muted)
                                                      : Theme::Token_Text_Subtle);
        const int labelS = 6;
        const int labelY = (height() - labelS) / 2;
        if (isChecked()) {
            const QRect onLabelR(trackX + 8, labelY, 1, labelS);
            p.fillRect(onLabelR, color);
        } else {
            const QRect offLabelR(trackX + switchTrackS.width() - trackRounding - labelS / 2 - 1,
                                  labelY, labelS, labelS);
            StyleHelper::drawCardBg(&p, offLabelR, Qt::NoBrush, color, labelS / 2);
        }
    }
    { // knob
        const int thumbPadding = checkedEnabled ? 3 : 2;
        const int thumbH = switchTrackS.height() - thumbPadding - thumbPadding;
        const int thumbW = isDown() ? (checkedEnabled ? 17 : 19)
                                    : thumbH;
        const int thumbRounding = thumbH / 2;
        const int thumbX = trackX + (isChecked() ? switchTrackS.width() - thumbW - thumbPadding
                                                 : thumbPadding);
        const QRect thumbR(thumbX, trackY + thumbPadding, thumbW, thumbH);
        const QBrush fill = creatorColor(isEnabled() ? Theme::Token_Basic_White
                                                     : Theme::Token_Foreground_Default);
        const QPen outline = checkedEnabled ? QPen(Qt::NoPen)
                                            : creatorColor(Theme::Token_Stroke_Subtle);
        StyleHelper::drawCardBg(&p, thumbR, fill, outline, thumbRounding);
    }
    { // switch text label
        const int switchAndGapWidth = switchTrackS.width() + HGapS;
        const QRect textR(ltr ? switchAndGapWidth : 0, 0, width() - switchAndGapWidth,
                          trackY + switchTrackS.height());
        p.setFont(SwitchLabelTf.font());
        p.setPen(isEnabled() ? SwitchLabelTf.color() : creatorColor(Theme::Token_Text_Subtle));
        const QString elidedLabel =
            p.fontMetrics().elidedText(text(), Qt::ElideRight, textR.width());
        p.drawText(textR, SwitchLabelTf.drawTextFlags, elidedLabel);
    }
}

QtcIconButton::QtcIconButton(QWidget *parent)
    : QAbstractButton(parent)
{
    setAttribute(Qt::WA_Hover);
}

void QtcIconButton::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)

    QPainter p(this);
    QRect r(QPoint(), size());

    if (m_containsMouse && isEnabled()) {
        QColor c = creatorColor(Theme::TextColorDisabled);
        c.setAlphaF(c.alphaF() * .5);
        StyleHelper::drawPanelBgRect(&p, r, c);
    }

    icon().paint(&p, r, Qt::AlignCenter);
}

void QtcIconButton::enterEvent(QEnterEvent *e)
{
    m_containsMouse = true;
    e->accept();
    update();
}

void QtcIconButton::leaveEvent(QEvent *e)
{
    m_containsMouse = false;
    e->accept();
    update();
}

QSize QtcIconButton::sizeHint() const
{
    QSize s = icon().actualSize(QSize(32, 16)) + QSize(8, 8);

    if (StyleHelper::toolbarStyle() == StyleHelper::ToolbarStyle::Relaxed)
        s += QSize(5, 5);

    return s;
}

namespace QtcWidgets {

Button::Button()
{
    ptr = new Implementation({}, QtcButton::LargePrimary, nullptr);
}

Button::Button(std::initializer_list<I> ps)
{
    ptr = new Implementation({}, QtcButton::LargePrimary, nullptr);
    Layouting::Tools::apply(this, ps);
}

void Button::setText(const QString &text)
{
    Layouting::Tools::access(this)->setText(text);
}

void Button::setIcon(const Icon &icon)
{
    Layouting::Tools::access(this)->setPixmap(icon.pixmap());
}

void Button::setRole(QtcButton::Role role)
{
    Layouting::Tools::access(this)->setRole(role);
}

void Button::onClicked(QObject *guard, const std::function<void()> &func)
{
    QObject::connect(Layouting::Tools::access(this), &QtcButton::clicked, guard, func);
}

IconButton::IconButton()
{
    ptr = new Implementation(nullptr);
}

IconButton::IconButton(std::initializer_list<I> ps)
{
    ptr = new Implementation(nullptr);
    Layouting::Tools::apply(this, ps);
}

void IconButton::setIcon(const Icon &icon)
{
    Layouting::Tools::access(this)->setIcon(icon.icon());
}

void IconButton::onClicked(QObject *guard, const std::function<void()> &func)
{
    QObject::connect(Layouting::Tools::access(this), &QtcIconButton::clicked, guard, func);
}

Switch::Switch()
{
    ptr = new Implementation({});
}

Switch::Switch(std::initializer_list<I> ps)
{
    ptr = new Implementation({});
    Layouting::Tools::apply(this, ps);
}

void Switch::setText(const QString &text)
{
    Layouting::Tools::access(this)->setText(text);
}

void Switch::setChecked(bool checked)
{
    Layouting::Tools::access(this)->setChecked(checked);
}

void Switch::onClicked(QObject *guard, const std::function<void()> &func)
{
    QObject::connect(Layouting::Tools::access(this), &QtcSwitch::clicked, guard, func);
}

Label::Label()
{
    ptr = new Implementation("", QtcLabel::Primary, nullptr);
}

Label::Label(std::initializer_list<I> ps)
{
    ptr = new Implementation("", QtcLabel::Primary, nullptr);
    Layouting::Tools::apply(this, ps);
}

void Label::setText(const QString &text)
{
    Layouting::Tools::access(this)->setText(text);
}

void Label::setRole(QtcLabel::Role role)
{
    Layouting::Tools::access(this)->setRole(role);
}

SearchBox::SearchBox()
{
    ptr = new Implementation();
}

SearchBox::SearchBox(std::initializer_list<I> ps)
{
    ptr = new Implementation();
    Layouting::Tools::apply(this, ps);
}

void SearchBox::setPlaceholderText(const QString &text)
{
    Layouting::Tools::access(this)->setPlaceholderText(text);
}

void SearchBox::setText(const QString &text)
{
    Layouting::Tools::access(this)->setText(text);
}

void SearchBox::onTextChanged(QObject *guard, const std::function<void(QString)> &func)
{
    QObject::connect(Layouting::Tools::access(this), &QtcSearchBox::textChanged, guard, func);
}

} // namespace QtcWidgets

} // namespace Utils
