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

#include "namedemangler.h"

#include "demanglerexceptions.h"
#include "parsetreenodes.h"

#include <limits>

namespace Debugger {
namespace Internal {

class NameDemanglerPrivate
{
public:
    bool demangle(const QString &mangledName);
    const QString &errorString() const { return m_errorString; }
    const QString &demangledName() const { return m_demangledName; }

private:
    GlobalParseState m_parseState;
    QString m_errorString;
    QString m_demangledName;
};


bool NameDemanglerPrivate::demangle(const QString &mangledName)
{
    bool success;
    try {
        m_parseState.m_mangledName = mangledName.toLatin1();
        m_parseState.m_pos = 0;
        m_demangledName.clear();

        if (!MangledNameRule::mangledRepresentationStartsWith(m_parseState.peek())) {
            m_demangledName = QLatin1String(m_parseState.m_mangledName);
            return true;
        }

        MangledNameRule::parse(&m_parseState, ParseTreeNode::Ptr());
        if (m_parseState.m_pos != m_parseState.m_mangledName.size())
            throw ParseException(QLatin1String("Unconsumed input"));
        if (m_parseState.m_parseStack.count() != 1) {
            throw ParseException(QString::fromLocal8Bit("There are %1 elements on the parse stack; "
                    "expected one.").arg(m_parseState.m_parseStack.count()));
        }

        // Uncomment for debugging.
        //m_parseState.stackTop()->print(0);

        m_demangledName = QLatin1String(m_parseState.stackTop()->toByteArray());
        success = true;
    } catch (const ParseException &p) {
        m_errorString = QString::fromLocal8Bit("Parse error at index %1 of mangled name '%2': %3.")
                .arg(m_parseState.m_pos).arg(mangledName, p.error);
        success = false;
    } catch (const InternalDemanglerException &e) {
        m_errorString = QString::fromLocal8Bit("Internal demangler error at function %1, file %2, "
                "line %3").arg(e.func, e.file).arg(e.line);
        success = false;
    }

    m_parseState.m_parseStack.clear();
    m_parseState.m_substitutions.clear();
    m_parseState.m_templateParams.clear();
    return success;
}


NameDemangler::NameDemangler() : d(new NameDemanglerPrivate) { }

NameDemangler::~NameDemangler()
{
    delete d;
}

bool NameDemangler::demangle(const QString &mangledName)
{
    return d->demangle(mangledName);
}

QString NameDemangler::errorString() const
{
    return d->errorString();
}

QString NameDemangler::demangledName() const
{
    return d->demangledName();
}

} // namespace Internal
} // namespace Debugger
