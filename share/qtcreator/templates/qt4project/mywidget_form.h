#ifndef %PRE_DEF%
#define %PRE_DEF%

#include <QtGui/%BASECLASS%>

namespace Ui
{
    class %CLASS%Class;
}

class %CLASS% : public %BASECLASS%
{
    Q_OBJECT

public:
    %CLASS%(QWidget *parent = 0);
    ~%CLASS%();

private:
    Ui::%CLASS%Class *ui;
};

#endif // %PRE_DEF%
