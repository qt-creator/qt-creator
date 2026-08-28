// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdocconfig.h"

#include <utils/algorithm.h>

#include <QRegularExpression>

using namespace Utils;

namespace QDoc::Internal {

static const QChar Pending(0x1f);
static const int MaxIncludeDepth = 20;
static const int MaxValueSplices = 1000;

QHash<QString, QString> buildVariables()
{
    static const QStringList names = {
        "BUILDDIR",
        "QDOC_PROJECT_ROOT",
        "QT_VERSION",
        "QT_VER",
        "QT_VERSION_TAG",
        "QT_SUPPORTED_MIN_MACOS_SDK_VERSION",
        "QT_SUPPORTED_MIN_MACOS_XCODE_VERSION",
        "QT_SUPPORTED_MIN_MACOS_VERSION",
        "QT_SUPPORTED_MAX_MACOS_VERSION_TESTED",
        "QT_SUPPORTED_MIN_IOS_SDK_VERSION",
        "QT_SUPPORTED_MIN_IOS_XCODE_VERSION",
        "QT_SUPPORTED_MIN_IOS_VERSION",
        "QT_SUPPORTED_MAX_IOS_VERSION_TESTED",
        "QT_SUPPORTED_MIN_VISIONOS_SDK_VERSION",
        "QT_SUPPORTED_MIN_VISIONOS_XCODE_VERSION",
        "QT_SUPPORTED_MIN_VISIONOS_VERSION",
        "QT_SUPPORTED_MAX_VISIONOS_VERSION_TESTED",
    };
    QHash<QString, QString> variables;
    for (const QString &name : names)
        variables.insert(name, QString());
    return variables;
}

static bool isMetaKeyChar(QChar c)
{
    return c.isLetterOrNumber() || c == '_' || c == '.' || c == '{' || c == '}' || c == ',';
}

static bool isConfigSpace(QChar c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

static bool isNameChar(QChar c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static QStringList splitTopLevel(const QString &text)
{
    QStringList parts;
    int depth = 0;
    QString current;
    for (const QChar &c : text) {
        if (c == '{') {
            ++depth;
            current += c;
        } else if (c == '}') {
            --depth;
            current += c;
        } else if (c == ',' && depth == 0) {
            parts.append(current);
            current.clear();
        } else {
            current += c;
        }
    }
    parts.append(current);
    return parts;
}

QStringList expandKey(const QString &key)
{
    const int open = key.indexOf('{');
    if (open == -1)
        return {key};
    int depth = 0;
    int close = -1;
    for (int i = open; i < key.size(); ++i) {
        if (key.at(i) == '{') {
            ++depth;
        } else if (key.at(i) == '}') {
            --depth;
            if (depth == 0) {
                close = i;
                break;
            }
        }
    }
    if (close == -1)
        return {key};
    const QString prefix = key.left(open);
    const QString suffix = key.mid(close + 1);
    QStringList out;
    for (const QString &part : splitTopLevel(key.mid(open + 1, close - open - 1)))
        out.append(expandKey(prefix + part + suffix));
    return out;
}

bool wildcardMatches(const QString &pattern, const QString &filePath)
{
    QString body;
    for (const QChar &c : pattern) {
        if (c == '*')
            body += "[^/]*";
        else if (c == '?')
            body += "[^/]";
        else if (QString(".+^${}()|[]\\").contains(c))
            body += '\\' + QString(c);
        else
            body += c;
    }
    const QRegularExpression re(body);
    return re.isValid() && re.match(filePath).hasMatch();
}

void Config::set(const QString &key, const QList<ConfigEntry> &values, bool replace)
{
    if (!m_vars.contains(key))
        m_order.append(key);
    if (replace || !m_vars.contains(key))
        m_vars.insert(key, values);
    else
        m_vars[key].append(values);
}

QStringList Config::values(const QString &key) const
{
    QStringList out;
    for (const ConfigEntry &entry : m_vars.value(key))
        out.append(entry.value);
    return out;
}

QString Config::value(const QString &key, const QString &fallback) const
{
    const QList<ConfigEntry> entries = m_vars.value(key);
    if (entries.isEmpty())
        return fallback;
    // Every value is concatenated, as ConfigVar::asString() does, with a space
    // inserted only where what has accumulated does not already end in a newline.
    QString result;
    for (const ConfigEntry &entry : entries) {
        if (!result.isEmpty() && !result.endsWith('\n'))
            result += ' ';
        result += entry.value;
    }
    return result;
}

bool Config::boolValue(const QString &key, bool fallback) const
{
    const QList<ConfigEntry> entries = m_vars.value(key);
    if (entries.isEmpty())
        return fallback;
    // ConfigVar::asBool() is QVariant(asString()).toBool(), false only for an empty
    // string, 0 and false, which is why documentationinheaders = indeed works.
    const QString text = value(key).trimmed().toLower();
    return !text.isEmpty() && text != "0" && text != "false";
}

int Config::intValue(const QString &key, int fallback) const
{
    bool ok = false;
    const int n = value(key).trimmed().toInt(&ok);
    return ok ? n : fallback;
}

FilePaths Config::pathList(const QString &key, bool mustExist) const
{
    FilePaths out;
    for (const ConfigEntry &entry : m_vars.value(key)) {
        const QString raw = entry.value.trimmed();
        if (raw.isEmpty())
            continue;
        const FilePath path = entry.dir.resolvePath(raw).cleanPath();
        if (mustExist && !path.isDir())
            continue;
        if (!out.contains(path))
            out.append(path);
    }
    return out;
}

FilePaths Config::fileList(const QString &key) const
{
    FilePaths out;
    for (const ConfigEntry &entry : m_vars.value(key)) {
        const QString raw = entry.value.trimmed();
        if (raw.isEmpty())
            continue;
        const FilePath path = entry.dir.resolvePath(raw).cleanPath();
        if (!out.contains(path))
            out.append(path);
    }
    return out;
}

bool Config::excludes(const FilePath &filePath) const
{
    const QString path = filePath.path();
    for (const FilePath &pattern : fileList("excludefiles")) {
        const QString raw = pattern.path();
        if (raw.contains('*') || raw.contains('?')) {
            if (wildcardMatches(raw, path))
                return true;
        } else if (pattern == filePath) {
            return true;
        }
    }
    for (const FilePath &dir : pathList("excludedirs")) {
        if (filePath.isChildOf(dir))
            return true;
    }
    return false;
}

QList<ConfigProblem> Config::missingPaths(const QStringList &keys) const
{
    QList<ConfigProblem> problems;
    for (const QString &key : keys) {
        for (const ConfigEntry &entry : m_vars.value(key)) {
            const QString raw = entry.value.trimmed();
            // A glob is resolved later by the caller, so it cannot be checked here.
            if (raw.isEmpty() || raw.contains('*') || raw.contains('?'))
                continue;
            if (!entry.dir.resolvePath(raw).exists()) {
                problems.append({QString("Cannot find file or directory: %1").arg(raw),
                                 entry.file,
                                 entry.line});
            }
        }
    }
    return problems;
}

QStringList Config::keysWithPrefix(const QString &prefix) const
{
    QStringList out;
    for (const QString &key : m_order) {
        if (key.startsWith(prefix))
            out.append(key);
    }
    return out;
}

namespace {

class ConfigParser
{
public:
    ConfigParser(Config &config, const QHash<QString, QString> &variables)
        : m_config(config)
        , m_variables(variables)
    {}

    void load(const FilePath &file, int depth, const FilePath &includedFrom, int includedFromLine)
    {
        const FilePath resolved = file.cleanPath();
        if (depth > MaxIncludeDepth) {
            m_config.problems.append({"Too many nested includes", resolved, 0});
            return;
        }
        // A diamond is fine and re-reading a file is meaningful in qdocconf, so only
        // refuse a file that is already on the current include stack.
        if (m_loading.contains(resolved))
            return;

        const Result<QByteArray> contents = resolved.fileContents();
        if (!contents) {
            m_config.problems.append(
                {QString("Cannot open '%1'").arg(resolved.toUserOutput())
                     + (includedFrom.isEmpty()
                            ? QString()
                            : QString(" included from '%1'").arg(includedFrom.fileName())),
                 includedFrom.isEmpty() ? resolved : includedFrom,
                 includedFromLine});
            return;
        }

        m_loading.insert(resolved);
        if (!m_config.files.contains(resolved))
            m_config.files.append(resolved);
        QString text = QString::fromUtf8(*contents);
        // QDoc opens configurations with QFile::Text, so its tokenizer never sees a
        // CRLF.
        text.replace("\r\n", "\n");
        parseText(text, resolved, resolved.parentDir(), depth);
        m_loading.remove(resolved);
    }

private:
    std::optional<QString> lookup(const QString &name) const
    {
        const auto it = m_variables.constFind(name);
        if (it != m_variables.constEnd())
            return it.value();
        const QString fromEnvironment = qEnvironmentVariable(name.toLocal8Bit().constData());
        if (qEnvironmentVariableIsSet(name.toLocal8Bit().constData()))
            return fromEnvironment;
        return {};
    }

    void parseText(const QString &initialText, const FilePath &file, const FilePath &baseDir,
                   int depth)
    {
        QString buf = initialText;
        int i = 0;
        int line = 1;
        int splices = 0;

        const auto skipSpaces = [&] {
            while (i < buf.size() && isConfigSpace(buf.at(i)) && buf.at(i) != '\n')
                ++i;
        };
        const auto skipToEol = [&] {
            while (i < buf.size() && buf.at(i) != '\n')
                ++i;
        };

        while (i < buf.size()) {
            const QChar ch = buf.at(i);
            if (isConfigSpace(ch)) {
                if (ch == '\n')
                    ++line;
                ++i;
                continue;
            }
            if (ch == '#') {
                skipToEol();
                continue;
            }
            if (!isMetaKeyChar(ch)) {
                m_config.problems.append(
                    {QString("Unexpected character '%1' at beginning of line").arg(ch),
                     file,
                     line});
                skipToEol();
                continue;
            }

            const int keyLine = line;
            QString rawKey;
            while (i < buf.size() && isMetaKeyChar(buf.at(i)))
                rawKey += buf.at(i++);
            skipSpaces();

            if (rawKey == "include") {
                readInclude(buf, &i, file, baseDir, depth, keyLine, skipSpaces, skipToEol);
                continue;
            }

            bool replace = true;
            if (i < buf.size() && buf.at(i) == '+') {
                replace = false;
                ++i;
            }
            if (i >= buf.size() || buf.at(i) != '=') {
                m_config.problems.append(
                    {QString("Expected '=' or '+=' after key '%1'").arg(rawKey), file, keyLine});
                skipToEol();
                continue;
            }
            ++i;
            skipSpaces();

            QStringList rhs;
            QStringList usedVariables;
            QString word;
            bool inQuote = false;
            // Set when a $VAR contributed to the current word, so an empty result
            // still counts as a value rather than being dropped.
            bool produced = false;

            for (;;) {
                if (i >= buf.size()) {
                    if (!word.isEmpty() || produced)
                        rhs.append(word);
                    break;
                }
                const QChar c = buf.at(i);

                if (c == '\\') {
                    ++i;
                    const QChar n = i < buf.size() ? buf.at(i) : QChar();
                    if (n == '\n' || (n == '\r' && i + 1 < buf.size() && buf.at(i + 1) == '\n')) {
                        ++line;
                        i += n == '\r' ? 2 : 1;
                        skipSpaces();
                    } else if (n >= '1' && n <= '7') {
                        word += QChar(n.digitValue());
                        ++i;
                    } else {
                        static const QString escapes = "abfnrtv";
                        static const QString replacements = "\a\b\f\n\r\t\v";
                        const int idx = escapes.indexOf(n);
                        if (idx != -1) {
                            word += replacements.at(idx);
                            ++i;
                        } else if (!n.isNull()) {
                            word += n;
                            ++i;
                        }
                    }
                } else if (isConfigSpace(c) || c == '#') {
                    if (inQuote) {
                        if (c == '\n') {
                            m_config.problems.append({"Unterminated string", file, keyLine});
                            if (!word.isEmpty() || produced)
                                rhs.append(word);
                            break;
                        }
                        word += c;
                        ++i;
                    } else {
                        if (!word.isEmpty() || produced) {
                            rhs.append(word);
                            word.clear();
                            produced = false;
                        }
                        if (c == '\n' || c == '#')
                            break;
                        skipSpaces();
                    }
                } else if (c == '"') {
                    if (inQuote) {
                        if (!word.isEmpty() || produced)
                            rhs.append(word);
                        word.clear();
                        produced = false;
                    }
                    inQuote = !inQuote;
                    ++i;
                } else if (c == '$') {
                    ++i;
                    bool braces = false;
                    if (i < buf.size() && buf.at(i) == '{') {
                        braces = true;
                        ++i;
                    }
                    QString name;
                    while (i < buf.size() && isNameChar(buf.at(i)))
                        name += buf.at(i++);
                    if (braces) {
                        // ${VAR,delim} overrides the list separator. The preview reads
                        // scalars, so the delimiter is skipped.
                        if (i < buf.size() && buf.at(i) == ',')
                            i += 2;
                        if (i < buf.size() && buf.at(i) == '}')
                            ++i;
                    }
                    if (!name.isEmpty()) {
                        if (!usedVariables.contains(name))
                            usedVariables.append(name);
                        const std::optional<QString> val = lookup(name);
                        if (!val) {
                            // It may still be a key of this configuration, declared in
                            // a file not read yet, so leave a mark and settle it once
                            // everything is loaded.
                            produced = true;
                            word += Pending + name + Pending;
                        } else if (braces && splices < MaxValueSplices) {
                            ++splices;
                            buf.insert(i, *val);
                        } else if (braces) {
                            if (splices == MaxValueSplices) {
                                ++splices;
                                m_config.problems.append(
                                    {QString("${%1} expanded too many times %2 recursive value?")
                                         .arg(name)
                                         .arg(QChar(0x2014)),
                                     file,
                                     keyLine});
                            }
                            word += *val;
                        } else {
                            word += *val;
                        }
                    }
                } else {
                    if (!inQuote && c == '=') {
                        m_config.problems.append({"Unexpected '='", file, keyLine});
                        skipToEol();
                        break;
                    }
                    word += c;
                    ++i;
                }
            }

            for (const QString &key : expandKey(rawKey)) {
                if (!usedVariables.isEmpty()) {
                    QStringList seen = m_config.expandedIn.value(key);
                    for (const QString &name : usedVariables) {
                        if (!seen.contains(name))
                            seen.append(name);
                    }
                    m_config.expandedIn.insert(key, seen);
                }
                QList<ConfigEntry> entries;
                for (const QString &value : rhs)
                    entries.append({value, baseDir, file, keyLine});
                m_config.set(key, entries, replace);
            }
        }
    }

    template<typename SkipSpaces, typename SkipToEol>
    void readInclude(const QString &buf, int *pos, const FilePath &file, const FilePath &baseDir,
                     int depth, int keyLine, SkipSpaces skipSpaces, SkipToEol skipToEol)
    {
        int &i = *pos;
        if (i >= buf.size() || buf.at(i) != '(') {
            m_config.problems.append({"Bad include syntax", file, keyLine});
            skipToEol();
            return;
        }
        ++i;
        skipSpaces();
        QString target;
        QString unresolved;
        while (i < buf.size() && !isConfigSpace(buf.at(i)) && buf.at(i) != '#'
               && buf.at(i) != ')') {
            if (buf.at(i) == '$') {
                ++i;
                QString name;
                while (i < buf.size() && isNameChar(buf.at(i)))
                    name += buf.at(i++);
                if (!name.isEmpty()) {
                    const std::optional<QString> val = lookup(name);
                    if (!val)
                        unresolved = name;
                    else
                        target += *val;
                }
            } else {
                target += buf.at(i++);
            }
        }
        skipToEol();

        if (!unresolved.isEmpty()) {
            m_config.problems.append(
                {QString("Cannot resolve $%1 in include().").arg(unresolved), file, keyLine});
        } else if (!target.isEmpty()) {
            load(baseDir.resolvePath(target), depth + 1, file, keyLine);
        }
    }

    Config &m_config;
    QHash<QString, QString> m_variables;
    QSet<FilePath> m_loading;
};

// Resolves the $key references left marked during parsing, mirroring
// Config::expandVariables(), which runs after every file has been read so a
// reference may point at a key declared later.
void expandPending(Config *config, QHash<QString, QList<ConfigEntry>> *vars,
                   const QStringList &order)
{
    const QRegularExpression marked(QString("%1([^%1]*)%1").arg(Pending));
    for (const QString &key : order) {
        const auto it = vars->find(key);
        if (it == vars->end())
            continue;
        for (ConfigEntry &entry : it.value()) {
            if (!entry.value.contains(Pending))
                continue;
            QStringList missing;
            QString out;
            int last = 0;
            for (const QRegularExpressionMatch &match : marked.globalMatch(entry.value)) {
                out += entry.value.mid(last, match.capturedStart() - last);
                const QString name = match.captured(1);
                if (vars->contains(name)) {
                    QString expanded;
                    for (const ConfigEntry &value : vars->value(name))
                        expanded += value.value;
                    out += expanded.remove(Pending);
                } else {
                    missing.append(name);
                }
                last = match.capturedEnd();
            }
            out += entry.value.mid(last);
            entry.value = out;
            if (missing.isEmpty())
                continue;
            config->problems.append(
                {QString("Environment or configuration variable '%1' undefined, so '%2' is "
                         "incomplete. The documentation build supplies it.")
                     .arg(missing.join("', '"), key),
                 entry.file,
                 entry.line});
        }
    }
}

} // namespace

Config parseConfig(const FilePath &rootFile, const QHash<QString, QString> &variables)
{
    Config config;
    config.rootFile = rootFile;
    QHash<QString, QString> all = buildVariables();
    // Someone who has BUILDDIR exported or configured gets their value, not the
    // empty stand-in.
    for (auto it = variables.cbegin(); it != variables.cend(); ++it)
        all.insert(it.key(), it.value());

    ConfigParser parser(config, all);
    parser.load(rootFile, 0, {}, 0);
    expandPending(&config, &config.m_vars, config.m_order);
    return config;
}

QHash<QString, Macro> buildMacroTable(const Config &config, const QString &format)
{
    static const QString prefix = "macro.";
    QHash<QString, Macro> table;

    for (const QString &key : config.keysWithPrefix(prefix)) {
        const QString rest = key.mid(prefix.size());
        if (rest.isEmpty())
            continue;
        const int dot = rest.indexOf('.');
        const QString name = dot == -1 ? rest : rest.left(dot);
        const QString sub = dot == -1 ? QString() : rest.mid(dot + 1);
        if (name.isEmpty())
            continue;

        Macro &macro = table[name];
        macro.name = name;
        const QString value = config.value(key);
        if (dot == -1)
            macro.def = value;
        else if (sub == "match")
            macro.match = value;
        else if (sub == format) {
            macro.raw = value;
            macro.hasRaw = true;
        }
    }

    const auto numParams = [](const QString &def) {
        int max = 0;
        for (const QChar &c : def) {
            const char16_t code = c.unicode();
            if (code > 0 && code < 8)
                max = qMax(max, int(code));
        }
        return max;
    };
    for (Macro &macro : table)
        macro.params = qMax(numParams(macro.def), numParams(macro.raw));

    // QDoc only registers a macro that has at least one definition. The exception is
    // a definition built from a variable the build passes in with -D: keeping it and
    // expanding to nothing beats reporting every use as an unknown command, since
    // the markup is fine and only the build's value is missing.
    for (const QString &name : table.keys()) {
        Macro &macro = table[name];
        if (!macro.def.isEmpty() || macro.hasRaw)
            continue;
        if (config.expandedIn.contains(prefix + name)) {
            macro.unresolved = true;
            macro.variables = config.expandedIn.value(prefix + name);
            continue;
        }
        table.remove(name);
    }
    return table;
}

static QString extensionOf(const FilePath &filePath)
{
    // Not FilePath::suffix(): it reports "ui.qml" for a .ui.qml file, while the
    // tables below, and QDoc's own, are keyed on the last extension.
    const QString name = filePath.fileName();
    const int dot = name.lastIndexOf('.');
    return dot == -1 ? QString() : name.mid(dot + 1).toLower();
}

QString commentMarkerFor(const FilePath &filePath)
{
    static const QHash<QString, QString> markers = {
        {"pro", "#!"},   {"pri", "#!"},   {"py", "#!"},   {"cmake", "#!"}, {"txt", "#!"},
        {"html", "<!--"}, {"qrc", "<!--"}, {"ui", "<!--"}, {"xml", "<!--"}, {"xq", "<!--"},
    };
    return markers.value(extensionOf(filePath), "//!");
}

QString languageFor(const FilePath &filePath)
{
    static const QHash<QString, QString> languages = {
        {"cpp", "cpp"},    {"cc", "cpp"},     {"cxx", "cpp"},  {"h", "cpp"},
        {"hpp", "cpp"},    {"qml", "qml"},    {"js", "javascript"}, {"py", "python"},
        {"json", "json"},  {"xml", "xml"},    {"ui", "xml"},   {"qrc", "xml"},
        {"html", "xml"},   {"pro", "qmake"},  {"pri", "qmake"}, {"txt", "cmake"},
        {"cmake", "cmake"},
    };
    return languages.value(extensionOf(filePath));
}

static int indentOf(const QString &line)
{
    int n = 0;
    while (n < line.size() && line.at(n) == ' ')
        ++n;
    return n;
}

// Collapses whitespace as trimWhiteSpace() in quoter.cpp does, so a pattern and a
// line are compared on equal terms: this is why \snippet foo.qml 0 finds //![0]
// though the delimiter QDoc builds is //! [0]. The transcription is literal,
// including the quirk that a space between two alphanumerics duplicates the second
// character instead of being kept, so int x = 3 becomes intxx=3. Both sides get the
// same treatment, so matching stays consistent with the real build.
QString trimWhiteSpace(const QString &text)
{
    enum State { Normal, MetAlnum, MetSpace };
    State state = Normal;
    QString out;

    for (const QChar &c : text) {
        if (c.isLetterOrNumber()) {
            if (state == Normal) {
                state = MetAlnum;
            } else {
                if (state == MetSpace)
                    out += c;
                state = Normal;
            }
            out += c;
        } else if (c.isSpace()) {
            if (state == MetAlnum)
                state = MetSpace;
        } else {
            state = Normal;
            out += c;
        }
    }
    return out;
}

// A pattern as it is compared against a line: a regular expression when it is
// wrapped in slashes, otherwise whitespace-collapsed text. Prepared once, since
// every pattern is compared against every line of the quoted file.
class QuotePattern
{
public:
    explicit QuotePattern(const QString &pattern)
    {
        if (pattern.isEmpty())
            return;
        if (pattern.size() > 2 && pattern.startsWith('/') && pattern.endsWith('/')) {
            m_regularExpression = QRegularExpression(pattern.mid(1, pattern.size() - 2));
            m_isRegularExpression = true;
            return;
        }
        m_text = trimWhiteSpace(pattern);
    }

    bool matches(const QString &line) const
    {
        if (m_isRegularExpression) {
            return m_regularExpression.isValid()
                   && m_regularExpression.match(trimmedLine(line)).hasMatch();
        }
        if (m_text.isEmpty())
            return false;
        // trimWhiteSpace() keeps every character that is not whitespace, so a line
        // without the pattern's last character cannot match. Testing that first keeps
        // the whitespace pass off the lines that cannot match anyway, which is nearly
        // all of them; the last character is used because a delimiter ends in ']',
        // which is rarer in a source file than the '/' it starts with.
        if (!line.contains(m_text.at(m_text.size() - 1)))
            return false;
        return trimWhiteSpace(trimmedLine(line)).contains(m_text);
    }

private:
    static QString trimmedLine(const QString &line)
    {
        QString trimmed = line;
        while (trimmed.endsWith('\n'))
            trimmed.chop(1);
        return trimmed;
    }

    QRegularExpression m_regularExpression;
    QString m_text;
    bool m_isRegularExpression = false;
};

bool matchesQuotePattern(const QString &line, const QString &pattern)
{
    return QuotePattern(pattern).matches(line);
}

static QStringList splitLines(const QString &text)
{
    QString normalized = text;
    normalized.replace("\r\n", "\n");
    return normalized.split('\n');
}

static std::optional<QString> extractSnippet(const QStringList &lines, const QString &identifier,
                                             const QString &marker)
{
    const QuotePattern delimiter(QString("%1 [%2]").arg(marker, identifier));

    int startIndex = -1;
    int markerIndent = 0;
    for (int i = 0; i < lines.size(); ++i) {
        if (delimiter.matches(lines.at(i))) {
            startIndex = i;
            markerIndent = indentOf(lines.at(i));
            break;
        }
    }
    if (startIndex == -1)
        return {};

    int endIndex = -1;
    for (int i = startIndex + 1; i < lines.size(); ++i) {
        if (delimiter.matches(lines.at(i))) {
            endIndex = i;
            break;
        }
    }
    if (endIndex == -1)
        return {};

    const QStringList body = lines.mid(startIndex + 1, endIndex - startIndex - 1);

    int minContentIndent = -1;
    for (const QString &line : body) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith("QT_BEGIN_NAMESPACE")
            || trimmed.startsWith("QT_END_NAMESPACE") || trimmed.startsWith(marker)) {
            continue;
        }
        const int indent = indentOf(line);
        if (minContentIndent == -1 || indent < minContentIndent)
            minContentIndent = indent;
    }
    const int unindent = minContentIndent == -1 ? markerIndent
                                                : qMin(markerIndent, minContentIndent);

    QStringList out;
    for (const QString &line : body) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith("QT_BEGIN_NAMESPACE"))
            continue;
        if (trimmed.startsWith("QT_END_NAMESPACE") || trimmed.startsWith(marker)) {
            out.append(QString());
            continue;
        }
        out.append(line.mid(qMin(unindent, indentOf(line))));
    }

    while (!out.isEmpty() && out.first().trimmed().isEmpty())
        out.removeFirst();
    while (!out.isEmpty() && out.last().trimmed().isEmpty())
        out.removeLast();
    return out.join('\n');
}

static int runGroup(const QStringList &lines, const QList<QuoteStep> &steps, int cursor,
                    QStringList *out)
{
    for (const QuoteStep &step : steps) {
        if (step.op == "printline") {
            if (cursor < lines.size())
                out->append(lines.at(cursor++));
        } else if (step.op == "skipline") {
            ++cursor;
        } else if (step.op == "printto") {
            const QuotePattern pattern(step.pattern);
            while (cursor < lines.size() && !pattern.matches(lines.at(cursor)))
                out->append(lines.at(cursor++));
        } else if (step.op == "printuntil") {
            const QuotePattern pattern(step.pattern);
            while (cursor < lines.size() && !pattern.matches(lines.at(cursor)))
                out->append(lines.at(cursor++));
            if (cursor < lines.size())
                out->append(lines.at(cursor++));
        } else if (step.op == "skipto") {
            const QuotePattern pattern(step.pattern);
            while (cursor < lines.size() && !pattern.matches(lines.at(cursor)))
                ++cursor;
        } else if (step.op == "skipuntil") {
            const QuotePattern pattern(step.pattern);
            while (cursor < lines.size() && !pattern.matches(lines.at(cursor)))
                ++cursor;
            if (cursor < lines.size())
                ++cursor;
        } else if (step.op == "dots") {
            bool ok = false;
            const int indent = step.pattern.toInt(&ok);
            out->append(QString(ok ? indent : 4, ' ') + "...");
        } else if (step.op == "codeline") {
            out->append(QString());
        }
    }
    return cursor;
}

// Runs the step sequences of one \quotefromfile over the file against one cursor:
// prose between two \printuntil commands ends the listing while the position in the
// file carries on. The common indentation is removed once across every group, so the
// listings keep their indentation relative to each other.
QStringList runQuoteSegments(const QString &text, const QList<QList<QuoteStep>> &groups)
{
    const QStringList lines = splitLines(text);
    int cursor = 0;
    QList<QStringList> segments;

    for (const QList<QuoteStep> &steps : groups) {
        QStringList out;
        cursor = runGroup(lines, steps, cursor, &out);
        segments.append(out);
    }

    int min = -1;
    for (const QStringList &out : segments) {
        for (const QString &line : out) {
            if (line.trimmed().isEmpty())
                continue;
            const int indent = indentOf(line);
            if (min == -1 || indent < min)
                min = indent;
        }
    }
    QStringList result;
    for (const QStringList &out : segments) {
        if (min <= 0) {
            result.append(out.join('\n'));
            continue;
        }
        QStringList shifted;
        for (const QString &line : out)
            shifted.append(line.mid(min));
        result.append(shifted.join('\n'));
    }
    return result;
}

static bool descendable(const FilePath &dir)
{
    return dir.isDir() && !dir.fileName().startsWith('.') && !dir.isSymLink();
}

static FilePath findByBasename(const FilePath &dir, const QString &base, int depth, int *budget)
{
    if (depth < 0 || (*budget)-- <= 0)
        return {};
    const FilePaths entries = dir.dirEntries(DirFilterFlag::AllEntries
                                             | DirFilterFlag::NoDotAndDotDot);
    for (const FilePath &entry : entries) {
        if (entry.fileName() == base && entry.isFile())
            return entry;
    }
    for (const FilePath &entry : entries) {
        if (descendable(entry)) {
            const FilePath found = findByBasename(entry, base, depth - 1, budget);
            if (!found.isEmpty())
                return found;
        }
    }
    return {};
}

// QDoc searches exampledirs and accepts a path relative to any of them. The
// basename fallback handles documentation that writes a longer path than the
// configured root; one budget spans all the roots, and the cache remembers failures
// too, since those are the expensive case.
FilePath findQuoteFile(const QString &name, const FilePaths &searchDirs,
                       QHash<QString, FilePath> *cache)
{
    if (name.isEmpty())
        return {};
    const FilePath written = FilePath::fromUserInput(name);
    if (written.isAbsolutePath())
        return written.isFile() ? written : FilePath();

    for (const FilePath &dir : searchDirs) {
        const FilePath candidate = dir.resolvePath(name);
        if (candidate.isFile())
            return candidate;
    }

    const QString base = written.fileName();
    int budget = 2000;
    for (const FilePath &dir : searchDirs) {
        const QString key = dir.path() + '\n' + base;
        if (cache && cache->contains(key)) {
            const FilePath hit = cache->value(key);
            if (!hit.isEmpty())
                return hit;
            continue;
        }
        const FilePath found = findByBasename(dir, base, 3, &budget);
        // A miss with the budget exhausted is not a real answer; do not keep it.
        if (cache && (!found.isEmpty() || budget > 0))
            cache->insert(key, found);
        if (!found.isEmpty())
            return found;
    }
    return {};
}

FilePath findImageFile(const QString &name, const FilePaths &searchDirs)
{
    static const QStringList extensions = {".png", ".jpg", ".jpeg", ".gif", ".svg", ".webp"};
    if (name.isEmpty())
        return {};
    QStringList candidates;
    if (extensionOf(FilePath::fromUserInput(name)).isEmpty()) {
        for (const QString &extension : extensions)
            candidates.append(name + extension);
    } else {
        candidates.append(name);
    }

    for (const FilePath &dir : searchDirs) {
        for (const QString &candidate : candidates) {
            const FilePath full = dir.resolvePath(candidate);
            if (full.isFile())
                return full;
        }
    }
    return {};
}

// An image a macro's raw HTML points at is relative to QDoc's output directory, not
// to any imagedirs root: every picture a generated page names has been copied into
// <outputdir>/images/ by then, on demand from imagedirs or wholesale from
// HTML.extraimages. A preview has no output directory, so the path is mapped back to
// a source file.
FilePath findRawImageFile(const QString &src, const FilePaths &searchDirs,
                          const FilePaths &extraImageFiles)
{
    static const QRegularExpression scheme("^(?:[a-z][a-z0-9+.-]*:|//)",
                                           QRegularExpression::CaseInsensitiveOption);
    if (src.isEmpty() || scheme.match(src).hasMatch())
        return {};

    const FilePath asWritten = findImageFile(src, searchDirs);
    if (!asWritten.isEmpty())
        return asWritten;

    QString wanted = src;
    wanted.replace('\\', '/');
    while (wanted.startsWith('/'))
        wanted.remove(0, 1);
    wanted.prepend('/');
    for (const FilePath &file : extraImageFiles) {
        if (file.path().endsWith(wanted))
            return file;
    }

    static const QRegularExpression imagesPrefix(R"(^images[\\/])");
    QString stripped = src;
    stripped.remove(imagesPrefix);
    return stripped == src ? FilePath() : findImageFile(stripped, searchDirs);
}

// main.cpp collects every image under exampledirs, keeps those whose path contains
// doc/images and adds the directory ending at that segment, which is how \image in an
// example's documentation resolves though the module's imagedirs never mentions it.
FilePaths findExampleImageDirs(const FilePaths &exampleDirs)
{
    FilePaths found;
    // Depth alone does not bound the walk; the budget makes the worst case a
    // truncated result rather than a frozen editor.
    int budget = 25000;

    const std::function<void(const FilePath &, int)> scan = [&](const FilePath &dir, int depth) {
        if (depth < 0 || budget-- <= 0)
            return;
        for (const FilePath &entry :
             dir.dirEntries(DirFilterFlag::Dirs | DirFilterFlag::NoDotAndDotDot)) {
            if (!descendable(entry))
                continue;
            if (entry.fileName() == "images" && dir.fileName() == "doc") {
                if (!found.contains(entry))
                    found.append(entry);
                continue;
            }
            scan(entry, depth - 1);
        }
    };

    for (const FilePath &dir : exampleDirs)
        scan(dir, 8);
    Utils::sort(found);
    return found;
}

std::optional<QString> extractSnippet(const QString &text, const QString &identifier,
                                      const QString &marker)
{
    return extractSnippet(splitLines(text), identifier, marker);
}

QuoteResult resolveQuote(const Node &node, const QList<QList<QuoteStep>> &sessionGroups,
                         const FilePaths &searchDirs, QuoteCache *cache)
{
    const FilePath resolved = findQuoteFile(node.name, searchDirs,
                                            cache ? &cache->fileSearch : nullptr);
    if (resolved.isEmpty())
        return {false, {}, {}, {}, QString("file not found in exampledirs: %1").arg(node.name)};

    const Result<QByteArray> contents = resolved.fileContents();
    if (!contents)
        return {false, {}, {}, {}, QString("cannot read %1").arg(node.name)};
    const QString text = QString::fromUtf8(*contents);
    const QString lang = languageFor(resolved);

    if (node.mode == "snippet") {
        const QString marker = commentMarkerFor(resolved);
        QStringList lines;
        if (cache) {
            QStringList &cached = cache->fileLines[resolved.path()];
            if (cached.isEmpty())
                cached = splitLines(text);
            lines = cached;
        } else {
            lines = splitLines(text);
        }
        const std::optional<QString> snippet = extractSnippet(lines, node.identifier, marker);
        if (!snippet) {
            return {false, {}, {}, {},
                    QString("snippet marker \"%1 [%2]\" not found in %3")
                        .arg(marker, node.identifier, resolved.fileName())};
        }
        return {true, *snippet, lang, resolved, {}};
    }

    QString trimmed = text;
    trimmed.replace("\r\n", "\n");
    while (!trimmed.isEmpty() && trimmed.at(trimmed.size() - 1).isSpace())
        trimmed.chop(1);

    if (node.mode == "quotefile")
        return {true, trimmed, lang, resolved, {}};

    const QList<QList<QuoteStep>> groups = sessionGroups.isEmpty()
                                               ? QList<QList<QuoteStep>>{node.steps}
                                               : sessionGroups;
    // Nothing was ever printed from this file, so show the file: \quotefromfile on
    // its own emits no atom at all, so this is the preview being helpful. The test is
    // session-wide, since an empty group beside one that prints is a listing with no
    // content.
    const bool nothingPrinted = !Utils::anyOf(groups, [](const QList<QuoteStep> &steps) {
        return !steps.isEmpty();
    });
    if (nothingPrinted)
        return {true, trimmed, lang, resolved, {}};

    // Keyed on the session, never on the path: a page may quote one file twice with
    // two independent cursors, and sharing by path would silently continue the first.
    QString key = resolved.path();
    for (const QList<QuoteStep> &steps : groups) {
        key += '\n';
        for (const QuoteStep &step : steps)
            key += step.op + ' ' + step.pattern + ';';
    }
    QStringList segments;
    if (cache && cache->sessionSegments.contains(key)) {
        segments = cache->sessionSegments.value(key);
    } else {
        segments = runQuoteSegments(text, groups);
        if (cache)
            cache->sessionSegments.insert(key, segments);
    }
    return {true, segments.value(node.groupIndex), lang, resolved, {}};
}

FilePaths findConfigFiles(const FilePath &startDir, int maxLevels)
{
    const auto confsIn = [](const FilePath &dir) {
        return dir.isDir() ? dir.dirEntries(FileFilter({"*.qdocconf"}, DirFilterFlag::Files),
                                            DirSortFlag::Name)
                           : FilePaths();
    };

    FilePaths found;
    FilePath dir = startDir;
    for (int level = 0; level < maxLevels && !dir.isEmpty(); ++level) {
        for (const FilePath &candidate : confsIn(dir)) {
            if (!found.contains(candidate))
                found.append(candidate);
        }
        for (const QString &sub : {QString("doc"), QString("doc/config")}) {
            for (const FilePath &candidate : confsIn(dir.pathAppended(sub))) {
                if (!found.contains(candidate))
                    found.append(candidate);
            }
        }
        const FilePath parent = dir.parentDir();
        if (parent == dir)
            break;
        dir = parent;
    }
    return found;
}

void ConfigResolver::setVariables(const QHash<QString, QString> &variables)
{
    if (m_variables == variables)
        return;
    m_variables = variables;
    clearCaches();
}

void ConfigResolver::clearCaches()
{
    m_parsed.clear();
    m_association.clear();
}

ConfigResolver::ParsedConfig &ConfigResolver::parseCached(const FilePath &confFile)
{
    const auto stampsMatch = [](const ParsedConfig &parsed) {
        for (auto it = parsed.stamps.cbegin(); it != parsed.stamps.cend(); ++it) {
            if (it.key().lastModified() != it.value())
                return false;
        }
        return true;
    };

    const auto cached = m_parsed.find(confFile);
    if (cached != m_parsed.end() && stampsMatch(*cached))
        return *cached;

    ParsedConfig parsed;
    parsed.config = parseConfig(confFile, m_variables);
    parsed.macros = buildMacroTable(parsed.config);
    for (const FilePath &file : parsed.config.files)
        parsed.stamps.insert(file, file.lastModified());
    m_parsed.insert(confFile, parsed);
    return m_parsed[confFile];
}

// Tie-breaker when several confs sit at the same level and none claims the file:
// prefer one in a doc directory, then the shortest name, which favours
// qtcore.qdocconf over a qtcore-project.qdocconf-style helper.
static FilePath preferByLayout(const FilePaths &candidates, const FilePath &dir)
{
    FilePath best;
    double bestScore = 0;
    for (const FilePath &conf : candidates) {
        const QString relative = conf.relativePathFromDir(dir);
        double score = relative.split('/').size();
        if (relative.contains("doc/"))
            score -= 10;
        if (relative.contains("config/"))
            score += 5;
        score += conf.fileName().size() / 100.0;
        if (best.isEmpty() || score < bestScore) {
            best = conf;
            bestScore = score;
        }
    }
    return best;
}

FilePath ConfigResolver::verifyBySourceDirs(const FilePath &file, const FilePaths &candidates)
{
    FilePath best;
    int bestLength = -1;
    for (const FilePath &conf : candidates) {
        const Config &config = parseCached(conf).config;
        // A configuration that disowns the file cannot be the right one, even if one
        // of its directories happens to contain it.
        if (config.excludes(file))
            continue;

        FilePaths dirs = config.pathList("sourcedirs");
        dirs.append(config.pathList("headerdirs"));
        dirs.append(config.pathList("exampledirs"));
        for (const FilePath &dir : dirs) {
            const int specificity = dir.path().size() + 1;
            if (file.isChildOf(dir) && specificity > bestLength) {
                bestLength = specificity;
                best = conf;
            }
        }
        for (const QString &key : {QString("sources"), QString("headers")}) {
            for (const ConfigEntry &entry : config.entries(key)) {
                const FilePath abs = entry.dir.resolvePath(entry.value).cleanPath();
                if (abs == file && file.path().size() > bestLength) {
                    bestLength = file.path().size();
                    best = conf;
                }
            }
        }
    }
    return best;
}

// Which conf declares the file is the authoritative answer, but parsing every
// candidate to find out is wasteful, so proximity comes first: walk up to the
// nearest ancestor holding candidates and ask those, then widen to all of them,
// since a module may reach sideways into another directory.
FilePath ConfigResolver::pick(const FilePath &file, const FilePaths &candidates)
{
    FilePath dir = file.parentDir();
    FilePath nearest;

    while (!dir.isEmpty()) {
        const FilePaths level = Utils::filtered(candidates, [&dir](const FilePath &conf) {
            return conf.isChildOf(dir);
        });
        if (!level.isEmpty()) {
            const FilePath claimed = verifyBySourceDirs(file, level);
            if (!claimed.isEmpty())
                return claimed;
            if (nearest.isEmpty())
                nearest = level.size() == 1 ? level.first() : preferByLayout(level, dir);
        }
        const FilePath parent = dir.parentDir();
        if (parent == dir)
            break;
        dir = parent;
    }

    const FilePath wider = verifyBySourceDirs(file, candidates);
    return wider.isEmpty() ? nearest : wider;
}

DocContext ConfigResolver::contextFor(const FilePath &file, const FilePaths &candidates)
{
    FilePath confFile = m_association.value(file);
    if (confFile.isEmpty() && !m_association.contains(file)) {
        confFile = candidates.isEmpty() ? FilePath() : pick(file, candidates);
        m_association.insert(file, confFile);
    }

    DocContext context;
    if (confFile.isEmpty())
        return context;

    ParsedConfig &parsed = parseCached(confFile);
    const Config &config = parsed.config;
    const FilePaths imageDirs = config.pathList("imagedirs");
    context.exampleDirs = config.pathList("exampledirs");

    // QDoc gives its FileResolver one root set: exampledirs, imagedirs and every
    // <example>/doc/images directory beneath exampledirs. That last group is what
    // makes \image work inside an example's documentation.
    if (!parsed.exampleImageDirsKnown) {
        parsed.exampleImageDirs = findExampleImageDirs(context.exampleDirs);
        parsed.exampleImageDirsKnown = true;
    }

    context.confFile = confFile;
    context.confFiles = config.files;
    context.project = config.value("project", confFile.completeBaseName());
    context.macros = parsed.macros;
    context.imageSearchDirs = imageDirs + parsed.exampleImageDirs + context.exampleDirs;
    // Pictures the build copies into the output wholesale, which is the only way a
    // \youtube thumbnail is reachable. Kept apart from imageSearchDirs on purpose:
    // adding those directories to the \image search would resolve images QDoc would
    // not.
    context.extraImageFiles = config.fileList("HTML.extraimages");
    context.sourceDirs = config.pathList("sourcedirs");
    for (const QString &value : config.values("defines")) {
        for (const QString &define : value.split(QRegularExpression(R"(\s+)"),
                                                 Qt::SkipEmptyParts)) {
            context.defines.insert(define.split('=').first());
        }
    }
    context.tabSize = config.intValue("tabsize", 8);
    context.spurious = config.values("spurious");
    for (const QString &value : config.values("codelanguages")) {
        for (const QString &language : value.split(QRegularExpression(R"([\s,]+)"),
                                                   Qt::SkipEmptyParts)) {
            context.codeLanguages.insert(language.toLower());
        }
    }
    context.reportMissingAltText = config.boolValue("reportmissingalttextforimages", false);
    context.problems = config.problems
                       + config.missingPaths({"imagedirs", "exampledirs", "sourcedirs",
                                              "headerdirs"});
    return context;
}

} // namespace QDoc::Internal
