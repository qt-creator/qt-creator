// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "storagesettings.h"

#include <coreplugin/icore.h>

#include <utils/hostosinfo.h>

#include <QRegularExpression>

using namespace Utils;

namespace TextEditor {

static const char cleanWhitespaceKey[] = "cleanWhitespace";
static const char inEntireDocumentKey[] = "inEntireDocument";
static const char addFinalNewLineKey[] = "addFinalNewLine";
static const char cleanIndentationKey[] = "cleanIndentation";
static const char skipTrailingWhitespaceKey[] = "skipTrailingWhitespace";
static const char ignoreFileTypesKey[] = "ignoreFileTypes";
static const char defaultTrailingWhitespaceBlacklist[] = "*.md, *.MD, Makefile";

StorageSettings::StorageSettings()
    : m_ignoreFileTypes(defaultTrailingWhitespaceBlacklist),
      m_cleanWhitespace(true),
      m_inEntireDocument(false),
      m_addFinalNewLine(true),
      m_cleanIndentation(true),
      m_skipTrailingWhitespace(true)
{
}

Store StorageSettings::toMap() const
{
    return {
        {cleanWhitespaceKey, m_cleanWhitespace},
        {inEntireDocumentKey, m_inEntireDocument},
        {addFinalNewLineKey, m_addFinalNewLine},
        {cleanIndentationKey, m_cleanIndentation},
        {skipTrailingWhitespaceKey, m_skipTrailingWhitespace},
        {ignoreFileTypesKey, m_ignoreFileTypes}
    };
}

void StorageSettings::fromMap(const Store &map)
{
    m_cleanWhitespace = map.value(cleanWhitespaceKey, m_cleanWhitespace).toBool();
    m_inEntireDocument = map.value(inEntireDocumentKey, m_inEntireDocument).toBool();
    m_addFinalNewLine = map.value(addFinalNewLineKey, m_addFinalNewLine).toBool();
    m_cleanIndentation = map.value(cleanIndentationKey, m_cleanIndentation).toBool();
    m_skipTrailingWhitespace = map.value(skipTrailingWhitespaceKey, m_skipTrailingWhitespace).toBool();
    m_ignoreFileTypes = map.value(ignoreFileTypesKey, m_ignoreFileTypes).toString();
}

bool StorageSettings::removeTrailingWhitespace(const QString &fileName) const
{
    // if the user has elected not to trim trailing whitespace altogether, then
    // early out here
    if (!m_skipTrailingWhitespace) {
        return true;
    }

    const QString ignoreFileTypesRegExp(R"(\s*((?>\*\.)?[\w\d\.\*]+)[,;]?\s*)");

    // use the ignore-files regex to extract the specified file patterns
    QRegularExpression re(ignoreFileTypesRegExp);
    QRegularExpressionMatchIterator iter = re.globalMatch(m_ignoreFileTypes);

    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        QString pattern = match.captured(1);

        QRegularExpression patternRegExp(QRegularExpression::wildcardToRegularExpression(pattern));
        QRegularExpressionMatch patternMatch = patternRegExp.match(fileName);
        if (patternMatch.hasMatch()) {
            // if the filename has a pattern we want to ignore, then we need to return
            // false ("don't remove trailing whitespace")
            return false;
        }
    }

    // the supplied pattern does not match, so we want to remove trailing whitespace
    return true;
}

bool StorageSettings::equals(const StorageSettings &ts) const
{
    return m_addFinalNewLine == ts.m_addFinalNewLine
        && m_cleanWhitespace == ts.m_cleanWhitespace
        && m_inEntireDocument == ts.m_inEntireDocument
        && m_cleanIndentation == ts.m_cleanIndentation
        && m_skipTrailingWhitespace == ts.m_skipTrailingWhitespace
        && m_ignoreFileTypes == ts.m_ignoreFileTypes;
}

} // namespace TextEditor
