#ifndef DETAILSWIDGET_H
#define DETAILSWIDGET_H

#include "utils_global.h"

#include <QtGui/QWidget>
#include <QtGui/QLabel>
#include <QtGui/QGridLayout>

namespace Utils {
class DetailsButton;

class QTCREATOR_UTILS_EXPORT DetailsWidget : public QWidget
{
    Q_OBJECT
public:
    DetailsWidget(QWidget *parent = 0);
    ~DetailsWidget();

    void setSummaryText(const QString &text);
    void setWidget(QWidget *widget);
    void setToolWidget(QWidget *widget);
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
