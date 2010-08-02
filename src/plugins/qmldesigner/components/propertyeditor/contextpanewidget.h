#ifndef CONTEXTPANEWIDGET_H
#define CONTEXTPANEWIDGET_H

#include <QFrame>
#include <QVariant>
#include <QGraphicsEffect>
#include <QWeakPointer>

namespace QmlJS {
    class PropertyReader;
}

namespace QmlDesigner {

class BauhausColorDialog;
class ContextPaneTextWidget;
class EasingContextPane;
class ContextPaneWidgetRectangle;
class ContextPaneWidgetImage;

class DragWidget : public QFrame
{
    Q_OBJECT

public:
    explicit DragWidget(QWidget *parent = 0);
    void setSecondaryTarget(QWidget* w)
    { m_secondaryTarget = w; }

protected:
    QPoint m_pos;
    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);

private:
    QGraphicsDropShadowEffect *m_dropShadowEffect;
    QGraphicsOpacityEffect *m_opacityEffect;
    QPoint m_oldPos;
    QWeakPointer<QWidget> m_secondaryTarget;
};

class ContextPaneWidget : public DragWidget
{
    Q_OBJECT

public:
    explicit ContextPaneWidget(QWidget *parent = 0);
    ~ContextPaneWidget();
    void activate(const QPoint &pos, const QPoint &alternative, const QPoint &alternative2);
    void rePosition(const QPoint &pos, const QPoint &alternative , const QPoint &alternative3);
    void deactivate();
    BauhausColorDialog *colorDialog();
    void setProperties(QmlJS::PropertyReader *propertyReader);
    void setPath(const QString &path);
    bool setType(const QString &typeName);
    bool acceptsType(const QString &typeName);
    QWidget* currentWidget() const { return m_currentWidget; }

public slots:
    void onTogglePane();
    void onShowColorDialog(bool, const QPoint &);

signals:
    void propertyChanged(const QString &, const QVariant &);
    void removeProperty(const QString &);
    void removeAndChangeProperty(const QString &, const QString &, const QVariant &, bool);

private slots:
    void onDisable();
    void onResetPosition(bool toggle);

protected:
    QWidget *createFontWidget();
    QWidget *createEasingWidget();
    QWidget *createImageWidget();
    QWidget *createBorderImageWidget();
    QWidget *createRectangleWidget();    

private:
    QWidget *m_currentWidget;
    ContextPaneTextWidget *m_textWidget;
    EasingContextPane *m_easingWidget;
    ContextPaneWidgetImage *m_imageWidget;
    ContextPaneWidgetImage *m_borderImageWidget;
    ContextPaneWidgetRectangle *m_rectangleWidget;
    QWeakPointer<BauhausColorDialog> m_bauhausColorDialog;
    QAction * m_resetAction;
    QString m_colorName;
    QPoint m_originalPos;
};

} //QmlDesigner

#endif // CONTEXTPANEWIDGET_H
