/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef NAME_DEMANGLER_H
#define NAME_DEMANGLER_H

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

class NameDemanglerPrivate;

class NameDemangler
{
public:
    NameDemangler();
    ~NameDemangler();

    /*
     * Demangles a mangled name. Also accepts a non-demangled name,
     * in which case it is not transformed.
     * Returns true <=> the name is not mangled or it is mangled correctly
     *                  according to the specification.
     */
    bool demangle(const QString &mangledName);

    /*
     * A textual description of the error encountered, if there was one.
     * Only valid if demangle() returned false.
     */
    QString errorString() const;

    /*
     * The demangled name. If the original name was not mangled, this
     * is identical to the input.
     * Only valid if demangle() returned true.
     */
    QString demangledName() const;

private:
    NameDemanglerPrivate *d;
};

} // namespace Internal
} // namespace Debugger

#endif // Include guard.
