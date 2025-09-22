// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "macroexpander.h"

#include "algorithm.h"
#include "commandline.h"
#include "environment.h"
#include "hostosinfo.h"
#include "stringutils.h"
#include "utilstr.h"

#include <QDir>
#include <QLoggingCategory>
#include <QMap>
#include <QRegularExpression>

namespace Utils {
namespace Internal {

const char kFilePathPostfix[] = ":FilePath";
const char kPathPostfix[] = ":Path";
const char kNativeFilePathPostfix[] = ":NativeFilePath";
const char kNativePathPostfix[] = ":NativePath";
const char kFileNamePostfix[] = ":FileName";
const char kFileBaseNamePostfix[] = ":FileBaseName";

class MacroExpanderPrivate
{
public:
    MacroExpanderPrivate() = default;

    static bool validateVarName(const QString &varName) { return !varName.startsWith("JS:"); }

    bool expandNestedMacros(const QString &str, int *pos, QString *ret)
    {
        QString varName;
        QString pattern, replace;
        QString defaultValue;
        QString *currArg = &varName;
        QChar prev;
        QChar c;
        QChar replacementChar;
        bool replaceAll = false;

        int i = *pos;
        int strLen = str.length();
        varName.reserve(strLen - i);
        for (; i < strLen; prev = c) {
            c = str.at(i++);
            if (c == '\\' && i < strLen) {
                c = str.at(i++);
                // For the replacement, do not skip the escape sequence when followed by a digit.
                // This is needed for enabling convenient capture group replacement,
                // like %{var/(.)(.)/\2\1}, without escaping the placeholders.
                if (currArg == &replace && c.isDigit())
                    *currArg += '\\';
                *currArg += c;
            } else if (c == '}') {
                if (varName.isEmpty()) { // replace "%{}" with "%"
                    *ret = QString('%');
                    *pos = i;
                    return true;
                }
                QSet<MacroExpanderPrivate *> seen;
                if (resolveMacro(varName, ret, seen)) {
                    *pos = i;
                    if (!pattern.isEmpty() && currArg == &replace) {
                        const QRegularExpression regexp(pattern);
                        if (regexp.isValid()) {
                            if (replaceAll) {
                                ret->replace(regexp, replace);
                            } else {
                                // There isn't an API for replacing once...
                                const QRegularExpressionMatch match = regexp.match(*ret);
                                if (match.hasMatch()) {
                                    *ret = ret->left(match.capturedStart(0))
                                           + match.captured(0).replace(regexp, replace)
                                           + ret->mid(match.capturedEnd(0));
                                }
                            }
                        }
                    }
                    return true;
                }
                if (!defaultValue.isEmpty()) {
                    *pos = i;
                    *ret = defaultValue;
                    return true;
                }
                return false;
            } else if (c == '{' && prev == '%') {
                if (!expandNestedMacros(str, &i, ret))
                    return false;
                varName.chop(1);
                varName += *ret;
            } else if (currArg == &varName && c == '-' && prev == ':' && validateVarName(varName)) {
                varName.chop(1);
                currArg = &defaultValue;
            } else if (currArg == &varName && (c == '/' || c == '#') && validateVarName(varName)) {
                replacementChar = c;
                currArg = &pattern;
                if (i < strLen && str.at(i) == replacementChar) {
                    ++i;
                    replaceAll = true;
                }
            } else if (currArg == &pattern && c == replacementChar) {
                currArg = &replace;
            } else {
                *currArg += c;
            }
        }
        return false;
    }

    int findMacro(const QString &str, int *pos, QString *ret)
    {
        forever {
            int openPos = str.indexOf("%{", *pos);
            if (openPos < 0)
                return 0;
            int varPos = openPos + 2;
            if (expandNestedMacros(str, &varPos, ret)) {
                *pos = openPos;
                return varPos - openPos;
            }
            // An actual expansion may be nested into a "false" one,
            // so we continue right after the last %{.
            *pos = openPos + 2;
        }
    }

    bool resolveMacro(const QString &name, QString *ret, QSet<MacroExpanderPrivate *> &seen)
    {
        // Prevent loops:
        const int count = seen.count();
        seen.insert(this);
        if (seen.count() == count)
            return false;

        bool found;
        *ret = value(name.toUtf8(), &found);
        if (found)
            return true;

        found = Utils::anyOf(m_subProviders, [name, ret, &seen] (const MacroExpanderProvider &p) -> bool {
            MacroExpander *expander = p();
            return expander && expander->d->resolveMacro(name, ret, seen);
        });

        if (found)
            return true;

        found = Utils::anyOf(m_extraResolvers, [name, ret] (const MacroExpander::ResolverFunction &resolver) {
            return resolver(name, ret);
        });

        if (found)
            return true;

        return this == globalMacroExpander()->d ? false : globalMacroExpander()->d->resolveMacro(name, ret, seen);
    }

    QString value(const QByteArray &variable, bool *found) const
    {
        MacroExpander::StringFunction sf = m_map.value(variable);
        if (sf) {
            if (found)
                *found = true;
            return sf();
        }

        for (auto it = m_prefixMap.constBegin(); it != m_prefixMap.constEnd(); ++it) {
            if (variable.startsWith(it.key())) {
                MacroExpander::PrefixFunction pf = it.value();
                if (found)
                    *found = true;
                return pf(QString::fromUtf8(variable.mid(it.key().size())));
            }
        }
        if (found)
            *found = false;

        return QString();
    }

    QHash<QByteArray, MacroExpander::StringFunction> m_map;
    QHash<QByteArray, MacroExpander::PrefixFunction> m_prefixMap;
    QList<MacroExpander::ResolverFunction> m_extraResolvers;
    struct Description
    {
        QString description;
        QByteArray exampleUsage;
    };
    QMap<QByteArray, Description> m_descriptions;
    QString m_displayName;
    QList<MacroExpanderProvider> m_subProviders;
    bool m_accumulating = false;

    bool m_aborted = false;
    int m_lockDepth = 0;
};

} // Internal

using namespace Internal;

/*!
    \class Utils::MacroExpander
    \inmodule QtCreator
    \brief The MacroExpander class manages \QC wide variables, that a user
    can enter into many string settings. The variables are replaced by an actual value when the string
    is used, similar to how environment variables are expanded by a shell.

    \section1 Variables

    Variable names can be basically any string without dollar sign and braces,
    though it is recommended to only use 7-bit ASCII without special characters and whitespace.

    If there are several variables that contain different aspects of the same object,
    it is convention to give them the same prefix, followed by a colon and a postfix
    that describes the aspect.
    Examples of this are \c{CurrentDocument:FilePath} and \c{CurrentDocument:Selection}.

    When the variable manager is requested to replace variables in a string, it looks for
    variable names enclosed in %{ and }, like %{CurrentDocument:FilePath}.

    Environment variables are accessible using the %{Env:...} notation.
    For example, to access the SHELL environment variable, use %{Env:SHELL}.

    \note The names of the variables are stored as QByteArray. They are typically
    7-bit-clean. In cases where this is not possible, UTF-8 encoding is
    assumed.

    \section1 Providing Variable Values

    Plugins can register variables together with a description through registerVariable().
    A typical setup is to register variables in the Plugin::initialize() function.

    \code
    void MyPlugin::initialize()
    {
        [...]
        MacroExpander::registerVariable(
            "MyVariable",
            Tr::tr("The current value of whatever I want."));
            [] {
                QString value;
                // do whatever is necessary to retrieve the value
                [...]
                return value;
            }
        );
        [...]
    }
    \endcode


    For variables that refer to a file, you should use the convenience function
    MacroExpander::registerFileVariables().
    The functions take a variable prefix, like \c MyFileVariable,
    and automatically handle standardized postfixes like \c{:FilePath},
    \c{:Path} and \c{:FileBaseName}, resulting in the combined variables, such as
    \c{MyFileVariable:FilePath}.

    \section1 Providing and Expanding Parametrized Strings

    Though it is possible to just ask the variable manager for the value of some variable in your
    code, the preferred use case is to give the user the possibility to parametrize strings, for
    example for settings.

    (If you ever think about doing the former, think twice. It is much more efficient
    to just ask the plugin that provides the variable value directly, without going through
    string conversions, and through the variable manager which will do a large scale poll. To be
    more concrete, using the example from the Providing Variable Values section: instead of
    calling \c{MacroExpander::value("MyVariable")}, it is much more efficient to just ask directly
    with \c{MyPlugin::variableValue()}.)

    \section2 User Interface

    If the string that you want to parametrize is settable by the user, through a QLineEdit or
    QTextEdit derived class, you should add a variable chooser to your UI, which allows adding
    variables to the string by browsing through a list. See Utils::VariableChooser for more
    details.

    \section2 Expanding Strings

    Expanding variable values in strings is done by "macro expanders".
    Utils::AbstractMacroExpander is the base class for these, and the variable manager
    provides an implementation that expands \QC variables through
    MacroExpander::macroExpander().

    There are several different ways to expand a string, covering the different use cases,
    listed here sorted by relevance:
    \list
    \li Using MacroExpander::expandedString(). This is the most comfortable way to get a string
        with variable values expanded, but also the least flexible one. If this is sufficient for
        you, use it.
    \li Using the Utils::expandMacros() functions. These take a string and a macro expander (for which
        you would use the one provided by the variable manager). Mostly the same as
        MacroExpander::expandedString(), but also has a variant that does the replacement inline
        instead of returning a new string.
    \li Using Utils::CommandLine::expandMacros(). This expands the string while conforming to the
        quoting rules of the platform it is run on. Use this function with the variable manager's
        macro expander if your string will be passed as a command line parameter string to an
        external command.
    \li Writing your own macro expander that nests the variable manager's macro expander. And then
        doing one of the above. This allows you to expand additional "local" variables/macros,
        that do not come from the variable manager.
    \endlist

*/

/*!
 * \internal
 */
MacroExpander::MacroExpander()
{
    d = new MacroExpanderPrivate;
}

/*!
 * \internal
 */
MacroExpander::~MacroExpander()
{
    delete d;
}

/*!
 * \internal
 */
bool MacroExpander::resolveMacro(const QString &name, QString *ret) const
{
    QSet<MacroExpanderPrivate *> seen;
    return d->resolveMacro(name, ret, seen);
}

/*!
 * Returns the value of the given \a variable. If \a found is given, it is
 * set to true if the variable has a value at all, false if not.
 */
QString MacroExpander::value(const QByteArray &variable, bool *found) const
{
    return d->value(variable, found);
}

static void expandMacros(QString *str, MacroExpanderPrivate *mx)
{
    QString rsts;

    for (int pos = 0; int len = mx->findMacro(*str, &pos, &rsts);) {
        str->replace(pos, len, rsts);
        pos += rsts.length();
    }
}

/*!
 * Returns \a stringWithVariables with all variables replaced by their values.
 * See the MacroExpander overview documentation for other ways to expand variables.
 *
 * \sa MacroExpander
 */
QString MacroExpander::expand(const QString &stringWithVariables) const
{
    if (d->m_lockDepth == 0)
        d->m_aborted = false;

    if (d->m_lockDepth > 10) { // Limit recursion.
        d->m_aborted = true;
        return QString();
    }

    ++d->m_lockDepth;

    QString res = stringWithVariables;
    expandMacros(&res, d);

    --d->m_lockDepth;

    if (d->m_lockDepth == 0 && d->m_aborted)
        return Tr::tr("Infinite recursion error") + QLatin1String(": ") + stringWithVariables;

    return res;
}

FilePath MacroExpander::expand(const FilePath &fileNameWithVariables) const
{
    // This is intentionally unsymmetric: We have already sanitized content
    // in fileNameWithVariables, so using .toString() is fine.
    // The macro expansion may introduce arbitrary new contents, so
    // sanitization using fromUserInput() needs to repeated.
    // We also cannot just operate on the scheme, host and path component
    // individually as we want to allow single variables to expand to fully
    // remote-qualified paths.
    return FilePath::fromUserInput(expand(fileNameWithVariables.toUrlishString()));
}

QByteArray MacroExpander::expand(const QByteArray &stringWithVariables) const
{
    return expand(QString::fromLatin1(stringWithVariables)).toLatin1();
}

QVariant MacroExpander::expandVariant(const QVariant &v) const
{
    const int type = v.typeId();
    if (type == QMetaType::QString) {
        return expand(v.toString());
    } else if (type == QMetaType::QStringList) {
        return Utils::transform(v.toStringList(),
                                [this](const QString &s) -> QVariant { return expand(s); });
    } else if (type == QMetaType::QVariantList) {
        return Utils::transform(v.toList(), [this](const QVariant &v) { return expandVariant(v); });
    } else if (type == QMetaType::QVariantMap) {
        const auto map = v.toMap();
        QVariantMap result;
        for (auto it = map.cbegin(), end = map.cend(); it != end; ++it)
            result.insert(it.key(), expandVariant(it.value()));
        return result;
    }
    return v;
}

Result<QString> MacroExpander::expandProcessArgs(const QString &argsWithVariables, OsType osType) const
{
    QString result = argsWithVariables;
    const bool ok = ProcessArgs::expandMacros(
        &result,
        [this](const QString &str, int *pos, QString *ret) { return d->findMacro(str, pos, ret); },
        osType);

    if (!ok) {
        return ResultError(
            Tr::tr("Failed to expand macros in process arguments: %1").arg(argsWithVariables));
    }
    return result;
}

static QByteArray fullPrefix(const QByteArray &prefix)
{
    QByteArray result = prefix;
    if (!result.endsWith(':'))
        result.append(':');
    return result;
}

/*!
 * Makes the given string-valued \a prefix known to the variable manager,
 * together with a localized \a description. Provide an example for the
 * value after the prefix in \a {examplePostfix}. That is used to show
 * an expanded example in the variable chooser.
 *
 * The \a value \c PrefixFunction will be called and gets the full variable name
 * with the prefix stripped as input. It is displayed to users if \a visible is
 * \c true.
 * Set \a availableForExpansion to \c false if the variable should only be documented,
 * but not actually get expanded.
 *
 * \sa registerVariable(), registerIntVariable(), registerFileVariables()
 */
void MacroExpander::registerPrefix(
    const QByteArray &prefix,
    const QByteArray &examplePostfix,
    const QString &description,
    const MacroExpander::PrefixFunction &value,
    bool visible,
    bool availableForExpansion)
{
    QByteArray tmp = fullPrefix(prefix);
    if (visible)
        d->m_descriptions.insert(tmp + "<value>", {description, tmp + examplePostfix});
    if (availableForExpansion)
        d->m_prefixMap.insert(tmp, value);
}

/*!
 * Makes the given string-valued \a variable known to the variable manager,
 * together with a localized \a description.
 *
 * The \a value \c StringFunction is called to retrieve the current value of the
 * variable. It is displayed to users if \a visibleInChooser is \c true.
 * Set \a availableForExpansion to \c false if the variable should only be documented,
 * but not actually get expanded.
 *
 * \sa registerFileVariables(), registerIntVariable(), registerPrefix()
 */
void MacroExpander::registerVariable(
    const QByteArray &variable,
    const QString &description,
    const StringFunction &value,
    bool visibleInChooser,
    bool availableForExpansion)
{
    if (visibleInChooser)
        d->m_descriptions.insert(variable, {description, variable});
    if (availableForExpansion)
        d->m_map.insert(variable, value);
}

/*!
 * Makes the given integral-valued \a variable known to the variable manager,
 * together with a localized \a description.
 *
 * The \a value \c IntFunction is called to retrieve the current value of the
 * variable.
 *
 * \sa registerVariable(), registerFileVariables(), registerPrefix()
 */
void MacroExpander::registerIntVariable(const QByteArray &variable,
    const QString &description, const MacroExpander::IntFunction &value)
{
    const MacroExpander::IntFunction valuecopy = value; // do not capture a reference in a lambda
    registerVariable(variable, description,
        [valuecopy] { return QString::number(valuecopy ? valuecopy() : 0); });
}

/*!
 * Convenience function to register several variables with the same \a prefix, that have a file
 * as a value. Takes the prefix and registers variables like \c{prefix:FilePath} and
 * \c{prefix:Path}, with descriptions that start with the given \a heading.
 * For example \c{registerFileVariables("CurrentDocument", Tr::tr("Current Document"))} registers
 * variables such as \c{CurrentDocument:FilePath} with description
 * "Current Document: Full path including file name."
 *
 * Takes a function that returns a FilePath as a \a base.
 *
 * The variable is displayed to users if \a visibleInChooser is \c true.
 * Set \a availableForExpansion to \c false if the variable should only be documented,
 * but not actually get expanded.
 *
 * \sa registerVariable(), registerIntVariable(), registerPrefix()
 */
void MacroExpander::registerFileVariables(
    const QByteArray &prefix,
    const QString &heading,
    const FileFunction &base,
    bool visibleInChooser,
    bool availableForExpansion)
{
    registerVariable(
        prefix + kFilePathPostfix,
        Tr::tr("%1: Full path including file name.").arg(heading),
        [base] { return base().toFSPathString(); },
        visibleInChooser, availableForExpansion);

    registerVariable(
        prefix + kPathPostfix,
        Tr::tr("%1: Full path excluding file name.").arg(heading),
        [base] { return base().parentDir().toFSPathString(); },
        visibleInChooser, availableForExpansion);

    registerVariable(
        prefix + kNativeFilePathPostfix,
        Tr::tr(
            "%1: Full path including file name, with native path separator (backslash on Windows).")
            .arg(heading),
        [base] { return base().nativePath(); },
        visibleInChooser, availableForExpansion);

    registerVariable(
        prefix + kNativePathPostfix,
        Tr::tr(
            "%1: Full path excluding file name, with native path separator (backslash on Windows).")
            .arg(heading),
        [base] { return base().parentDir().nativePath(); },
        visibleInChooser, availableForExpansion);

    registerVariable(
        prefix + kFileNamePostfix,
        Tr::tr("%1: File name without path.").arg(heading),
        [base] { return base().fileName(); },
        visibleInChooser, availableForExpansion);

    registerVariable(
        prefix + kFileBaseNamePostfix,
        Tr::tr("%1: File base name without path and suffix.").arg(heading),
        [base] { return base().baseName(); },
        visibleInChooser, availableForExpansion);

    registerVariable(
        prefix + ":DirName",
        Tr::tr("%1: File name of the parent directory.").arg(heading),
        [base] { return base().parentDir().fileName(); },
        visibleInChooser, availableForExpansion);
}

void MacroExpander::registerExtraResolver(const MacroExpander::ResolverFunction &value)
{
    d->m_extraResolvers.append(value);
}

/*!
 * Returns all registered variable names.
 *
 * \sa registerVariable()
 * \sa registerFileVariables()
 */
QList<QByteArray> MacroExpander::visibleVariables() const
{
    return d->m_descriptions.keys();
}

/*!
 * Returns the description that was registered for the \a variable.
 */
QString MacroExpander::variableDescription(const QByteArray &variable) const
{
    return d->m_descriptions.value(variable).description;
}

QByteArray MacroExpander::variableExampleUsage(const QByteArray &variable) const
{
    return d->m_descriptions.value(variable).exampleUsage;
}

bool MacroExpander::isPrefixVariable(const QByteArray &variable) const
{
    return d->m_prefixMap.contains(fullPrefix(variable));
}

MacroExpanderProviders MacroExpander::subProviders() const
{
    return d->m_subProviders;
}

QString MacroExpander::displayName() const
{
    return d->m_displayName;
}

void MacroExpander::setDisplayName(const QString &displayName)
{
    d->m_displayName = displayName;
}

void MacroExpander::registerSubProvider(const MacroExpanderProvider &provider)
{
    d->m_subProviders.append(provider);
}

void MacroExpander::clearSubProviders()
{
    d->m_subProviders.clear();
}

bool MacroExpander::isAccumulating() const
{
    return d->m_accumulating;
}

void MacroExpander::setAccumulating(bool on)
{
    d->m_accumulating = on;
}


// MacroExpanderProvider

MacroExpanderProvider::MacroExpanderProvider(QObject *guard,
                                             const std::function<MacroExpander *()> &creator)
    : m_guard(guard), m_creator(creator)
{}

MacroExpanderProvider::MacroExpanderProvider(QObject *guard, MacroExpander *expander)
    : m_guard(guard), m_creator([expander] { return expander; })
{}

MacroExpanderProvider::MacroExpanderProvider(MacroExpander *expander)
    : m_guard(qApp), m_creator([expander] { return expander; })
{}

MacroExpander *MacroExpanderProvider::operator()() const
{
    QTC_ASSERT(m_guard, return nullptr);
    return m_creator ? m_creator() : nullptr;
}


// GlobalMacroExpander

class GlobalMacroExpander : public MacroExpander
{
public:
    GlobalMacroExpander()
    {
        setDisplayName(Tr::tr("Global variables"));
        registerPrefix(
            "Env",
            HostOsInfo::isWindowsHost() ? "USERNAME" : "USER",
            Tr::tr("Access environment variables."),
            [](const QString &value) { return qtcEnvironmentVariable(value); });
    }
};

/*!
 * Returns the expander for globally registered variables.
 */
MacroExpander *globalMacroExpander()
{
    static GlobalMacroExpander theGlobalExpander;
    return &theGlobalExpander;
}

} // namespace Utils
