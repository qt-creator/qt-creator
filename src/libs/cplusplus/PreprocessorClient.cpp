/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "PreprocessorClient.h"

using namespace CPlusPlus;

/*!
    \class Client
    \brief The Client class implements a notification interface for the
    C++ preprocessor.
*/

/*!
    \fn void Client::macroAdded(const Macro &macro)

    Called whenever a new \a macro is defined.
*/

/*!
    \fn void Client::passedMacroDefinitionCheck(int bytesOffset,
                                                int utf16charsOffset,
                                                int line,
                                                const Macro &macro)

    Called when the preprocessor checks whether \a macro at \a line with
    \a bytesOffset and \a utf16charsOffset is defined and the result is
    positive.

    \sa failedMacroDefinitionCheck()
*/

/*!
    \fn void Client::failedMacroDefinitionCheck(int bytesOffset,
                                                int utf16charsOffset,
                                                const ByteArrayRef &name)

    Called when the preprocessor checks whether the macro specified by \a name
    is defined with \a bytesOffset and \a utf16charsOffset and the result is
    negative.

    \sa passedMacroDefinitionCheck()
*/

/*!
    \fn void Client::startExpandingMacro(int bytesOffset,
                                         int utf16charsOffset,
                                         int line,
                                         const Macro &macro,
                                         const QVector<MacroArgumentReference> &actuals
                                               = QVector<MacroArgumentReference>())

    Called when starting to expand \a macro at \a line with \a bytesOffset,
    \a utf16charsOffset, and \a actuals.

    \sa stopExpandingMacro()
*/

Client::Client()
{ }

Client::~Client()
{ }
