/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef UTILS_MACROEXPANDER_H
#define UTILS_MACROEXPANDER_H

#include "stringutils.h"

#include <functional>

namespace Utils {

namespace Internal { class MacroExpanderPrivate; }

class QTCREATOR_UTILS_EXPORT MacroExpander : public AbstractMacroExpander
{
public:
    explicit MacroExpander();
    ~MacroExpander();

    bool resolveMacro(const QString &name, QString *ret);

    QString value(const QByteArray &variable, bool *found = 0);

    QString expandedString(const QString &stringWithVariables);

    typedef std::function<QString(QString)> PrefixFunction;
    typedef std::function<QString()> StringFunction;
    typedef std::function<int()> IntFunction;

    void registerPrefix(const QByteArray &prefix,
        const QString &description, const PrefixFunction &value);

    void registerVariable(const QByteArray &variable,
        const QString &description, const StringFunction &value);

    void registerIntVariable(const QByteArray &variable,
        const QString &description, const IntFunction &value);

    void registerFileVariables(const QByteArray &prefix,
        const QString &heading, const StringFunction &value);

    QList<QByteArray> variables();
    QString variableDescription(const QByteArray &variable);

private:
    MacroExpander(const MacroExpander &) Q_DECL_EQ_DELETE;
    void operator=(const MacroExpander &) Q_DECL_EQ_DELETE;

    Internal::MacroExpanderPrivate *d;
};

} // namespace Utils

#endif // UTILS_MACROEXPANDER_H
