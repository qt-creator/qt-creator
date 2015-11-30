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

#ifndef QMLJSBUNDLE_H
#define QMLJSBUNDLE_H

#include <qmljs/qmljs_global.h>
#include <qmljs/persistenttrie.h>
#include <qmljs/qmljsdialect.h>

#include <QString>
#include <QHash>

QT_FORWARD_DECLARE_CLASS(QTextStream)

namespace Utils { class JsonObjectValue; }

namespace QmlJS {

/* !
 \class QmlJS::QmlBundle

 A Qmlbundle represents a set of qml libraries, with a list of their exports

 Note that searchPaths, installPaths and implicitImports are PersistentTries
 and not QStringLists.
 This makes merging easier, and the order is not important for our use case.
*/
class QMLJS_EXPORT QmlBundle
{
    typedef PersistentTrie::Trie Trie;
public:
    QmlBundle(const QmlBundle &o);
    QmlBundle();
    QmlBundle(const QString &name,
              const Trie &searchPaths,
              const Trie &installPaths,
              const Trie &supportedImports,
              const Trie &implicitImports);

    QString name() const;
    Trie installPaths() const;
    Trie searchPaths() const;
    Trie implicitImports() const;
    Trie supportedImports() const;

    void merge(const QmlBundle &o);
    void intersect(const QmlBundle &o);
    QmlBundle mergeF(const QmlBundle &o) const;
    QmlBundle intersectF(const QmlBundle &o) const;
    bool isEmpty() const;
    void replaceVars(const QHash<QString, QString> &replacements);
    QmlBundle replaceVarsF(const QHash<QString, QString> &replacements) const;

    bool writeTo(const QString &path) const;
    bool writeTo(QTextStream &stream, const QString &indent = QString()) const;
    QString toString(const QString &indent = QString());
    bool readFrom(QString path, QStringList *errors);
    bool operator==(const QmlBundle &o) const;
    bool operator!=(const QmlBundle &o) const;
private:
    static void printEscaped(QTextStream &s, const QString &str);
    static void writeTrie(QTextStream &stream, const Trie &t, const QString &indent);
    QStringList maybeReadTrie(Trie &trie, Utils::JsonObjectValue *config, const QString &path,
                          const QString &propertyName, bool required = false);

    QString m_name;
    Trie m_searchPaths;
    Trie m_installPaths;
    Trie m_supportedImports;
    Trie m_implicitImports;
};

class QMLJS_EXPORT QmlLanguageBundles
{
public:
    QmlBundle bundleForLanguage(Dialect l) const;
    void mergeBundleForLanguage(Dialect l, const QmlBundle &bundle);
    QList<Dialect> languages() const;
    void mergeLanguageBundles(const QmlLanguageBundles &);
private:
    QHash<Dialect,QmlBundle> m_bundles;
};
} // namespace QmlJS

#endif // QMLJSBUNDLE_H
