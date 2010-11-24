#ifndef NODEINSTANCECLIENTINTERFACE_H
#define NODEINSTANCECLIENTINTERFACE_H

#include <QtGlobal>

namespace QmlDesigner {

class ValuesChangedCommand;
class PixmapChangedCommand;
class InformationChangedCommand;

class NodeInstanceClientInterface
{
public:
    virtual void informationChanged(const InformationChangedCommand &command) = 0;
    virtual void valuesChanged(const ValuesChangedCommand &command) = 0;
    virtual void pixmapChanged(const PixmapChangedCommand &command) = 0;
    virtual void flush() {};
    virtual qint64 bytesToWrite() const {return 0;}

};

}

#endif // NODEINSTANCECLIENTINTERFACE_H
