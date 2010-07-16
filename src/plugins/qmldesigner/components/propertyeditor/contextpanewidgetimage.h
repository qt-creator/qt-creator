#ifndef CONTEXTPANEWIDGETIMAGE_H
#define CONTEXTPANEWIDGETIMAGE_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
    class ContextPaneWidgetImage;
}
QT_END_NAMESPACE

namespace QmlJS {
    class PropertyReader;
}

namespace QmlDesigner {

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
    void setPixmap(const QString &fileName);

protected:
    void changeEvent(QEvent *e);

private:
    Ui::ContextPaneWidgetImage *ui;
    QString m_path;
};

} //QmlDesigner

#endif // CONTEXTPANEWIDGETIMAGE_H
