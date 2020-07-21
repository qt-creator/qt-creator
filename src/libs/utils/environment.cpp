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
#include "stringutils.h"

#include <QDebug>
#include <QDir>
#include <QProcessEnvironment>
#include <QSet>
#include <QCoreApplication>

Q_GLOBAL_STATIC_WITH_ARGS(Utils::Environment, staticSystemEnvironment,
                          (QProcessEnvironment::systemEnvironment().toStringList()))

Q_GLOBAL_STATIC(QVector<Utils::EnvironmentProvider>, environmentProviders)

namespace Utils {

QProcessEnvironment Environment::toProcessEnvironment() const
{
    QProcessEnvironment result;
    for (auto it = m_values.constBegin(); it != m_values.constEnd(); ++it) {
        if (it.value().second)
            result.insert(it.key().name, expandedValueForKey(key(it)));
    }
    return result;
}

void Environment::appendOrSetPath(const QString &value)
{
    appendOrSet("PATH", QDir::toNativeSeparators(value),
                QString(OsSpecificAspects::pathListSeparator(m_osType)));
}

void Environment::prependOrSetPath(const QString &value)
{
    prependOrSet("PATH", QDir::toNativeSeparators(value),
            QString(OsSpecificAspects::pathListSeparator(m_osType)));
}

void Environment::appendOrSet(const QString &key, const QString &value, const QString &sep)
{
    QTC_ASSERT(!key.contains('='), return );
    const auto it = findKey(key);
    if (it == m_values.end()) {
        m_values.insert(DictKey(key, nameCaseSensitivity()), qMakePair(value, true));
    } else {
        // Append unless it is already there
        const QString toAppend = sep + value;
        if (!it.value().first.endsWith(toAppend))
            it.value().first.append(toAppend);
    }
}

void Environment::prependOrSet(const QString &key, const QString &value, const QString &sep)
{
    QTC_ASSERT(!key.contains('='), return );
    const auto it = findKey(key);
    if (it == m_values.end()) {
        m_values.insert(DictKey(key, nameCaseSensitivity()), qMakePair(value, true));
    } else {
        // Prepend unless it is already there
        const QString toPrepend = value + sep;
        if (!it.value().first.startsWith(toPrepend))
            it.value().first.prepend(toPrepend);
    }
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

void Environment::prependOrSetLibrarySearchPaths(const QStringList &values)
{
    Utils::reverseForeach(values, [this](const QString &value) {
        prependOrSetLibrarySearchPath(value);
    });
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

FilePath Environment::searchInDirectory(const QStringList &execs, const FilePath &directory,
                                        QSet<FilePath> &alreadyChecked) const
{
    const int checkedCount = alreadyChecked.count();
    alreadyChecked.insert(directory);

    if (directory.isEmpty() || alreadyChecked.count() == checkedCount)
        return FilePath();

    const QString dir = directory.toString();

    QFileInfo fi;
    for (const QString &exec : execs) {
        fi.setFile(dir, exec);
        if (fi.isFile() && fi.isExecutable())
            return FilePath::fromString(fi.absoluteFilePath());
    }
    return FilePath();
}

QStringList Environment::appendExeExtensions(const QString &executable) const
{
    QStringList execs(executable);
    const QFileInfo fi(executable);
    if (m_osType == OsTypeWindows) {
        // Check all the executable extensions on windows:
        // PATHEXT is only used if the executable has no extension
        if (fi.suffix().isEmpty()) {
            const QStringList extensions = expandedValueForKey("PATHEXT").split(';');

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
            const FilePath f1 = FilePath::fromString(i1);
            const FilePath f2 = FilePath::fromString(i2);
            if (f1 == f2)
                return true;
            if (FileUtils::resolveSymlinks(f1) == FileUtils::resolveSymlinks(f2))
                return true;
            if (FileUtils::fileId(f1) == FileUtils::fileId(f2))
                return true;
        }
    }
    return false;
}

QString Environment::expandedValueForKey(const QString &key) const
{
    return expandVariables(value(key));
}

FilePath Environment::searchInPath(const QString &executable,
                                   const FilePaths &additionalDirs,
                                   const PathFilter &func) const
{
    if (executable.isEmpty())
        return FilePath();

    const QString exec = QDir::cleanPath(expandVariables(executable));
    const QFileInfo fi(exec);

    const QStringList execs = appendExeExtensions(exec);

    if (fi.isAbsolute()) {
        for (const QString &path : execs) {
            QFileInfo pfi = QFileInfo(path);
            if (pfi.isFile() && pfi.isExecutable())
                return FilePath::fromString(path);
        }
        return FilePath::fromString(exec);
    }

    QSet<FilePath> alreadyChecked;
    for (const FilePath &dir : additionalDirs) {
        FilePath tmp = searchInDirectory(execs, dir, alreadyChecked);
        if (!tmp.isEmpty() && (!func || func(tmp)))
            return tmp;
    }

    if (executable.contains('/'))
        return FilePath();

    for (const FilePath &p : path()) {
        FilePath tmp = searchInDirectory(execs, p, alreadyChecked);
        if (!tmp.isEmpty() && (!func || func(tmp)))
            return tmp;
    }
    return FilePath();
}

FilePaths Environment::findAllInPath(const QString &executable,
                                        const FilePaths &additionalDirs,
                                        const Environment::PathFilter &func) const
{
    if (executable.isEmpty())
        return {};

    const QString exec = QDir::cleanPath(expandVariables(executable));
    const QFileInfo fi(exec);

    const QStringList execs = appendExeExtensions(exec);

    if (fi.isAbsolute()) {
        for (const QString &path : execs) {
            QFileInfo pfi = QFileInfo(path);
            if (pfi.isFile() && pfi.isExecutable())
                return {FilePath::fromString(path)};
        }
        return {FilePath::fromString(exec)};
    }

    QSet<FilePath> result;
    QSet<FilePath> alreadyChecked;
    for (const FilePath &dir : additionalDirs) {
        FilePath tmp = searchInDirectory(execs, dir, alreadyChecked);
        if (!tmp.isEmpty() && (!func || func(tmp)))
            result << tmp;
    }

    if (!executable.contains('/')) {
        for (const FilePath &p : path()) {
            FilePath tmp = searchInDirectory(execs, p, alreadyChecked);
            if (!tmp.isEmpty() && (!func || func(tmp)))
                result << tmp;
        }
    }
    return result.values();
}

FilePaths Environment::path() const
{
    return pathListValue("PATH");
}

FilePaths Environment::pathListValue(const QString &varName) const
{
    const QStringList pathComponents = expandedValueForKey(varName)
            .split(OsSpecificAspects::pathListSeparator(m_osType), Qt::SkipEmptyParts);
    return transform(pathComponents, &FilePath::fromUserInput);
}

void Environment::modifySystemEnvironment(const EnvironmentItems &list)
{
    staticSystemEnvironment->modify(list);
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
                    const auto it = findKey(result.mid(vStart, i - vStart - 1));
                    if (it != m_values.constEnd()) {
                        result.replace(vStart - 1, i - vStart + 1, it->first);
                        i = vStart - 1 + it->first.length();
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
                    const_iterator it = constFind(result.mid(vStart, i - 1 - vStart));
                    if (it != constEnd()) {
                        result.replace(vStart - 2, i - vStart + 2, it->first);
                        i = vStart - 2 + it->first.length();
                    }
                    state = BASE;
                }
            } else if (state == VARIABLE) {
                if (!c.isLetterOrNumber() && c != '_') {
                    const_iterator it = constFind(result.mid(vStart, i - vStart - 1));
                    if (it != constEnd()) {
                        result.replace(vStart - 1, i - vStart, it->first);
                        i = vStart - 1 + it->first.length();
                    }
                    state = BASE;
                }
            }
        }
        if (state == VARIABLE) {
            const_iterator it = constFind(result.mid(vStart));
            if (it != constEnd())
                result.replace(vStart - 1, result.length() - vStart + 1, it->first);
        }
    }
    return result;
}

FilePath Environment::expandVariables(const FilePath &variables) const
{
    return FilePath::fromString(expandVariables(variables.toString()));
}

QStringList Environment::expandVariables(const QStringList &variables) const
{
    return Utils::transform(variables, [this](const QString &i) { return expandVariables(i); });
}

void EnvironmentProvider::addProvider(EnvironmentProvider &&provider)
{
    environmentProviders->append(std::move(provider));
}

const QVector<EnvironmentProvider> EnvironmentProvider::providers()
{
    return *environmentProviders;
}

optional<EnvironmentProvider> EnvironmentProvider::provider(const QByteArray &id)
{
    const int index = indexOf(*environmentProviders, equal(&EnvironmentProvider::id, id));
    if (index >= 0)
        return make_optional(environmentProviders->at(index));
    return nullopt;
}

} // namespace Utils
