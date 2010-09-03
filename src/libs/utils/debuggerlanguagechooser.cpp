#include "debuggerlanguagechooser.h"

#include <QHBoxLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>

namespace Utils {

DebuggerLanguageChooser::DebuggerLanguageChooser(QWidget *parent) :
    QWidget(parent)
{
    m_useCppDebugger = new QCheckBox(tr("C++"), this);
    m_useQmlDebugger = new QCheckBox(tr("QML"), this);

    connect(m_useCppDebugger, SIGNAL(toggled(bool)),
            this, SLOT(useCppDebuggerToggled(bool)));
    connect(m_useQmlDebugger, SIGNAL(toggled(bool)),
            this, SLOT(useQmlDebuggerToggled(bool)));

    m_debugServerPortLabel = new QLabel(tr("Debug Port:"), this);
    m_debugServerPort = new QSpinBox(this);
    m_debugServerPort->setMinimum(1);
    m_debugServerPort->setMaximum(65535);

    m_debugServerPortLabel->setBuddy(m_debugServerPort);

    connect(m_useQmlDebugger, SIGNAL(toggled(bool)), m_debugServerPort, SLOT(setEnabled(bool)));
    connect(m_useQmlDebugger, SIGNAL(toggled(bool)), m_debugServerPortLabel, SLOT(setEnabled(bool)));
    connect(m_debugServerPort, SIGNAL(valueChanged(int)), this, SLOT(onDebugServerPortChanged(int)));

    QHBoxLayout *qmlLayout = new QHBoxLayout;
    qmlLayout->setMargin(0);
    qmlLayout->addWidget(m_useQmlDebugger);
    qmlLayout->addWidget(m_debugServerPortLabel);
    qmlLayout->addWidget(m_debugServerPort);
    qmlLayout->addStretch();

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addWidget(m_useCppDebugger);
    layout->addLayout(qmlLayout);

    setLayout(layout);
}

bool DebuggerLanguageChooser::cppChecked() const
{
    return m_useCppDebugger->isChecked();
}

bool DebuggerLanguageChooser::qmlChecked() const
{
    return m_useQmlDebugger->isChecked();
}

uint DebuggerLanguageChooser::qmlDebugServerPort() const
{
    return m_debugServerPort->value();
}

void DebuggerLanguageChooser::setCppChecked(bool value)
{
    m_useCppDebugger->setChecked(value);
}

void DebuggerLanguageChooser::setQmlChecked(bool value)
{
    m_useQmlDebugger->setChecked(value);
    m_debugServerPortLabel->setEnabled(value);
    m_debugServerPort->setEnabled(value);
}

void DebuggerLanguageChooser::setQmlDebugServerPort(uint port)
{
    m_debugServerPort->setValue(port);
}

void DebuggerLanguageChooser::useCppDebuggerToggled(bool toggled)
{
    emit cppLanguageToggled(toggled);
    if (!toggled && !m_useQmlDebugger->isChecked())
        m_useQmlDebugger->setChecked(true);
}

void DebuggerLanguageChooser::useQmlDebuggerToggled(bool toggled)
{
    emit qmlLanguageToggled(toggled);
    if (!toggled && !m_useCppDebugger->isChecked())
        m_useCppDebugger->setChecked(true);
}

void DebuggerLanguageChooser::onDebugServerPortChanged(int port)
{
    emit qmlDebugServerPortChanged((uint)port);
}

} // namespace Utils
