/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#ifndef CDBSYMBOLGROUPCONTEXT_TPL_H
#define CDBSYMBOLGROUPCONTEXT_TPL_H

#include <QtCore/QDebug>

namespace Debugger {
namespace Internal {

enum { debugSgRecursion = 0 };

/* inline static */ bool CdbSymbolGroupContext::isSymbolDisplayable(const DEBUG_SYMBOL_PARAMETERS &p)
{
    if (p.Flags & (DEBUG_SYMBOL_IS_LOCAL|DEBUG_SYMBOL_IS_ARGUMENT))
        return true;
    // Do not display static members.
    if (p.Flags & DEBUG_SYMBOL_READ_ONLY)
        return false;
    return true;
}

template <class OutputIterator>
bool CdbSymbolGroupContext::getChildSymbols(const QString &prefix, OutputIterator it, QString *errorMessage)
{
    unsigned long start;
    unsigned long parentId;
    if (!getChildSymbolsPosition(prefix, &start, &parentId, errorMessage))
        return false;
    // Skip over expanded children
    const unsigned long end = m_symbolParameters.size();
    for (unsigned long s = start; s < end; ++s) {
        const DEBUG_SYMBOL_PARAMETERS &p = m_symbolParameters.at(s);
        if (p.ParentSymbol == parentId && isSymbolDisplayable(p)) {
            *it = symbolAt(s);
            ++it;
        }
    }
    return true;
}

} // namespace Internal
} // namespace Debugger

#endif // CDBSYMBOLGROUPCONTEXT_TPL_H
