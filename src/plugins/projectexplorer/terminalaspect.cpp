/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "terminalaspect.h"

#include <projectexplorer/project.h>
#include <projectexplorer/runconfiguration.h>

#include <QCheckBox>
#include <QDebug>
#include <QFormLayout>
#include <QLabel>

const char USE_TERMINAL_KEY[] = "Qt4ProjectManager.Qt4RunConfiguration.UseTerminal";

namespace ProjectExplorer {

/*!
    \class ProjectExplorer::TerminalAspect
*/

TerminalAspect::TerminalAspect(RunConfiguration *runConfig, bool useTerminal, bool isForced)
    : IRunConfigurationAspect(runConfig), m_useTerminal(useTerminal), m_isForced(isForced), m_checkBox(0)
{
    setDisplayName(tr("Terminal"));
    setId("TerminalAspect");
}

IRunConfigurationAspect *TerminalAspect::create(RunConfiguration *runConfig) const
{
    return new TerminalAspect(runConfig, false, false);
}

IRunConfigurationAspect *TerminalAspect::clone(RunConfiguration *runConfig) const
{
    return new TerminalAspect(runConfig, m_useTerminal, m_isForced);
}

void TerminalAspect::addToMainConfigurationWidget(QWidget *parent, QLayout *layout)
{
    QTC_CHECK(!m_checkBox);
    m_checkBox = new QCheckBox(tr("Run in terminal"), parent);
    m_checkBox->setChecked(m_useTerminal);
    layout->addWidget(m_checkBox);
    connect(m_checkBox.data(), &QAbstractButton::clicked, this, [this] {
        m_isForced = true;
        setUseTerminal(true);
    });
}

void TerminalAspect::fromMap(const QVariantMap &map)
{
    if (map.contains(QLatin1String(USE_TERMINAL_KEY))) {
        m_useTerminal = map.value(QLatin1String(USE_TERMINAL_KEY)).toBool();
        m_isForced = true;
    } else {
        m_isForced = false;
    }
}

void TerminalAspect::toMap(QVariantMap &data) const
{
    if (m_isForced)
        data.insert(QLatin1String(USE_TERMINAL_KEY), m_useTerminal);
}

bool TerminalAspect::useTerminal() const
{
    return m_useTerminal;
}

void TerminalAspect::setUseTerminal(bool useTerminal)
{
    if (m_useTerminal != useTerminal) {
        m_useTerminal = useTerminal;
        emit useTerminalChanged(useTerminal);
    }
}

} // namespace ProjectExplorer
