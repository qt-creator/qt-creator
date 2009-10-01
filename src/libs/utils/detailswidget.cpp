#include "detailswidget.h"
#include "detailsbutton.h"

#include <QtGui/QGridLayout>
#include <QtCore/QStack>
#include <QtGui/QPainter>

using namespace Utils;

DetailsWidget::DetailsWidget(QWidget *parent)
    : QWidget(parent),
      m_widget(0),
      m_toolWidget(0)
{
    m_grid = new QGridLayout(this);
    //m_grid->setMargin(0);
    m_summaryLabel = new QLabel(this);
    m_summaryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_summaryLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_detailsButton = new DetailsButton(this);

    m_grid->addWidget(m_summaryLabel, 0, 0, 2, 0);
    m_grid->addWidget(m_detailsButton, 1, 2);

    connect(m_detailsButton, SIGNAL(clicked()),
            this, SLOT(detailsButtonClicked()));
}

DetailsWidget::~DetailsWidget()
{

}

void DetailsWidget::paintEvent(QPaintEvent *paintEvent)
{
    //TL-->                 ___________  <-- TR
    //                     |           |
    //ML->   ______________| <--MM     | <--MR
    //       |                         |
    //BL->   |_________________________| <-- BR


    QWidget::paintEvent(paintEvent);

    if (!m_detailsButton->isToggled())
        return;

    QRect detailsGeometry = m_detailsButton->geometry();
    QRect widgetGeometry = m_widget->geometry();

    QPoint tl(detailsGeometry.topLeft());
    tl += QPoint(-3, -3);

    QPoint tr(detailsGeometry.topRight());
    tr += QPoint(3, -3);

    QPoint mm(detailsGeometry.left() - 3, widgetGeometry.top() - 3);

    QPoint ml(1, mm.y());

    QPoint mr(tr.x(), mm.y());

    int bottom = geometry().height() - 3;
    QPoint bl(1, bottom);
    QPoint br(tr.x(), bottom);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);

    p.setBrush(palette().dark());
    p.drawRoundedRect(QRect(tl, br), 5, 5);
    p.drawRoundedRect(QRect(ml, br), 5, 5);
}

void DetailsWidget::detailsButtonClicked()
{
    if (m_widget)
        m_widget->setVisible(m_detailsButton->isToggled());
    fixUpLayout();
}

void DetailsWidget::setSummaryText(const QString &text)
{
    m_summaryLabel->setText(text);
}

void DetailsWidget::setWidget(QWidget *widget)
{
    if (m_widget == widget)
        return;
    if (m_widget)
        m_grid->removeWidget(m_widget);
    m_grid->addWidget(widget, 2, 0, 1, 3);
    m_widget = widget;
    m_widget->setVisible(m_detailsButton->isToggled());
}

void DetailsWidget::setToolWidget(QWidget *widget)
{
    if (m_toolWidget == widget)
        return;
    if (m_toolWidget)
        m_grid->removeWidget(m_toolWidget);
    m_grid->addWidget(widget, 1, 1);
    m_toolWidget = widget;
}

void DetailsWidget::fixUpLayout()
{
    QWidget *parent = m_widget;
    QStack<QWidget *> widgets;
    while((parent = parent->parentWidget()) && parent && parent->layout()) {
        widgets.push(parent);
        parent->layout()->update();
    }

    while(!widgets.isEmpty()) {
        widgets.pop()->layout()->activate();
    }
}
