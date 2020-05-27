#include "%{ProjectName}.h"

#include <qul/application.h>
#include <qul/qul.h>

int main()
{
    Qul::initPlatform();
    Qul::Application app;
    static %{ProjectName} item;
    app.setRootItem(&item);
    app.exec();
    return 0;
}
