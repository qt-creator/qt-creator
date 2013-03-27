/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPLUSPLUS_ICONS_H
#define CPLUSPLUS_ICONS_H

#include <cplusplus/CPlusPlusForwardDeclarations.h>

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

    enum IconType {
        ClassIconType = 0,
        EnumIconType,
        EnumeratorIconType,
        FuncPublicIconType,
        FuncProtectedIconType,
        FuncPrivateIconType,
        NamespaceIconType,
        VarPublicIconType,
        VarProtectedIconType,
        VarPrivateIconType,
        SignalIconType,
        SlotPublicIconType,
        SlotProtectedIconType,
        SlotPrivateIconType,
        KeywordIconType,
        MacroIconType,
        UnknownIconType
    };

    static IconType iconTypeForSymbol(const Symbol *symbol);
    QIcon iconForType(IconType type) const;

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
