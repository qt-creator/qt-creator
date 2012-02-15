#include "%ObjectName:l%.%CppHeaderSuffix%"

#include <qdeclarative.h>

%ObjectName%::%ObjectName%(QDeclarativeItem *parent):
        QDeclarativeItem(parent)
{
    // By default, QDeclarativeItem does not draw anything. If you subclass
    // QDeclarativeItem to create a visual item, you will need to uncomment the
    // following line:

    // setFlag(ItemHasNoContents, false);
}

%ObjectName%::~%ObjectName%()
{
}
