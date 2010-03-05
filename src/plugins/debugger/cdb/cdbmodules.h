/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CDBMODULES_H
#define CDBMODULES_H

#include <QtCore/QStringList>

#include "cdbcom.h"

namespace Debugger {
namespace Internal {

class Module;
class Symbol;

bool getModuleList(CIDebugSymbols *syms, QList<Module> *modules, QString *errorMessage);
bool getModuleNameList(CIDebugSymbols *syms, QStringList *modules, QString *errorMessage);
// Search symbols matching a pattern. Does not filter on module names.
bool searchSymbols(CIDebugSymbols *syms, const QString &pattern,
                   QStringList *matches, QString *errorMessage);

// ResolveSymbol: For symbols that are missing the module specifier,
// find the module and expand: "main" -> "project!main".

enum ResolveSymbolResult { ResolveSymbolOk, ResolveSymbolAmbiguous,
                           ResolveSymbolNotFound, ResolveSymbolError };

// Resolve a symbol that is unique to all modules
ResolveSymbolResult resolveSymbol(CIDebugSymbols *syms, QString *symbol, QString *errorMessage);

// Resolve symbol overload with an additional regexp pattern to filter on modules.
ResolveSymbolResult resolveSymbol(CIDebugSymbols *syms, const QString &pattern, QString *symbol, QString *errorMessage);

// List symbols of a module
bool getModuleSymbols(CIDebugSymbols *syms, const QString &moduleName,
                      QList<Symbol> *symbols, QString *errorMessage);

} // namespace Internal
} // namespace Debugger

#endif // CDBMODULES_H
