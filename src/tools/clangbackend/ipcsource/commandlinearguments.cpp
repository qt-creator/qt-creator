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

#include "commandlinearguments.h"

#include <utf8string.h>

#include <iostream>

namespace ClangBackEnd {

CommandLineArguments::CommandLineArguments(const char *filePath,
                                           const std::vector<const char *> &projectPartArguments,
                                           const Utf8StringVector &fileArguments,
                                           bool addVerboseOption)
{
    const auto elementsToReserve = projectPartArguments.size()
            + uint(fileArguments.size())
            + (addVerboseOption ? 1 : 0);
    m_arguments.reserve(elementsToReserve);

    m_arguments = projectPartArguments;
    for (const auto &argument : fileArguments)
        m_arguments.push_back(argument.constData());
    if (addVerboseOption)
        m_arguments.push_back("-v");
    m_arguments.push_back(filePath);
}

const char * const *CommandLineArguments::data() const
{
    return m_arguments.data();
}

int CommandLineArguments::count() const
{
    return int(m_arguments.size());
}

const char *CommandLineArguments::at(int position) const
{
    return m_arguments.at(uint(position));
}

static Utf8String maybeQuoted(const char *argumentAsCString)
{
    const auto quotationMark = Utf8StringLiteral("\"");
    const auto argument = Utf8String::fromUtf8(argumentAsCString);

    if (argument.contains(quotationMark))
        return argument;

    return quotationMark + argument + quotationMark;
}

void CommandLineArguments::print() const
{
    using namespace std;
    cerr << "Arguments to libclang:";
    for (const auto &argument : m_arguments)
        cerr << ' ' << maybeQuoted(argument).constData();
    cerr << endl;
}

} // namespace ClangBackEnd
