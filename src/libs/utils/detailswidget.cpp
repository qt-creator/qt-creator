/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "detailswidget.h"
#include "detailsbutton.h"

#include <QtCore/QStack>

#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QPainter>
#include <QtGui/QScrollArea>
#include <QtGui/QApplication>

namespace Utils {

    static const int MARGIN=8;

    // This widget is using a grid layout and places the items
    // in the following way:
    //
    // +------------+-------------------------+---------------+
    // + toolWidget | summaryLabel            | detailsButton |
    // +------------+-------------------------+---------------+
    // |            | widget                                  |
    // +------------+-------------------------+---------------+

    struct DetailsWidgetPrivate {
        DetailsWidgetPrivate();

        DetailsButton *m_detailsButton;
        QGridLayout *m_grid;
        QLabel *m_summaryLabel;
        QWidget *m_toolWidget;
        QWidget *m_widget;

        QPixmap m_collapsedPixmap;
        QPixmap m_expandedPixmap;

        DetailsWidget::State m_state;
        bool m_hovered;
    };

    DetailsWidgetPrivate::DetailsWidgetPrivate() :
            m_detailsButton(new DetailsButton),
            m_grid(new QGridLayout),
            m_summaryLabel(new QLabel),
            m_toolWidget(0),
            m_widget(0),
            m_state(DetailsWidget::Collapsed),
            m_hovered(false)
    {
    }

    DetailsWidget::DetailsWidget(QWidget *parent) :
            QWidget(parent),
            d(new DetailsWidgetPrivate)
    {
        d->m_summaryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        d->m_summaryLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        d->m_summaryLabel->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);

        d->m_grid->setContentsMargins(0, 0, 0, 0);
        d->m_grid->setSpacing(0);
        d->m_grid->addWidget(d->m_summaryLabel, 0, 1);
        d->m_grid->addWidget(d->m_detailsButton, 0, 2);
        setLayout(d->m_grid);

        connect(d->m_detailsButton, SIGNAL(toggled(bool)),
                this, SLOT(setExpanded(bool)));
        updateControls();
    }

    DetailsWidget::~DetailsWidget()
    {
        delete d;
    }

    void DetailsWidget::paintEvent(QPaintEvent *paintEvent)
    {
        QWidget::paintEvent(paintEvent);

        QPainter p(this);

        QPoint topLeft(d->m_summaryLabel->geometry().left(), contentsRect().top());
        const QRect paintArea(topLeft, contentsRect().bottomRight());

        if (d->m_state != Expanded) {
            if (d->m_collapsedPixmap.isNull() ||
                d->m_collapsedPixmap.size() != size())
                d->m_collapsedPixmap = cacheBackground(paintArea.size(), false);
            p.drawPixmap(paintArea, d->m_collapsedPixmap);
        } else {
            if (d->m_expandedPixmap.isNull() ||
                d->m_expandedPixmap.size() != size())
                d->m_expandedPixmap = cacheBackground(paintArea.size(), true);
            p.drawPixmap(paintArea, d->m_expandedPixmap);
        }
    }

    void DetailsWidget::enterEvent(QEvent * event)
    {
        QWidget::enterEvent(event);
        changeHoverState(true);
    }

    void DetailsWidget::leaveEvent(QEvent * event)
    {
        QWidget::leaveEvent(event);
        changeHoverState(false);
    }

    void DetailsWidget::setSummaryText(const QString &text)
    {
        d->m_summaryLabel->setText(text);
    }

    QString DetailsWidget::summaryText() const
    {
        return d->m_summaryLabel->text();
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
        updateControls();
    }

    void DetailsWidget::setExpanded(bool expanded)
    {
        setState(expanded ? Expanded : Collapsed);
    }

    void DetailsWidget::updateControls()
    {
        if (d->m_widget)
            d->m_widget->setVisible(d->m_state == Expanded || d->m_state == NoSummary);
        d->m_detailsButton->setChecked(d->m_state == Expanded && d->m_widget);
        //d->m_summaryLabel->setEnabled(d->m_state == Collapsed && d->m_widget);
        d->m_detailsButton->setVisible(d->m_state != NoSummary);
        d->m_summaryLabel->setVisible(d->m_state != NoSummary);
        {
            QWidget *w = this;
            while (w) {
                if (w->layout())
                    w->layout()->activate();
                if (QScrollArea *area = qobject_cast<QScrollArea*>(w)) {
                    QEvent e(QEvent::LayoutRequest);
                    QCoreApplication::sendEvent(area, &e);
                }
                w = w->parentWidget();
            }
        }
    }

    QWidget *DetailsWidget::widget() const
    {
        return d->m_widget;
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
            d->m_grid->addWidget(d->m_widget, 1, 1, 1, 2);
        }
        updateControls();
    }

    void DetailsWidget::setToolWidget(QWidget *widget)
    {
        if (d->m_toolWidget == widget)
            return;

        d->m_toolWidget = widget;

        if (!d->m_toolWidget)
            return;

        d->m_toolWidget->adjustSize();
        d->m_grid->addWidget(d->m_toolWidget, 0, 0, 1, 1, Qt::AlignCenter);

        d->m_grid->setColumnMinimumWidth(0, d->m_toolWidget->width());
        d->m_grid->setRowMinimumHeight(0, d->m_toolWidget->height());

        changeHoverState(d->m_hovered);
    }

    QWidget *DetailsWidget::toolWidget() const
    {
        return d->m_toolWidget;
    }

    QPixmap DetailsWidget::cacheBackground(const QSize &size, bool expanded)
    {
        QPixmap pixmap(size);
        pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);

        QRect topRect(0, 0, size.width(), d->m_summaryLabel->height());
        QRect fullRect(0, 0, size.width(), size.height());
        p.fillRect(fullRect, QColor(255, 255, 255, 40));

        QColor highlight = palette().highlight().color();
        highlight.setAlpha(0.5);
        if (expanded) {
            p.fillRect(topRect, highlight);
        }

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
        p.setPen(QPen(palette().color(QPalette::Mid)));

        return pixmap;
    }

    void DetailsWidget::changeHoverState(bool hovered)
    {
        if (!d->m_toolWidget)
            return;

        d->m_toolWidget->setVisible(hovered);

        d->m_hovered = hovered;
    }

} // namespace Utils
