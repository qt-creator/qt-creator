/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "debuggerlanguagechooser.h"

#include <QtGui/QHBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QSpinBox>
#include <QtGui/QLabel>

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

    m_debugServerPortLabel = new QLabel(tr("Debug port:"), this);
    m_debugServerPort = new QSpinBox(this);
    m_debugServerPort->setMinimum(1);
    m_debugServerPort->setMaximum(65535);

    m_debugServerPortLabel->setBuddy(m_debugServerPort);

    m_qmlDebuggerInfoLabel = new QLabel(tr("<a href=\"qthelp://com.nokia.qtcreator/doc/creator-debugging-qml.html\">What are the prerequisites?</a>"));
    connect(m_qmlDebuggerInfoLabel, SIGNAL(linkActivated(QString)),
            this, SIGNAL(openHelpUrl(QString)));

    connect(m_useQmlDebugger, SIGNAL(toggled(bool)), m_debugServerPort, SLOT(setEnabled(bool)));
    connect(m_useQmlDebugger, SIGNAL(toggled(bool)), m_debugServerPortLabel, SLOT(setEnabled(bool)));
    connect(m_debugServerPort, SIGNAL(valueChanged(int)), this, SLOT(onDebugServerPortChanged(int)));

    QHBoxLayout *qmlLayout = new QHBoxLayout;
    qmlLayout->setMargin(0);
    qmlLayout->addWidget(m_useQmlDebugger);
    qmlLayout->addWidget(m_debugServerPortLabel);
    qmlLayout->addWidget(m_debugServerPort);
    qmlLayout->addWidget(m_qmlDebuggerInfoLabel);
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
