/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "detailswidget.h"
#include "detailsbutton.h"

#include <QtCore/QStack>
#include <QtCore/QPropertyAnimation>

#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QCheckBox>
#include <QtGui/QPainter>
#include <QtGui/QScrollArea>
#include <QtGui/QApplication>

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

    static const int MARGIN=8;

    struct DetailsWidgetPrivate {
        DetailsWidgetPrivate(QWidget *parent);

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

    DetailsWidgetPrivate::DetailsWidgetPrivate(QWidget * parent) :
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
    }

    DetailsWidget::DetailsWidget(QWidget *parent) :
            QWidget(parent),
            d(new DetailsWidgetPrivate(this))
    {
        d->m_summaryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        d->m_summaryLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        d->m_summaryLabel->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);

        d->m_summaryCheckBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        d->m_summaryCheckBox->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
        d->m_summaryCheckBox->setAttribute(Qt::WA_LayoutUsesWidgetRect); /* broken layout on mac otherwise */
        d->m_summaryCheckBox->setVisible(false);

        d->m_additionalSummaryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        d->m_additionalSummaryLabel->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
        d->m_additionalSummaryLabel->setWordWrap(true);
        d->m_additionalSummaryLabel->setVisible(false);

        d->m_grid->setContentsMargins(0, 0, 0, 0);
        d->m_grid->setSpacing(0);
        d->m_grid->addWidget(d->m_summaryLabel, 0, 0);
        d->m_grid->addWidget(d->m_detailsButton, 0, 2);
        d->m_grid->addWidget(d->m_additionalSummaryLabel, 1, 0, 1, 3);
        setLayout(d->m_grid);

        connect(d->m_detailsButton, SIGNAL(toggled(bool)),
                this, SLOT(setExpanded(bool)));
        connect(d->m_summaryCheckBox, SIGNAL(toggled(bool)),
                this, SIGNAL(checked(bool)));
        updateControls();
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
        QWidget *widget = b ? static_cast<QWidget *>(d->m_summaryCheckBox) : static_cast<QWidget *>(d->m_summaryLabel);
        d->m_grid->addWidget(widget, 0, 0);
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
        QPoint topLeft(topLeftWidget->geometry().left(), contentsRect().top());
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
        d->m_summaryCheckBox->setText(text);
    }

    QString DetailsWidget::summaryText() const
    {
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
        d->m_summaryLabel->setVisible(d->m_state != NoSummary && !d->m_useCheckBox);
        d->m_summaryCheckBox->setVisible(d->m_state != NoSummary && d->m_useCheckBox);
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
            d->m_grid->addWidget(d->m_widget, 2, 0, 1, 3);
        }
        updateControls();
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

#ifdef Q_WS_MAC
        d->m_toolWidget->setOpacity(1.0);
#endif
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

        int topHeight = qMax(d->m_detailsButton->height(),
                             d->m_useCheckBox ? d->m_summaryCheckBox->height() : d->m_summaryLabel->height());
        QRect topRect(0, 0, size.width(), topHeight);
        QRect fullRect(0, 0, size.width(), size.height());
#ifdef Q_WS_MAC
        p.fillRect(fullRect, qApp->palette().window().color());
#endif
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
#ifdef Q_OS_MAC
        d->m_toolWidget->setVisible(hovered);
#else
        d->m_toolWidget->fadeTo(hovered ? 1.0 : 0);
#endif
        d->m_hovered = hovered;
    }

} // namespace Utils
