#ifndef DETAILSWIDGET_H
#define DETAILSWIDGET_H

#include "utils_global.h"

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
    Q_PROPERTY(bool expanded READ expanded WRITE setExpanded DESIGNABLE true)
public:
    DetailsWidget(QWidget *parent = 0);
    ~DetailsWidget();

    void setSummaryText(const QString &text);
    QString summaryText() const;

    bool expanded() const;
    void setExpanded(bool);

    void setWidget(QWidget *widget);
    QWidget *widget() const;

    void setToolWidget(QWidget *widget);
    QWidget *toolWidget() const;

protected:
    void paintEvent(QPaintEvent *paintEvent);

private slots:
    void detailsButtonClicked();

private:
    void fixUpLayout();
    QLabel *m_summaryLabel;
    DetailsButton *m_detailsButton;

    QWidget *m_widget;
    QWidget *m_toolWidget;
    QGridLayout *m_grid;
};
}

#endif // DETAILSWIDGET_H
