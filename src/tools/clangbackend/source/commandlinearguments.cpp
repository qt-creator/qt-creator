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

#include "commandlinearguments.h"

#include "clangfilepath.h"

#include <utf8string.h>
#include <utils/algorithm.h>
#include <utils/qtcprocess.h>

#include <QByteArray>
#include <QtCore/qdebug.h>

#include <iostream>

static QList<QByteArray> splitArgs(QString &argsString)
{
    QList<QByteArray> result;
    Utils::QtcProcess::ArgIterator it(&argsString);
    while (it.next())
        result.append(it.value().toUtf8());
    return result;
}

template<size_t Size>
static QList<QByteArray> extraOptions(const char(&environment)[Size])
{
    if (!qEnvironmentVariableIsSet(environment))
        return QList<QByteArray>();
    QString arguments = QString::fromLocal8Bit(qgetenv(environment));
    return splitArgs(arguments);
}

static QList<QByteArray> extraClangCodeModelPrependOptions() {
    constexpr char ccmPrependOptions[] = "QTC_CLANG_CCM_CMD_PREPEND";
    static const QList<QByteArray> options = extraOptions(ccmPrependOptions);
    if (!options.isEmpty())
        qWarning() << "ClangCodeModel options are prepended with " << options;
    return options;
}

static QList<QByteArray> extraClangCodeModelAppendOptions() {
    constexpr char ccmAppendOptions[] = "QTC_CLANG_CCM_CMD_APPEND";
    static const QList<QByteArray> options = extraOptions(ccmAppendOptions);
    if (!options.isEmpty())
        qWarning() << "ClangCodeModel options are appended with " << options;
    return options;
}

namespace ClangBackEnd {

CommandLineArguments::CommandLineArguments(const char *filePath,
                                           const Utf8StringVector &compilationArguments,
                                           bool addVerboseOption)
    : m_prependArgs(extraClangCodeModelPrependOptions()),
      m_appendArgs(extraClangCodeModelAppendOptions())
{
    const int elementsToReserve = m_prependArgs.size()
            + compilationArguments.size()
            + (addVerboseOption ? 1 : 0)
            + m_appendArgs.size();
    m_arguments.reserve(static_cast<size_t>(elementsToReserve));

    for (const auto &argument : m_prependArgs)
        m_arguments.push_back(argument.constData());
    for (const auto &argument : compilationArguments)
        m_arguments.push_back(argument.constData());
    if (addVerboseOption)
        m_arguments.push_back("-v");
    for (const auto &argument : m_appendArgs)
        m_arguments.push_back(argument.constData());
    m_nativeFilePath = FilePath::toNativeSeparators(Utf8String::fromUtf8(filePath));
    m_arguments.push_back(m_nativeFilePath.constData());
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
    const QString argumentAsQString = QString::fromUtf8(argumentAsCString);
    const QString quotedArgument = Utils::QtcProcess::quoteArg(argumentAsQString);

    return Utf8String::fromString(quotedArgument);
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
