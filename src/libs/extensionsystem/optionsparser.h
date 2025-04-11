// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/result.h>

#include <QStringList>
#include <QMap>

namespace ExtensionSystem::Internal {

class PluginManagerPrivate;

class OptionsParser
{
public:
    OptionsParser(const QStringList &args,
        const QMap<QString, bool> &appOptions,
        QMap<QString, QString> *foundAppOptions,
        PluginManagerPrivate *pmPrivate);

    Utils::Result<> parse();

    static const char *NO_LOAD_OPTION;
    static const char *LOAD_OPTION;
    static const char *TEST_OPTION;
    static const char *NOTEST_OPTION;
    static const char *SCENARIO_OPTION;
    static const char *PROFILE_OPTION;
    static const char *TRACE_OPTION;
    static const char *NO_CRASHCHECK_OPTION;

private:
    // return value indicates if the option was processed
    // it doesn't indicate success (--> m_hasError)
    bool checkForEndOfOptions();
    bool checkForLoadOption();
    bool checkForNoLoadOption();
    bool checkForTestOptions();
    bool checkForScenarioOption();
    bool checkForAppOption();
    bool checkForPluginOption();
    bool checkForProfilingOption();
    bool checkForTraceOption();
    bool checkForNoCrashcheckOption();
    bool checkForUnknownOption();
    void forceDisableAllPluginsExceptTestedAndForceEnabled();

    enum TokenType { OptionalToken, RequiredToken };
    bool nextToken(TokenType type = OptionalToken);

    const QStringList &m_args;
    const QMap<QString, bool> &m_appOptions;
    QMap<QString, QString> *m_foundAppOptions;
    PluginManagerPrivate *m_pmPrivate;

    // state
    QString m_currentArg;
    QStringList::const_iterator m_it;
    QStringList::const_iterator m_end;
    bool m_isDependencyRefreshNeeded = false;
    Utils::Result<> m_result = Utils::ResultOk;
};

} // namespace ExtensionSystem::Internal
