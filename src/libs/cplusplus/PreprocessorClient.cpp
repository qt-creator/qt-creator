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

#include "PreprocessorClient.h"

using namespace CPlusPlus;

/*!
    \class Client
    \brief A notification interface for for C++ preprocessor.
*/

/*!
    \fn void Client::macroAdded(const Macro &macro)

    Called whenever a new macro is defined.
*/

/*!
    \fn void Client::passedMacroDefinitionCheck(unsigned offset, const Macro &macro)

    Called when the preprocessor checks whether a macro is defined or not and the
    result is positive.

    \sa failedMacroDefinitionCheck()
*/

/*!
    \fn void Client::failedMacroDefinitionCheck(unsigned offset, const QByteArray &name)

    Called when the preprocessor checks whether a macro is defined or not and the
    result is negative.

    \sa passedMacroDefinitionCheck()
*/

/*!
    \fn void Client::startExpandingMacro(unsigned offset, const Macro &macro, const QByteArray &originalText, bool inCondition = false, const QVector<MacroArgumentReference> &actuals = QVector<MacroArgumentReference>())

    Called when starting to expand a macro. The parameter \a inCondition indicates whether the
    expansion is happening inside a preprocessor conditional.

    \sa stopExpandingMacro()
*/

Client::Client()
{ }

Client::~Client()
{ }
