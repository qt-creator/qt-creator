// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "optionsparser.h"

#include "extensionsystemtr.h"
#include "pluginmanager.h"
#include "pluginmanager_p.h"

#include <utils/algorithm.h>

using namespace Utils;

namespace ExtensionSystem::Internal {

const char END_OF_OPTIONS[] = "--";
const char *OptionsParser::NO_LOAD_OPTION = "-noload";
const char *OptionsParser::LOAD_OPTION = "-load";
const char *OptionsParser::TEST_OPTION = "-test";
const char *OptionsParser::NOTEST_OPTION = "-notest";
const char *OptionsParser::SCENARIO_OPTION = "-scenario";
const char *OptionsParser::PROFILE_OPTION = "-profile";
const char *OptionsParser::TRACE_OPTION = "-trace";
const char *OptionsParser::NO_CRASHCHECK_OPTION = "-no-crashcheck";

OptionsParser::OptionsParser(const QStringList &args,
        const QMap<QString, bool> &appOptions,
        QMap<QString, QString> *foundAppOptions,
        PluginManagerPrivate *pmPrivate)
    : m_args(args), m_appOptions(appOptions),
      m_foundAppOptions(foundAppOptions),
      m_pmPrivate(pmPrivate),
      m_it(m_args.constBegin()),
      m_end(m_args.constEnd()),
      m_isDependencyRefreshNeeded(false),
      m_result(ResultOk)
{
    ++m_it; // jump over program name
    if (m_foundAppOptions)
        m_foundAppOptions->clear();
    m_pmPrivate->arguments.clear();
    m_pmPrivate->argumentsForRestart.clear();
}

Result<> OptionsParser::parse()
{
    while (m_result) {
        if (!nextToken()) // move forward
            break;
        if (checkForEndOfOptions())
            break;
        if (checkForLoadOption())
            continue;
        if (checkForNoLoadOption())
            continue;
        if (checkForProfilingOption())
            continue;
        if (checkForTraceOption())
            continue;
        if (checkForNoCrashcheckOption())
            continue;
#ifdef EXTENSIONSYSTEM_WITH_TESTOPTION
        if (checkForTestOptions())
            continue;
        if (checkForScenarioOption())
            continue;
#endif
        if (checkForAppOption())
            continue;
        if (checkForPluginOption())
            continue;
        if (checkForUnknownOption())
            break;
        // probably a file or something
        m_pmPrivate->arguments << m_currentArg;
    }
    if (PluginManager::testRunRequested()) {
        m_isDependencyRefreshNeeded = true;
        forceDisableAllPluginsExceptTestedAndForceEnabled();
    }
    if (m_isDependencyRefreshNeeded)
        m_pmPrivate->enableDependenciesIndirectly();
    return m_result;
}

bool OptionsParser::checkForEndOfOptions()
{
    if (m_currentArg != QLatin1String(END_OF_OPTIONS))
        return false;
    while (nextToken()) {
        m_pmPrivate->arguments << m_currentArg;
    }
    return true;
}

bool OptionsParser::checkForTestOptions()
{
    if (m_currentArg == QLatin1String(TEST_OPTION)) {
        if (nextToken(RequiredToken)) {
            if (m_currentArg == QLatin1String("all")) {
                m_pmPrivate->testSpecs
                    = Utils::transform<std::vector>(m_pmPrivate->loadQueue(), [](PluginSpec *spec) {
                          return PluginManagerPrivate::TestSpec(spec);
                      });
            } else {
                QStringList args = m_currentArg.split(QLatin1Char(','));
                const QString pluginId = args.takeFirst();
                if (PluginSpec *spec = m_pmPrivate->pluginById(pluginId.toLower())) {
                    if (m_pmPrivate->containsTestSpec(spec)) {
                        m_result = ResultError(Tr::tr("The plugin \"%1\" is specified twice for testing.").arg(pluginId));
                    } else {
                        m_pmPrivate->testSpecs.emplace_back(spec, args);
                    }
                } else {
                    m_result = ResultError(Tr::tr("The plugin \"%1\" does not exist.").arg(pluginId));
                }
            }
        }
        return true;
    }

    if (m_currentArg == QLatin1String(NOTEST_OPTION)) {
        if (nextToken(RequiredToken)) {
            if (PluginSpec *spec = m_pmPrivate->pluginById(m_currentArg.toLower())) {
                if (!m_pmPrivate->containsTestSpec(spec)) {
                    m_result = ResultError(Tr::tr("The plugin \"%1\" is not tested.").arg(m_currentArg));
                } else {
                    m_pmPrivate->removeTestSpec(spec);
                }
            } else {
                m_result = ResultError(Tr::tr("The plugin \"%1\" does not exist.").arg(m_currentArg));
            }
        }
        return true;
    }

    return false;
}

bool OptionsParser::checkForScenarioOption()
{
    if (m_currentArg == QLatin1String(SCENARIO_OPTION)) {
        if (nextToken(RequiredToken)) {
            if (!m_pmPrivate->m_requestedScenario.isEmpty()) {
                m_result = ResultError(Tr::tr(
                        "Cannot request scenario \"%1\" as it was already requested.")
                        .arg(m_currentArg, m_pmPrivate->m_requestedScenario));
            } else {
                // It's called before we register scenarios, so we don't check if the requested
                // scenario was already registered yet.
                m_pmPrivate->m_requestedScenario = m_currentArg;
            }
        }
        return true;
    }
    return false;
}

bool OptionsParser::checkForLoadOption()
{
    if (m_currentArg != QLatin1String(LOAD_OPTION))
        return false;
    if (nextToken(RequiredToken)) {
        if (m_currentArg == QLatin1String("all")) {
            for (PluginSpec *spec : std::as_const(m_pmPrivate->pluginSpecs))
                spec->setForceEnabled(true);
            m_isDependencyRefreshNeeded = true;
        } else {
            PluginSpec *spec = m_pmPrivate->pluginById(m_currentArg.toLower());
            if (!spec) {
                m_result = ResultError(Tr::tr("The plugin \"%1\" does not exist.").arg(m_currentArg));
            } else {
                spec->setForceEnabled(true);
                m_isDependencyRefreshNeeded = true;
            }
        }
        m_pmPrivate->argumentsForRestart << QLatin1String(LOAD_OPTION) << m_currentArg;
    }
    return true;
}

bool OptionsParser::checkForNoLoadOption()
{
    if (m_currentArg != QLatin1String(NO_LOAD_OPTION))
        return false;
    if (nextToken(RequiredToken)) {
        if (m_currentArg == QLatin1String("all")) {
            for (PluginSpec *spec : std::as_const(m_pmPrivate->pluginSpecs))
                spec->setForceDisabled(true);
            m_isDependencyRefreshNeeded = true;
        } else {
            PluginSpec *spec = m_pmPrivate->pluginById(m_currentArg.toLower());
            if (!spec) {
                m_result = ResultError(Tr::tr("The plugin \"%1\" does not exist.").arg(m_currentArg));
            } else {
                spec->setForceDisabled(true);
                // recursively disable all plugins that require this plugin
                for (PluginSpec *dependantSpec : PluginManager::pluginsRequiringPlugin(spec))
                    dependantSpec->setForceDisabled(true);
                m_isDependencyRefreshNeeded = true;
            }
        }
        m_pmPrivate->argumentsForRestart << QLatin1String(NO_LOAD_OPTION) << m_currentArg;
    }
    return true;
}

bool OptionsParser::checkForAppOption()
{
    if (!m_appOptions.contains(m_currentArg))
        return false;
    QString option = m_currentArg;
    QString argument;
    if (m_appOptions.value(m_currentArg) && nextToken(RequiredToken)) {
        //argument required
        argument = m_currentArg;
    }
    if (m_foundAppOptions)
        m_foundAppOptions->insert(option, argument);
    return true;
}

bool OptionsParser::checkForProfilingOption()
{
    if (m_currentArg != QLatin1String(PROFILE_OPTION))
        return false;
    m_pmPrivate->increaseProfilingVerbosity();
    return true;
}

bool OptionsParser::checkForTraceOption()
{
    if (m_currentArg != QLatin1String(TRACE_OPTION))
        return false;
    if (nextToken(RequiredToken)) {
        m_pmPrivate->enableTracing(m_currentArg);
    }
    return true;
}

bool OptionsParser::checkForNoCrashcheckOption()
{
    if (m_currentArg != QLatin1String(NO_CRASHCHECK_OPTION))
        return false;
    m_pmPrivate->enableCrashCheck = false;
    return true;
}

bool OptionsParser::checkForPluginOption()
{
    bool requiresParameter;
    PluginSpec *spec = m_pmPrivate->pluginForOption(m_currentArg, &requiresParameter);
    if (!spec)
        return false;
    spec->addArgument(m_currentArg);
    m_pmPrivate->argumentsForRestart << m_currentArg;
    if (requiresParameter && nextToken(RequiredToken)) {
        spec->addArgument(m_currentArg);
        m_pmPrivate->argumentsForRestart << m_currentArg;
    }
    return true;
}

bool OptionsParser::checkForUnknownOption()
{
    if (!m_currentArg.startsWith(QLatin1Char('-')))
        return false;
    m_result = ResultError(Tr::tr("Unknown option %1").arg(m_currentArg));
    return true;
}

void OptionsParser::forceDisableAllPluginsExceptTestedAndForceEnabled()
{
    for (const PluginManagerPrivate::TestSpec &testSpec : m_pmPrivate->testSpecs)
        testSpec.pluginSpec->setForceEnabled(true);
    for (PluginSpec *spec : std::as_const(m_pmPrivate->pluginSpecs)) {
        if (!spec->isForceEnabled() && !spec->isRequired())
            spec->setForceDisabled(true);
    }
}

bool OptionsParser::nextToken(OptionsParser::TokenType type)
{
    if (m_it == m_end) {
        if (type == OptionsParser::RequiredToken) {
            m_result = Utils::ResultError(Tr::tr("The option %1 requires an argument.").arg(m_currentArg));
        }
        return false;
    }
    m_currentArg = *m_it;
    ++m_it;
    return true;
}

} // namespace ExtensionSystem::Internal
