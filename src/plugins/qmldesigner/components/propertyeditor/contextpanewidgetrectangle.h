#ifndef CONTEXTPANEWIDGETRECTANGLE_H
#define CONTEXTPANEWIDGETRECTANGLE_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
    class ContextPaneWidgetRectangle;
}
QT_END_NAMESPACE

namespace QmlJS {
    class PropertyReader;
}

namespace QmlDesigner {

class ContextPaneWidgetRectangle : public QWidget
{
    Q_OBJECT

public:
    explicit ContextPaneWidgetRectangle(QWidget *parent = 0);
    ~ContextPaneWidgetRectangle();
    void setProperties(QmlJS::PropertyReader *propertyReader);

public slots:
    void onBorderColorButtonToggled(bool);
    void onColorButtonToggled(bool);
    void onColorDialogApplied(const QColor &color);
    void onColorDialogCancled();
    void onGradientClicked();
    void onColorNoneClicked();
    void onColorSolidClicked();
    void onBorderNoneClicked();
    void onBorderSolidClicked();
    void onGradientLineDoubleClicked(const QPoint &);
    void onUpdateGradient();

signals:
    void propertyChanged(const QString &, const QVariant &);
    void removeProperty(const QString &);
    void removeAndChangeProperty(const QString &, const QString &, const QVariant &, bool removeFirst);

protected:
    void changeEvent(QEvent *e);
    void timerEvent(QTimerEvent *event);

private:
    void setColor();
    Ui::ContextPaneWidgetRectangle *ui;
    bool m_hasBorder;
    bool m_hasGradient;
    bool m_none;
    bool m_gradientLineDoubleClicked;
    int m_gradientTimer;
};

} //QmlDesigner

#endif // CONTEXTPANEWIDGETRECTANGLE_H
