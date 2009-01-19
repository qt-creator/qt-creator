#include "%INCLUDE%"
#include "%UI_HDR%"

%CLASS%::%CLASS%(QWidget *parent)
    : %BASECLASS%(parent), ui(new Ui::%CLASS%Class)
{
    ui->setupUi(this);
}

%CLASS%::~%CLASS%()
{
    delete ui;
}
