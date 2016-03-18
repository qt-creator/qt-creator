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

#pragma once

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
        StructIconType,
        EnumIconType,
        EnumeratorIconType,
        FuncPublicIconType,
        FuncProtectedIconType,
        FuncPrivateIconType,
        FuncPublicStaticIconType,
        FuncProtectedStaticIconType,
        FuncPrivateStaticIconType,
        NamespaceIconType,
        VarPublicIconType,
        VarProtectedIconType,
        VarPrivateIconType,
        VarPublicStaticIconType,
        VarProtectedStaticIconType,
        VarPrivateStaticIconType,
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
    QIcon _structIcon;
    QIcon _enumIcon;
    QIcon _enumeratorIcon;
    QIcon _funcPublicIcon;
    QIcon _funcProtectedIcon;
    QIcon _funcPrivateIcon;
    QIcon _funcPublicStaticIcon;
    QIcon _funcProtectedStaticIcon;
    QIcon _funcPrivateStaticIcon;
    QIcon _namespaceIcon;
    QIcon _varPublicIcon;
    QIcon _varProtectedIcon;
    QIcon _varPrivateIcon;
    QIcon _varPublicStaticIcon;
    QIcon _varProtectedStaticIcon;
    QIcon _varPrivateStaticIcon;
    QIcon _signalIcon;
    QIcon _slotPublicIcon;
    QIcon _slotProtectedIcon;
    QIcon _slotPrivateIcon;
    QIcon _keywordIcon;
    QIcon _macroIcon;
};

} // namespace CPlusPlus
