/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef DEBUGGER_JSON_H
#define DEBUGGER_JSON_H

#include <QtCore/QByteArray>
#include <QtCore/QList>

namespace Debugger {
namespace Internal {

class JsonValue
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
        Object,
        Array,
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

} // namespace Internal
} // namespace Debugger

//Q_DECLARE_METATYPE(GdbDebugger::Internal::JsonValue);

#endif // DEBUGGER_JSON_H
