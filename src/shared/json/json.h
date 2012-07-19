/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef JSON_H
#define JSON_H

#include "json_global.h"

#include <QByteArray>
#include <QStringList>
#include <QVector>

namespace Json {

class JSON_EXPORT JsonValue
{
public:
    JsonValue() : m_type(Invalid) {}
    explicit JsonValue(const QByteArray &str) { fromString(str); }

    QByteArray m_name;
    QByteArray m_data;
    QList<JsonValue> m_children;

    enum Type {
        Invalid,
        String,
        Number,
        Boolean,
        Object,
        NullObject,
        Array
    };

    Type m_type;

    inline Type type() const { return m_type; }
    inline QByteArray name() const { return m_name; }
    inline bool hasName(const char *name) const { return m_name == name; }

    inline bool isValid() const { return m_type != Invalid; }
    inline bool isNumber() const { return m_type == Number; }
    inline bool isString() const { return m_type == String; }
    inline bool isObject() const { return m_type == Object; }
    inline bool isArray() const { return m_type == Array; }


    inline QByteArray data() const { return m_data; }
    inline const QList<JsonValue> &children() const { return m_children; }
    inline int childCount() const { return m_children.size(); }

    const JsonValue &childAt(int index) const { return m_children[index]; }
    JsonValue &childAt(int index) { return m_children[index]; }
    JsonValue findChild(const char *name) const;

    QByteArray toString(bool multiline = false, int indent = 0) const;
    void fromString(const QByteArray &str);
    void setStreamOutput(const QByteArray &name, const QByteArray &content);

    QVariant toVariant() const;

private:
    static QByteArray parseCString(const char *&from, const char *to);
    static QByteArray parseNumber(const char *&from, const char *to);
    static QByteArray escapeCString(const QByteArray &ba);
    static QString escapeCString(const QString &ba);
    void parsePair(const char *&from, const char *to);
    void parseValue(const char *&from, const char *to);
    void parseObject(const char *&from, const char *to);
    void parseArray(const char *&from, const char *to);

    void dumpChildren(QByteArray *str, bool multiline, int indent) const;
};

/* Thin wrapper around QByteArray for formatting JSON input. Use as in:
 * JsonInputStream(byteArray) << '{' <<  "bla" << ':' << "blup" << '}';
 * Note that strings get double quotes and JSON-escaping, characters should be
 * used for the array/hash delimiters.
 * */
class JSON_EXPORT JsonInputStream {
public:
    explicit JsonInputStream(QByteArray &a) : m_target(a) {}

    JsonInputStream &operator<<(char c) {  m_target.append(c); return *this; }
    JsonInputStream &operator<<(const char *c)  { appendCString(c); return *this; }
    JsonInputStream &operator<<(const QByteArray &a)  { appendCString(a.constData()); return *this; }
    JsonInputStream &operator<<(const QString &c) { appendString(c); return *this; }

    // Format as array
    JsonInputStream &operator<<(const QStringList &c);

    // Format as array
    JsonInputStream &operator<<(const QVector<QByteArray> &ba);

    //Format as array
    JsonInputStream &operator<<(const QList<int> &in);

    JsonInputStream &operator<<(bool b);

    JsonInputStream &operator<<(int i)
        { m_target.append(QByteArray::number(i)); return *this; }
    JsonInputStream &operator<<(unsigned i)
        { m_target.append(QByteArray::number(i)); return *this; }
    JsonInputStream &operator<<(quint64 i)
        { m_target.append(QByteArray::number(i)); return *this; }

private:
    void appendString(const QString &);
    void appendCString(const char *c);

    QByteArray &m_target;
};

} //namespace Json

#endif // JSON_H
