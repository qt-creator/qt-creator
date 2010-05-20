#ifndef DECLARATIVEWIDGETVIEW_H
#define DECLARATIVEWIDGETVIEW_H

#include <QWidget>
#include <QUrl>

QT_BEGIN_NAMESPACE
class QDeclarativeEngine;
class QDeclarativeContext;
class QDeclarativeError;
QT_END_NAMESPACE

namespace QmlDesigner {

class DeclarativeWidgetViewPrivate;

class DeclarativeWidgetView : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QUrl source READ source WRITE setSource DESIGNABLE true)
public:
    explicit DeclarativeWidgetView(QWidget *parent = 0);

    virtual ~DeclarativeWidgetView();

    QUrl source() const;
    void setSource(const QUrl&);

    QDeclarativeEngine* engine();
    QDeclarativeContext* rootContext();

    QWidget *rootWidget() const;

    enum Status { Null, Ready, Loading, Error };
    Status status() const;

signals:
    void statusChanged(DeclarativeWidgetView::Status);

protected:
    virtual void setRootWidget(QWidget *);

private Q_SLOTS:
    void continueExecute();

private:
     friend class DeclarativeWidgetViewPrivate;
     DeclarativeWidgetViewPrivate *d;

};

} //QmlDesigner

#endif // DECLARATIVEWIDGETVIEW_H
