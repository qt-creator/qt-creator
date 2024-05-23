// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ldparser.h"
#include "projectexplorerconstants.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace {
    // opt. drive letter + filename: (2 brackets)
    const char * const FILE_PATTERN = "(([A-Za-z]:)?[^:]+\\.[^:]+):";
    // line no. or elf segment + offset (1 bracket)
    const char * const POSITION_PATTERN = "(\\S+|\\(\\..+?[+-]0x[a-fA-F0-9]+\\)):";
    const char * const COMMAND_PATTERN = "^(.*[\\\\/])?([a-z0-9]+-[a-z0-9]+-[a-z0-9]+-)?(ld|gold)(-[0-9\\.]+)?(\\.exe)?: ";
    const char *const RANLIB_PATTERN = "ranlib(.exe)?: (file: (.*) has no symbols)$";
}

LdParser::LdParser()
{
    setObjectName(QLatin1String("LdParser"));
    m_ranlib.setPattern(QLatin1String(RANLIB_PATTERN));
    QTC_CHECK(m_ranlib.isValid());
    m_regExpLinker.setPattern(QLatin1Char('^') +
                              QString::fromLatin1(FILE_PATTERN) + QLatin1Char('(') +
                              QString::fromLatin1(FILE_PATTERN) + QLatin1String(")?(") +
                              QLatin1String(POSITION_PATTERN) + QLatin1String(")?\\s(.+)$"));
    QTC_CHECK(m_regExpLinker.isValid());

    m_regExpGccNames.setPattern(QLatin1String(COMMAND_PATTERN));
    QTC_CHECK(m_regExpGccNames.isValid());
}

Utils::OutputLineParser::Result LdParser::handleLine(const QString &line, Utils::OutputFormat type)
{
    if (type != Utils::StdErrFormat)
        return Status::NotHandled;

    QString lne = rightTrimmed(line);
    if (lne.startsWith(QLatin1String("TeamBuilder "))
            || lne.startsWith(QLatin1String("distcc["))
            || lne.contains(QLatin1String("ar: creating "))) {
        return Status::NotHandled;
    }

    const auto getStatus = [&lne] { return lne.endsWith(':') ? Status::InProgress : Status::Done; };

    // ld on macOS
    if (lne.startsWith("Undefined symbols for architecture") && getStatus() == Status::InProgress) {
        createOrAmendTask(Task::Error, lne, line);
        return Status::InProgress;
    }

    if (!currentTask().isNull() && isContinuation(line)) {
        static const QRegularExpression locRegExp("    (?<symbol>\\S+) in (?<file>\\S+)");
        const QRegularExpressionMatch match = locRegExp.match(lne);
        LinkSpecs linkSpecs;
        Utils::FilePath filePath;
        bool handle = false;
        if (match.hasMatch()) {
            handle = true;
            filePath = absoluteFilePath(Utils::FilePath::fromString(match.captured("file")));
            addLinkSpecForAbsoluteFilePath(linkSpecs, filePath, 0, match, "file");
            currentTask().setFile(filePath);
        } else {
            handle = !lne.isEmpty() && lne.at(0).isSpace();
        }
        if (handle) {
            createOrAmendTask(Task::Unknown, {}, line, true, filePath);
            return {Status::InProgress, linkSpecs};
        }
    }

    if (lne.startsWith("collect2:") || lne.startsWith("collect2.exe:")) {
        createOrAmendTask(Task::Error, lne, line);
        return getStatus();
    }

    QRegularExpressionMatch match = m_ranlib.match(lne);
    if (match.hasMatch()) {
        createOrAmendTask(Task::Warning, match.captured(2), line);
        return getStatus();
    }

    match = m_regExpGccNames.match(lne);
    if (match.hasMatch()) {
        QString description = lne.mid(match.capturedLength());
        Task::TaskType type = Task::Error;
        if (description.startsWith(QLatin1String("warning: "))) {
            type = Task::Warning;
            description = description.mid(9);
        } else if (description.startsWith(QLatin1String("fatal: ")))  {
            description = description.mid(7);
        }
        createOrAmendTask(type, description, line);
        return getStatus();
    }

    match = m_regExpLinker.match(lne);
    if (match.hasMatch() && lne.count(':') >= 2) {
        bool ok;
        int lineno = match.captured(7).toInt(&ok);
        if (!ok)
            lineno = -1;
        Utils::FilePath filePath
                = absoluteFilePath(Utils::FilePath::fromUserInput(match.captured(1)));
        int capIndex = 1;
        const QString sourceFileName = match.captured(4);
        if (!sourceFileName.isEmpty()
            && !sourceFileName.startsWith(QLatin1String("(.text"))
            && !sourceFileName.startsWith(QLatin1String("(.data"))) {
            filePath = absoluteFilePath(Utils::FilePath::fromUserInput(sourceFileName));
            capIndex = 4;
        }
        QString description = match.captured(8).trimmed();
        static const QStringList keywords{
            "File format not recognized",
            "undefined reference",
            "multiple definition",
            "first defined here",
            "feupdateenv is not implemented and will always fail", // yes, this is quite special ...
        };
        const auto descriptionContainsKeyword = [&description](const QString &keyword) {
            return description.contains(keyword);
        };
        Task::TaskType type = Task::Unknown;
        const bool hasKeyword = Utils::anyOf(keywords, descriptionContainsKeyword);
        if (description.startsWith(QLatin1String("warning: "), Qt::CaseInsensitive)) {
            type = Task::Warning;
            description = description.mid(9);
        } else if (hasKeyword) {
            type = Task::Error;
        }
        if (hasKeyword || filePath.fileName().endsWith(".o")) {
            LinkSpecs linkSpecs;
            addLinkSpecForAbsoluteFilePath(linkSpecs, filePath, lineno, match, capIndex);
            createOrAmendTask(type, description, line, false, filePath, lineno, 0, linkSpecs);
            return {getStatus(), linkSpecs};
        }
    }

    return Status::NotHandled;
}

bool LdParser::isContinuation(const QString &line) const
{
    return currentTask().details.last().endsWith(':') || (!line.isEmpty() && line.at(0).isSpace());
}
