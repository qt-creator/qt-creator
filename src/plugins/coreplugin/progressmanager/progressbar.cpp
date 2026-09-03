// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "progressbar.h"

#include <utils/icon.h>
#include <utils/layoutbuilder.h>
#include <utils/qtdesignwidgets.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QEnterEvent>
#include <QLabel>
#include <QPainter>

using namespace Core;
using namespace Utils;

namespace Core::Internal {

class CloseButton : public QAbstractButton
{
public:
    CloseButton(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
};

void CloseButton::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    if (underMouse() && isEnabled())
        StyleHelper::drawCardBg(&p, rect(), creatorColor(Theme::BackgroundColorHover));
    static const QIcon icon = Icon({{":/utils/images/close_small.png", Theme::PanelTextColorMid}},
                                   Icon::Tint).icon();
    icon.paint(&p, rect(), Qt::AlignCenter, isEnabled() ? QIcon::Mode::Normal
                                                        : QIcon::Mode::Disabled);
}

void CloseButton::enterEvent(QEnterEvent *event)
{
    if (isEnabled())
        update();
    QAbstractButton::enterEvent(event);
}

void CloseButton::leaveEvent(QEvent *event)
{
    if (isEnabled())
        update();
    QAbstractButton::leaveEvent(event);
}

CloseButton::CloseButton(QWidget *parent)
    : QAbstractButton(parent)
{
    setAttribute(Qt::WA_LayoutUsesWidgetRect);
    setFixedSize(14, 10);
    setFocusPolicy(Qt::NoFocus);
}

ProgressBar::ProgressBar(Role role, QWidget *parent)
    : QWidget(parent)
    , m_progressBar(new Utils::QtcProgressBar)
{
    setMinimumWidth(role == Default ? 200 : 70);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    m_progressBar->setTextVisible(false);
    m_progressBar->setRange(m_minimum, m_maximum);
    m_progressBar->setValue(m_value);
    m_progressBar->setBackgroundColor(Theme::ProgressBarBackgroundColor);
    updateColor();

    const int margin = StyleHelper::SpacingTokens::PaddingHM;
    const qreal marginThird = margin / 3.0;
    using namespace Layouting;
    if (role == Default) {
        constexpr StyleHelper::TextFormat titleTf {
            .themeColor = Theme::Token_Text_Default,
            .uiElement = StyleHelper::UiElementCaptionStrong,
            .drawTextFlags = Qt::AlignCenter,
        };

        m_titleLabel = new QLabel;
        StyleHelper::applyTf(m_titleLabel, titleTf, false);
        m_titleLabel->setTextInteractionFlags(Qt::NoTextInteraction);

        m_cancelButton = new CloseButton;
        connect(m_cancelButton, &QAbstractButton::clicked, this, &ProgressBar::clicked);

        const int commonHeight = qMax(m_cancelButton->minimumHeight(),
                                      m_progressBar->minimumHeight());
        m_cancelButton->setFixedHeight(commonHeight);
        m_progressBar->setFixedHeight(commonHeight);

        m_subtitleLabel = new QLabel;
        StyleHelper::applyTf(m_subtitleLabel, titleTf, false);
        m_subtitleLabel->setTextInteractionFlags(Qt::NoTextInteraction);
        m_subtitleLabel->setVisible(false);

        Column {
            Row {
                m_titleLabel,
                customMargins(0, 0, margin, 0),
            },
            Row {
                m_progressBar,
                Space(marginThird),
                m_cancelButton,
                spacing(0),
                customMargins(0, 0, marginThird * 2, 0),
            },
            Row {
                m_subtitleLabel,
                customMargins(0, 0, margin, 0),
            },
            spacing(StyleHelper::SpacingTokens::GapVS),
            customMargins(margin, margin, 0, margin),
        }.attachTo(this);
    } else {
        Column {
            m_progressBar,
            customMargins(margin, margin, margin, margin),
        }.attachTo(this);
    }
}

void ProgressBar::reset()
{
    m_value = m_minimum;
    m_progressBar->setValue(m_value);
}

void ProgressBar::setRange(int minimum, int maximum)
{
    m_minimum = minimum;
    m_maximum = maximum;
    if (m_value < m_minimum || m_value > m_maximum)
        m_value = m_minimum;
    m_progressBar->setRange(m_minimum, m_maximum);
    m_progressBar->setValue(m_value);
}

void ProgressBar::setValue(int value)
{
    value = qBound(m_minimum, value, m_maximum);
    if (m_value == value) {
        return;
    }
    m_value = value;
    m_progressBar->setValue(m_value);
}

void ProgressBar::setFinished(bool b)
{
    if (b == m_finished)
        return;
    m_finished = b;
    updateColor();
    updateCancelButton();
}

void ProgressBar::updateColor()
{
    const Theme::Color themeColor = m_error ? Theme::ProgressBarColorError
                                    : m_finished ? Theme::ProgressBarColorFinished
                                                 : Theme::ProgressBarColorNormal;
    m_progressBar->setFillColor(themeColor);
}

void ProgressBar::updateCancelButton()
{
    if (!m_cancelButton)
        return;
    m_cancelButton->setEnabled(m_cancelEnabled);
    m_cancelButton->setVisible(!m_finished);
}

QString ProgressBar::title() const
{
    return m_titleLabel ? m_titleLabel->text() : QString();
}

bool ProgressBar::hasError() const
{
    return m_error;
}

void ProgressBar::setTitle(const QString &title)
{
    if (m_titleLabel)
        m_titleLabel->setText(title);
}

void ProgressBar::setSubtitle(const QString &subtitle)
{
    if (!m_subtitleLabel)
        return;
    m_subtitleLabel->setText(subtitle);
    m_subtitleLabel->setVisible(!subtitle.isEmpty());
}

QString ProgressBar::subtitle() const
{
    return m_subtitleLabel ? m_subtitleLabel->text() : QString();
}

void ProgressBar::setCancelEnabled(bool enabled)
{
    m_cancelEnabled = enabled;
    updateCancelButton();
}

bool ProgressBar::isCancelEnabled() const
{
    return m_cancelEnabled;
}

void ProgressBar::setError(bool on)
{
    m_error = on;
    updateColor();
}

} // namespace Core::Internal
