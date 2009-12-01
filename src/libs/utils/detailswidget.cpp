#include "detailswidget.h"
#include "detailsbutton.h"

#include <QtGui/QGridLayout>
#include <QtCore/QStack>
#include <QtGui/QLabel>
#include <QtGui/QGridLayout>
#include <QtGui/QPainter>

using namespace Utils;

namespace {
const int MARGIN=8;
}

// This widget is using a grid layout and places the items
// in the following way:
//
// +------------+-------------------------+---------------+
// + toolWidget | summaryLabel            | detailsButton |
// +------------+-------------------------+---------------+
// |            | widget                                  |
// +------------+-------------------------+---------------+

DetailsWidget::DetailsWidget(QWidget *parent) :
    QWidget(parent),
    m_detailsButton(new DetailsButton(this)),
    m_grid(new QGridLayout(this)),
    m_summaryLabel(new QLabel(this)),
    m_toolWidget(0),
    m_widget(0),
    m_hovered(false)
{
    m_summaryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_summaryLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_summaryLabel->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);

    m_grid->setContentsMargins(0, 0, 0, 0);
    m_grid->setSpacing(0);
    m_grid->addWidget(m_summaryLabel, 0, 1);
    m_grid->addWidget(m_detailsButton, 0, 2);

    m_detailsButton->setEnabled(false);

    connect(m_detailsButton, SIGNAL(toggled(bool)),
            this, SLOT(setExpanded(bool)));
}

DetailsWidget::~DetailsWidget()
{

}

void DetailsWidget::paintEvent(QPaintEvent *paintEvent)
{
    QWidget::paintEvent(paintEvent);

    QPainter p(this);

    const QRect paintArea(m_summaryLabel->geometry().topLeft(),
                          contentsRect().bottomRight());

    if (!isExpanded()) {
        if (m_collapsedPixmap.isNull() ||
            m_collapsedPixmap.size() != size())
            m_collapsedPixmap = cacheBackground(paintArea.size(), false);
        p.drawPixmap(paintArea, m_collapsedPixmap);
    } else {
        if (m_expandedPixmap.isNull() ||
            m_expandedPixmap.size() != size())
            m_expandedPixmap = cacheBackground(paintArea.size(), true);
        p.drawPixmap(paintArea, m_expandedPixmap);
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
    m_summaryLabel->setText(text);
}

QString DetailsWidget::summaryText() const
{
    return m_summaryLabel->text();
}

bool DetailsWidget::isExpanded() const
{
    if (!m_widget)
        return false;
    return m_widget->isVisible();
}

void DetailsWidget::setExpanded(bool visible)
{
    if (!m_widget)
        return;

    m_summaryLabel->setEnabled(!visible);
    m_widget->setVisible(visible);
    m_detailsButton->setChecked(visible);
}

QWidget *DetailsWidget::widget() const
{
    return m_widget;
}

void DetailsWidget::setWidget(QWidget *widget)
{
    if (m_widget == widget)
        return;

    const bool wasExpanded(isExpanded());

    if (m_widget)
        m_grid->removeWidget(m_widget);
    m_widget = widget;

    if (widget) {
        m_widget->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
        m_grid->addWidget(widget, 1, 1, 1, 2);
        setExpanded(wasExpanded);
    } else {
        m_detailsButton->setEnabled(false);
    }
    m_detailsButton->setEnabled(0 != m_widget);
}

void DetailsWidget::setToolWidget(QWidget *widget)
{
    if (m_toolWidget == widget)
        return;

    m_toolWidget = widget;

    if (!m_toolWidget)
        return;

    m_toolWidget->adjustSize();
    m_grid->addWidget(m_toolWidget, 0, 0, 1, 1, Qt::AlignCenter);

    m_grid->setColumnMinimumWidth(0, m_toolWidget->width());
    m_grid->setRowMinimumHeight(0, m_toolWidget->height());

    changeHoverState(m_hovered);
}

QWidget *DetailsWidget::toolWidget() const
{
    return m_toolWidget;
}

QPixmap DetailsWidget::cacheBackground(const QSize &size, bool expanded)
{
    QLinearGradient lg;
    lg.setCoordinateMode(QGradient::ObjectBoundingMode);
    lg.setFinalStop(0, 1);

    lg.setColorAt(0, palette().color(QPalette::Midlight));
    lg.setColorAt(1, palette().color(QPalette::Button));

    QPixmap pixmap(size);
    QPainter p(&pixmap);
    p.setBrush(lg);
    p.setPen(QPen(palette().color(QPalette::Mid)));

    p.drawRect(0, 0, size.width() - 1, size.height() - 1);

    if (expanded) {
        p.drawLine(0, m_summaryLabel->height() - 1,
                   m_summaryLabel->width(), m_summaryLabel->height() - 1);
    }

    return pixmap;
}

void DetailsWidget::changeHoverState(bool hovered)
{
    m_hovered = hovered;
    if (!m_toolWidget)
        return;

    m_toolWidget->setVisible(m_hovered);
}
