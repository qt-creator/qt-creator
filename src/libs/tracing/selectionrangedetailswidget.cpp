// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "selectionrangedetailswidget.h"

#include "timelineformattime.h"

#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QSize>
#include <QToolButton>
#include <QVBoxLayout>

namespace Timeline {

static QColor themeColor(Utils::Theme::Color role)
{
    if (Utils::creatorTheme())
        return Utils::creatorTheme()->color(role);
    return QColor();
}

class SelectionTitleButton : public QToolButton
{
public:
    explicit SelectionTitleButton(QWidget *parent) : QToolButton(parent)
    {
        setAttribute(Qt::WA_Hover);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        if (underMouse()) {
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(Qt::NoPen);
            p.setBrush(themeColor(Utils::Theme::FancyToolButtonHoverColor));
            p.drawRoundedRect(rect(), 3, 3);
        }
        const QRect r((width() - iconSize().width()) / 2,
                      (height() - iconSize().height()) / 2,
                      iconSize().width(), iconSize().height());
        icon().paint(&p, r);
    }
};

static QLabel *makeBoldLabel(const QString &text)
{
    auto label = new QLabel(text);
    label->setTextFormat(Qt::PlainText);
    QFont f = label->font();
    f.setBold(true);
    label->setFont(f);
    return label;
}

SelectionRangeDetailsWidget::SelectionRangeDetailsWidget(QWidget *parent)
    : QWidget(parent)
{
    hide();
    setAutoFillBackground(true);
    setMinimumWidth(150);

    auto outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(1, 1, 1, 1);
    outerLayout->setSpacing(0);

    // Title bar
    auto titleBar = new QWidget;
    titleBar->setFixedHeight(24);
    titleBar->setAutoFillBackground(true);

    auto titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(8, 0, 4, 0);
    titleLayout->setSpacing(2);

    auto titleLabel = makeBoldLabel(tr("Selection"));
    titleLayout->addWidget(titleLabel, 1);

    auto closeBtn = new SelectionTitleButton(titleBar);
    closeBtn->setIcon(Utils::Icons::CLOSE_TOOLBAR.icon());
    closeBtn->setIconSize(QSize(16, 16));
    closeBtn->setToolTip(tr("Close"));
    titleLayout->addWidget(closeBtn);
    connect(closeBtn, &QToolButton::clicked,
            this, &SelectionRangeDetailsWidget::closeRequested);

    outerLayout->addWidget(titleBar);

    // Separator line
    auto separator = new QWidget;
    separator->setFixedHeight(1);
    separator->setAutoFillBackground(true);
    {
        auto pal = separator->palette();
        pal.setColor(QPalette::Window, themeColor(Utils::Theme::PanelTextColorMid));
        separator->setPalette(pal);
    }
    outerLayout->addWidget(separator);

    // Content area
    auto contentWidget = new QWidget;
    contentWidget->setAutoFillBackground(true);

    auto form = new QFormLayout(contentWidget);
    form->setContentsMargins(10, 5, 10, 5);
    form->setSpacing(5);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_startValue = new QLabel;
    m_startValue->setTextFormat(Qt::PlainText);
    form->addRow(makeBoldLabel(tr("Start:")), m_startValue);

    m_endKey = makeBoldLabel(tr("End:"));
    m_endValue = new QLabel;
    m_endValue->setTextFormat(Qt::PlainText);
    form->addRow(m_endKey, m_endValue);

    m_durationKey = makeBoldLabel(tr("Duration:"));
    m_durationValue = new QLabel;
    m_durationValue->setTextFormat(Qt::PlainText);
    form->addRow(m_durationKey, m_durationValue);

    outerLayout->addWidget(contentWidget);

    // Theme colors
    auto pal = palette();
    pal.setColor(QPalette::Window, themeColor(Utils::Theme::Timeline_PanelBackgroundColor));
    pal.setColor(QPalette::WindowText, themeColor(Utils::Theme::Timeline_TextColor));
    setPalette(pal);

    auto titlePal = titleBar->palette();
    titlePal.setColor(QPalette::Window, themeColor(Utils::Theme::Timeline_PanelHeaderColor));
    titlePal.setColor(QPalette::WindowText, themeColor(Utils::Theme::PanelTextColorLight));
    titleBar->setPalette(titlePal);
    titleLabel->setPalette(titlePal);
}

void SelectionRangeDetailsWidget::setData(qint64 start, qint64 end,
                                           qint64 referenceDuration, bool showDuration)
{
    m_startValue->setText(formatTime(start, referenceDuration));
    m_endKey->setVisible(showDuration);
    m_endValue->setVisible(showDuration);
    m_durationKey->setVisible(showDuration);
    m_durationValue->setVisible(showDuration);
    if (showDuration) {
        m_endValue->setText(formatTime(end, referenceDuration));
        m_durationValue->setText(formatTime(end - start, referenceDuration));
    }
    show();
    raise();
    adjustSize();
}

void SelectionRangeDetailsWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragOffset = event->pos();
        m_dragging = true;
        m_didDrag = false;
    }
    event->accept();
}

void SelectionRangeDetailsWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        if (!m_didDrag && (event->pos() - m_dragOffset).manhattanLength() > 4)
            m_didDrag = true;
        if (m_didDrag)
            move(mapToParent(event->pos()) - m_dragOffset);
    }
    event->accept();
}

void SelectionRangeDetailsWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_dragging && !m_didDrag)
            emit recenterRequested();
        m_dragging = false;
        m_didDrag = false;
    }
    event->accept();
}

void SelectionRangeDetailsWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setPen(themeColor(Utils::Theme::PanelTextColorMid));
    p.setBrush(Qt::NoBrush);
    p.drawRect(0, 0, width() - 1, height() - 1);
}

} // namespace Timeline
