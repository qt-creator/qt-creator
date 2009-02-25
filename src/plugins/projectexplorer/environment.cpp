/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "environment.h"

#include <QtCore/QProcess>
#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QDebug>

using namespace ProjectExplorer;

QList<EnvironmentItem> EnvironmentItem::fromStringList(QStringList list)
{
    QList<EnvironmentItem> result;
    foreach (const QString &string, list) {
        int pos = string.indexOf(QLatin1Char('='));
        if (pos == -1) {
            EnvironmentItem item(string, "");
            item.unset = true;
            result.append(item);
        } else {
            EnvironmentItem item(string.left(pos), string.mid(pos+1));
            result.append(item);
        }
    }
    return result;
}

QStringList EnvironmentItem::toStringList(QList<EnvironmentItem> list)
{
    QStringList result;
    foreach (const EnvironmentItem &item, list) {
        if (item.unset)
            result << QString(item.name);
        else
            result << QString(item.name + '=' + item.value);
    }
    return result;
}

Environment::Environment()
{
}

Environment::Environment(QStringList env)
{
    foreach (const QString &s, env) {
        int i = s.indexOf("=");
        if (i >= 0) {
#ifdef Q_OS_WIN
            m_values.insert(s.left(i).toUpper(), s.mid(i+1));
#else
            m_values.insert(s.left(i), s.mid(i+1));
#endif
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

void Environment::set(const QString &key, const QString &value)
{
#ifdef Q_OS_WIN
    QString _key = key.toUpper();
#else
    const QString &_key = key;
#endif
    m_values.insert(_key, value);
}

void Environment::unset(const QString &key)
{
#ifdef Q_OS_WIN
    QString _key = key.toUpper();
#else
    const QString &_key = key;
#endif
    m_values.remove(_key);
}

void Environment::appendOrSet(const QString &key, const QString &value, const QString &sep)
{
#ifdef Q_OS_WIN
    QString _key = key.toUpper();
#else
    const QString &_key = key;
#endif
    QMap<QString, QString>::const_iterator it = m_values.constFind(key);
    if (it == m_values.constEnd()) {
        m_values.insert(_key, value);
    } else {
        QString tmp = *it + sep + value;
        m_values.insert(_key, tmp);
    }

}

void Environment::prependOrSet(const QString&key, const QString &value, const QString &sep)
{
#ifdef Q_OS_WIN
    QString _key = key.toUpper();
#else
    const QString &_key = key;
#endif
    QMap<QString, QString>::const_iterator it = m_values.constFind(key);
    if (it == m_values.constEnd()) {
        m_values.insert(_key, value);
    } else {
        QString tmp = value + sep + *it;
        m_values.insert(_key, tmp);
    }
}

void Environment::appendOrSetPath(const QString &value)
{
#ifdef Q_OS_WIN
    QString sep = ";";
#else
    QString sep = ":";
#endif
    appendOrSet("PATH", QDir::toNativeSeparators(value), sep);
}

void Environment::prependOrSetPath(const QString &value)
{
#ifdef Q_OS_WIN
    QString sep = ";";
#else
    QString sep = ":";
#endif
    prependOrSet("PATH", QDir::toNativeSeparators(value), sep);
}

Environment Environment::systemEnvironment()
{
    return Environment(QProcess::systemEnvironment());
}

void Environment::clear()
{
    m_values.clear();
}

QString Environment::searchInPath(QString executable)
{
//    qDebug()<<"looking for "<<executable<< "in PATH: "<<m_values.value("PATH");
    if (executable.isEmpty())
        return QString::null;
    QFileInfo fi(executable);
    if (fi.isAbsolute() && fi.exists())
        return executable;
#ifdef Q_OS_WIN
    if (!executable.endsWith(QLatin1String(".exe")))
        executable.append(QLatin1String(".exe"));
#endif
    const QChar slash = QLatin1Char('/');
    foreach (const QString &p, path()) {
//        qDebug()<<"trying"<<path + '/' + executable;
        QString fp = p;
        fp += slash;
        fp += executable;
        const QFileInfo fi(fp);
        if (fi.exists()) {
//            qDebug()<<"returning "<<fi.absoluteFilePath();
            return fi.absoluteFilePath();
        }
    }
    return QString::null;
}

QStringList Environment::path() const
{
#ifdef Q_OS_WIN
    QString sep = ";";
#else
    QString sep = ":";
#endif
    return m_values.value("PATH").split(sep);
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

Environment::const_iterator Environment::find(const QString &name)
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
                        if (c == '(')
                            end = value.indexOf(')', i);
                        else if (c == '{')
                            end = value.indexOf('}', i);
                        if (end != -1) {
                            const QString &name = value.mid(i+2, end-i-2);
                            Environment::const_iterator it = find(name);
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

QStringList Environment::parseCombinedArgString(const QString &program)
{
    QStringList args;
    QString tmp;
    int quoteCount = 0;
    bool inQuote = false;

    // handle quoting. tokens can be surrounded by double quotes
    // "hello world". three consecutive double quotes represent
    // the quote character itself.
    for (int i = 0; i < program.size(); ++i) {
        if (program.at(i) == QLatin1Char('"')) {
            ++quoteCount;
            if (quoteCount == 3) {
                // third consecutive quote
                quoteCount = 0;
                tmp += program.at(i);
            }
            continue;
        }
        if (quoteCount) {
            if (quoteCount == 1)
                inQuote = !inQuote;
            quoteCount = 0;
        }
        if (!inQuote && program.at(i).isSpace()) {
            if (!tmp.isEmpty()) {
                args += tmp;
                tmp.clear();
            }
        } else {
            tmp += program.at(i);
        }
    }
    if (!tmp.isEmpty())
        args += tmp;
    return args;
}

QString Environment::joinArgumentList(const QStringList &arguments)
{
    QString result;
    foreach (QString arg, arguments) {
        if (!result.isEmpty())
            result += QLatin1Char(' ');
        arg.replace(QLatin1String("\""), QLatin1String("\"\"\""));
        if (arg.contains(QLatin1Char(' ')))
            arg = "\"" + arg + "\"";
        result += arg;
    }
    return result;
}

