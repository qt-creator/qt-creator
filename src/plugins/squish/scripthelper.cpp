// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "scripthelper.h"

#include "suiteconf.h"

#include <utils/filepath.h>

#include <QXmlStreamReader>

namespace Squish {
namespace Internal {

static QByteArray startApplication(Language language,
                                   const QString &application,
                                   const QString &args)
{
    const QString app = application.contains(' ') ? QString("\\\"" + application + "\\\"")
                                                  : application;
    QStringList parameters;
    parameters << app;
    if (!args.isEmpty())
        parameters << QString(args).replace('"', "\\\"");

    switch (language) {
    case Language::Perl:
    case Language::JavaScript:
        return QByteArray("startApplication(\"" + parameters.join(' ').toUtf8() + "\");");
    case Language::Python:
    case Language::Ruby:
        return QByteArray("startApplication(\"" + parameters.join(' ').toUtf8() + "\")");
    case Language::Tcl:
        return QByteArray("startApplication \"" + parameters.join(' ').toUtf8() + "\"");
    }
    return {};
}

static QByteArrayList mainFunctionHead(Language language)
{
    switch (language) {
    case Language::Python: return { "def main():" };
    case Language::Perl: return { "sub main {" };
    case Language::JavaScript: return { "function main()", "{" };
    case Language::Ruby: return { "def main" };
    case Language::Tcl: return { "proc main {} {" };
    }
    return {};
}

static QByteArrayList functionFooter(Language language)
{
    switch (language) {
    case Language::Python: return {};
    case Language::Perl: return { "}" };
    case Language::JavaScript: return { "}" };
    case Language::Ruby: return {};
    case Language::Tcl: return { "}" };
    }
    return {};
}

struct SnippetParts
{
    QByteArrayList header;
    QByteArrayList script;
};

static SnippetParts getSnippetPartsFrom(const Utils::FilePath &snippetFile)
{
    SnippetParts parts;
    QXmlStreamReader reader(snippetFile.fileContents().value_or(QByteArray()));

    QString content;
    while (!reader.atEnd()) {
        QXmlStreamReader::TokenType tokenType = reader.readNext();
        switch (tokenType) {
        case QXmlStreamReader::EndElement: {
            const QString currentName = reader.name().toString();
            if (currentName == "scriptHeader") {
                const QStringList lines = content.split('\n');
                for (const QString &line : lines)
                    parts.header.append(line.toUtf8() + '\n');
                content.clear();
            } else if (currentName == "scriptSnippet") {
                const QStringList lines = content.split('\n');
                for (const QString &line : lines)
                    parts.script.append(line.toUtf8() + '\n');
                content.clear();
            }
            break;
        }
        case QXmlStreamReader::Characters: {
            QString text = reader.text().toString();
            if (reader.isCDATA() || !text.trimmed().isEmpty()) {
                if (!reader.isCDATA())
                    text = text.trimmed();
                content.append(text).append('\n');
            }
            break;
        }
        case QXmlStreamReader::Invalid:
            return {};
        case QXmlStreamReader::StartDocument:
        case QXmlStreamReader::EndDocument:
        case QXmlStreamReader::StartElement:
        default:
            break;
        }
    }
    return parts;
}

ScriptHelper::ScriptHelper(Language language)
    : m_language(language)
{}

bool ScriptHelper::writeScriptFile(const Utils::FilePath &outScriptFile,
                                   const Utils::FilePath &snippetFile,
                                   const QString &application,
                                   const QString &arguments) const
{
    if (!snippetFile.isReadableFile())
        return false;
    if (!outScriptFile.parentDir().isWritableDir())
        return false;

    const SnippetParts snippetParts = getSnippetPartsFrom(snippetFile);
    if (snippetParts.script.isEmpty()) // if there is no recording avoid overwriting
        return false;

    QByteArray data;
    for (const QByteArray &line : snippetParts.header)
        data.append(line);

    for (const QByteArray &line : mainFunctionHead(m_language))
        data.append(line).append('\n');

    data.append("    ").append(startApplication(m_language, application, arguments)).append('\n');

    for (const QByteArray &line : snippetParts.script)
        data.append(line);

    for (const QByteArray &line : functionFooter(m_language))
        data.append(line).append('\n');

    const Utils::expected_str<qint64> result = outScriptFile.writeFileContents(data);
    QTC_ASSERT_EXPECTED(result, return false);
    return true;
}

} // namespace Internal
} // namespace Squish
