#ifndef QMLAPPLICATIONVIEWER_H
#define QMLAPPLICATIONVIEWER_H

#include <QtDeclarative/QDeclarativeView>

class QmlApplicationViewer : public QDeclarativeView
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
    void show();

private:
    class QmlApplicationViewerPrivate *m_d;
};

#endif // QMLAPPLICATIONVIEWER_H
