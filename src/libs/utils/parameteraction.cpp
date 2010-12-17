/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "parameteraction.h"

namespace Utils {

ParameterAction::ParameterAction(const QString &emptyText,
                                     const QString &parameterText,
                                     EnablingMode mode,
                                     QObject* parent) :
    QAction(emptyText, parent),
    m_emptyText(emptyText),
    m_parameterText(parameterText),
    m_enablingMode(mode)
{
}

QString ParameterAction::emptyText() const
{
    return m_emptyText;
}

void ParameterAction::setEmptyText(const QString &t)
{
    m_emptyText = t;
}

QString ParameterAction::parameterText() const
{
    return m_parameterText;
}

void ParameterAction::setParameterText(const QString &t)
{
    m_parameterText = t;
}

ParameterAction::EnablingMode ParameterAction::enablingMode() const
{
    return m_enablingMode;
}

void ParameterAction::setEnablingMode(EnablingMode m)
{
    m_enablingMode = m;
}

void ParameterAction::setParameter(const QString &p)
{
    const bool enabled = !p.isEmpty();
    if (enabled) {
        setText(m_parameterText.arg(p));
    } else {
        setText(m_emptyText);
    }
    if (m_enablingMode == EnabledWithParameter)
        setEnabled(enabled);
}

} // namespace Utils
