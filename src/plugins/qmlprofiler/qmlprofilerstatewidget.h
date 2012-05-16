#ifndef QMLPROFILERSTATEWIDGET_H
#define QMLPROFILERSTATEWIDGET_H

#include <QWidget>

#include "qmlprofilerstatemanager.h"
#include "qmlprofilerdatamodel.h"

namespace QmlProfiler {
namespace Internal {

class QmlProfilerStateWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QmlProfilerStateWidget(QmlProfilerStateManager *stateManager,
                                    QmlProfilerDataModel *dataModel, QWidget *parent = 0);
    ~QmlProfilerStateWidget();

private slots:
    void updateDisplay();
    void dataStateChanged();
    void profilerStateChanged();
    void reposition();

protected:
    void paintEvent(QPaintEvent *event);

private:
    class QmlProfilerStateWidgetPrivate;
    QmlProfilerStateWidgetPrivate *d;
};

}
}

#endif // QMLPROFILERSTATEWIDGET_H
