#ifndef QMLAPPLICATIONVIEW_H
#define QMLAPPLICATIONVIEW_H

#include <QtDeclarative/QDeclarativeView>

class QmlApplicationView : public QDeclarativeView
{
public:
    enum Orientation {
        LockPortrait,
        LockLandscape,
        Auto
    };

    QmlApplicationView(const QString &mainQmlFile, QWidget *parent = 0);
    virtual ~QmlApplicationView();

    void setImportPathList(const QStringList &importPaths);
    void setOrientation(Orientation orientation);
    void setLoadDummyData(bool loadDummyData);

private:
    class QmlApplicationViewPrivate *m_d;
};

#endif // QMLAPPLICATIONVIEW_H
