#ifndef %PRE_DEF%
#define %PRE_DEF%

#include <%BASECLASS%>

namespace Ui
{
    class %CLASS%;
}

class %CLASS% : public %BASECLASS%
{
    Q_OBJECT

public:
    %CLASS%(QWidget *parent = 0);
    ~%CLASS%();

private:
    Ui::%CLASS% *ui;
};

#endif // %PRE_DEF%
