/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "loggingmanager.h"

#include <utils/filepath.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QLibraryInfo>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>

//
//    WARNING! Do not use qDebug(), qWarning() or similar inside this file -
//             same applies for indirect usages (e.g. QTC_ASSERT() and the like).
//             Using static functions of QLoggingCategory may cause dead locks as well.
//

namespace Core {
namespace Internal {

static QtMessageHandler s_originalMessageHandler = nullptr;

static LoggingViewManager *s_instance = nullptr;

static QString levelToString(QtMsgType t)
{
    switch (t) {
    case QtMsgType::QtCriticalMsg: return {"critical"};
    case QtMsgType::QtDebugMsg: return {"debug"};
    case QtMsgType::QtInfoMsg: return {"info"};
    case QtMsgType::QtWarningMsg: return {"warning"};
    default:
        return {"fatal"}; // wrong but we don't care
    }
}

static QtMsgType parseLevel(const QString &level)
{
    switch (level.at(0).toLatin1()) {
    case 'c': return QtMsgType::QtCriticalMsg;
    case 'd': return QtMsgType::QtDebugMsg;
    case 'i': return QtMsgType::QtInfoMsg;
    case 'w': return QtMsgType::QtWarningMsg;
    default:
        return QtMsgType::QtFatalMsg;  // wrong but we don't care
    }
}

static bool parseLine(const QString &line, FilterRuleSpec *filterRule)
{
    const QStringList parts = line.split('=');
    if (parts.size() != 2)
        return false;

    const QString category = parts.at(0);
    static const QRegularExpression regex("^(.+?)(\\.(debug|info|warning|critical))?$");
    const QRegularExpressionMatch match = regex.match(category);
    if (!match.hasMatch())
        return false;

    const QString categoryName = match.captured(1);
    if (categoryName.size() > 2) {
        if (categoryName.mid(1, categoryName.size() - 2).contains('*'))
            return false;
    } else if (categoryName.size() == 2) {
        if (categoryName.count('*') == 2)
            return false;
    }
    filterRule->category = categoryName;

    if (match.capturedLength(2) == 0)
        filterRule->level = Utils::nullopt;
    else
        filterRule->level = Utils::make_optional(parseLevel(match.captured(2).mid(1)));

    const QString enabled = parts.at(1);
    if (enabled == "true" || enabled == "false") {
        filterRule->enabled = (enabled == "true");
        return true;
    }
    return false;
}

static QList<FilterRuleSpec> fetchOriginalRules()
{
    QList<FilterRuleSpec> rules;

    auto appendRulesFromFile = [&rules](const QString &fileName) {
        QSettings iniSettings(fileName, QSettings::IniFormat);
        iniSettings.beginGroup("Rules");
        const QStringList keys = iniSettings.allKeys();
        for (const QString &key : keys) {
            const QString value = iniSettings.value(key).toString();
            FilterRuleSpec filterRule;
            if (parseLine(key + "=" + value, &filterRule))
                rules.append(filterRule);
        }
        iniSettings.endGroup();
    };

    Utils::FilePath iniFile = Utils::FilePath::fromString(
                QLibraryInfo::location(QLibraryInfo::DataPath)).pathAppended("qtlogging.ini");
    if (iniFile.exists())
        appendRulesFromFile(iniFile.toString());

    const QString qtProjectString = QStandardPaths::locate(QStandardPaths::GenericConfigLocation,
                                                           "QtProject/qtlogging.ini");
    if (!qtProjectString.isEmpty())
        appendRulesFromFile(qtProjectString);

    iniFile = Utils::FilePath::fromString(qEnvironmentVariable("QT_LOGGING_CONF"));
    if (iniFile.exists())
        appendRulesFromFile(iniFile.toString());

    if (qEnvironmentVariableIsSet("QT_LOGGING_RULES")) {
        const QStringList rulesStrings = qEnvironmentVariable("QT_LOGGING_RULES").split(';');
        for (const QString &rule : rulesStrings) {
            FilterRuleSpec filterRule;
            if (parseLine(rule, &filterRule))
                rules.append(filterRule);
        }
    }
    return rules;
}

LoggingViewManager::LoggingViewManager(QObject *parent)
    : QObject(parent)
    , m_originalLoggingRules(qEnvironmentVariable("QT_LOGGING_RULES"))
{
    qRegisterMetaType<Core::Internal::LoggingCategoryEntry>();
    s_instance = this;
    s_originalMessageHandler = qInstallMessageHandler(logMessageHandler);
    m_enabled = true;
    m_originalRules = fetchOriginalRules();
    prefillCategories();
    QLoggingCategory::setFilterRules("*=true");
}

LoggingViewManager::~LoggingViewManager()
{
    m_enabled = false;
    qInstallMessageHandler(s_originalMessageHandler);
    s_originalMessageHandler = nullptr;
    qputenv("QT_LOGGING_RULES", m_originalLoggingRules.toLocal8Bit());
    QLoggingCategory::setFilterRules("*=false");
    resetFilterRules();
    s_instance = nullptr;
}

LoggingViewManager *LoggingViewManager::instance()
{
    return s_instance;
}

void LoggingViewManager::logMessageHandler(QtMsgType type, const QMessageLogContext &context,
                                           const QString &mssg)
{
    if (!s_instance->m_enabled) {
        if (s_instance->enabledInOriginalRules(context, type))
            s_originalMessageHandler(type, context, mssg);
        return;
    }

    if (!context.category) {
        s_originalMessageHandler(type, context, mssg);
        return;
    }

    const QString category = QString::fromLocal8Bit(context.category);
    auto it = s_instance->m_categories.find(category);
    if (it == s_instance->m_categories.end()) {
        if (!s_instance->m_listQtInternal && category.startsWith("qt."))
            return;
        LoggingCategoryEntry entry;
        entry.level = QtMsgType::QtDebugMsg;
        entry.enabled = (category == "default") || s_instance->enabledInOriginalRules(context, type);
        it = s_instance->m_categories.insert(category, entry);
        emit s_instance->foundNewCategory(category, entry);
    }

    const LoggingCategoryEntry entry = it.value();
    if (entry.enabled && enabled(type, entry.level)) {
        const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
        emit s_instance->receivedLog(timestamp, category,
                                     LoggingViewManager::messageTypeToString(type), mssg);
    }
}

bool LoggingViewManager::isCategoryEnabled(const QString &category)
{
    auto entry = m_categories.find(category);
    if (entry == m_categories.end()) // shall not happen - paranoia
        return false;

    return entry.value().enabled;
}

void LoggingViewManager::setCategoryEnabled(const QString &category, bool enabled)
{
    auto entry = m_categories.find(category);
    if (entry == m_categories.end()) // shall not happen - paranoia
        return;

    entry->enabled = enabled;
}

void LoggingViewManager::setLogLevel(const QString &category, QtMsgType type)
{
    auto entry = m_categories.find(category);
    if (entry == m_categories.end()) // shall not happen - paranoia
        return;

    entry->level = type;
}

void LoggingViewManager::setListQtInternal(bool listQtInternal)
{
    m_listQtInternal = listQtInternal;
}

void LoggingViewManager::appendOrUpdate(const QString &category, const LoggingCategoryEntry &entry)
{
    auto it = m_categories.find(category);
    bool append = it == m_categories.end();
    m_categories.insert(category, entry);
    if (append)
        emit foundNewCategory(category, entry);
    else
        emit updatedCategory(category, entry);
}

/*
 * Does not check categories for being present, will perform early exit if m_categories is not empty
 */
void LoggingViewManager::prefillCategories()
{
    if (!m_categories.isEmpty())
        return;

    for (int i = 0, end = m_originalRules.size(); i < end; ++i) {
        const FilterRuleSpec &rule = m_originalRules.at(i);
        if (rule.category.startsWith('*') || rule.category.endsWith('*'))
            continue;

        bool enabled = rule.enabled;
        // check following rules whether they might overwrite
        for (int j = i + 1; j < end; ++j) {
            const FilterRuleSpec &secondRule = m_originalRules.at(j);
            const QRegularExpression regex(
                        QRegularExpression::wildcardToRegularExpression(secondRule.category));
            if (!regex.match(rule.category).hasMatch())
                continue;

            if (secondRule.level.has_value() && rule.level != secondRule.level)
                continue;

            enabled = secondRule.enabled;
        }
        LoggingCategoryEntry entry;
        entry.level = rule.level.value_or(QtMsgType::QtInfoMsg);
        entry.enabled = enabled;
        m_categories.insert(rule.category, entry);
    }
}

void LoggingViewManager::resetFilterRules()
{
    for (const FilterRuleSpec &rule : qAsConst(m_originalRules)) {
        const QString level = rule.level.has_value() ? '.' + levelToString(rule.level.value())
                                                     : QString();
        const QString ruleString = rule.category + level + '=' + (rule.enabled ? "true" : "false");
        QLoggingCategory::setFilterRules(ruleString);
    }
}

bool LoggingViewManager::enabledInOriginalRules(const QMessageLogContext &context, QtMsgType type)
{
    if (!context.category)
        return false;
    const QString category = QString::fromUtf8(context.category);
    bool result = false;
    for (const FilterRuleSpec &rule : qAsConst(m_originalRules)) {
        const QRegularExpression regex(
                    QRegularExpression::wildcardToRegularExpression(rule.category));
        if (regex.match(category).hasMatch()) {
            if (rule.level.has_value()) {
                if (rule.level.value() == type)
                    result = rule.enabled;
            } else {
                result = rule.enabled;
            }
        }
    }
    return result;
}

} // namespace Internal
} // namespace Core
