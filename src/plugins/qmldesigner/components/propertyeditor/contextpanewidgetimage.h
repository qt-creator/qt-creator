#ifndef CONTEXTPANEWIDGETIMAGE_H
#define CONTEXTPANEWIDGETIMAGE_H

#include <QWidget>
#include <QFrame>
#include <QLabel>
#include <qdrawutil.h>
#include <contextpanewidget.h>

QT_BEGIN_NAMESPACE
namespace Ui {
    class ContextPaneWidgetImage;
    class ContextPaneWidgetBorderImage;
}
class QLabel;
class QSlider;
class FileWidget;
QT_END_NAMESPACE

namespace QmlJS {
    class PropertyReader;
}

namespace QmlDesigner {    

class PreviewLabel : public QLabel
{
    Q_OBJECT

public:
    PreviewLabel(QWidget *parent = 0);
    void setZoom(int);
    void setIsBorderImage(bool b);
    void setMargins(int left, int top, int right, int bottom);
    int leftMarging() const { return m_left; }
    int topMarging() const { return m_top; }
    int rightMarging() const { return m_right; }
    int bottomMarging() const { return m_bottom; }

signals:
    void leftMarginChanged();
    void topMarginChanged();
    void bottomMarginChanged();
    void rightMarginChanged();

protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);
    void leaveEvent(QEvent* event );
private:
    bool m_showBorders;
    int m_left, m_right, m_top, m_bottom;
    bool m_dragging_left;
    bool m_dragging_right;
    bool m_dragging_top;
    bool m_dragging_bottom;
    QPoint m_startPos;
    int m_zoom;
    bool m_borderImage;
};

class PreviewDialog : public DragWidget
{
    Q_OBJECT

public:
    PreviewDialog(QWidget *parent = 0);
    void setPixmap(const QPixmap &p, int zoom = 1);
    void setZoom(int z);
    void setIsBorderImage(bool b);
    PreviewLabel *previewLabel() const;
    int zoom() { return m_zoom; }

public slots:
    void onTogglePane();

protected:
    void wheelEvent(QWheelEvent* event);

private:
    PreviewLabel *m_label;
    QSlider *m_slider;
    int m_zoom;
    QPixmap m_pixmap;
    bool m_borderImage;
};

class ContextPaneWidgetImage : public QWidget
{
    Q_OBJECT

public:
    explicit ContextPaneWidgetImage(QWidget *parent = 0, bool borderImage = false);
    ~ContextPaneWidgetImage();
    void setProperties(QmlJS::PropertyReader *propertyReader);
    void setPath(const QString& path);   
    PreviewDialog* previewDialog();

signals:
    void propertyChanged(const QString &, const QVariant &);
    void removeProperty(const QString &);
    void removeAndChangeProperty(const QString &, const QString &, const QVariant &, bool removeFirst);

public slots:
    void onStretchChanged();
    void onVerticalStretchChanged();
    void onHorizontalStretchChanged();
    void onFileNameChanged();
    void onPixmapDoubleClicked();
    void setPixmap(const QString &fileName);
    void onLeftMarginsChanged();
    void onTopMarginsChanged();
    void onBottomMarginsChanged();
    void onRightMarginsChanged();

protected:
    void changeEvent(QEvent *e);
    void hideEvent(QHideEvent* event);
    void showEvent(QShowEvent* event);

private:
    Ui::ContextPaneWidgetImage *ui;
    Ui::ContextPaneWidgetBorderImage *uiBorderImage;
    QString m_path;
    QWeakPointer<PreviewDialog> m_previewDialog;    
    FileWidget *m_fileWidget;
    QLabel *m_sizeLabel;
    bool m_borderImage;
    bool previewWasVisible;
};

class LabelFilter: public QObject {

    Q_OBJECT
public:
    LabelFilter(QObject* parent =0) : QObject(parent) {}
signals:
    void doubleClicked();
protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

class WheelFilter: public QObject {

    Q_OBJECT
public:
    WheelFilter(QObject* parent =0) : QObject(parent) {}
    void setTarget(QObject *target) { m_target = target; }
protected:
    bool eventFilter(QObject *obj, QEvent *event);
    QObject *m_target;
};





} //QmlDesigner

#endif // CONTEXTPANEWIDGETIMAGE_H
