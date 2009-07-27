#ifndef IWELCOMEPAGE_H
#define IWELCOMEPAGE_H


#include "extensionsystem_global.h"

#include <QObject>

namespace ExtensionSystem {

class IWelcomePagePrivate;

class EXTENSIONSYSTEM_EXPORT IWelcomePage : public QObject
{
    Q_OBJECT

public:
    IWelcomePage();

    virtual QWidget *page() = 0;
    virtual QString title() const = 0;
    virtual int priority() const { return 0; }

private:
    // not used atm
    IWelcomePagePrivate *m_d;
};

}

#endif // IWELCOMEPAGE_H
