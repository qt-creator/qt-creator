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
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
/*
  Copyright 2005 Roberto Raggi <roberto@kdevelop.org>
  Copyright 2006 Hamish Rodda <rodda@kde.org>

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

#include "pp-macro-expander.h"

#include <QtCore/QDebug>

#include "pp-internal.h"
#include "pp-engine.h"

pp_frame::pp_frame(pp_macro* __expandingMacro, const QList<QString>& __actuals)
  : expandingMacro(__expandingMacro)
  , actuals(__actuals)
{
}

QString pp_macro_expander::resolve_formal(const QString& name)
{
  Q_ASSERT(!name.isEmpty());

  if (!m_frame)
    return QString();

  Q_ASSERT(m_frame->expandingMacro != 0);

  const QStringList& formals = m_frame->expandingMacro->formals;

  for (int index = 0; index < formals.size(); ++index) {
    if (name == formals[index]) {
      if (index < m_frame->actuals.size())
        return m_frame->actuals[index];
      else
        Q_ASSERT(0); // internal error?
    }
  }

  return QString();
}

pp_macro_expander::pp_macro_expander(pp* engine, pp_frame* frame)
  : m_engine(engine)
  , m_frame(frame)
{
}

void pp_macro_expander::operator()(Stream& input, Stream& output)
{
  skip_blanks(input, output);

  while (!input.atEnd())
  {
    if (input == '\n')
    {
      output << input;

      skip_blanks(++input, output);

      if (!input.atEnd() && input == '#')
        break;
    }
    else if (input == '#')
    {
      skip_blanks(++input, output);

      QString identifier = skip_identifier(input);
      output << '\"';

      Stream is(&identifier);
      operator()(is, output);

      output << '\"';
    }
    else if (input == '\"')
    {
      skip_string_literal(input, output);
    }
    else if (input == '\'')
    {
      skip_char_literal(input, output);
    }
    else if (PPInternal::isComment(input))
    {
      skip_comment_or_divop(input, output);
    }
    else if (input.current().isSpace())
    {
      do {
        if (input == '\n' || !input.current().isSpace())
          break;

      } while (!(++input).atEnd());

      output << ' ';
    }
    else if (input.current().isNumber())
    {
      skip_number (input, output);
    }
    else if (input.current().isLetter() || input == '_')
    {
      QString name = skip_identifier (input);

      // search for the paste token
      qint64 blankStart = input.pos();
      skip_blanks (input, PPInternal::devnull());
      if (!input.atEnd() && input == '#') {
        ++input;

        if (!input.atEnd() && input == '#')
          skip_blanks(++input, PPInternal::devnull());
        else
          input.seek(blankStart);

      } else {
        input.seek(blankStart);
      }

      Q_ASSERT(name.length() >= 0 && name.length() < 512);

      QString actual = resolve_formal(name);
      if (!actual.isEmpty()) {
        output << actual;
        continue;
      }

      pp_macro* macro = m_engine->environment().value(name, 0);
      if (! macro || macro->hidden || m_engine->hideNextMacro())
      {
        m_engine->setHideNextMacro(name == "defined");
        output << name;
        continue;
      }

      if (!macro->function_like)
      {
        pp_macro_expander expand_macro(m_engine);
        macro->hidden = true;
        Stream ms(&macro->definition, QIODevice::ReadOnly);
        expand_macro(ms, output);
        macro->hidden = false;
        continue;
      }

      // function like macro
      if (input.atEnd() || input != '(')
      {
        output << name;
        continue;
      }

      QList<QString> actuals;
      ++input; // skip '('

      pp_macro_expander expand_actual(m_engine, m_frame);

      qint64 before = input.pos();
      {
        actual.clear();

        {
          Stream as(&actual);
          skip_argument_variadics(actuals, macro, input, as);
        }

        if (input.pos() != before)
        {
          QString newActual;
          {
            Stream as(&actual);
            Stream nas(&newActual);
            expand_actual(as, nas);
          }
          actuals.append(newActual);
        }
      }

      // TODO: why separate from the above?
      while (!input.atEnd() && input == ',')
      {
        actual.clear();
        ++input; // skip ','

        {
          {
            Stream as(&actual);
            skip_argument_variadics(actuals, macro, input, as);
          }

          QString newActual;
          {
            Stream as(&actual);
            Stream nas(&newActual);
            expand_actual(as, nas);
          }
          actuals.append(newActual);
        }
      }

      //Q_ASSERT(!input.atEnd() && input == ')');

      ++input; // skip ')'

#if 0 // ### enable me
      assert ((macro->variadics && macro->formals.size () >= actuals.size ())
                  || macro->formals.size() == actuals.size());
#endif

      pp_frame frame(macro, actuals);
      pp_macro_expander expand_macro(m_engine, &frame);
      macro->hidden = true;
      Stream ms(&macro->definition, QIODevice::ReadOnly);
      expand_macro(ms, output);
      macro->hidden = false;

    } else {
      output << input;
      ++input;
    }
  }
}

void pp_macro_expander::skip_argument_variadics (const QList<QString>& __actuals, pp_macro *__macro, Stream& input, Stream& output)
{
  qint64 first;

  do {
    first = input.pos();
    skip_argument(input, output);

  } while ( __macro->variadics
            && first != input.pos()
            && !input.atEnd()
            && input == '.'
            && (__actuals.size() + 1) == __macro->formals.size());
}

// kate: indent-width 2;
