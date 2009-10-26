#ifndef IWELCOMEPAGE_H
#define IWELCOMEPAGE_H


#include "utils_global.h"

#include <QObject>

namespace Utils {

class IWelcomePagePrivate;

class QTCREATOR_UTILS_EXPORT IWelcomePage : public QObject
{
    Q_OBJECT

public:
    IWelcomePage();
    virtual ~IWelcomePage();

    virtual QWidget *page() = 0;
    virtual QString title() const = 0;
    virtual int priority() const { return 0; }

private:
    // not used atm
    IWelcomePagePrivate *m_d;
};

}

#endif // IWELCOMEPAGE_H
