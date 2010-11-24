#ifndef CHILDRENCHANGEEVENTFILTER_H
#define CHILDRENCHANGEEVENTFILTER_H

#include <QObject>

namespace QmlDesigner {
namespace Internal {

class ChildrenChangeEventFilter : public QObject
{
    Q_OBJECT
public:
    ChildrenChangeEventFilter(QObject *parent);


signals:
    void childrenChanged(QObject *object);

protected:
    bool eventFilter(QObject *object, QEvent *event);

};

} // namespace Internal
} // namespace QmlDesigner

#endif // CHILDRENCHANGEEVENTFILTER_H
