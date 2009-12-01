#ifndef DETAILSWIDGET_H
#define DETAILSWIDGET_H

#include "utils_global.h"

#include <QtGui/QPixmap>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QGridLayout;
QT_END_NAMESPACE

namespace Utils {
class DetailsButton;

class QTCREATOR_UTILS_EXPORT DetailsWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString summaryText READ summaryText WRITE setSummaryText DESIGNABLE true)
    Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded DESIGNABLE true)

public:
    DetailsWidget(QWidget *parent = 0);
    ~DetailsWidget();

    void setSummaryText(const QString &text);
    QString summaryText() const;

    bool isExpanded() const;

    void setWidget(QWidget *widget);
    QWidget *widget() const;

    void setToolWidget(QWidget *widget);
    QWidget *toolWidget() const;

public slots:
    void setExpanded(bool);

protected:
    void paintEvent(QPaintEvent *paintEvent);
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);

private:
    QPixmap cacheBackground(const QSize &size, bool expanded);
    void changeHoverState(bool hovered);

    DetailsButton *m_detailsButton;
    QGridLayout *m_grid;
    QLabel *m_summaryLabel;
    QWidget *m_toolWidget;
    QWidget *m_widget;

    QPixmap m_collapsedPixmap;
    QPixmap m_expandedPixmap;

    bool m_hovered;
};
}

#endif // DETAILSWIDGET_H
