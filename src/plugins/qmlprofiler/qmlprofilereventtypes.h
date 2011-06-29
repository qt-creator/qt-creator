#ifndef QMLPROFILEREVENTTYPES_H
#define QMLPROFILEREVENTTYPES_H

namespace QmlProfiler {
namespace Internal {

enum QmlEventType {
    Painting,
    Compiling,
    Creating,
    Binding,
    HandlingSignal,

    MaximumQmlEventType
};

}
}

#endif //QMLPROFILEREVENTTYPES_H
