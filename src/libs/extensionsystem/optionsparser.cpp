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

#include "optionsparser.h"

#include "pluginmanager.h"
#include "pluginmanager_p.h"
#include "pluginspec_p.h"

#include <utils/algorithm.h>

#include <QCoreApplication>

using namespace ExtensionSystem;
using namespace ExtensionSystem::Internal;

const char END_OF_OPTIONS[] = "--";
const char *OptionsParser::NO_LOAD_OPTION = "-noload";
const char *OptionsParser::LOAD_OPTION = "-load";
const char *OptionsParser::TEST_OPTION = "-test";
const char *OptionsParser::NOTEST_OPTION = "-notest";
const char *OptionsParser::PROFILE_OPTION = "-profile";

OptionsParser::OptionsParser(const QStringList &args,
        const QMap<QString, bool> &appOptions,
        QMap<QString, QString> *foundAppOptions,
        QString *errorString,
        PluginManagerPrivate *pmPrivate)
    : m_args(args), m_appOptions(appOptions),
      m_foundAppOptions(foundAppOptions),
      m_errorString(errorString),
      m_pmPrivate(pmPrivate),
      m_it(m_args.constBegin()),
      m_end(m_args.constEnd()),
      m_isDependencyRefreshNeeded(false),
      m_hasError(false)
{
    ++m_it; // jump over program name
    if (m_errorString)
        m_errorString->clear();
    if (m_foundAppOptions)
        m_foundAppOptions->clear();
    m_pmPrivate->arguments.clear();
}

bool OptionsParser::parse()
{
    while (!m_hasError) {
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
#ifdef WITH_TESTS
        if (checkForTestOptions())
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
    return !m_hasError;
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
                m_pmPrivate->testSpecs =
                        Utils::transform(m_pmPrivate->loadQueue(), [](PluginSpec *spec) {
                            return PluginManagerPrivate::TestSpec(spec);
                        });
            } else {
                QStringList args = m_currentArg.split(QLatin1Char(','));
                const QString pluginName = args.takeFirst();
                if (PluginSpec *spec = m_pmPrivate->pluginByName(pluginName)) {
                    if (m_pmPrivate->containsTestSpec(spec)) {
                        if (m_errorString)
                            *m_errorString = QCoreApplication::translate("PluginManager",
                                                                         "The plugin \"%1\" is specified twice for testing.").arg(pluginName);
                        m_hasError = true;
                    } else {
                        m_pmPrivate->testSpecs.append(PluginManagerPrivate::TestSpec(spec, args));
                    }
                } else  {
                    if (m_errorString)
                        *m_errorString = QCoreApplication::translate("PluginManager",
                                                                     "The plugin \"%1\" does not exist.").arg(pluginName);
                    m_hasError = true;
                }
            }
        }
        return true;
    } else if (m_currentArg == QLatin1String(NOTEST_OPTION)) {
        if (nextToken(RequiredToken)) {
            if (PluginSpec *spec = m_pmPrivate->pluginByName(m_currentArg)) {
                if (!m_pmPrivate->containsTestSpec(spec)) {
                    if (m_errorString)
                        *m_errorString = QCoreApplication::translate("PluginManager",
                                                                     "The plugin \"%1\" is not tested.").arg(m_currentArg);
                    m_hasError = true;
                } else {
                    m_pmPrivate->removeTestSpec(spec);
                }
            } else {
                if (m_errorString)
                    *m_errorString = QCoreApplication::translate("PluginManager",
                                                                 "The plugin \"%1\" does not exist.").arg(m_currentArg);
                m_hasError = true;
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
            foreach (PluginSpec *spec, m_pmPrivate->pluginSpecs)
                spec->d->setForceEnabled(true);
            m_isDependencyRefreshNeeded = true;
        } else {
            PluginSpec *spec = m_pmPrivate->pluginByName(m_currentArg);
            if (!spec) {
                if (m_errorString)
                    *m_errorString = QCoreApplication::translate("PluginManager",
                                                                 "The plugin \"%1\" does not exist.")
                        .arg(m_currentArg);
                m_hasError = true;
            } else {
                spec->d->setForceEnabled(true);
                m_isDependencyRefreshNeeded = true;
            }
        }
    }
    return true;
}

bool OptionsParser::checkForNoLoadOption()
{
    if (m_currentArg != QLatin1String(NO_LOAD_OPTION))
        return false;
    if (nextToken(RequiredToken)) {
        if (m_currentArg == QLatin1String("all")) {
            foreach (PluginSpec *spec, m_pmPrivate->pluginSpecs)
                spec->d->setForceDisabled(true);
            m_isDependencyRefreshNeeded = true;
        } else {
            PluginSpec *spec = m_pmPrivate->pluginByName(m_currentArg);
            if (!spec) {
                if (m_errorString)
                    *m_errorString = QCoreApplication::translate("PluginManager",
                                                                 "The plugin \"%1\" does not exist.").arg(m_currentArg);
                m_hasError = true;
            } else {
                spec->d->setForceDisabled(true);
                // recursively disable all plugins that require this plugin
                foreach (PluginSpec *dependantSpec, PluginManager::pluginsRequiringPlugin(spec))
                    dependantSpec->d->setForceDisabled(true);
                m_isDependencyRefreshNeeded = true;
            }
        }
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
    m_pmPrivate->initProfiling();
    return true;
}

bool OptionsParser::checkForPluginOption()
{
    bool requiresParameter;
    PluginSpec *spec = m_pmPrivate->pluginForOption(m_currentArg, &requiresParameter);
    if (!spec)
        return false;
    spec->addArgument(m_currentArg);
    if (requiresParameter && nextToken(RequiredToken))
        spec->addArgument(m_currentArg);
    return true;
}

bool OptionsParser::checkForUnknownOption()
{
    if (!m_currentArg.startsWith(QLatin1Char('-')))
        return false;
    if (m_errorString)
        *m_errorString = QCoreApplication::translate("PluginManager",
                                                     "Unknown option %1").arg(m_currentArg);
    m_hasError = true;
    return true;
}

void OptionsParser::forceDisableAllPluginsExceptTestedAndForceEnabled()
{
    for (const PluginManagerPrivate::TestSpec &testSpec : m_pmPrivate->testSpecs)
        testSpec.pluginSpec->d->setForceEnabled(true);
    for (PluginSpec *spec : m_pmPrivate->pluginSpecs) {
        if (!spec->isForceEnabled() && !spec->isRequired())
            spec->d->setForceDisabled(true);
    }
}

bool OptionsParser::nextToken(OptionsParser::TokenType type)
{
    if (m_it == m_end) {
        if (type == OptionsParser::RequiredToken) {
            m_hasError = true;
            if (m_errorString)
                *m_errorString = QCoreApplication::translate("PluginManager",
                                                             "The option %1 requires an argument.").arg(m_currentArg);
        }
        return false;
    }
    m_currentArg = *m_it;
    ++m_it;
    return true;
}
