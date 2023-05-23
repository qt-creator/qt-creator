// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "parameteraction.h"

/*!
    \class Utils::ParameterAction
    \inmodule QtCreator

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
