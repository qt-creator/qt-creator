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

#include "PreprocessorEnvironment.h"
#include "Macro.h"
#include <cstring>

using namespace CPlusPlus;

Environment::Environment()
    : currentLine(0),
      hideNext(false),
      _macros(0),
      _allocated_macros(0),
      _macro_count(-1),
      _hash(0),
      _hash_count(401)
{
}

Environment::~Environment()
{
    if (_macros) {
        qDeleteAll(firstMacro(), lastMacro());
        free(_macros);
    }

    if (_hash)
        free(_hash);
}

unsigned Environment::macroCount() const
{
    return _macro_count + 1;
}

Macro *Environment::macroAt(unsigned index) const
{
    return _macros[index];
}

Macro *Environment::bind(const Macro &__macro)
{
    Q_ASSERT(! __macro.name().isEmpty());

    Macro *m = new Macro (__macro);
    m->_hashcode = hashCode(m->name());

    if (++_macro_count == _allocated_macros) {
        if (! _allocated_macros)
            _allocated_macros = 401;
        else
            _allocated_macros <<= 1;

        _macros = (Macro **) realloc(_macros, sizeof(Macro *) * _allocated_macros);
    }

    _macros[_macro_count] = m;

    if (! _hash || _macro_count > (_hash_count >> 1)) {
        rehash();
    } else {
        const unsigned h = m->_hashcode % _hash_count;
        m->_next = _hash[h];
        _hash[h] = m;
    }

    return m;
}

Macro *Environment::remove(const QByteArray &name)
{
    Macro macro;
    macro.setName(name);
    macro.setHidden(true);
    macro.setFileName(currentFile);
    macro.setLine(currentLine);
    return bind(macro);
}

bool Environment::isBuiltinMacro(const QByteArray &s) const
{
    if (s.length() != 8)
        return false;

    if (s[0] == '_') {
        if (s[1] == '_') {
            if (s[2] == 'D') {
                if (s[3] == 'A') {
                    if (s[4] == 'T') {
                        if (s[5] == 'E') {
                            if (s[6] == '_') {
                                if (s[7] == '_') {
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
            else if (s[2] == 'F') {
                if (s[3] == 'I') {
                    if (s[4] == 'L') {
                        if (s[5] == 'E') {
                            if (s[6] == '_') {
                                if (s[7] == '_') {
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
            else if (s[2] == 'L') {
                if (s[3] == 'I') {
                    if (s[4] == 'N') {
                        if (s[5] == 'E') {
                            if (s[6] == '_') {
                                if (s[7] == '_') {
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
            else if (s[2] == 'T') {
                if (s[3] == 'I') {
                    if (s[4] == 'M') {
                        if (s[5] == 'E') {
                            if (s[6] == '_') {
                                if (s[7] == '_') {
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

Macro *Environment::resolve(const QByteArray &name) const
{
    if (! _macros)
        return 0;

    Macro *it = _hash[hashCode(name) % _hash_count];
    for (; it; it = it->_next) {
        if (it->name() != name)
            continue;
        else if (it->isHidden())
            return 0;
        else break;
    }
    return it;
}

unsigned Environment::hashCode(const QByteArray &s)
{
    unsigned hash_value = 0;

    for (int i = 0; i < s.size (); ++i)
        hash_value = (hash_value << 5) - hash_value + s.at (i);

    return hash_value;
}

void Environment::rehash()
{
    if (_hash) {
        free(_hash);
        _hash_count <<= 1;
    }

    _hash = (Macro **) calloc(_hash_count, sizeof(Macro *));

    for (Macro **it = firstMacro(); it != lastMacro(); ++it) {
        Macro *m= *it;
        const unsigned h = m->_hashcode % _hash_count;
        m->_next = _hash[h];
        _hash[h] = m;
    }
}
