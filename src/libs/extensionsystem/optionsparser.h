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

#include <QStringList>
#include <QMap>

namespace ExtensionSystem {
namespace Internal {

class PluginManagerPrivate;

class OptionsParser
{
public:
    OptionsParser(const QStringList &args,
        const QMap<QString, bool> &appOptions,
        QMap<QString, QString> *foundAppOptions,
        QString *errorString,
        PluginManagerPrivate *pmPrivate);

    bool parse();

    static const char *NO_LOAD_OPTION;
    static const char *LOAD_OPTION;
    static const char *TEST_OPTION;
    static const char *NOTEST_OPTION;
    static const char *PROFILE_OPTION;
private:
    // return value indicates if the option was processed
    // it doesn't indicate success (--> m_hasError)
    bool checkForEndOfOptions();
    bool checkForLoadOption();
    bool checkForNoLoadOption();
    bool checkForTestOptions();
    bool checkForAppOption();
    bool checkForPluginOption();
    bool checkForProfilingOption();
    bool checkForUnknownOption();

    enum TokenType { OptionalToken, RequiredToken };
    bool nextToken(TokenType type = OptionalToken);

    const QStringList &m_args;
    const QMap<QString, bool> &m_appOptions;
    QMap<QString, QString> *m_foundAppOptions;
    QString *m_errorString;
    PluginManagerPrivate *m_pmPrivate;

    // state
    QString m_currentArg;
    QStringList::const_iterator m_it;
    QStringList::const_iterator m_end;
    bool m_isDependencyRefreshNeeded;
    bool m_hasError;
};

} // namespace Internal
} // namespace ExtensionSystem
