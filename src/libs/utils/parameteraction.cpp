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

#include "parameteraction.h"

/*!
    \class Utils::ParameterAction

    \brief The ParameterAction class is intended for actions that act on a 'current',
     string-type parameter (typically a file name), for example 'Save file %1'.

    The action has 2 states:
    \list
    \li <no current parameter> displaying "Do XX" (empty text)
    \li <parameter present> displaying "Do XX with %1".
    \endlist

    Provides a slot to set the parameter, changing display
    and enabled state accordingly.
    The text passed in should already be translated; parameterText
    should contain a %1 where the parameter is to be inserted.
*/

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
    if (enabled)
        setText(m_parameterText.arg(p));
    else
        setText(m_emptyText);
    if (m_enablingMode == EnabledWithParameter)
        setEnabled(enabled);
}

} // namespace Utils
