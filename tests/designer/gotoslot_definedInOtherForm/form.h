#pragma once

#include <QWidget>

class Form : public QWidget
{
    Q_OBJECT

public:
    explicit Form(QWidget *parent = 0);

private:
    void onPushButtonClicked();
};
