// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qmljsglobal_p.h"
#include "qmljs/parser/qmljssourcelocation_p.h"

#include "qmljs/parser/qmljsmemorypool_p.h"

#include <QString>
#include <QSet>

#include <qmljs/qmljsconstants.h>
QT_QML_BEGIN_NAMESPACE

namespace QmlJS {

class Lexer;
class MemoryPool;

class QML_PARSER_EXPORT Directives {
public:
    virtual ~Directives() {}

    virtual void pragmaLibrary()
    {
    }

    virtual void importFile(const QString &jsfile, const QString &module, int line, int column)
    {
        Q_UNUSED(jsfile);
        Q_UNUSED(module);
        Q_UNUSED(line);
        Q_UNUSED(column);
    }

    virtual void importModule(const QString &uri, const QString &version, const QString &module, int line, int column)
    {
        Q_UNUSED(uri);
        Q_UNUSED(version);
        Q_UNUSED(module);
        Q_UNUSED(line);
        Q_UNUSED(column);
    }
};

class Engine
{
    Lexer *_lexer = nullptr;
    Directives *_directives = nullptr;
    MemoryPool _pool;
    QList<SourceLocation> _comments;
    QStringList _extraCode;
    QString _code;

public:
    void setCode(const QString &code) { _code = code; }
    const QString &code() const { return _code; }

    void addComment(int pos, int len, int line, int col)
    {
        if (len > 0)
            _comments.append(QmlJS::SourceLocation(pos, len, line, col));
    }

    const QList<SourceLocation> comments() const { return _comments; }

    Lexer *lexer() const { return _lexer; }
    void setLexer(Lexer *lexer) { _lexer = lexer; }

    Directives *directives() const { return _directives; }
    void setDirectives(Directives *directives) { _directives = directives; }

    MemoryPool *pool() { return &_pool; }
    const MemoryPool *pool() const { return &_pool; }

    QStringView midRef(int position, int size) { return QStringView{_code}.mid(position, size); }

    QStringView newStringRef(const QString &text)
    {
        _extraCode.append(text);
        return QStringView{_extraCode.last()};
    }

    QStringView newStringRef(const QChar *chars, int size)
    {
        return newStringRef(QString(chars, size));
    }
};

} // end of namespace QmlJS

QT_QML_END_NAMESPACE

