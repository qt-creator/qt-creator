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

enum class State {
    NotRunning,
    Running
};

static QString colorButtonStyleSheet(const QColor &bgColor)
{
    QString rc("border-width: 2px; border-radius: 2px; border-color: black; ");
    rc += bgColor.isValid() ? "border-style: solid; background:" + bgColor.name() + ";"
                            : QString("border-style: dotted;");
    return rc;
}

static QColor stateToColor(State state)
{
    switch (state) {
    case State::NotRunning: return Qt::gray;
    case State::Running: return Qt::yellow;
    }
    return {};
}

class StateIndicator : public QLabel
{
public:
    StateIndicator(QWidget *parent = nullptr)
        : QLabel(parent)
    {
        setFixedSize(30, 30);
        m_spinner = new Spinner(SpinnerSize::Small, this);
        m_spinner->hide();
        updateState();
    }

    void setState(State state)
    {
        if (m_state == state)
            return;
        m_state = state;
        updateState();
    }

private:
    void updateState()
    {
        setStyleSheet(colorButtonStyleSheet(stateToColor(m_state)));
        if (m_state == State::Running)
            m_spinner->show();
        else
            m_spinner->hide();
    }
    State m_state = State::NotRunning;
    Spinner *m_spinner = nullptr;
};

class StateWidget : public QWidget
{
public:
    StateWidget() : m_stateIndicator(new StateIndicator(this)) {
        QBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_stateIndicator);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }
    void setState(State state) { m_stateIndicator->setState(state); }

protected:
    StateIndicator *m_stateIndicator = nullptr;
};

QGroupBox *createGroupBox(SpinnerSize size, QWidget *widget)
{
    const QMetaEnum spinnerSize = QMetaEnum::fromType<SpinnerSize>();
    QGroupBox *groupBox = new QGroupBox(spinnerSize.valueToKey(int(size)));

    StateWidget *stateWidget = new StateWidget;
    QToolButton *startButton = new QToolButton;
    startButton->setText("Start");
    QToolButton *stopButton = new QToolButton;
    stopButton->setText("Stop");

    QVBoxLayout *mainLayout = new QVBoxLayout(groupBox);
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addWidget(stateWidget);
    topLayout->addWidget(startButton);
    topLayout->addWidget(stopButton);
    topLayout->addStretch();
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(widget);

    Spinner *spinner = new Spinner(size, widget);
    spinner->hide(); // TODO: make the default hidden?

    QObject::connect(startButton, &QAbstractButton::clicked, groupBox, [=] {
        stateWidget->setState(State::Running);
        spinner->show();
        widget->setEnabled(false);
    });
    QObject::connect(stopButton, &QAbstractButton::clicked, groupBox, [=] {
        stateWidget->setState(State::NotRunning);
        spinner->hide();
        widget->setEnabled(true);
    });
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

    mainWidget.show();

    return app.exec();
}
