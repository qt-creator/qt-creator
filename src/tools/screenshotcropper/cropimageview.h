#ifndef CROPIMAGEVIEW_H
#define CROPIMAGEVIEW_H

#include <QWidget>

class CropImageView : public QWidget
{
    Q_OBJECT

public:
    explicit CropImageView(QWidget *parent = 0);

    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

public slots:
    void setImage(const QImage &image);
    void setArea(const QRect &area);

signals:
    void cropAreaChanged(const QRect &area);

private:
    QImage m_image;
    QRect m_area;
};

#endif // CROPIMAGEVIEW_H
