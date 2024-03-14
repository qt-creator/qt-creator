// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <spinner/spinner.h>

#include <QApplication>
#include <QBoxLayout>
#include <QCalendarWidget>
#include <QGroupBox>
#include <QLabel>
#include <QMetaEnum>
#include <QTextEdit>
#include <QToolButton>

using namespace SpinnerSolution;

QGroupBox *createGroupBox(SpinnerSize size, QWidget *widget)
{
    const QMetaEnum spinnerSize = QMetaEnum::fromType<SpinnerSize>();
    QGroupBox *groupBox = new QGroupBox(spinnerSize.valueToKey(int(size)));

    SpinnerWidget *spinnerWidget = new SpinnerWidget;
    QToolButton *startButton = new QToolButton;
    startButton->setText("Start");
    QToolButton *stopButton = new QToolButton;
    stopButton->setText("Stop");

    QVBoxLayout *mainLayout = new QVBoxLayout(groupBox);
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addWidget(spinnerWidget);
    topLayout->addWidget(startButton);
    topLayout->addWidget(stopButton);
    topLayout->addStretch();
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(widget);

    Spinner *spinner = new Spinner(size, widget);
    spinner->hide(); // TODO: make the default hidden?

    QObject::connect(startButton, &QAbstractButton::clicked, groupBox, [=] {
        spinnerWidget->setState(SpinnerState::Running);
        spinner->show();
        widget->setEnabled(false);
    });
    QObject::connect(stopButton, &QAbstractButton::clicked, groupBox, [=] {
        spinnerWidget->setState(SpinnerState::NotRunning);
        spinner->hide();
        widget->setEnabled(true);
    });
    return groupBox;
}

QGroupBox *createColorsGroupBox()
{
    QGroupBox *groupBox = new QGroupBox("Spinner::setColor(const QColor &color)");
    auto layout = new QHBoxLayout(groupBox);
    for (auto color : {Qt::red, Qt::darkGreen, Qt::blue, Qt::darkYellow}) {
        auto widget = new QWidget;
        widget->setFixedSize(30, 30);
        auto spinner = new Spinner(SpinnerSize::Medium, widget);
        spinner->setColor(color);
        layout->addWidget(widget);
    }
    return groupBox;
}

static QWidget *hr()
{
    auto frame = new QFrame;
    frame->setFrameShape(QFrame::HLine);
    frame->setFrameShadow(QFrame::Sunken);
    return frame;
}

static QString pangram(int count)
{
    return QStringList(count, "The quick brown fox jumps over the lazy dog.").join(' ');
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget mainWidget;
    mainWidget.setWindowTitle("Spinner Example");

    QVBoxLayout *mainLayout = new QVBoxLayout(&mainWidget);

    QLabel *smallWidget = new QLabel;
    smallWidget->setFixedWidth(30);
    QGroupBox *smallGroupBox = createGroupBox(SpinnerSize::Small, smallWidget);
    mainLayout->addWidget(smallGroupBox);

    mainLayout->addWidget(hr());

    QCalendarWidget *mediumWidget = new QCalendarWidget;
    mediumWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QGroupBox *mediumGroupBox = createGroupBox(SpinnerSize::Medium, mediumWidget);
    mainLayout->addWidget(mediumGroupBox);

    mainLayout->addWidget(hr());

    QTextEdit *largeWidget = new QTextEdit;
    largeWidget->setText(pangram(25));
    QGroupBox *largeGroupBox = createGroupBox(SpinnerSize::Large, largeWidget);
    mainLayout->addWidget(largeGroupBox);

    mainLayout->addWidget(createColorsGroupBox());

    mainWidget.show();

    return app.exec();
}
