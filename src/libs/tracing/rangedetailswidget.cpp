// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rangedetailswidget.h"

#include <utils/elidinglabel.h>
#include <utils/icon.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QFont>
#include <QFrame>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QPlainTextEdit>
#include <QSize>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace Timeline {

static QColor themeColor(Utils::Theme::Color role)
{
    if (Utils::creatorTheme())
        return Utils::creatorTheme()->color(role);
    return QColor();
}

// Resize grip -- defined here, not exposed in the header
class ResizeGrip : public QWidget
{
public:
    explicit ResizeGrip(QWidget *parent) : QWidget(parent)
    {
        setFixedSize(GripSize, GripSize);
        setCursor(Qt::SizeHorCursor);
    }

    static constexpr int GripSize = 10;

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        const QPoint pts[3] = {{0, GripSize}, {GripSize, GripSize}, {GripSize, 0}};
        p.setPen(Qt::NoPen);
        p.setBrush(themeColor(Utils::Theme::Timeline_PanelHeaderColor));
        p.drawPolygon(pts, 3);
    }

    void mousePressEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::LeftButton) {
            m_pressGlobal = e->globalPosition().toPoint();
            m_initialW = parentWidget()->width();
            m_pressed = true;
        }
        e->accept();
    }

    void mouseMoveEvent(QMouseEvent *e) override
    {
        if (m_pressed) {
            const int dx = e->globalPosition().toPoint().x() - m_pressGlobal.x();
            const int newW = qMax(150, m_initialW + dx);
            parentWidget()->resize(newW, parentWidget()->height());
        }
        e->accept();
    }

    void mouseReleaseEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::LeftButton)
            m_pressed = false;
        e->accept();
    }

private:
    QPoint m_pressGlobal;
    int m_initialW = 0;
    bool m_pressed = false;
};


// Title bar icon button -- matches QML ImageToolButton background behaviour
class TitleBarButton : public QToolButton
{
public:
    explicit TitleBarButton(QWidget *parent) : QToolButton(parent)
    {
        setAttribute(Qt::WA_Hover);
        setCheckable(false);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        if (isChecked() || isDown()) {
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(Qt::NoPen);
            p.setBrush(themeColor(Utils::Theme::FancyToolButtonSelectedColor));
            p.drawRoundedRect(rect(), 3, 3);
        } else if (underMouse()) {
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(Qt::NoPen);
            p.setBrush(themeColor(Utils::Theme::FancyToolButtonHoverColor));
            p.drawRoundedRect(rect(), 3, 3);
        }
        const QRect r((width() - iconSize().width()) / 2,
                      (height() - iconSize().height()) / 2,
                      iconSize().width(), iconSize().height());
        icon().paint(&p, r, Qt::AlignCenter,
                     isEnabled() ? QIcon::Normal : QIcon::Disabled,
                     isChecked() ? QIcon::On : QIcon::Off);
    }
};


RangeDetailsWidget::RangeDetailsWidget(QWidget *parent)
    : QWidget(parent)
{
    hide();
    setAutoFillBackground(true);
    setMinimumWidth(150);

    auto outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(1, 1, 1, 1);
    outerLayout->setSpacing(0);

    // Title bar
    m_titleBar = new QWidget;
    m_titleBar->setAutoFillBackground(true);
    m_titleBar->setFixedHeight(24);

    auto titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(8, 0, 4, 0);
    titleLayout->setSpacing(2);

    m_titleLabel = new Utils::ElidingLabel(m_titleBar);
    QFont boldFont = m_titleLabel->font();
    boldFont.setBold(true);
    m_titleLabel->setFont(boldFont);
    m_titleLabel->setTextFormat(Qt::PlainText);
    titleLayout->addWidget(m_titleLabel, 1);

    const QIcon editIcon = Utils::Icon(
        {{":/tracing/images/edit.png", Utils::Theme::IconsBaseColor}}).icon();
    const QIcon lockOpenIcon = Utils::Icons::UNLOCKED_TOOLBAR.icon();
    const QIcon lockClosedIcon = Utils::Icons::LOCKED_TOOLBAR.icon();
    const QIcon arrowUpIcon = Utils::Icons::ARROW_UP.icon();
    const QIcon arrowDownIcon = Utils::Icons::ARROW_DOWN.icon();

    const QSize iconSize(16, 16);

    m_editBtn = new TitleBarButton(m_titleBar);
    m_editBtn->setIcon(editIcon);
    m_editBtn->setIconSize(iconSize);
    m_editBtn->setToolTip("Edit note");
    titleLayout->addWidget(m_editBtn);
    connect(m_editBtn, &QToolButton::clicked, this, [this] {
        m_noteEdit->setVisible(true);
        fitHeight();
        m_noteEdit->setFocus();
    });

    m_lockBtn = new TitleBarButton(m_titleBar);
    m_lockBtn->setIcon(lockOpenIcon);
    m_lockBtn->setIconSize(iconSize);
    m_lockBtn->setCheckable(true);
    m_lockBtn->setToolTip("Lock");
    titleLayout->addWidget(m_lockBtn);
    connect(m_lockBtn, &QToolButton::toggled, this, [this, lockOpenIcon, lockClosedIcon](bool checked) {
        m_locked = checked;
        m_lockBtn->setIcon(checked ? lockClosedIcon : lockOpenIcon);
        emit lockChanged(checked);
    });

    // When content is visible the arrow points up (click to collapse), and vice versa.
    m_collapseBtn = new TitleBarButton(m_titleBar);
    m_collapseBtn->setIcon(arrowUpIcon);
    m_collapseBtn->setIconSize(iconSize);
    m_collapseBtn->setToolTip("Collapse");
    titleLayout->addWidget(m_collapseBtn);
    connect(m_collapseBtn, &QToolButton::clicked, this, [this, arrowUpIcon, arrowDownIcon] {
        m_collapsed = !m_collapsed;
        m_contentFrame->setVisible(!m_collapsed);
        m_collapseBtn->setIcon(m_collapsed ? arrowDownIcon : arrowUpIcon);
        m_collapseBtn->setToolTip(QLatin1String(m_collapsed ? "Expand" : "Collapse"));
        fitHeight();
    });

    outerLayout->addWidget(m_titleBar);

    auto separator = new QWidget;
    separator->setFixedHeight(1);
    separator->setAutoFillBackground(true);
    {
        auto pal = separator->palette();
        pal.setColor(QPalette::Window, themeColor(Utils::Theme::PanelTextColorMid));
        separator->setPalette(pal);
    }
    outerLayout->addWidget(separator);

    // Content frame (key-value grid + note)
    m_contentFrame = new QFrame;
    m_contentFrame->setFrameShape(QFrame::NoFrame);
    m_contentFrame->setAutoFillBackground(true);

    auto contentLayout = new QVBoxLayout(m_contentFrame);
    contentLayout->setContentsMargins(10, 5, 10, 0);
    contentLayout->setSpacing(5);

    m_formWidget = new QWidget;
    m_form = new QFormLayout(m_formWidget);
    m_form->setContentsMargins(0, 0, 0, 0);
    m_form->setSpacing(5);
    m_form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    contentLayout->addWidget(m_formWidget);

    m_noteEdit = new QPlainTextEdit;
    m_noteEdit->setPlaceholderText("Add note...");
    m_noteEdit->setFixedHeight(60);
    QFont noteFont = m_noteEdit->font();
    noteFont.setItalic(true);
    m_noteEdit->setFont(noteFont);
    m_noteEdit->setFrameShape(QFrame::NoFrame);
    m_noteEdit->setAutoFillBackground(true);
    m_noteEdit->document()->setDocumentMargin(0);
    m_noteEdit->setVisible(false);
    m_noteEdit->viewport()->installEventFilter(this);
    contentLayout->addWidget(m_noteEdit);

    outerLayout->addWidget(m_contentFrame);

    // Bottom row: stretch + resize grip in the right corner
    auto bottomRow = new QHBoxLayout;
    bottomRow->setContentsMargins(0, 0, 0, 0);
    bottomRow->setSpacing(0);
    bottomRow->addStretch(1);
    bottomRow->addWidget(new ResizeGrip(this));
    outerLayout->addLayout(bottomRow);

    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(1000);
    connect(m_saveTimer, &QTimer::timeout, this, [this] {
        emit noteChanged(m_noteEdit->toPlainText());
    });
    connect(m_noteEdit, &QPlainTextEdit::textChanged, this, &RangeDetailsWidget::scheduleNoteSave);

    // Apply theme colors
    auto pal = palette();
    pal.setColor(QPalette::Window, themeColor(Utils::Theme::Timeline_PanelBackgroundColor));
    pal.setColor(QPalette::WindowText, themeColor(Utils::Theme::Timeline_TextColor));
    setPalette(pal);

    auto titlePal = m_titleBar->palette();
    titlePal.setColor(QPalette::Window, themeColor(Utils::Theme::Timeline_PanelHeaderColor));
    titlePal.setColor(QPalette::WindowText, themeColor(Utils::Theme::PanelTextColorLight));
    m_titleBar->setPalette(titlePal);
    m_titleLabel->setPalette(titlePal);

    auto notePal = m_noteEdit->palette();
    notePal.setColor(QPalette::Base, themeColor(Utils::Theme::Timeline_PanelBackgroundColor));
    notePal.setColor(QPalette::Text, themeColor(Utils::Theme::Timeline_HighlightColor));
    m_noteEdit->setPalette(notePal);

}

void RangeDetailsWidget::setData(const QString &title,
                                  const QList<QPair<QString, QString>> &content,
                                  const QString &noteText)
{
    saveNote();

    m_titleLabel->setText(title);
    rebuildRows(content);

    QSignalBlocker blocker(m_noteEdit);
    m_noteEdit->setPlainText(noteText);
    m_noteEdit->setVisible(!noteText.isEmpty());
    m_saveTimer->stop();

    // show() before fitHeight() so that inserting m_formWidget into a visible
    // parent triggers polishing of the child labels; without it, font metrics
    // are wrong on the first hover and the height comes out incorrect.
    show();
    raise();
    fitHeight();
}

void RangeDetailsWidget::setLocked(bool locked)
{
    m_lockBtn->setChecked(locked);
}

void RangeDetailsWidget::saveNote()
{
    if (m_saveTimer->isActive()) {
        m_saveTimer->stop();
        emit noteChanged(m_noteEdit->toPlainText());
    }
}

void RangeDetailsWidget::clear()
{
    saveNote();
    hide();
}

bool RangeDetailsWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_noteEdit->viewport() && event->type() == QEvent::FocusOut) {
        if (m_saveTimer->isActive()) {
            m_saveTimer->stop();
            emit noteChanged(m_noteEdit->toPlainText());
        }
        if (m_noteEdit->toPlainText().isEmpty()) {
            m_noteEdit->setVisible(false);
            fitHeight();
        }
    }
    return false;
}

void RangeDetailsWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setPen(themeColor(Utils::Theme::PanelTextColorMid));
    p.setBrush(Qt::NoBrush);
    p.drawRect(0, 0, width() - 1, height() - 1);
}

void RangeDetailsWidget::fitHeight()
{
    const int w = qMax(minimumWidth(), width() > 0 ? width() : 300);
    layout()->invalidate();
    const int h = layout()->hasHeightForWidth()
                  ? layout()->heightForWidth(w)
                  : layout()->sizeHint().height();
    resize(w, h);
    setMinimumWidth(150);
    setMaximumWidth(QWIDGETSIZE_MAX);
}

void RangeDetailsWidget::rebuildRows(const QList<QPair<QString, QString>> &content)
{
    auto *cl = static_cast<QVBoxLayout *>(m_contentFrame->layout());
    const int idx = cl->indexOf(m_formWidget);
    delete m_formWidget; // deletes m_form and all child labels
    m_formWidget = new QWidget(m_contentFrame);
    m_form = new QFormLayout(m_formWidget);
    m_form->setContentsMargins(0, 0, 0, 0);
    m_form->setSpacing(5);
    m_form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    cl->insertWidget(idx, m_formWidget);

    const int pairs = content.size();
    for (int i = 0; i < pairs; ++i) {
        const QString key = content.at(i).first;
        const QString val = content.at(i).second;

        auto keyLabel = new QLabel(key + ":");
        keyLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        keyLabel->setTextFormat(Qt::PlainText);
        keyLabel->setAutoFillBackground(false);
        QFont boldFont = keyLabel->font();
        boldFont.setBold(true);
        keyLabel->setFont(boldFont);

        auto valLabel = new QLabel(val);
        valLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        valLabel->setTextFormat(Qt::PlainText);
        valLabel->setWordWrap(true);
        valLabel->setAutoFillBackground(false);

        m_form->addRow(keyLabel, valLabel);
    }
}

void RangeDetailsWidget::scheduleNoteSave()
{
    m_saveTimer->start();
}

void RangeDetailsWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragOffset = event->pos();
        m_dragging = m_titleBar->geometry().contains(event->pos());
        m_didDrag = false;
    }
    event->accept();
}

void RangeDetailsWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        if (!m_didDrag && (event->pos() - m_dragOffset).manhattanLength() > 4)
            m_didDrag = true;
        if (m_didDrag)
            move(mapToParent(event->pos()) - m_dragOffset);
    }
    event->accept();
}

void RangeDetailsWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (!m_didDrag)
            emit recenterOnItem();
        m_dragging = false;
        m_didDrag = false;
    }
    event->accept();
}

} // namespace Timeline
