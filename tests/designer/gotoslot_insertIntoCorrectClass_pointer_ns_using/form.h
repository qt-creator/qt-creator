// Copyright header

#ifndef N_FORM_H
#define N_FORM_H

#include <QWidget>

namespace N {
namespace Ui {
class Form;
}
}

using namespace N;

class Form : public QWidget
{
    Q_OBJECT

public:
    explicit Form(QWidget *parent = 0);
    ~Form();

private:
    Ui::Form *ui;
};

#endif // N_FORM_H
