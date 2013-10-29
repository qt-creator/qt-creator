#ifndef APPLICATIONUI_H
#define APPLICATIONUI_H

#include <QObject>

namespace bb {
    namespace cascades {
        class Application;
    }
}

class ApplicationUI : public QObject
{
    Q_OBJECT
public:
    ApplicationUI(bb::cascades::Application *app);
    virtual ~ApplicationUI() {}
};

#endif /* APPLICATIONUI_H */
