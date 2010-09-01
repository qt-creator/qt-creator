#include "debuggerlanguagechooser.h"

#include <QHBoxLayout>
#include <QCheckBox>

namespace Utils {

DebuggerLanguageChooser::DebuggerLanguageChooser(QWidget *parent) :
    QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    setLayout(layout);
    m_useCppDebugger = new QCheckBox(tr("C++"), this);
    m_useQmlDebugger = new QCheckBox(tr("QML"), this);
    layout->setMargin(0);
    layout->addWidget(m_useCppDebugger);
    layout->addWidget(m_useQmlDebugger);

    connect(m_useCppDebugger, SIGNAL(toggled(bool)),
            this, SLOT(useCppDebuggerToggled(bool)));
    connect(m_useQmlDebugger, SIGNAL(toggled(bool)),
            this, SLOT(useQmlDebuggerToggled(bool)));
}

void DebuggerLanguageChooser::setCppChecked(bool value)
{
    m_useCppDebugger->setChecked(value);
}

void DebuggerLanguageChooser::setQmlChecked(bool value)
{
    m_useQmlDebugger->setChecked(value);
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

} // namespace Utils
