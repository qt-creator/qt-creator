#ifndef QMLAPPLICATIONVIEWER_H
#define QMLAPPLICATIONVIEWER_H

#ifdef QMLINSPECTOR
#include <qdeclarativeviewobserver.h>
class QmlApplicationViewer : public QmlViewer::QDeclarativeViewObserver
#else // QMLINSPECTOR
#include <QtDeclarative/QDeclarativeView>
class QmlApplicationViewer : public QDeclarativeView
#endif // QMLINSPECTOR
{
public:
    enum Orientation {
        LockPortrait,
        LockLandscape,
        Auto
    };

    QmlApplicationViewer(QWidget *parent = 0);
    virtual ~QmlApplicationViewer();

    void setMainQmlFile(const QString &file);
    void addImportPath(const QString &path);
    void setOrientation(Orientation orientation);
    void setLoadDummyData(bool loadDummyData);

private:
    class QmlApplicationViewerPrivate *m_d;
};

#endif // QMLAPPLICATIONVIEWER_H
