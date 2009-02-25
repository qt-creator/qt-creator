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

#include "pp.h"
#include "pp-cctype.h"
#include "pp-macro-expander.h"
#include <QDateTime>

using namespace CPlusPlus;

inline static bool comment_p (const char *__first, const char *__last)
{
    if (__first == __last)
        return false;

    if (*__first != '/')
        return false;

    if (++__first == __last)
        return false;

    return (*__first == '/' || *__first == '*');
}

MacroExpander::MacroExpander (Environment &env, pp_frame *frame)
    : env(env), frame(frame),
      lines(0), generated_lines(0)
{ }

const QByteArray *MacroExpander::resolve_formal(const QByteArray &__name)
{
    if (! (frame && frame->expanding_macro))
        return 0;

    const QVector<QByteArray> formals = frame->expanding_macro->formals();
    for (int index = 0; index < formals.size(); ++index) {
        const QByteArray formal = formals.at(index);

        if (formal == __name && index < frame->actuals.size())
            return &frame->actuals.at(index);
    }

    return 0;
}

const char *MacroExpander::operator () (const char *__first, const char *__last,
                                        QByteArray *__result)
{
    generated_lines = 0;
    __first = skip_blanks (__first, __last);
    lines = skip_blanks.lines;

    while (__first != __last)
    {
        if (*__first == '\n')
        {
            __result->append("\n# ");
            __result->append(QByteArray::number(env.currentLine));
            __result->append(' ');
            __result->append('"');
            __result->append(env.currentFile);
            __result->append('"');
            __result->append('\n');
            ++lines;

            __first = skip_blanks (++__first, __last);
            lines += skip_blanks.lines;

            if (__first != __last && *__first == '#')
                break;
        }
        else if (*__first == '#')
        {
            __first = skip_blanks (++__first, __last);
            lines += skip_blanks.lines;

            const char *end_id = skip_identifier (__first, __last);
            const QByteArray fast_name(__first, end_id - __first);
            __first = end_id;

            if (const QByteArray *actual = resolve_formal (fast_name))
            {
                __result->append('\"');

                const char *actual_begin = actual->constData ();
                const char *actual_end = actual_begin + actual->size ();

                for (const char *it = skip_whitespaces (actual_begin, actual_end);
                        it != actual_end; ++it)
                {
                    if (*it == '"' || *it == '\\')
                    {
                        __result->append('\\');
                        __result->append(*it);
                    }
                    else if (*it == '\n')
                    {
                        __result->append('"');
                        __result->append('\n');
                        __result->append('"');
                    }
                    else
                        __result->append(*it);
                }

                __result->append('\"');
            }
            else
                __result->append('#'); // ### warning message?
        }
        else if (*__first == '\"')
        {
            const char *next_pos = skip_string_literal (__first, __last);
            lines += skip_string_literal.lines;
            __result->append(__first, next_pos - __first);
            __first = next_pos;
        }
        else if (*__first == '\'')
        {
            const char *next_pos = skip_char_literal (__first, __last);
            lines += skip_char_literal.lines;
            __result->append(__first, next_pos - __first);
            __first = next_pos;
        }
        else if (comment_p (__first, __last))
        {
            __first = skip_comment_or_divop (__first, __last);
            int n = skip_comment_or_divop.lines;
            lines += n;

            while (n-- > 0)
                __result->append('\n');
        }
        else if (pp_isspace (*__first))
        {
            for (; __first != __last; ++__first)
            {
                if (*__first == '\n' || !pp_isspace (*__first))
                    break;
            }

            __result->append(' ');
        }
        else if (pp_isdigit (*__first))
        {
            const char *next_pos = skip_number (__first, __last);
            lines += skip_number.lines;
            __result->append(__first, next_pos - __first);
            __first = next_pos;
        }
        else if (pp_isalpha (*__first) || *__first == '_')
        {
            const char *name_begin = __first;
            const char *name_end = skip_identifier (__first, __last);
            __first = name_end; // advance

            // search for the paste token
            const char *next = skip_blanks (__first, __last);
            bool paste = false;
            if (next != __last && *next == '#')
            {
                paste = true;
                ++next;
                if (next != __last && *next == '#')
                    __first = skip_blanks(++next, __last);
            }

            const QByteArray fast_name(name_begin, name_end - name_begin);

            if (const QByteArray *actual = resolve_formal (fast_name))
            {
                const char *begin = actual->constData ();
                const char *end = begin + actual->size ();
                if (paste) {
                    for (--end; end != begin - 1; --end) {
                        if (! pp_isspace(*end))
                            break;
                    }
                    ++end;
                }
                __result->append(begin, end - begin);
                continue;
            }

            Macro *macro = env.resolve (fast_name);
            if (! macro || macro->isHidden() || env.hideNext)
            {
                if (fast_name.size () == 7 && fast_name [0] == 'd' && fast_name == "defined")
                    env.hideNext = true;
                else
                    env.hideNext = false;

                if (fast_name.size () == 8 && fast_name [0] == '_' && fast_name [1] == '_')
                {
                    if (fast_name == "__LINE__")
                    {
                        char buf [16];
                        const size_t count = qsnprintf (buf, 16, "%d", env.currentLine + lines);
                        __result->append(buf, count);
                        continue;
                    }

                    else if (fast_name == "__FILE__")
                    {
                        __result->append('"');
                        __result->append(env.currentFile);
                        __result->append('"');
                        continue;
                    }

                    else if (fast_name == "__DATE__")
                    {
                        __result->append('"');
                        __result->append(QDate::currentDate().toString().toUtf8());
                        __result->append('"');
                        continue;
                    }

                    else if (fast_name == "__TIME__")
                    {
                        __result->append('"');
                        __result->append(QTime::currentTime().toString().toUtf8());
                        __result->append('"');
                        continue;
                    }

                }

                __result->append(name_begin, name_end - name_begin);
                continue;
            }

            if (! macro->isFunctionLike())
            {
                Macro *m = 0;

                if (! macro->definition().isEmpty())
                {
                    macro->setHidden(true);

                    QByteArray __tmp;
                    __tmp.reserve (256);

                    MacroExpander expand_macro (env);
                    expand_macro (macro->definition(), &__tmp);
                    generated_lines += expand_macro.lines;

                    if (! __tmp.isEmpty ())
                    {
                        const char *__tmp_begin = __tmp.constBegin();
                        const char *__tmp_end = __tmp.constEnd();
                        const char *__begin_id = skip_whitespaces (__tmp_begin, __tmp_end);
                        const char *__end_id = skip_identifier (__begin_id, __tmp_end);

                        if (__end_id == __tmp_end)
                        {
                            const QByteArray __id (__begin_id, __end_id - __begin_id);
                            m = env.resolve (__id);
                        }

                        if (! m)
                            *__result += __tmp;
                    }

                    macro->setHidden(false);
                }

                if (! m)
                    continue;

                macro = m;
            }

            // function like macro
            const char *arg_it = skip_whitespaces (__first, __last);

            if (arg_it == __last || *arg_it != '(')
            {
                __result->append(name_begin, name_end - name_begin);
                lines += skip_whitespaces.lines;
                __first = arg_it;
                continue;
            }

            QVector<QByteArray> actuals;
            actuals.reserve (5);
            ++arg_it; // skip '('

            MacroExpander expand_actual (env, frame);

            const char *arg_end = skip_argument_variadics (actuals, macro, arg_it, __last);
            if (arg_it != arg_end)
            {
                const QByteArray actual (arg_it, arg_end - arg_it);
                QByteArray expanded;
                expand_actual (actual.constBegin (), actual.constEnd (), &expanded);
                actuals.push_back (expanded);
                arg_it = arg_end;
            }

            while (arg_it != __last && *arg_end == ',')
            {
                ++arg_it; // skip ','

                arg_end = skip_argument_variadics (actuals, macro, arg_it, __last);
                const QByteArray actual (arg_it, arg_end - arg_it);
                QByteArray expanded;
                expand_actual (actual.constBegin (), actual.constEnd (), &expanded);
                actuals.push_back (expanded);
                arg_it = arg_end;
            }

            if (! (arg_it != __last && *arg_it == ')'))
                return __last;

            ++arg_it; // skip ')'
            __first = arg_it;

            pp_frame frame (macro, actuals);
            MacroExpander expand_macro (env, &frame);
            macro->setHidden(true);
            expand_macro (macro->definition(), __result);
            macro->setHidden(false);
            generated_lines += expand_macro.lines;
        }
        else
            __result->append(*__first++);
    }

    return __first;
}

const char *MacroExpander::skip_argument_variadics (QVector<QByteArray> const &__actuals,
                                                    Macro *__macro,
                                                    const char *__first, const char *__last)
{
    const char *arg_end = skip_argument (__first, __last);

    while (__macro->isVariadic() && __first != arg_end && arg_end != __last && *arg_end == ','
           && (__actuals.size () + 1) == __macro->formals().size ())
    {
        arg_end = skip_argument (++arg_end, __last);
    }

    return arg_end;
}
