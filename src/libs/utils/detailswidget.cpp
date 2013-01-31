/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "detailswidget.h"
#include "detailsbutton.h"
#include "hostosinfo.h"

#include <QStack>
#include <QPropertyAnimation>

#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPainter>
#include <QScrollArea>
#include <QApplication>

/*!
    \class Utils::DetailsWidget

    \brief Widget a button to expand a 'Details' area.

    This widget is using a grid layout and places the items
    in the following way:

    \code
+------------+-------------------------+---------------+
+summaryLabel|              toolwidget | detailsButton |
+------------+-------------------------+---------------+
+                additional summary                    |
+------------+-------------------------+---------------+
|                  widget                              |
+------------+-------------------------+---------------+
    \endcode
*/

namespace Utils {

static const int MARGIN = 8;

class DetailsWidgetPrivate
{
public:
    DetailsWidgetPrivate(QWidget *parent);

    void updateControls();
    void changeHoverState(bool hovered);

    QWidget *q;
    DetailsButton *m_detailsButton;
    QGridLayout *m_grid;
    QLabel *m_summaryLabel;
    QCheckBox *m_summaryCheckBox;
    QLabel *m_additionalSummaryLabel;
    Utils::FadingPanel *m_toolWidget;
    QWidget *m_widget;

    QPixmap m_collapsedPixmap;
    QPixmap m_expandedPixmap;

    DetailsWidget::State m_state;
    bool m_hovered;
    bool m_useCheckBox;
};

DetailsWidgetPrivate::DetailsWidgetPrivate(QWidget *parent) :
        q(parent),
        m_detailsButton(new DetailsButton),
        m_grid(new QGridLayout),
        m_summaryLabel(new QLabel(parent)),
        m_summaryCheckBox(new QCheckBox(parent)),
        m_additionalSummaryLabel(new QLabel(parent)),
        m_toolWidget(0),
        m_widget(0),
        m_state(DetailsWidget::Collapsed),
        m_hovered(false),
        m_useCheckBox(false)
{
    QHBoxLayout *summaryLayout = new QHBoxLayout;
    summaryLayout->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
    summaryLayout->setSpacing(0);

    m_summaryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    m_summaryLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_summaryLabel->setContentsMargins(0, 0, 0, 0);
    summaryLayout->addWidget(m_summaryLabel);

    m_summaryCheckBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_summaryCheckBox->setAttribute(Qt::WA_LayoutUsesWidgetRect); /* broken layout on mac otherwise */
    m_summaryCheckBox->setVisible(false);
    m_summaryCheckBox->setContentsMargins(0, 0, 0, 0);
    summaryLayout->addWidget(m_summaryCheckBox);

    m_additionalSummaryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_additionalSummaryLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_additionalSummaryLabel->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
    m_additionalSummaryLabel->setWordWrap(true);
    m_additionalSummaryLabel->setVisible(false);

    m_grid->setContentsMargins(0, 0, 0, 0);
    m_grid->setSpacing(0);
    m_grid->addLayout(summaryLayout, 0, 0);
    m_grid->addWidget(m_detailsButton, 0, 2);
    m_grid->addWidget(m_additionalSummaryLabel, 1, 0, 1, 3);
}

QPixmap DetailsWidget::createBackground(const QSize &size, int topHeight, QWidget *widget)
{
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);

    QRect topRect(0, 0, size.width(), topHeight);
    QRect fullRect(0, 0, size.width(), size.height());
    if (HostOsInfo::isMacHost())
        p.fillRect(fullRect, qApp->palette().window().color());
    else
        p.fillRect(fullRect, QColor(255, 255, 255, 40));

    QLinearGradient lg(topRect.topLeft(), topRect.bottomLeft());
    lg.setColorAt(0, QColor(255, 255, 255, 130));
    lg.setColorAt(1, QColor(255, 255, 255, 0));
    p.fillRect(topRect, lg);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.translate(0.5, 0.5);
    p.setPen(QColor(0, 0, 0, 40));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(fullRect.adjusted(0, 0, -1, -1), 2, 2);
    p.setBrush(Qt::NoBrush);
    p.setPen(QColor(255,255,255,140));
    p.drawRoundedRect(fullRect.adjusted(1, 1, -2, -2), 2, 2);
    p.setPen(QPen(widget->palette().color(QPalette::Mid)));

    return pixmap;
}

void DetailsWidgetPrivate::updateControls()
{
    if (m_widget)
        m_widget->setVisible(m_state == DetailsWidget::Expanded || m_state == DetailsWidget::NoSummary);
    m_detailsButton->setChecked(m_state == DetailsWidget::Expanded && m_widget);
    m_detailsButton->setVisible(m_state == DetailsWidget::Expanded || m_state == DetailsWidget::Collapsed);
    m_summaryLabel->setVisible(m_state != DetailsWidget::NoSummary && !m_useCheckBox);
    m_summaryCheckBox->setVisible(m_state != DetailsWidget::NoSummary && m_useCheckBox);

    for (QWidget *w = q; w; w = w->parentWidget()) {
        if (w->layout())
            w->layout()->activate();
        if (QScrollArea *area = qobject_cast<QScrollArea*>(w)) {
            QEvent e(QEvent::LayoutRequest);
            QCoreApplication::sendEvent(area, &e);
        }
    }
}

void DetailsWidgetPrivate::changeHoverState(bool hovered)
{
    if (!m_toolWidget)
        return;
    if (HostOsInfo::isMacHost())
        m_toolWidget->setOpacity(hovered ? 1.0 : 0);
    else
        m_toolWidget->fadeTo(hovered ? 1.0 : 0);
    m_hovered = hovered;
}


DetailsWidget::DetailsWidget(QWidget *parent) :
        QWidget(parent),
        d(new DetailsWidgetPrivate(this))
{
    setLayout(d->m_grid);

    setUseCheckBox(false);

    connect(d->m_detailsButton, SIGNAL(toggled(bool)),
            this, SLOT(setExpanded(bool)));
    connect(d->m_summaryCheckBox, SIGNAL(toggled(bool)),
            this, SIGNAL(checked(bool)));
    connect(d->m_summaryLabel, SIGNAL(linkActivated(QString)),
            this, SIGNAL(linkActivated(QString)));
    d->updateControls();
}

DetailsWidget::~DetailsWidget()
{
    delete d;
}

bool DetailsWidget::useCheckBox()
{
    return d->m_useCheckBox;
}

void DetailsWidget::setUseCheckBox(bool b)
{
    d->m_useCheckBox = b;
    d->m_summaryLabel->setVisible(b);
    d->m_summaryCheckBox->setVisible(!b);
}

void DetailsWidget::setChecked(bool b)
{
    d->m_summaryCheckBox->setChecked(b);
}

bool DetailsWidget::isChecked() const
{
    return d->m_useCheckBox && d->m_summaryCheckBox->isChecked();
}

void DetailsWidget::setSummaryFontBold(bool b)
{
    QFont f;
    f.setBold(b);
    d->m_summaryCheckBox->setFont(f);
    d->m_summaryLabel->setFont(f);
}

void DetailsWidget::setIcon(const QIcon &icon)
{
    d->m_summaryCheckBox->setIcon(icon);
}

void DetailsWidget::paintEvent(QPaintEvent *paintEvent)
{
    QWidget::paintEvent(paintEvent);

    QPainter p(this);

    QWidget *topLeftWidget = d->m_useCheckBox ? static_cast<QWidget *>(d->m_summaryCheckBox) : static_cast<QWidget *>(d->m_summaryLabel);
    QPoint topLeft(topLeftWidget->geometry().left() - MARGIN, contentsRect().top());
    const QRect paintArea(topLeft, contentsRect().bottomRight());

    int topHeight = d->m_useCheckBox ? d->m_summaryCheckBox->height() : d->m_summaryLabel->height();
    if (d->m_state == DetailsWidget::Expanded || d->m_state == DetailsWidget::Collapsed) // Details Button is shown
        topHeight = qMax(d->m_detailsButton->height(), topHeight);

    if (d->m_state == Collapsed) {
        if (d->m_collapsedPixmap.isNull() ||
            d->m_collapsedPixmap.size() != size())
            d->m_collapsedPixmap = createBackground(paintArea.size(), topHeight, this);
        p.drawPixmap(paintArea, d->m_collapsedPixmap);
    } else {
        if (d->m_expandedPixmap.isNull() ||
            d->m_expandedPixmap.size() != size())
            d->m_expandedPixmap = createBackground(paintArea.size(), topHeight, this);
        p.drawPixmap(paintArea, d->m_expandedPixmap);
    }
}

void DetailsWidget::enterEvent(QEvent * event)
{
    QWidget::enterEvent(event);
    d->changeHoverState(true);
}

void DetailsWidget::leaveEvent(QEvent * event)
{
    QWidget::leaveEvent(event);
    d->changeHoverState(false);
}

void DetailsWidget::setSummaryText(const QString &text)
{
    if (d->m_useCheckBox)
        d->m_summaryCheckBox->setText(text);
    else
        d->m_summaryLabel->setText(text);
}

QString DetailsWidget::summaryText() const
{
    if (d->m_useCheckBox)
        return d->m_summaryCheckBox->text();
    return d->m_summaryLabel->text();
}

QString DetailsWidget::additionalSummaryText() const
{
    return d->m_additionalSummaryLabel->text();
}

void DetailsWidget::setAdditionalSummaryText(const QString &text)
{
    d->m_additionalSummaryLabel->setText(text);
    d->m_additionalSummaryLabel->setVisible(!text.isEmpty());
}

DetailsWidget::State DetailsWidget::state() const
{
    return d->m_state;
}

void DetailsWidget::setState(State state)
{
    if (state == d->m_state)
        return;
    d->m_state = state;
    d->updateControls();
    emit expanded(d->m_state == Expanded);
}

void DetailsWidget::setExpanded(bool expanded)
{
    setState(expanded ? Expanded : Collapsed);
}

QWidget *DetailsWidget::widget() const
{
    return d->m_widget;
}

QWidget *DetailsWidget::takeWidget()
{
    QWidget *widget = d->m_widget;
    d->m_widget = 0;
    d->m_grid->removeWidget(widget);
    if (widget)
        widget->setParent(0);
    return widget;
}

void DetailsWidget::setWidget(QWidget *widget)
{
    if (d->m_widget == widget)
        return;

    if (d->m_widget) {
        d->m_grid->removeWidget(d->m_widget);
        delete d->m_widget;
    }

    d->m_widget = widget;

    if (d->m_widget) {
        d->m_widget->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
        d->m_grid->addWidget(d->m_widget, 2, 0, 1, 3);
    }
    d->updateControls();
}

void DetailsWidget::setToolWidget(Utils::FadingPanel *widget)
{
    if (d->m_toolWidget == widget)
        return;

    d->m_toolWidget = widget;

    if (!d->m_toolWidget)
        return;

    d->m_toolWidget->adjustSize();
    d->m_grid->addWidget(d->m_toolWidget, 0, 1, 1, 1, Qt::AlignRight);

    if (HostOsInfo::isMacHost())
        d->m_toolWidget->setOpacity(1.0);
    d->changeHoverState(d->m_hovered);
}

QWidget *DetailsWidget::toolWidget() const
{
    return d->m_toolWidget;
}

} // namespace Utils
