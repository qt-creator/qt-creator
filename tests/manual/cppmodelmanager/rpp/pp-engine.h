/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
/*
  Copyright 2005 Roberto Raggi <roberto@kdevelop.org>

  Permission to use, copy, modify, distribute, and sell this software and its
  documentation for any purpose is hereby granted without fee, provided that
  the above copyright notice appear in all copies and that both that
  copyright notice and this permission notice appear in supporting
  documentation.

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
  KDEVELOP TEAM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
  AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef PP_ENGINE_H
#define PP_ENGINE_H

#include <QHash>
#include <QString>
#include <QStack>

#include "pp-macro.h"
#include "pp-macro-expander.h"
#include "pp-scanner.h"

class Preprocessor;

class pp
{
  QHash<QString, pp_macro*>& m_environment;
  pp_macro_expander expand;
  pp_skip_identifier skip_identifier;
  pp_skip_comment_or_divop skip_comment_or_divop;
  pp_skip_blanks skip_blanks;
  pp_skip_number skip_number;
  QStack<QString> m_files;
  Preprocessor* m_preprocessor;

  class ErrorMessage
  {
    int _M_line;
    int _M_column;
    QString _M_fileName;
    QString _M_message;

  public:
    ErrorMessage ():
      _M_line (0),
      _M_column (0) {}

    inline int line () const { return _M_line; }
    inline void setLine (int line) { _M_line = line; }

    inline int column () const { return _M_column; }
    inline void setColumn (int column) { _M_column = column; }

    inline QString fileName () const { return _M_fileName; }
    inline void setFileName (const QString &fileName) { _M_fileName = fileName; }

    inline QString message () const { return _M_message; }
    inline void setMessage (const QString &message) { _M_message = message; }
  };

  QList<ErrorMessage> _M_error_messages;

  enum { MAX_LEVEL = 512 };
  int _M_skipping[MAX_LEVEL];
  int _M_true_test[MAX_LEVEL];
  int iflevel;
  int nextToken;
  bool haveNextToken;
  bool hideNext;

  long token_value;
  QString token_text;

  enum TOKEN_TYPE
  {
    TOKEN_NUMBER = 1000,
    TOKEN_IDENTIFIER,
    TOKEN_DEFINED,
    TOKEN_LT_LT,
    TOKEN_LT_EQ,
    TOKEN_GT_GT,
    TOKEN_GT_EQ,
    TOKEN_EQ_EQ,
    TOKEN_NOT_EQ,
    TOKEN_OR_OR,
    TOKEN_AND_AND,
  };

  enum PP_DIRECTIVE_TYPE
  {
    PP_UNKNOWN_DIRECTIVE,
    PP_DEFINE,
    PP_INCLUDE,
    PP_ELIF,
    PP_ELSE,
    PP_ENDIF,
    PP_IF,
    PP_IFDEF,
    PP_IFNDEF,
    PP_UNDEF
  };

public:
  pp(Preprocessor* preprocessor, QHash<QString, pp_macro*>& environment);

  QList<ErrorMessage> errorMessages () const;
  void clearErrorMessages ();

  void reportError (const QString &fileName, int line, int column, const QString &message);

  long eval_expression (Stream& input);

  QString processFile(const QString& filename);
  QString processFile(QIODevice* input);
  QString processFile(const QByteArray& input);
  QByteArray processFile(const QByteArray& input, const QString &fileName);

  inline QString currentfile() const {
      return m_files.top();
  }

  void operator () (Stream& input, Stream& output);

  void checkMarkNeeded(Stream& input, Stream& output);

  bool hideNextMacro() const;
  void setHideNextMacro(bool hideNext);

  QHash<QString, pp_macro*>& environment();

private:
  int skipping() const;
  bool test_if_level();

  PP_DIRECTIVE_TYPE find_directive (const QString& directive) const;

  QString find_header_protection(Stream& input);

  void skip(Stream& input, Stream& output, bool outputText = true);

  long eval_primary(Stream& input);

  long eval_multiplicative(Stream& input);

  long eval_additive(Stream& input);

  long eval_shift(Stream& input);

  long eval_relational(Stream& input);

  long eval_equality(Stream& input);

  long eval_and(Stream& input);

  long eval_xor(Stream& input);

  long eval_or(Stream& input);

  long eval_logical_and(Stream& input);

  long eval_logical_or(Stream& input);

  long eval_constant_expression(Stream& input);

  void handle_directive(const QString& directive, Stream& input, Stream& output);

  void handle_include(Stream& input, Stream& output);

  void handle_define(Stream& input);

  void handle_if(Stream& input);

  void handle_else();

  void handle_elif(Stream& input);

  void handle_endif();

  void handle_ifdef(bool check_undefined, Stream& input);

  void handle_undef(Stream& input);

  int next_token (Stream& input);
  int next_token_accept (Stream& input);
  void accept_token();
};

#endif // PP_ENGINE_H

// kate: indent-width 2;

