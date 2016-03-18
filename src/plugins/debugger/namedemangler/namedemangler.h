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
