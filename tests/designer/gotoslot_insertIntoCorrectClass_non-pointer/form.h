// Copyright header

#pragma once

#include "ui_form.h"

#include <QWidget>

class Form;
struct MyClass
{
    Form *form;
};

class Form : public QWidget
{
    Q_OBJECT

public:
    explicit Form(QWidget *parent = 0);

private:
    Ui::Form ui;
};
