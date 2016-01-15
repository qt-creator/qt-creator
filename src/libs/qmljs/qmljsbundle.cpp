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

#include "qmljsbundle.h"

#include <utils/json.h>

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QHash>

namespace QmlJS {
typedef PersistentTrie::Trie Trie;

QmlBundle::QmlBundle(const QmlBundle &o)
    : m_name(o.m_name), m_searchPaths(o.searchPaths()), m_installPaths(o.installPaths()),
      m_supportedImports(o.m_supportedImports), m_implicitImports(o.m_implicitImports)
{ }

QmlBundle::QmlBundle()
{ }

QmlBundle::QmlBundle(const QString &bundleName, const Trie &searchPaths,
                     const Trie &installPaths, const Trie &supportedImports,
                     const Trie &implicitImports)
    : m_name(bundleName), m_searchPaths(searchPaths), m_installPaths(installPaths),
      m_supportedImports(supportedImports), m_implicitImports(implicitImports)
{ }


QString QmlBundle::name() const
{
    return m_name;
}

Trie QmlBundle::installPaths() const
{
    return m_installPaths;
}

Trie QmlBundle::searchPaths() const
{
    return m_searchPaths;
}

Trie QmlBundle::implicitImports() const
{
    return m_implicitImports;
}


Trie QmlBundle::supportedImports() const
{
    return m_supportedImports;
}


void QmlBundle::merge(const QmlBundle &o)
{
    *this = mergeF(o);
}


void QmlBundle::intersect(const QmlBundle &o)
{
    *this = intersectF(o);
}


QmlBundle QmlBundle::mergeF(const QmlBundle &o) const
{
    return QmlBundle(QString::fromLatin1("(%1)||(%2)").arg(name()).arg(o.name()),
                     searchPaths().mergeF(o.searchPaths()),
                     installPaths().mergeF(o.installPaths()),
                     supportedImports().mergeF(o.supportedImports()),
                     implicitImports().mergeF(o.implicitImports()));
}

QmlBundle QmlBundle::intersectF(const QmlBundle &o) const
{
    return QmlBundle(QString::fromLatin1("(%1)&&(%2)").arg(name()).arg(o.name()),
                     searchPaths().mergeF(o.searchPaths()),
                     installPaths().mergeF(o.installPaths()),
                     supportedImports().intersectF(o.supportedImports()),
                     implicitImports().mergeF(o.implicitImports()));
}

bool QmlBundle::isEmpty() const
{
    return m_implicitImports.isEmpty() && m_installPaths.isEmpty()
            && m_searchPaths.isEmpty() && m_supportedImports.isEmpty();
}

void QmlBundle::replaceVars(const QHash<QString, QString> &replacements)
{
    m_implicitImports.replace(replacements);
    m_installPaths.replace(replacements);
    m_searchPaths.replace(replacements);
    m_supportedImports.replace(replacements);
}

QmlBundle QmlBundle::replaceVarsF(const QHash<QString, QString> &replacements) const
{
    QmlBundle res(*this);
    res.replaceVars(replacements);
    return res;
}

bool QmlBundle::writeTo(const QString &path) const
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream stream(&f);
    return writeTo(stream);
}

bool QmlBundle::operator==(const QmlBundle &o) const
{
    return o.implicitImports() == implicitImports()
            && o.installPaths() == installPaths()
            && o.supportedImports() == supportedImports(); // name is not considered
}

bool QmlBundle::operator!=(const QmlBundle &o) const
{
    return !((*this) == o);
}

void QmlBundle::printEscaped(QTextStream &s, const QString &str)
{
    s << QLatin1Char('"');
    QString::const_iterator i = str.constBegin(), iLast = str.constBegin(),
            iEnd = str.constEnd();
    while (i != iEnd) {
        if ((*i) != QLatin1Char('"')) {
            s << QStringRef(&str, static_cast<int>(iLast - str.constBegin())
                            , static_cast<int>(i - iLast) ).toString()
              << QLatin1Char('\\');
            iLast = i;
        }
        ++i;
    }
    s << QStringRef(&str, static_cast<int>(iLast - str.constBegin())
                    , static_cast<int>(i - iLast) ).toString();
}

void QmlBundle::writeTrie(QTextStream &stream, const Trie &t, const QString &indent) {
    stream << QLatin1Char('[');
    bool firstLine = true;
    foreach (const QString &i, t.stringList()) {
        if (firstLine)
            firstLine = false;
        else
            stream << QLatin1Char(',');
        stream << QLatin1String("\n") << indent << QLatin1String("    ");
        printEscaped(stream, i);
    }
    stream << QLatin1Char(']');
}

bool QmlBundle::writeTo(QTextStream &stream, const QString &indent) const
{
    QString innerIndent = QString::fromLatin1("    ").append(indent);
    stream << indent << QLatin1String("{\n")
           << indent << QLatin1String("    \"name\": ");
    printEscaped(stream, name());
    stream << QLatin1String(",\n")
           << indent << QLatin1String("    \"searchPaths\": ");
    writeTrie(stream, searchPaths(), innerIndent);
    stream << QLatin1String(",\n")
           << indent << QLatin1String("    \"installPaths\": ");
    writeTrie(stream, installPaths(), innerIndent);
    stream << QLatin1String(",\n")
           << indent << QLatin1String("    \"supportedImports\": ");
    writeTrie(stream, supportedImports(), innerIndent);
    stream << QLatin1String(",\n")
           << QLatin1String("    \"implicitImports\": ");
    writeTrie(stream, implicitImports(), innerIndent);
    stream << QLatin1String("\n")
           << indent << QLatin1Char('}');
    return true;
}

QString QmlBundle::toString(const QString &indent)
{
    QString res;
    QTextStream s(&res);
    writeTo(s, indent);
    return res;
}

QStringList QmlBundle::maybeReadTrie(Trie &trie, Utils::JsonObjectValue *config,
                                 const QString &path, const QString &propertyName, bool required)
{
    QStringList res;
    if (!config->hasMember(propertyName)) {
        if (required)
            res << QString::fromLatin1("Missing required property \"%1\" from %2").arg(propertyName,
                                                                                       path);
        return res;
    }
    Utils::JsonValue *imp0 = config->member(propertyName);
    Utils::JsonArrayValue *imp = ((imp0 != 0) ? imp0->toArray() : 0);
    if (imp != 0) {
        foreach (Utils::JsonValue *v, imp->elements()) {
            Utils::JsonStringValue *impStr = ((v != 0) ? v->toString() : 0);
            if (impStr != 0) {
                trie.insert(impStr->value());
            } else {
                res.append(QString::fromLatin1("Expected all elements of array in property \"%1\" "
                                               "to be strings in QmlBundle at %2.")
                           .arg(propertyName, path));
                break;
            }
        }
    } else {
        res.append(QString::fromLatin1("Expected string array in property \"%1\" in QmlBundle at %2.")
                   .arg(propertyName, path));
    }
    return res;
}

bool QmlBundle::readFrom(QString path, QStringList *errors)
{
    Utils::JsonMemoryPool pool;

    using namespace Utils;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errors)
            (*errors) << QString::fromLatin1("Could not open file at %1 .").arg(path);
        return false;
    }
    JsonObjectValue *config = JsonValue::create(QString::fromUtf8(f.readAll()), &pool)->toObject();
    if (config == 0) {
        if (errors)
            (*errors) << QString::fromLatin1("Could not parse json object in file at %1 .").arg(path);
        return false;
    }
    QStringList errs;
    if (config->hasMember(QLatin1String("name"))) {
        JsonValue *n0 = config->member(QLatin1String("name"));
        JsonStringValue *n = ((n0 != 0) ? n0->toString() : 0);
        if (n != 0)
            m_name = n->value();
        else
            errs.append(QString::fromLatin1("Property \"name\" in QmlBundle at %1 is expected "
                                             "to be a string.").arg(path));
    } else {
        errs.append(QString::fromLatin1("Missing required property \"name\" in QmlBundle "
                                         "at %1 .").arg(path));
    }
    errs << maybeReadTrie(m_searchPaths, config, path, QLatin1String("searchPaths"));
    errs << maybeReadTrie(m_installPaths, config, path, QLatin1String("installPaths"));
    errs << maybeReadTrie(m_supportedImports, config, path, QLatin1String("supportedImports")
                             , true);
    errs << maybeReadTrie(m_implicitImports, config, path, QLatin1String("implicitImports"));
    if (errors)
        (*errors) << errs;
    return errs.isEmpty();
}

QmlBundle QmlLanguageBundles::bundleForLanguage(Dialect l) const
{
    if (m_bundles.contains(l))
        return m_bundles.value(l);
    return QmlBundle();
}

void QmlLanguageBundles::mergeBundleForLanguage(Dialect l, const QmlBundle &bundle)
{
    if (bundle.isEmpty())
        return;
    if (m_bundles.contains(l))
        m_bundles[l].merge(bundle);
    else
        m_bundles.insert(l,bundle);
}

QList<Dialect> QmlLanguageBundles::languages() const
{
    return m_bundles.keys();
}

void QmlLanguageBundles::mergeLanguageBundles(const QmlLanguageBundles &o)
{
    foreach (Dialect l, o.languages())
        mergeBundleForLanguage(l, o.bundleForLanguage(l));
}

} // end namespace QmlJS
