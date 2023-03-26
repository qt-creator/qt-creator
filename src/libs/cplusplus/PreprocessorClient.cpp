// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
