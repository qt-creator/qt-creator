#ifndef QMLAPPLICATIONVIEW_H
#define QMLAPPLICATIONVIEW_H

#ifdef QMLINSPECTOR
#include <qdeclarativedesignview.h>
class QmlApplicationView : public QmlViewer::QDeclarativeDesignView
#else // QMLINSPECTOR
#include <QtDeclarative/QDeclarativeView>
class QmlApplicationView : public QDeclarativeView
#endif // QMLINSPECTOR
{
public:
    enum Orientation {
        LockPortrait,
        LockLandscape,
        Auto
    };

    QmlApplicationView(QWidget *parent = 0);
    virtual ~QmlApplicationView();

    void setMainQmlFile(const QString &file);
    void addImportPath(const QString &path);
    void setOrientation(Orientation orientation);
    void setLoadDummyData(bool loadDummyData);

private:
    class QmlApplicationViewPrivate *m_d;
};

#endif // QMLAPPLICATIONVIEW_H
