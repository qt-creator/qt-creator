#pragma once

#include <QApplication>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

class ApplicationWindow : public QWidget
{
public:
    ApplicationWindow()
    {
        resize(600, 400);
        setWindowTitle("Hello World");

        auto textEdit = new QTextEdit;
        textEdit->setText("Hallo");

        auto pushButton = new QPushButton("Quit");

        auto l = new QVBoxLayout(this);
        l->addWidget(textEdit);
        l->addWidget(pushButton);

        connect(pushButton, &QPushButton::clicked,
                qApp, &QApplication::quit);
    }
};
