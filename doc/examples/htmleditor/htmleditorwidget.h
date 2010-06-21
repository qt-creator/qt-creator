#ifndef RTEDITORWIDGET_H
#define RTEDITORWIDGET_H

#include<QTabWidget>

struct HTMLEditorWidgetData;
class HTMLEditorWidget:public QTabWidget
{
    Q_OBJECT

public:
    HTMLEditorWidget(QWidget* parent = 0);
    ~HTMLEditorWidget();

    void setContent(const QByteArray& ba, const QString& path=QString());
    QByteArray content() const;

    QString title() const;

protected slots:
    void slotCurrentTabChanged(int tab);
    void slotContentModified();

signals:
    void contentModified();
    void titleChanged(const QString&);

private:
    HTMLEditorWidgetData* d;
};

#endif // RTEDITORWIDGET_H
