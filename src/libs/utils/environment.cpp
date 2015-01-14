/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "environment.h"

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
            QString ldLibraryPath = value(QLatin1String("LD_LIBRARY_PATH"));
            QDir lib(QCoreApplication::applicationDirPath());
            lib.cd(QLatin1String("../lib"));
            QString toReplace = lib.path();
            lib.cd(QLatin1String("qtcreator"));
            toReplace.append(QLatin1Char(':'));
            toReplace.append(lib.path());

            if (ldLibraryPath.startsWith(toReplace))
                set(QLatin1String("LD_LIBRARY_PATH"), ldLibraryPath.remove(0, toReplace.length()));
        }
    }
};

Q_GLOBAL_STATIC(SystemEnvironment, staticSystemEnvironment)

namespace Utils {

static bool sortEnvironmentItem(const EnvironmentItem &a, const EnvironmentItem &b)
{
    return a.name < b.name;
}

void EnvironmentItem::sort(QList<EnvironmentItem> *list)
{
    qSort(list->begin(), list->end(), &sortEnvironmentItem);
}

QList<EnvironmentItem> EnvironmentItem::fromStringList(const QStringList &list)
{
    QList<EnvironmentItem> result;
    foreach (const QString &string, list) {
        int pos = string.indexOf(QLatin1Char('='), 1);
        if (pos == -1) {
            EnvironmentItem item(string, QString());
            item.unset = true;
            result.append(item);
        } else {
            EnvironmentItem item(string.left(pos), string.mid(pos+1));
            result.append(item);
        }
    }
    return result;
}

QStringList EnvironmentItem::toStringList(const QList<EnvironmentItem> &list)
{
    QStringList result;
    foreach (const EnvironmentItem &item, list) {
        if (item.unset)
            result << QString(item.name);
        else
            result << QString(item.name + QLatin1Char('=') + item.value);
    }
    return result;
}

Environment::Environment(const QStringList &env, OsType osType) : m_osType(osType)
{
    foreach (const QString &s, env) {
        int i = s.indexOf(QLatin1Char('='), 1);
        if (i >= 0) {
            if (m_osType == OsTypeWindows)
                m_values.insert(s.left(i).toUpper(), s.mid(i+1));
            else
                m_values.insert(s.left(i), s.mid(i+1));
        }
    }
}

QStringList Environment::toStringList() const
{
    QStringList result;
    const QMap<QString, QString>::const_iterator end = m_values.constEnd();
    for (QMap<QString, QString>::const_iterator it = m_values.constBegin(); it != end; ++it) {
        QString entry = it.key();
        entry += QLatin1Char('=');
        entry += it.value();
        result.push_back(entry);
    }
    return result;
}

QProcessEnvironment Environment::toProcessEnvironment() const
{
    QProcessEnvironment result;
    const QMap<QString, QString>::const_iterator end = m_values.constEnd();
    for (QMap<QString, QString>::const_iterator it = m_values.constBegin(); it != end; ++it)
        result.insert(it.key(), it.value());
    return result;
}

void Environment::set(const QString &key, const QString &value)
{
    m_values.insert(m_osType == OsTypeWindows ? key.toUpper() : key, value);
}

void Environment::unset(const QString &key)
{
    m_values.remove(m_osType == OsTypeWindows ? key.toUpper() : key);
}

void Environment::appendOrSet(const QString &key, const QString &value, const QString &sep)
{
    const QString &_key = m_osType == OsTypeWindows ? key.toUpper() : key;
    QMap<QString, QString>::iterator it = m_values.find(_key);
    if (it == m_values.end()) {
        m_values.insert(_key, value);
    } else {
        // Append unless it is already there
        const QString toAppend = sep + value;
        if (!it.value().endsWith(toAppend))
            it.value().append(toAppend);
    }
}

void Environment::prependOrSet(const QString&key, const QString &value, const QString &sep)
{
    const QString &_key = m_osType == OsTypeWindows ? key.toUpper() : key;
    QMap<QString, QString>::iterator it = m_values.find(_key);
    if (it == m_values.end()) {
        m_values.insert(_key, value);
    } else {
        // Prepend unless it is already there
        const QString toPrepend = value + sep;
        if (!it.value().startsWith(toPrepend))
            it.value().prepend(toPrepend);
    }
}

void Environment::appendOrSetPath(const QString &value)
{
    appendOrSet(QLatin1String("PATH"), QDir::toNativeSeparators(value),
            QString(OsSpecificAspects(m_osType).pathListSeparator()));
}

void Environment::prependOrSetPath(const QString &value)
{
    prependOrSet(QLatin1String("PATH"), QDir::toNativeSeparators(value),
            QString(OsSpecificAspects(m_osType).pathListSeparator()));
}

void Environment::prependOrSetLibrarySearchPath(const QString &value)
{
    switch (m_osType) {
    case OsTypeWindows: {
        const QChar sep = QLatin1Char(';');
        const QLatin1String path("PATH");
        prependOrSet(path, QDir::toNativeSeparators(value), QString(sep));
        break;
    }
    case OsTypeLinux:
    case OsTypeOtherUnix: {
        const QChar sep = QLatin1Char(':');
        const QLatin1String path("LD_LIBRARY_PATH");
        prependOrSet(path, QDir::toNativeSeparators(value), QString(sep));
        break;
    }
    default: // we could set DYLD_LIBRARY_PATH on Mac but it is unnecessary in practice
        break;
    }
}

Environment Environment::systemEnvironment()
{
    return *staticSystemEnvironment();
}

void Environment::clear()
{
    m_values.clear();
}

FileName Environment::searchInDirectory(const QStringList &execs, QString directory) const
{
    const QChar slash = QLatin1Char('/');
    if (directory.isEmpty())
        return FileName();
    // Avoid turing / into // on windows which triggers windows to check
    // for network drives!
    if (!directory.endsWith(slash))
        directory += slash;

    foreach (const QString &exec, execs) {
        QFileInfo fi(directory + exec);
        if (fi.exists() && fi.isFile() && fi.isExecutable())
            return FileName::fromString(fi.absoluteFilePath());
    }
    return FileName();
}

FileName Environment::searchInPath(const QString &executable,
                                   const QStringList &additionalDirs) const
{
    if (executable.isEmpty())
        return FileName();

    QString exec = QDir::cleanPath(expandVariables(executable));
    QFileInfo fi(exec);

    QStringList execs(exec);
    if (m_osType == OsTypeWindows) {
        // Check all the executable extensions on windows:
        // PATHEXT is only used if the executable has no extension
        if (fi.suffix().isEmpty()) {
            QStringList extensions = value(QLatin1String("PATHEXT")).split(QLatin1Char(';'));

            foreach (const QString &ext, extensions) {
                QString tmp = executable + ext.toLower();
                if (fi.isAbsolute()) {
                    if (QFile::exists(tmp))
                        return FileName::fromString(tmp);
                } else {
                    execs << tmp;
                }
            }
        }
    }

    if (fi.isAbsolute())
        return FileName::fromString(exec);

    QSet<QString> alreadyChecked;
    foreach (const QString &dir, additionalDirs) {
        if (alreadyChecked.contains(dir))
            continue;
        alreadyChecked.insert(dir);
        FileName tmp = searchInDirectory(execs, dir);
        if (!tmp.isEmpty())
            return tmp;
    }

    if (executable.indexOf(QLatin1Char('/')) != -1)
        return FileName();

    foreach (const QString &p, path()) {
        if (alreadyChecked.contains(p))
            continue;
        alreadyChecked.insert(p);
        FileName tmp = searchInDirectory(execs, QDir::fromNativeSeparators(p));
        if (!tmp.isEmpty())
            return tmp;
    }
    return FileName();
}

QStringList Environment::path() const
{
    return m_values.value(QLatin1String("PATH"))
            .split(OsSpecificAspects(m_osType).pathListSeparator(), QString::SkipEmptyParts);
}

QString Environment::value(const QString &key) const
{
    return m_values.value(key);
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
    QMap<QString, QString>::const_iterator it = m_values.constFind(name);
    if (it == m_values.constEnd())
        return constEnd();
    else
        return it;
}

int Environment::size() const
{
    return m_values.size();
}

void Environment::modify(const QList<EnvironmentItem> & list)
{
    Environment resultEnvironment = *this;
    foreach (const EnvironmentItem &item, list) {
        if (item.unset) {
            resultEnvironment.unset(item.name);
        } else {
            // TODO use variable expansion
            QString value = item.value;
            for (int i=0; i < value.size(); ++i) {
                if (value.at(i) == QLatin1Char('$')) {
                    if ((i + 1) < value.size()) {
                        const QChar &c = value.at(i+1);
                        int end = -1;
                        if (c == QLatin1Char('('))
                            end = value.indexOf(QLatin1Char(')'), i);
                        else if (c == QLatin1Char('{'))
                            end = value.indexOf(QLatin1Char('}'), i);
                        if (end != -1) {
                            const QString &name = value.mid(i+2, end-i-2);
                            Environment::const_iterator it = constFind(name);
                            if (it != constEnd())
                                value.replace(i, end-i+1, it.value());
                        }
                    }
                }
            }
            resultEnvironment.set(item.name, value);
        }
    }
    *this = resultEnvironment;
}

QList<EnvironmentItem> Environment::diff(const Environment &other) const
{
    QMap<QString, QString>::const_iterator thisIt = constBegin();
    QMap<QString, QString>::const_iterator otherIt = other.constBegin();

    QList<EnvironmentItem> result;
    while (thisIt != constEnd() || otherIt != other.constEnd()) {
        if (thisIt == constEnd()) {
            result.append(EnvironmentItem(otherIt.key(), otherIt.value()));
            ++otherIt;
        } else if (otherIt == constEnd()) {
            EnvironmentItem item(thisIt.key(), QString());
            item.unset = true;
            result.append(item);
            ++thisIt;
        } else if (thisIt.key() < otherIt.key()) {
            EnvironmentItem item(thisIt.key(), QString());
            item.unset = true;
            result.append(item);
            ++thisIt;
        } else if (thisIt.key() > otherIt.key()) {
            result.append(EnvironmentItem(otherIt.key(), otherIt.value()));
            ++otherIt;
        } else {
            result.append(EnvironmentItem(otherIt.key(), otherIt.value()));
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
    return value(QLatin1String(m_osType == OsTypeWindows ? "USERNAME" : "USER"));
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
            if (result.at(i++) == QLatin1Char('%')) {
                if (vStart > 0) {
                    const_iterator it = m_values.constFind(result.mid(vStart, i - vStart - 1).toUpper());
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
                if (c == QLatin1Char('$'))
                    state = OPTIONALVARIABLEBRACE;
            } else if (state == OPTIONALVARIABLEBRACE) {
                if (c == QLatin1Char('{')) {
                    state = BRACEDVARIABLE;
                    vStart = i;
                } else if (c.isLetterOrNumber() || c == QLatin1Char('_')) {
                    state = VARIABLE;
                    vStart = i - 1;
                } else {
                    state = BASE;
                }
            } else if (state == BRACEDVARIABLE) {
                if (c == QLatin1Char('}')) {
                    const_iterator it = m_values.constFind(result.mid(vStart, i - 1 - vStart));
                    if (it != constEnd()) {
                        result.replace(vStart - 2, i - vStart + 2, *it);
                        i = vStart - 2 + it->length();
                    }
                    state = BASE;
                }
            } else if (state == VARIABLE) {
                if (!c.isLetterOrNumber() && c != QLatin1Char('_')) {
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
    QStringList results;
    foreach (const QString &i, variables)
        results << expandVariables(i);
    return results;
}

} // namespace Utils
