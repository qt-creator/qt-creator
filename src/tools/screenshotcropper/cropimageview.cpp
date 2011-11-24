#include "cropimageview.h"
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>

CropImageView::CropImageView(QWidget *parent)
    : QWidget(parent)
{
}

void CropImageView::mousePressEvent(QMouseEvent *event)
{
    setArea(QRect(event->pos(), m_area.bottomRight()));
    update();
}

void CropImageView::mouseMoveEvent(QMouseEvent *event)
{
    setArea(QRect(m_area.topLeft(), event->pos()));
    update();
}

void CropImageView::mouseReleaseEvent(QMouseEvent *event)
{
    mouseMoveEvent(event);
}

void CropImageView::setImage(const QImage &image)
{
    m_image = image;
    setMinimumSize(image.size());
    update();
}

void CropImageView::setArea(const QRect &area)
{
    m_area = m_image.rect().intersected(area);
    emit cropAreaChanged(m_area);
    update();
}

void CropImageView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);

    if (!m_image.isNull())
        p.drawImage(0, 0, m_image);

    if (!m_area.isNull()) {
        p.setPen(Qt::white);
        p.drawRect(m_area);
        QPen redDashes;
        redDashes.setColor(Qt::red);
        redDashes.setStyle(Qt::DashLine);
        p.setPen(redDashes);
        p.drawRect(m_area);
    }
}

