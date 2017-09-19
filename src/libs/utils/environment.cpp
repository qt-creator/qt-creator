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

#include "environment.h"

#include "algorithm.h"
#include "qtcassert.h"

#include <QDebug>
#include <QDir>
#include <QProcessEnvironment>
#include <QSet>
#include <QCoreApplication>

class SystemEnvironment : public Utils::Environment
{
public:
    SystemEnvironment()
        : Environment(QProcessEnvironment::systemEnvironment().toStringList())
    {
        if (Utils::HostOsInfo::isLinuxHost()) {
            QString ldLibraryPath = value("LD_LIBRARY_PATH");
            QDir lib(QCoreApplication::applicationDirPath());
            lib.cd("../lib");
            QString toReplace = lib.path();
            lib.cd("qtcreator");
            toReplace.append(':');
            toReplace.append(lib.path());

            if (ldLibraryPath.startsWith(toReplace))
                set("LD_LIBRARY_PATH", ldLibraryPath.remove(0, toReplace.length()));
        }
    }
};

Q_GLOBAL_STATIC(SystemEnvironment, staticSystemEnvironment)

static QMap<QString, QString>::iterator findKey(QMap<QString, QString> &input, Utils::OsType osType,
                                                const QString &key)
{
    const Qt::CaseSensitivity casing
            = (osType == Utils::OsTypeWindows) ? Qt::CaseInsensitive : Qt::CaseSensitive;
    for (auto it = input.begin(); it != input.end(); ++it) {
        if (key.compare(it.key(), casing) == 0)
            return it;
    }
    return input.end();
}

static QMap<QString, QString>::const_iterator findKey(const QMap<QString, QString> &input,
                                                      Utils::OsType osType,
                                                      const QString &key)
{
    const Qt::CaseSensitivity casing
            = (osType == Utils::OsTypeWindows) ? Qt::CaseInsensitive : Qt::CaseSensitive;
    for (auto it = input.constBegin(); it != input.constEnd(); ++it) {
        if (key.compare(it.key(), casing) == 0)
            return it;
    }
    return input.constEnd();
}

namespace Utils {

enum : char
{
#ifdef Q_OS_WIN
    pathSepC = ';'
#else
    pathSepC = ':'
#endif
};

void EnvironmentItem::sort(QList<EnvironmentItem> *list)
{
    Utils::sort(*list, &EnvironmentItem::name);
}

QList<EnvironmentItem> EnvironmentItem::fromStringList(const QStringList &list)
{
    QList<EnvironmentItem> result;
    for (const QString &string : list) {
        int pos = string.indexOf('=', 1);
        if (pos == -1)
            result.append(EnvironmentItem(string, QString(), EnvironmentItem::Unset));
        else
            result.append(EnvironmentItem(string.left(pos), string.mid(pos + 1)));
    }
    return result;
}

QStringList EnvironmentItem::toStringList(const QList<EnvironmentItem> &list)
{
    return Utils::transform(list, [](const EnvironmentItem &item) {
        if (item.operation == EnvironmentItem::Unset)
            return QString(item.name);
        return QString(item.name + '=' + item.value);
    });
}

static QString expand(const Environment *e, QString value)
{
    int replaceCount = 0;
    for (int i = 0; i < value.size(); ++i) {
        if (value.at(i) == '$') {
            if ((i + 1) < value.size()) {
                const QChar &c = value.at(i+1);
                int end = -1;
                if (c == '(')
                    end = value.indexOf(')', i);
                else if (c == '{')
                    end = value.indexOf('}', i);
                if (end != -1) {
                    const QString &name = value.mid(i + 2, end - i - 2);
                    Environment::const_iterator it = e->constFind(name);
                    if (it != e->constEnd())
                        value.replace(i, end - i + 1, it.value());
                    ++replaceCount;
                    QTC_ASSERT(replaceCount < 100, break);
                }
            }
        }
    }
    return value;
}

QDebug operator<<(QDebug debug, const EnvironmentItem &i)
{
    QDebugStateSaver saver(debug);
    debug.noquote();
    debug.nospace();
    debug << "EnvironmentItem(";
    switch (i.operation) {
    case Utils::EnvironmentItem::Set:
        debug << "set \"" << i.name << "\" to \"" << i.value << '"';
        break;
    case Utils::EnvironmentItem::Unset:
        debug << "unset \"" << i.name << '"';
        break;
    case Utils::EnvironmentItem::Prepend:
        debug << "prepend to \"" << i.name << "\":\"" << i.value << '"';
        break;
    case Utils::EnvironmentItem::Append:
        debug << "append to \"" << i.name << "\":\"" << i.value << '"';
        break;
    }
    debug << ')';
    return debug;
}

void EnvironmentItem::apply(Environment *e, Operation op) const
{
    switch (op) {
    case Set:
        e->set(name, expand(e, value));
        break;
    case Unset:
        e->unset(name);
        break;
    case Prepend: {
        const Environment::const_iterator it = e->constFind(name);
        if (it != e->constEnd()) {
            QString v = it.value();
            const QChar pathSep{QLatin1Char(pathSepC)};
            int sepCount = 0;
            if (v.startsWith(pathSep))
                ++sepCount;
            if (value.endsWith(pathSep))
                ++sepCount;
            if (sepCount == 2)
                v.remove(0, 1);
            else if (sepCount == 0)
                v.prepend(pathSep);
            v.prepend(expand(e, value));
            e->set(name, v);
        } else {
            apply(e, Set);
        }
    }
        break;
    case Append: {
        const Environment::const_iterator it = e->constFind(name);
        if (it != e->constEnd()) {
            QString v = it.value();
            const QChar pathSep{QLatin1Char(pathSepC)};
            int sepCount = 0;
            if (v.endsWith(pathSep))
                ++sepCount;
            if (value.startsWith(pathSep))
                ++sepCount;
            if (sepCount == 2)
                v.chop(1);
            else if (sepCount == 0)
                v.append(pathSep);
            v.append(expand(e, value));
            e->set(name, v);
        } else {
            apply(e, Set);
        }
    }
        break;
    }
}

Environment::Environment(const QStringList &env, OsType osType) : m_osType(osType)
{
    for (const QString &s : env) {
        int i = s.indexOf('=', 1);
        if (i >= 0) {
            const QString key = s.left(i);
            const QString value = s.mid(i + 1);
            set(key, value);
        }
    }
}

QStringList Environment::toStringList() const
{
    QStringList result;
    for (auto it = m_values.constBegin(); it != m_values.constEnd(); ++it)
        result.append(it.key() + '=' + it.value());
    return result;
}

QProcessEnvironment Environment::toProcessEnvironment() const
{
    QProcessEnvironment result;
    for (auto it = m_values.constBegin(); it != m_values.constEnd(); ++it)
        result.insert(it.key(), it.value());
    return result;
}

void Environment::set(const QString &key, const QString &value)
{
    auto it = findKey(m_values, m_osType, key);
    if (it == m_values.end())
        m_values.insert(key, value);
    else
        it.value() = value;
}

void Environment::unset(const QString &key)
{
    auto it = findKey(m_values, m_osType, key);
    if (it != m_values.end())
        m_values.erase(it);
}

void Environment::appendOrSet(const QString &key, const QString &value, const QString &sep)
{
    auto it = findKey(m_values, m_osType, key);
    if (it == m_values.end()) {
        m_values.insert(key, value);
    } else {
        // Append unless it is already there
        const QString toAppend = sep + value;
        if (!it.value().endsWith(toAppend))
            it.value().append(toAppend);
    }
}

void Environment::prependOrSet(const QString&key, const QString &value, const QString &sep)
{
    auto it = findKey(m_values, m_osType, key);
    if (it == m_values.end()) {
        m_values.insert(key, value);
    } else {
        // Prepend unless it is already there
        const QString toPrepend = value + sep;
        if (!it.value().startsWith(toPrepend))
            it.value().prepend(toPrepend);
    }
}

void Environment::appendOrSetPath(const QString &value)
{
    appendOrSet("PATH", QDir::toNativeSeparators(value),
                QString(OsSpecificAspects(m_osType).pathListSeparator()));
}

void Environment::prependOrSetPath(const QString &value)
{
    prependOrSet("PATH", QDir::toNativeSeparators(value),
            QString(OsSpecificAspects(m_osType).pathListSeparator()));
}

void Environment::prependOrSetLibrarySearchPath(const QString &value)
{
    switch (m_osType) {
    case OsTypeWindows: {
        const QChar sep = ';';
        prependOrSet("PATH", QDir::toNativeSeparators(value), QString(sep));
        break;
    }
    case OsTypeMac: {
        const QString sep =  ":";
        const QString nativeValue = QDir::toNativeSeparators(value);
        prependOrSet("DYLD_LIBRARY_PATH", nativeValue, sep);
        prependOrSet("DYLD_FRAMEWORK_PATH", nativeValue, sep);
        break;
    }
    case OsTypeLinux:
    case OsTypeOtherUnix: {
        const QChar sep = ':';
        prependOrSet("LD_LIBRARY_PATH", QDir::toNativeSeparators(value), QString(sep));
        break;
    }
    default:
        break;
    }
}

Environment Environment::systemEnvironment()
{
    return *staticSystemEnvironment();
}

const char lcMessages[] = "LC_MESSAGES";
const char language[] = "LANGUAGE";
const char englishLocale[] = "en_US.utf8";
const char languageEnglishLocales[] = "en_US:en";

void Environment::setupEnglishOutput(Environment *environment)
{
    QTC_ASSERT(environment, return);
    environment->set(lcMessages, englishLocale);
    environment->set(language, languageEnglishLocales);
}

void Environment::setupEnglishOutput(QProcessEnvironment *environment)
{
    QTC_ASSERT(environment, return);
    environment->insert(lcMessages, englishLocale);
    environment->insert(language, languageEnglishLocales);
}

void Environment::setupEnglishOutput(QStringList *environment)
{
    QTC_ASSERT(environment, return);
    Environment env(*environment);
    setupEnglishOutput(&env);
    *environment = env.toStringList();
}

void Environment::clear()
{
    m_values.clear();
}

FileName Environment::searchInDirectory(const QStringList &execs, const FileName &directory,
                                        QSet<FileName> &alreadyChecked) const
{
    const int checkedCount = alreadyChecked.count();
    alreadyChecked.insert(directory);

    if (directory.isEmpty() || alreadyChecked.count() == checkedCount)
        return FileName();

    const QString dir = directory.toString();

    QFileInfo fi;
    for (const QString &exec : execs) {
        fi.setFile(dir, exec);
        if (fi.isFile() && fi.isExecutable())
            return FileName::fromString(fi.absoluteFilePath());
    }
    return FileName();
}

QStringList Environment::appendExeExtensions(const QString &executable) const
{
    QStringList execs(executable);
    const QFileInfo fi(executable);
    if (m_osType == OsTypeWindows) {
        // Check all the executable extensions on windows:
        // PATHEXT is only used if the executable has no extension
        if (fi.suffix().isEmpty()) {
            const QStringList extensions = value("PATHEXT").split(';');

            for (const QString &ext : extensions)
                execs << executable + ext.toLower();
        }
    }
    return execs;
}

bool Environment::isSameExecutable(const QString &exe1, const QString &exe2) const
{
    const QStringList exe1List = appendExeExtensions(exe1);
    const QStringList exe2List = appendExeExtensions(exe2);
    for (const QString &i1 : exe1List) {
        for (const QString &i2 : exe2List) {
            if (FileName::fromString(i1) == FileName::fromString(i2))
                return true;
        }
    }
    return false;
}

FileName Environment::searchInPath(const QString &executable,
                                   const FileNameList &additionalDirs,
                                   const PathFilter &func) const
{
    if (executable.isEmpty())
        return FileName();

    const QString exec = QDir::cleanPath(expandVariables(executable));
    const QFileInfo fi(exec);

    const QStringList execs = appendExeExtensions(exec);

    if (fi.isAbsolute()) {
        for (const QString &path : execs) {
            QFileInfo pfi = QFileInfo(path);
            if (pfi.isFile() && pfi.isExecutable())
                return FileName::fromString(path);
        }
        return FileName::fromString(exec);
    }

    QSet<FileName> alreadyChecked;
    for (const FileName &dir : additionalDirs) {
        FileName tmp = searchInDirectory(execs, dir, alreadyChecked);
        if (!tmp.isEmpty() && (!func || func(tmp)))
            return tmp;
    }

    if (executable.contains('/'))
        return FileName();

    for (const FileName &p : path()) {
        FileName tmp = searchInDirectory(execs, p, alreadyChecked);
        if (!tmp.isEmpty() && (!func || func(tmp)))
            return tmp;
    }
    return FileName();
}

FileNameList Environment::path() const
{
    const QStringList pathComponents = value("PATH")
            .split(OsSpecificAspects(m_osType).pathListSeparator(), QString::SkipEmptyParts);
    return Utils::transform(pathComponents, &FileName::fromUserInput);
}

QString Environment::value(const QString &key) const
{
    const auto it = findKey(m_values, m_osType, key);
    return it != m_values.end() ? it.value() : QString();
}

QString Environment::key(Environment::const_iterator it) const
{
    return it.key();
}

QString Environment::value(Environment::const_iterator it) const
{
    return it.value();
}

Environment::const_iterator Environment::constBegin() const
{
    return m_values.constBegin();
}

Environment::const_iterator Environment::constEnd() const
{
    return m_values.constEnd();
}

Environment::const_iterator Environment::constFind(const QString &name) const
{
    return m_values.constFind(name);
}

int Environment::size() const
{
    return m_values.size();
}

void Environment::modify(const QList<EnvironmentItem> & list)
{
    Environment resultEnvironment = *this;
    for (const EnvironmentItem &item : list)
        item.apply(&resultEnvironment);
    *this = resultEnvironment;
}

QList<EnvironmentItem> Environment::diff(const Environment &other, bool checkAppendPrepend) const
{
    QMap<QString, QString>::const_iterator thisIt = constBegin();
    QMap<QString, QString>::const_iterator otherIt = other.constBegin();

    QList<EnvironmentItem> result;
    while (thisIt != constEnd() || otherIt != other.constEnd()) {
        if (thisIt == constEnd()) {
            result.append(EnvironmentItem(otherIt.key(), otherIt.value()));
            ++otherIt;
        } else if (otherIt == other.constEnd()) {
            result.append(EnvironmentItem(thisIt.key(), QString(), EnvironmentItem::Unset));
            ++thisIt;
        } else if (thisIt.key() < otherIt.key()) {
            result.append(EnvironmentItem(thisIt.key(), QString(), EnvironmentItem::Unset));
            ++thisIt;
        } else if (thisIt.key() > otherIt.key()) {
            result.append(EnvironmentItem(otherIt.key(), otherIt.value()));
            ++otherIt;
        } else {
            const QString &oldValue = thisIt.value();
            const QString &newValue = otherIt.value();
            if (oldValue != newValue) {
                if (checkAppendPrepend && newValue.startsWith(oldValue)) {
                    QString appended = newValue.right(newValue.size() - oldValue.size());
                    if (appended.startsWith(QLatin1Char(pathSepC)))
                        appended.remove(0, 1);
                    result.append(EnvironmentItem(otherIt.key(), appended,
                                                  EnvironmentItem::Append));
                } else if (checkAppendPrepend && newValue.endsWith(oldValue)) {
                    QString prepended = newValue.left(newValue.size() - oldValue.size());
                    if (prepended.endsWith(QLatin1Char(pathSepC)))
                        prepended.chop(1);
                    result.append(EnvironmentItem(otherIt.key(), prepended,
                                                  EnvironmentItem::Prepend));
                } else {
                    result.append(EnvironmentItem(otherIt.key(), newValue));
                }
            }
            ++otherIt;
            ++thisIt;
        }
    }
    return result;
}

bool Environment::hasKey(const QString &key) const
{
    return m_values.contains(key);
}

QString Environment::userName() const
{
    return value(QString::fromLatin1(m_osType == OsTypeWindows ? "USERNAME" : "USER"));
}

bool Environment::operator!=(const Environment &other) const
{
    return !(*this == other);
}

bool Environment::operator==(const Environment &other) const
{
    return m_osType == other.m_osType && m_values == other.m_values;
}

/** Expand environment variables in a string.
 *
 * Environment variables are accepted in the following forms:
 * $SOMEVAR, ${SOMEVAR} on Unix and %SOMEVAR% on Windows.
 * No escapes and quoting are supported.
 * If a variable is not found, it is not substituted.
 */
QString Environment::expandVariables(const QString &input) const
{
    QString result = input;

    if (m_osType == OsTypeWindows) {
        for (int vStart = -1, i = 0; i < result.length(); ) {
            if (result.at(i++) == '%') {
                if (vStart > 0) {
                    const_iterator it = findKey(m_values, m_osType, result.mid(vStart, i - vStart - 1));
                    if (it != m_values.constEnd()) {
                        result.replace(vStart - 1, i - vStart + 1, *it);
                        i = vStart - 1 + it->length();
                        vStart = -1;
                    } else {
                        vStart = i;
                    }
                } else {
                    vStart = i;
                }
            }
        }
    } else {
        enum { BASE, OPTIONALVARIABLEBRACE, VARIABLE, BRACEDVARIABLE } state = BASE;
        int vStart = -1;

        for (int i = 0; i < result.length();) {
            QChar c = result.at(i++);
            if (state == BASE) {
                if (c == '$')
                    state = OPTIONALVARIABLEBRACE;
            } else if (state == OPTIONALVARIABLEBRACE) {
                if (c == '{') {
                    state = BRACEDVARIABLE;
                    vStart = i;
                } else if (c.isLetterOrNumber() || c == '_') {
                    state = VARIABLE;
                    vStart = i - 1;
                } else {
                    state = BASE;
                }
            } else if (state == BRACEDVARIABLE) {
                if (c == '}') {
                    const_iterator it = m_values.constFind(result.mid(vStart, i - 1 - vStart));
                    if (it != constEnd()) {
                        result.replace(vStart - 2, i - vStart + 2, *it);
                        i = vStart - 2 + it->length();
                    }
                    state = BASE;
                }
            } else if (state == VARIABLE) {
                if (!c.isLetterOrNumber() && c != '_') {
                    const_iterator it = m_values.constFind(result.mid(vStart, i - vStart - 1));
                    if (it != constEnd()) {
                        result.replace(vStart - 1, i - vStart, *it);
                        i = vStart - 1 + it->length();
                    }
                    state = BASE;
                }
            }
        }
        if (state == VARIABLE) {
            const_iterator it = m_values.constFind(result.mid(vStart));
            if (it != constEnd())
                result.replace(vStart - 1, result.length() - vStart + 1, *it);
        }
    }
    return result;
}

QStringList Environment::expandVariables(const QStringList &variables) const
{
    return Utils::transform(variables, [this](const QString &i) { return expandVariables(i); });
}

} // namespace Utils
