#include "%ProjectName%.hpp"

#include <bb/cascades/Application>
#include <Qt/qdeclarativedebug.h>

Q_DECL_EXPORT int main(int argc, char **argv)
{
    bb::cascades::Application app(argc, argv);

    new %ProjectName%(&app);

    return bb::cascades::Application::exec();
}
