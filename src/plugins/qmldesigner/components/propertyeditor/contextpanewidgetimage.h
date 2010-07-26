#ifndef CONTEXTPANEWIDGETIMAGE_H
#define CONTEXTPANEWIDGETIMAGE_H

#include <QWidget>
#include <QFrame>
#include <contextpanewidget.h>

QT_BEGIN_NAMESPACE
namespace Ui {
    class ContextPaneWidgetImage;
}
class QLabel;
QT_END_NAMESPACE

namespace QmlJS {
    class PropertyReader;
}

namespace QmlDesigner {

class PreviewDialog : public DragWidget
{
    Q_OBJECT

public:
    PreviewDialog(QWidget *parent = 0);
    void setPixmap(const QPixmap &p);
public slots:
    void onTogglePane();

private:
    QLabel * m_label;
};

class ContextPaneWidgetImage : public QWidget
{
    Q_OBJECT

public:
    explicit ContextPaneWidgetImage(QWidget *parent = 0);
    ~ContextPaneWidgetImage();
    void setProperties(QmlJS::PropertyReader *propertyReader);
    void setPath(const QString& path);

signals:
    void propertyChanged(const QString &, const QVariant &);
    void removeProperty(const QString &);
    void removeAndChangeProperty(const QString &, const QString &, const QVariant &, bool removeFirst);

public slots:
    void onStretchChanged();
    void onFileNameChanged();
    void onPixmapDoubleClicked();
    void setPixmap(const QString &fileName);

protected:
    void changeEvent(QEvent *e);

private:
    Ui::ContextPaneWidgetImage *ui;
    QString m_path;
    PreviewDialog *m_previewDialog;
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




} //QmlDesigner

#endif // CONTEXTPANEWIDGETIMAGE_H
