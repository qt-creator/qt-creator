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

class ContextPaneWidget : public QFrame
{
    Q_OBJECT

public:
    explicit ContextPaneWidget(QWidget *parent = 0);
    ~ContextPaneWidget();
    void activate(const QPoint &pos, const QPoint &alternative);
    void rePosition(const QPoint &pos, const QPoint &alternative);
    void deactivate();
    BauhausColorDialog *colorDialog();
    void setProperties(QmlJS::PropertyReader *propertyReader);
    bool setType(const QString &typeName);
    QWidget* currentWidget() const { return m_currentWidget; }

public slots:
    void onTogglePane();
    void onShowColorDialog(bool, const QPoint &);

signals:
    void propertyChanged(const QString &, const QVariant &);
    void removeProperty(const QString &);
    void removeAndChangeProperty(const QString &, const QString &, const QVariant &);

private slots:
    void onDisable();

protected:
    QWidget *createFontWidget();
    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);

private:
    QWidget *m_currentWidget;
    ContextPaneTextWidget *m_textWidget;
    QPoint m_oldPos;
    QGraphicsDropShadowEffect *m_dropShadowEffect;
    QGraphicsOpacityEffect *m_opacityEffect;
    QWeakPointer<BauhausColorDialog> m_bauhausColorDialog;
    QString m_colorName;
    int m_xPos;
};

} //QmlDesigner

#endif // CONTEXTPANEWIDGET_H
