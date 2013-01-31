/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef GLSLLEXER_H
#define GLSLLEXER_H

#include "glsl.h"
#include <qstring.h>
#include <qstringlist.h>

namespace GLSL {

class GLSL_EXPORT Token
{
public:
    int kind;
    int position;
    int length;
    int line; // ### remove

    union {
        int matchingBrace;
        int i; // integer value
        const QString *string; // string value
        void *ptr;
    };

    Token()
        : kind(0), position(0), length(0), line(0), ptr(0) {}

    bool is(int k) const { return k == kind; }
    bool isNot(int k) const { return k != kind; }

    int begin() const { return position; }
    int end() const { return position + length; }
};

class GLSL_EXPORT Lexer
{
public:
    Lexer(Engine *engine, const char *source, unsigned size);
    ~Lexer();

    enum
    {
        // Extra flag bits added to tokens by Lexer::classify() that
        // indicate which variant of GLSL the keyword belongs to.
        Variant_GLSL_120            = 0x00010000,   // 1.20 and higher
        Variant_GLSL_150            = 0x00020000,   // 1.50 and higher
        Variant_GLSL_400            = 0x00040000,   // 4.00 and higher
        Variant_GLSL_ES_100         = 0x00080000,   // ES 1.00 and higher
        Variant_VertexShader        = 0x00200000,
        Variant_FragmentShader      = 0x00400000,
        Variant_Reserved            = 0x80000000,
        Variant_Mask                = 0xFFFF0000,
        Variant_All                 = 0xFFFF0000
    };

    union Value {
        int i;
        const QString *string;
        void *ptr;
    };

    Engine *engine() const { return _engine; }

    int state() const { return _state; }
    void setState(int state) { _state = state; }

    int variant() const { return _variant; }
    void setVariant(int flags) { _variant = flags; }

    bool scanKeywords() const { return _scanKeywords; }
    void setScanKeywords(bool scanKeywords) { _scanKeywords = scanKeywords; }

    bool scanComments() const { return _scanComments; }
    void setScanComments(bool scanComments) { _scanComments = scanComments; }

    int yylex(Token *tk);
    int findKeyword(const char *word, int length) const;

    void *yyval() const { return _yyval.ptr; }

    static QStringList keywords(int variant);

private:
    static int classify(const char *s, int len);

    void yyinp();
    int yylex_helper(const char **position, int *line);

    void warning(int line, const QString &message);
    void error(int line, const QString &message);

private:
    Engine *_engine;
    const char *_source;
    const char *_it;
    int _size;
    int _yychar;
    int _lineno;
    int _state;
    int _variant;
    unsigned _scanKeywords: 1;
    unsigned _scanComments: 1;
    Value _yyval;
};

} // namespace GLSL

#endif // GLSLLEXER_H
