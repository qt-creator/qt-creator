#ifndef DIRNAVIGATIONFACTORY_H
#define DIRNAVIGATIONFACTORY_H

#include <coreplugin/inavigationwidgetfactory.h>

class DirNavigationFactory : public Core::INavigationWidgetFactory
{
public:
    DirNavigationFactory(){ }
    ~DirNavigationFactory() { }
    Core::NavigationView createWidget();
    QString displayName();
};

#endif // DIRNAVIGATIONFACTORY_H
