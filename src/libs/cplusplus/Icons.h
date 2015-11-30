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

#endif // CPLUSPLUS_ICONS_H
