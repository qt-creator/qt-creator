/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef CPLUSPLUS_ICONS_H
#define CPLUSPLUS_ICONS_H

#include "CPlusPlusForwardDeclarations.h"

#include <QIcon>

namespace CPlusPlus {

class Symbol;

class CPLUSPLUS_EXPORT Icons
{
public:
    Icons();

    QIcon iconForSymbol(const Symbol *symbol) const;

    QIcon keywordIcon() const;
    QIcon macroIcon() const;

private:
    QIcon _classIcon;
    QIcon _enumIcon;
    QIcon _enumeratorIcon;
    QIcon _funcPublicIcon;
    QIcon _funcProtectedIcon;
    QIcon _funcPrivateIcon;
    QIcon _namespaceIcon;
    QIcon _varPublicIcon;
    QIcon _varProtectedIcon;
    QIcon _varPrivateIcon;
    QIcon _signalIcon;
    QIcon _slotPublicIcon;
    QIcon _slotProtectedIcon;
    QIcon _slotPrivateIcon;
    QIcon _keywordIcon;
    QIcon _macroIcon;
};

} // namespace CPlusPlus

#endif // CPLUSPLUS_ICONS_H
