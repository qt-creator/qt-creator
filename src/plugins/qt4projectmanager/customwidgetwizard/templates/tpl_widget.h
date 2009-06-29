#ifndef @WIDGET_INCLUDE_GUARD@
#define @WIDGET_INCLUDE_GUARD@

#include <QtGui/@WIDGET_BASE_CLASS@>

class @WIDGET_CLASS@ : public @WIDGET_BASE_CLASS@
{
    Q_OBJECT

public:
    @WIDGET_CLASS@(QWidget *parent = 0);
};

#endif
