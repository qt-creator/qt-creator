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
