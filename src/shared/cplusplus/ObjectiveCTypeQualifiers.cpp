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

#include "ObjectiveCTypeQualifiers.h"

CPLUSPLUS_BEGIN_NAMESPACE

static inline int classify2(const char *s) {
  if (s[0] == 'i') {
    if (s[1] == 'n') {
      return Token_in;
    }
  }
  return Token_identifier;
}

static inline int classify3(const char *s) {
  if (s[0] == 'o') {
    if (s[1] == 'u') {
      if (s[2] == 't') {
        return Token_out;
      }
    }
  }
  return Token_identifier;
}

static inline int classify5(const char *s) {
  if (s[0] == 'b') {
    if (s[1] == 'y') {
      if (s[2] == 'r') {
        if (s[3] == 'e') {
          if (s[4] == 'f') {
            return Token_byref;
          }
        }
      }
    }
  }
  else if (s[0] == 'i') {
    if (s[1] == 'n') {
      if (s[2] == 'o') {
        if (s[3] == 'u') {
          if (s[4] == 't') {
            return Token_inout;
          }
        }
      }
    }
  }
  return Token_identifier;
}

static inline int classify6(const char *s) {
  if (s[0] == 'b') {
    if (s[1] == 'y') {
      if (s[2] == 'c') {
        if (s[3] == 'o') {
          if (s[4] == 'p') {
            if (s[5] == 'y') {
              return Token_bycopy;
            }
          }
        }
      }
    }
  }
  else if (s[0] == 'o') {
    if (s[1] == 'n') {
      if (s[2] == 'e') {
        if (s[3] == 'w') {
          if (s[4] == 'a') {
            if (s[5] == 'y') {
              return Token_oneway;
            }
          }
        }
      }
    }
  }
  return Token_identifier;
}

int classifyObjectiveCTypeQualifiers(const char *s, int n) {
  switch (n) {
    case 2: return classify2(s);
    case 3: return classify3(s);
    case 5: return classify5(s);
    case 6: return classify6(s);
    default: return Token_identifier;
  } // switch
}

CPLUSPLUS_END_NAMESPACE
