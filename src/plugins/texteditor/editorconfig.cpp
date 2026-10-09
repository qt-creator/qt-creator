// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "editorconfig.h"

#include "extraencodingsettings.h"
#include "marginsettings.h"
#include "storagesettings.h"
#include "tabsettings.h"
#include "texteditortr.h"

#include <utils/filepath.h>
#include <utils/layoutbuilder.h>

#include <QDateTime>
#include <QDeadlineTimer>
#include <QHash>
#include <QMutex>
#include <QRegularExpression>

#include <chrono>
#include <memory>
#include <optional>

using namespace Utils;

namespace TextEditor {

namespace Internal {

const int kMaxSectionNameLength = 4096;
const int kMaxKeyLength = 1024;
const int kMaxValueLength = 4096;
const int kMaxBraceDepth = 32;
const qint64 kMaxFileSize = 1024 * 1024;
const std::chrono::seconds kRecheckInterval(2);

struct NumericRange
{
    int min = 0;
    int max = 0;
};

class EditorConfigGlob
{
public:
    explicit EditorConfigGlob(const QString &glob);

    bool matches(const QString &relativeFilePath) const;

private:
    QRegularExpression m_regExp;
    QList<NumericRange> m_ranges;
};

struct EditorConfigSection
{
    EditorConfigGlob glob;
    QList<std::pair<QString, QString>> properties;
};

struct EditorConfigFile
{
    bool root = false;
    QList<EditorConfigSection> sections;
};

static int findClosingBracket(const QString &glob, int open)
{
    for (int i = open + 1; i < glob.size(); ++i) {
        const QChar c = glob.at(i);
        if (c == '\\')
            ++i;
        else if (c == ']')
            return i;
    }
    return -1;
}

static int findClosingBrace(const QString &glob, int open)
{
    int depth = 0;
    for (int i = open; i < glob.size(); ++i) {
        const QChar c = glob.at(i);
        if (c == '\\') {
            ++i;
        } else if (c == '{') {
            ++depth;
        } else if (c == '}') {
            if (--depth == 0)
                return i;
        }
    }
    return -1;
}

static QStringList splitAlternatives(const QString &content)
{
    QStringList result;
    int depth = 0;
    int start = 0;
    for (int i = 0; i < content.size(); ++i) {
        const QChar c = content.at(i);
        if (c == '\\') {
            ++i;
        } else if (c == '{') {
            ++depth;
        } else if (c == '}') {
            --depth;
        } else if (c == ',' && depth == 0) {
            result.append(content.mid(start, i - start));
            start = i + 1;
        }
    }
    result.append(content.mid(start));
    return result;
}

static QString bracketToRegExp(const QString &content)
{
    QString result = "[";
    int i = 0;
    const bool negated = content.startsWith('!') || content.startsWith('^');
    if (negated) {
        result += "^/";
        i = 1;
    }
    const int first = i;
    for (; i < content.size(); ++i) {
        QChar c = content.at(i);
        if (c == '-' && i > first && i + 1 < content.size()) {
            result += c;
            continue;
        }
        if (c == '\\' && i + 1 < content.size())
            c = content.at(++i);
        if (c == '\\' || c == ']' || c == '[' || c == '^' || c == '-')
            result += '\\';
        result += c;
    }
    result += ']';
    return result;
}

static QString globToRegExp(const QString &glob, QList<NumericRange> &ranges, int depth = 0)
{
    static const QRegularExpression rangeRegExp(R"(^([+-]?\d+)\.\.([+-]?\d+)$)");

    QString result;
    for (int i = 0; i < glob.size(); ++i) {
        const QChar c = glob.at(i);
        if (c == '\\') {
            if (i + 1 < glob.size())
                ++i;
            result += QRegularExpression::escape(glob.mid(i, 1));
        } else if (c == '*') {
            if (i + 1 < glob.size() && glob.at(i + 1) == '*') {
                const bool afterSlash = i > 0 && glob.at(i - 1) == '/';
                ++i;
                if (afterSlash && i + 1 < glob.size() && glob.at(i + 1) == '/') {
                    // "a/**/b" also matches "a/b".
                    ++i;
                    result += "(?:.*/)?";
                } else {
                    result += ".*";
                }
            } else {
                result += "[^/]*";
            }
        } else if (c == '?') {
            result += "[^/]";
        } else if (c == '[') {
            const int close = findClosingBracket(glob, i);
            const QString content = close < 0 ? QString() : glob.mid(i + 1, close - i - 1);
            if (content.isEmpty() || content.contains('/')) {
                result += "\\[";
            } else {
                result += bracketToRegExp(content);
                i = close;
            }
        } else if (c == '{') {
            const int close = depth < kMaxBraceDepth ? findClosingBrace(glob, i) : -1;
            if (close < 0) {
                result += "\\{";
                continue;
            }
            const QString content = glob.mid(i + 1, close - i - 1);
            i = close;
            const QRegularExpressionMatch match = rangeRegExp.match(content);
            if (match.hasMatch()) {
                const int first = match.captured(1).toInt();
                const int second = match.captured(2).toInt();
                ranges.append({qMin(first, second), qMax(first, second)});
                result += "([+-]?\\d+)";
                continue;
            }
            const QStringList alternatives = splitAlternatives(content);
            if (alternatives.size() < 2) {
                result += "\\{" + globToRegExp(content, ranges, depth + 1) + "\\}";
                continue;
            }
            QStringList converted;
            for (const QString &alternative : alternatives)
                converted.append(globToRegExp(alternative, ranges, depth + 1));
            result += "(?:" + converted.join('|') + ')';
        } else {
            result += QRegularExpression::escape(QString(c));
        }
    }
    return result;
}

/*!
    A glob without a slash matches the file name in any subdirectory, one with
    a slash is anchored at the directory of the \c .editorconfig file.
*/
EditorConfigGlob::EditorConfigGlob(const QString &glob)
{
    QString pattern = glob;
    QString prefix;
    if (pattern.contains('/')) {
        if (pattern.startsWith('/'))
            pattern.remove(0, 1);
        if (pattern.startsWith(QLatin1String("**/"))) {
            pattern.remove(0, 3);
            prefix = "(?:.*/)?";
        }
    } else {
        prefix = "(?:.*/)?";
    }

    m_regExp.setPattern(
        QRegularExpression::anchoredPattern(prefix + globToRegExp(pattern, m_ranges)));
}

/*!
    Returns whether \a relativeFilePath, given relative to the directory
    of the \c .editorconfig file, matches the glob.

    Numeric ranges are checked after matching, as in the reference
    implementation: a file that a range rejects does not get to match a
    later alternative of the same braces.
*/
bool EditorConfigGlob::matches(const QString &relativeFilePath) const
{
    if (!m_regExp.isValid())
        return false;

    const QRegularExpressionMatch match = m_regExp.match(relativeFilePath);
    if (!match.hasMatch())
        return false;
    for (int i = 0; i < m_ranges.size(); ++i) {
        if (match.capturedStart(i + 1) < 0)
            continue;
        bool ok = false;
        const int number = match.captured(i + 1).toInt(&ok);
        if (!ok || number < m_ranges.at(i).min || number > m_ranges.at(i).max)
            return false;
    }
    return true;
}

bool editorConfigGlobMatches(const QString &glob, const QString &relativeFilePath)
{
    return EditorConfigGlob(glob).matches(relativeFilePath);
}

static EditorConfigFile parseEditorConfig(const QByteArray &contents)
{
    EditorConfigFile result;
    QString text = QString::fromUtf8(contents);
    if (text.startsWith(QChar(0xfeff)))
        text.remove(0, 1);

    int currentSection = -1;
    bool inInvalidSection = false;
    for (const QString &rawLine : text.split('\n')) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith('#') || line.startsWith(';'))
            continue;
        if (line.startsWith('[')) {
            inInvalidSection = !line.endsWith(']')
                               || line.size() - 2 > kMaxSectionNameLength;
            if (!inInvalidSection) {
                result.sections.append(
                    {EditorConfigGlob(line.mid(1, line.size() - 2)), {}});
                currentSection = result.sections.size() - 1;
            }
            continue;
        }
        const int equals = line.indexOf('=');
        if (equals <= 0 || inInvalidSection)
            continue;
        const QString key = line.left(equals).trimmed().toLower();
        const QString value = line.mid(equals + 1).trimmed();
        if (key.size() > kMaxKeyLength || value.size() > kMaxValueLength)
            continue;
        if (currentSection < 0) {
            if (key == "root")
                result.root = value.toLower() == "true";
            continue;
        }
        result.sections[currentSection].properties.append({key, value});
    }
    return result;
}

// A lookup checks every directory up to the root, which is costly on remote devices, so
// the result for a directory, including a missing file, is reused for a short while.
struct CachedEditorConfigFile
{
    QDateTime lastModified;
    std::shared_ptr<const EditorConfigFile> file;
    QDeadlineTimer recheck;
};

struct EditorConfigCache
{
    QMutex mutex;
    QHash<FilePath, CachedEditorConfigFile> files;
};

static EditorConfigCache &editorConfigCache()
{
    static EditorConfigCache cache;
    return cache;
}

int editorConfigCacheSize()
{
    EditorConfigCache &cache = editorConfigCache();
    QMutexLocker locker(&cache.mutex);
    return cache.files.size();
}

static std::shared_ptr<const EditorConfigFile> editorConfigFileInDirectory(const FilePath &dir)
{
    EditorConfigCache &cache = editorConfigCache();
    const FilePath filePath = dir.pathAppended(".editorconfig");
    std::optional<CachedEditorConfigFile> cached;
    {
        QMutexLocker locker(&cache.mutex);
        if (const auto it = cache.files.constFind(filePath); it != cache.files.constEnd())
            cached = *it;
    }
    if (cached && !cached->recheck.hasExpired())
        return cached->file;

    const QDateTime lastModified = filePath.lastModified();
    std::shared_ptr<const EditorConfigFile> file;
    if (cached && cached->file && cached->lastModified == lastModified) {
        file = cached->file;
    } else if (lastModified.isValid()) {
        const Result<QByteArray> contents = filePath.fileContents(kMaxFileSize + 1);
        if (contents && contents->size() <= kMaxFileSize)
            file = std::make_shared<const EditorConfigFile>(parseEditorConfig(*contents));
    }

    QMutexLocker locker(&cache.mutex);
    if (cache.files.size() >= kMaxCachedEditorConfigDirectories) {
        cache.files.removeIf([](const auto &it) { return it.value().recheck.hasExpired(); });
        if (cache.files.size() >= kMaxCachedEditorConfigDirectories)
            cache.files.clear();
    }
    cache.files.insert(filePath, {lastModified, file, QDeadlineTimer(kRecheckInterval)});
    return file;
}

/*!
    Returns the EditorConfig properties that apply to \a filePath, collected
    from the \c .editorconfig files in its directory and the directories above
    it up to the first one declaring \c {root = true}.

    Property names are lower case. A property whose value is \c unset is
    not contained in the result.
*/
QMap<QString, QString> editorConfigPropertiesForFile(const FilePath &filePath)
{
    // A relative path would be resolved against the working directory.
    if (!filePath.isAbsolutePath())
        return {};

    QList<std::pair<FilePath, std::shared_ptr<const EditorConfigFile>>> files;
    for (FilePath dir = filePath.parentDir(); !dir.isEmpty();) {
        if (std::shared_ptr<const EditorConfigFile> file = editorConfigFileInDirectory(dir)) {
            const bool root = file->root;
            files.prepend({dir, file});
            if (root)
                break;
        }
        const FilePath parent = dir.parentDir();
        if (parent == dir)
            break;
        dir = parent;
    }

    QMap<QString, QString> properties;
    for (const auto &[dir, file] : std::as_const(files)) {
        const QString relativePath = filePath.relativeChildPath(dir).path();
        if (relativePath.isEmpty())
            continue;
        for (const EditorConfigSection &section : file->sections) {
            if (!section.glob.matches(relativePath))
                continue;
            for (const auto &[key, value] : section.properties) {
                if (value.toLower() == "unset")
                    properties.remove(key);
                else
                    properties.insert(key, value);
            }
        }
    }
    return properties;
}

} // namespace Internal

/*!
    \class TextEditor::EditorConfigProperties
    \brief The EditorConfigProperties class holds the settings an \c .editorconfig
    file defines for a file.

    Properties that are not set leave the corresponding editor settings untouched.

    \sa {https://editorconfig.org}
*/

EditorConfigProperties EditorConfigProperties::forFile(const FilePath &filePath)
{
    if (filePath.isEmpty() || !globalEditorConfigSettings().enabled())
        return {};
    return fromProperties(Internal::editorConfigPropertiesForFile(filePath));
}

static std::optional<int> numberInRange(const QString &value, int min, int max)
{
    bool ok = false;
    const int number = value.toInt(&ok);
    if (ok && number >= min && number <= max)
        return number;
    return {};
}

// Within the limits of the tab settings page.
static std::optional<int> indentationWidth(const QString &value)
{
    return numberInRange(value, 1, 20);
}

static std::optional<bool> boolean(const QString &value)
{
    if (value == "true")
        return true;
    if (value == "false")
        return false;
    return {};
}

EditorConfigProperties EditorConfigProperties::fromProperties(
    const QMap<QString, QString> &properties)
{
    const auto value = [&properties](const QString &key) {
        return properties.value(key).toLower();
    };

    EditorConfigProperties result;

    const QString indentStyle = value("indent_style");
    if (indentStyle == "tab")
        result.indentWithTabs = true;
    else if (indentStyle == "space")
        result.indentWithTabs = false;

    result.tabWidth = indentationWidth(value("tab_width"));

    const QString indentSize = value("indent_size");
    if (indentSize == "tab" || (indentSize.isEmpty() && result.indentWithTabs.value_or(false))) {
        if (result.tabWidth)
            result.indentSize = result.tabWidth;
        else
            result.indentSizeIsTabWidth = true;
    } else {
        result.indentSize = indentationWidth(indentSize);
    }
    if (result.indentSize && !result.tabWidth)
        result.tabWidth = result.indentSize;

    const QString endOfLine = value("end_of_line");
    if (endOfLine == "lf")
        result.lineTerminationMode = TextFileFormat::LFLineTerminator;
    else if (endOfLine == "crlf")
        result.lineTerminationMode = TextFileFormat::CRLFLineTerminator;

    const QString charset = value("charset");
    TextEncoding encoding;
    if (charset == "utf-8" || charset == "utf-8-bom") {
        encoding = TextEncoding(TextEncoding::Utf8);
        result.utf8Bom = charset == "utf-8-bom";
    } else if (charset == "latin1") {
        encoding = TextEncoding(TextEncoding::Latin1);
    } else if (charset == "utf-16be") {
        encoding = TextEncoding("UTF-16BE");
    } else if (charset == "utf-16le") {
        encoding = TextEncoding("UTF-16LE");
    }
    if (encoding.isValid())
        result.encoding = encoding;

    result.trimTrailingWhitespace = boolean(value("trim_trailing_whitespace"));
    result.insertFinalNewline = boolean(value("insert_final_newline"));
    result.maxLineLength = numberInRange(value("max_line_length"), 1, 999);

    return result;
}

bool EditorConfigProperties::isEmpty() const
{
    return !indentWithTabs && !indentSize && !indentSizeIsTabWidth && !tabWidth
           && !lineTerminationMode && !encoding && !utf8Bom && !trimTrailingWhitespace
           && !insertFinalNewline && !maxLineLength;
}

void EditorConfigProperties::applyTo(TabSettingsData &tabSettings) const
{
    if (indentWithTabs) {
        tabSettings.m_tabPolicy = *indentWithTabs ? TabSettingsData::TabsOnlyTabPolicy
                                                  : TabSettingsData::SpacesOnlyTabPolicy;
        tabSettings.m_autoDetect = false;
    }
    if (tabWidth)
        tabSettings.m_tabSize = *tabWidth;
    if (indentSize) {
        tabSettings.m_indentSize = *indentSize;
        tabSettings.m_autoDetect = false;
    } else if (indentSizeIsTabWidth) {
        tabSettings.m_indentSize = tabSettings.m_tabSize;
        tabSettings.m_autoDetect = false;
    }
}

void EditorConfigProperties::applyTo(StorageSettingsData &storageSettings) const
{
    if (trimTrailingWhitespace) {
        storageSettings.m_cleanWhitespace = *trimTrailingWhitespace;
        if (*trimTrailingWhitespace) {
            storageSettings.m_inEntireDocument = true;
            storageSettings.m_cleanIndentation = false;
            storageSettings.m_skipTrailingWhitespace = false;
        }
    }
    if (insertFinalNewline)
        storageSettings.m_addFinalNewLine = *insertFinalNewline;
}

void EditorConfigProperties::applyTo(ExtraEncodingSettingsData &extraEncodingSettings) const
{
    if (utf8Bom) {
        extraEncodingSettings.m_utf8BomSetting = *utf8Bom ? ExtraEncodingSettingsData::AlwaysAdd
                                                          : ExtraEncodingSettingsData::AlwaysDelete;
    }
}

void EditorConfigProperties::applyTo(MarginSettingsData &marginSettings) const
{
    if (maxLineLength)
        marginSettings.m_marginColumn = *maxLineLength;
}

EditorConfigSettings::EditorConfigSettings()
{
    setAutoApply(false);

    setSettingsGroup("textEditorConfigSettings");

    enabled.setSettingsKey("enabled");
    enabled.setDefaultValue(true);
    enabled.setLabelText(Tr::tr("Use settings from .editorconfig files"));
    enabled.setToolTip(Tr::tr("Applies the indentation, encoding, line ending and whitespace "
                              "settings of .editorconfig files to files when opening them."));

    setLayouter([this] {
        using namespace Layouting;
        return Column {
            Group {
                title(Tr::tr("EditorConfig")),
                Column { enabled }
            }
        };
    });

    readSettings();
}

void EditorConfigSettings::apply()
{
    AspectContainer::apply();
    AspectContainer::writeSettings();
}

EditorConfigSettings &globalEditorConfigSettings()
{
    static EditorConfigSettings theGlobalEditorConfigSettings;
    return theGlobalEditorConfigSettings;
}

} // namespace TextEditor
