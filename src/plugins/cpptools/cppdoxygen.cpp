/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include <QString>
#include "cppdoxygen.h"

using namespace CppTools;

/*
  TODO:
    ~
    @
    $
    \
    #
    f[
    f]
    f$
*/

static const char *doxy_token_spell[] = {
    "identifier",

    "arg",
    "attention",
    "author",
    "callgraph",
    "code",
    "dot",
    "else",
    "endcode",
    "endcond",
    "enddot",
    "endhtmlonly",
    "endif",
    "endlatexonly",
    "endlink",
    "endmanonly",
    "endverbatim",
    "endxmlonly",
    "hideinitializer",
    "htmlonly",
    "interface",
    "internal",
    "invariant",
    "latexonly",
    "li",
    "manonly",
    "n",
    "nosubgrouping",
    "note",
    "only",
    "post",
    "pre",
    "remarks",
    "return",
    "returns",
    "sa",
    "see",
    "showinitializer",
    "since",
    "test",
    "todo",
    "verbatim",
    "warning",
    "xmlonly",

    "a",
    "addtogroup",
    "anchor",
    "b",
    "c",
    "class",
    "cond",
    "copydoc",
    "def",
    "dontinclude",
    "dotfile",
    "e",
    "elseif",
    "em",
    "enum",
    "example",
    "exception",
    "exceptions",
    "file",
    "htmlinclude",
    "if",
    "ifnot",
    "include",
    "link",
    "namespace",
    "p",
    "package",
    "ref",
    "relates",
    "relatesalso",
    "retval",
    "throw",
    "throws",
    "verbinclude",
    "version",
    "xrefitem",

    "param",

    "image",

    "defgroup",
    "page",
    "paragraph",
    "section",
    "struct",
    "subsection",
    "subsubsection",
    "union",
    "weakgroup",

    "addindex",
    "brief",
    "bug",
    "date",
    "deprecated",
    "fn",
    "ingroup",
    "line",
    "mainpage",
    "name",
    "overload",
    "par",
    "short",
    "skip",
    "skipline",
    "typedef",
    "until",
    "var"
};

static inline int classify1(const QChar *s) {
  if (s[0].unicode() == 'a') {
    return T_DOXY_A;
  }
  else if (s[0].unicode() == 'b') {
    return T_DOXY_B;
  }
  else if (s[0].unicode() == 'c') {
    return T_DOXY_C;
  }
  else if (s[0].unicode() == 'e') {
    return T_DOXY_E;
  }
  else if (s[0].unicode() == 'n') {
    return T_DOXY_N;
  }
  else if (s[0].unicode() == 'p') {
    return T_DOXY_P;
  }
  return T_DOXY_IDENTIFIER;
}

static inline int classify2(const QChar *s) {
  if (s[0].unicode() == 'e') {
    if (s[1].unicode() == 'm') {
      return T_DOXY_EM;
    }
  }
  else if (s[0].unicode() == 'f') {
    if (s[1].unicode() == 'n') {
      return T_DOXY_FN;
    }
  }
  else if (s[0].unicode() == 'i') {
    if (s[1].unicode() == 'f') {
      return T_DOXY_IF;
    }
  }
  else if (s[0].unicode() == 'l') {
    if (s[1].unicode() == 'i') {
      return T_DOXY_LI;
    }
  }
  else if (s[0].unicode() == 's') {
    if (s[1].unicode() == 'a') {
      return T_DOXY_SA;
    }
  }
  return T_DOXY_IDENTIFIER;
}

static inline int classify3(const QChar *s) {
  if (s[0].unicode() == 'a') {
    if (s[1].unicode() == 'r') {
      if (s[2].unicode() == 'g') {
        return T_DOXY_ARG;
      }
    }
  }
  else if (s[0].unicode() == 'b') {
    if (s[1].unicode() == 'u') {
      if (s[2].unicode() == 'g') {
        return T_DOXY_BUG;
      }
    }
  }
  else if (s[0].unicode() == 'd') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'f') {
        return T_DOXY_DEF;
      }
    }
    else if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 't') {
        return T_DOXY_DOT;
      }
    }
  }
  else if (s[0].unicode() == 'p') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'r') {
        return T_DOXY_PAR;
      }
    }
    else if (s[1].unicode() == 'r') {
      if (s[2].unicode() == 'e') {
        return T_DOXY_PRE;
      }
    }
  }
  else if (s[0].unicode() == 'r') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'f') {
        return T_DOXY_REF;
      }
    }
  }
  else if (s[0].unicode() == 's') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'e') {
        return T_DOXY_SEE;
      }
    }
  }
  else if (s[0].unicode() == 'v') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'r') {
        return T_DOXY_VAR;
      }
    }
  }
  return T_DOXY_IDENTIFIER;
}

static inline int classify4(const QChar *s) {
  if (s[0].unicode() == 'c') {
    if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'e') {
          return T_DOXY_CODE;
        }
      }
      else if (s[2].unicode() == 'n') {
        if (s[3].unicode() == 'd') {
          return T_DOXY_COND;
        }
      }
    }
  }
  else if (s[0].unicode() == 'd') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'e') {
          return T_DOXY_DATE;
        }
      }
    }
  }
  else if (s[0].unicode() == 'e') {
    if (s[1].unicode() == 'l') {
      if (s[2].unicode() == 's') {
        if (s[3].unicode() == 'e') {
          return T_DOXY_ELSE;
        }
      }
    }
    else if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'u') {
        if (s[3].unicode() == 'm') {
          return T_DOXY_ENUM;
        }
      }
    }
  }
  else if (s[0].unicode() == 'f') {
    if (s[1].unicode() == 'i') {
      if (s[2].unicode() == 'l') {
        if (s[3].unicode() == 'e') {
          return T_DOXY_FILE;
        }
      }
    }
  }
  else if (s[0].unicode() == 'l') {
    if (s[1].unicode() == 'i') {
      if (s[2].unicode() == 'n') {
        if (s[3].unicode() == 'e') {
          return T_DOXY_LINE;
        }
        else if (s[3].unicode() == 'k') {
          return T_DOXY_LINK;
        }
      }
    }
  }
  else if (s[0].unicode() == 'n') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'm') {
        if (s[3].unicode() == 'e') {
          return T_DOXY_NAME;
        }
      }
    }
    else if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'e') {
          return T_DOXY_NOTE;
        }
      }
    }
  }
  else if (s[0].unicode() == 'o') {
    if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'l') {
        if (s[3].unicode() == 'y') {
          return T_DOXY_ONLY;
        }
      }
    }
  }
  else if (s[0].unicode() == 'p') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'g') {
        if (s[3].unicode() == 'e') {
          return T_DOXY_PAGE;
        }
      }
    }
    else if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 's') {
        if (s[3].unicode() == 't') {
          return T_DOXY_POST;
        }
      }
    }
  }
  else if (s[0].unicode() == 's') {
    if (s[1].unicode() == 'k') {
      if (s[2].unicode() == 'i') {
        if (s[3].unicode() == 'p') {
          return T_DOXY_SKIP;
        }
      }
    }
  }
  else if (s[0].unicode() == 't') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 's') {
        if (s[3].unicode() == 't') {
          return T_DOXY_TEST;
        }
      }
    }
    else if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'o') {
          return T_DOXY_TODO;
        }
      }
    }
  }
  return T_DOXY_IDENTIFIER;
}

static inline int classify5(const QChar *s) {
  if (s[0].unicode() == 'b') {
    if (s[1].unicode() == 'r') {
      if (s[2].unicode() == 'i') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'f') {
            return T_DOXY_BRIEF;
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'c') {
    if (s[1].unicode() == 'l') {
      if (s[2].unicode() == 'a') {
        if (s[3].unicode() == 's') {
          if (s[4].unicode() == 's') {
            return T_DOXY_CLASS;
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'e') {
    if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'i') {
          if (s[4].unicode() == 'f') {
            return T_DOXY_ENDIF;
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'i') {
    if (s[1].unicode() == 'f') {
      if (s[2].unicode() == 'n') {
        if (s[3].unicode() == 'o') {
          if (s[4].unicode() == 't') {
            return T_DOXY_IFNOT;
          }
        }
      }
    }
    else if (s[1].unicode() == 'm') {
      if (s[2].unicode() == 'a') {
        if (s[3].unicode() == 'g') {
          if (s[4].unicode() == 'e') {
            return T_DOXY_IMAGE;
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'p') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'r') {
        if (s[3].unicode() == 'a') {
          if (s[4].unicode() == 'm') {
            return T_DOXY_PARAM;
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 's') {
    if (s[1].unicode() == 'h') {
      if (s[2].unicode() == 'o') {
        if (s[3].unicode() == 'r') {
          if (s[4].unicode() == 't') {
            return T_DOXY_SHORT;
          }
        }
      }
    }
    else if (s[1].unicode() == 'i') {
      if (s[2].unicode() == 'n') {
        if (s[3].unicode() == 'c') {
          if (s[4].unicode() == 'e') {
            return T_DOXY_SINCE;
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 't') {
    if (s[1].unicode() == 'h') {
      if (s[2].unicode() == 'r') {
        if (s[3].unicode() == 'o') {
          if (s[4].unicode() == 'w') {
            return T_DOXY_THROW;
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'u') {
    if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'i') {
        if (s[3].unicode() == 'o') {
          if (s[4].unicode() == 'n') {
            return T_DOXY_UNION;
          }
        }
      }
      else if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'i') {
          if (s[4].unicode() == 'l') {
            return T_DOXY_UNTIL;
          }
        }
      }
    }
  }
  return T_DOXY_IDENTIFIER;
}

static inline int classify6(const QChar *s) {
  if (s[0].unicode() == 'a') {
    if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'c') {
        if (s[3].unicode() == 'h') {
          if (s[4].unicode() == 'o') {
            if (s[5].unicode() == 'r') {
              return T_DOXY_ANCHOR;
            }
          }
        }
      }
    }
    else if (s[1].unicode() == 'u') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'h') {
          if (s[4].unicode() == 'o') {
            if (s[5].unicode() == 'r') {
              return T_DOXY_AUTHOR;
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'e') {
    if (s[1].unicode() == 'l') {
      if (s[2].unicode() == 's') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'i') {
            if (s[5].unicode() == 'f') {
              return T_DOXY_ELSEIF;
            }
          }
        }
      }
    }
    else if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'd') {
          if (s[4].unicode() == 'o') {
            if (s[5].unicode() == 't') {
              return T_DOXY_ENDDOT;
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'r') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'u') {
          if (s[4].unicode() == 'r') {
            if (s[5].unicode() == 'n') {
              return T_DOXY_RETURN;
            }
          }
        }
        else if (s[3].unicode() == 'v') {
          if (s[4].unicode() == 'a') {
            if (s[5].unicode() == 'l') {
              return T_DOXY_RETVAL;
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 's') {
    if (s[1].unicode() == 't') {
      if (s[2].unicode() == 'r') {
        if (s[3].unicode() == 'u') {
          if (s[4].unicode() == 'c') {
            if (s[5].unicode() == 't') {
              return T_DOXY_STRUCT;
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 't') {
    if (s[1].unicode() == 'h') {
      if (s[2].unicode() == 'r') {
        if (s[3].unicode() == 'o') {
          if (s[4].unicode() == 'w') {
            if (s[5].unicode() == 's') {
              return T_DOXY_THROWS;
            }
          }
        }
      }
    }
  }
  return T_DOXY_IDENTIFIER;
}

static inline int classify7(const QChar *s) {
  if (s[0].unicode() == 'c') {
    if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 'p') {
        if (s[3].unicode() == 'y') {
          if (s[4].unicode() == 'd') {
            if (s[5].unicode() == 'o') {
              if (s[6].unicode() == 'c') {
                return T_DOXY_COPYDOC;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'd') {
    if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'f') {
          if (s[4].unicode() == 'i') {
            if (s[5].unicode() == 'l') {
              if (s[6].unicode() == 'e') {
                return T_DOXY_DOTFILE;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'e') {
    if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'c') {
          if (s[4].unicode() == 'o') {
            if (s[5].unicode() == 'd') {
              if (s[6].unicode() == 'e') {
                return T_DOXY_ENDCODE;
              }
            }
            else if (s[5].unicode() == 'n') {
              if (s[6].unicode() == 'd') {
                return T_DOXY_ENDCOND;
              }
            }
          }
        }
        else if (s[3].unicode() == 'l') {
          if (s[4].unicode() == 'i') {
            if (s[5].unicode() == 'n') {
              if (s[6].unicode() == 'k') {
                return T_DOXY_ENDLINK;
              }
            }
          }
        }
      }
    }
    else if (s[1].unicode() == 'x') {
      if (s[2].unicode() == 'a') {
        if (s[3].unicode() == 'm') {
          if (s[4].unicode() == 'p') {
            if (s[5].unicode() == 'l') {
              if (s[6].unicode() == 'e') {
                return T_DOXY_EXAMPLE;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'i') {
    if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'c') {
        if (s[3].unicode() == 'l') {
          if (s[4].unicode() == 'u') {
            if (s[5].unicode() == 'd') {
              if (s[6].unicode() == 'e') {
                return T_DOXY_INCLUDE;
              }
            }
          }
        }
      }
      else if (s[2].unicode() == 'g') {
        if (s[3].unicode() == 'r') {
          if (s[4].unicode() == 'o') {
            if (s[5].unicode() == 'u') {
              if (s[6].unicode() == 'p') {
                return T_DOXY_INGROUP;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'm') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'n') {
        if (s[3].unicode() == 'o') {
          if (s[4].unicode() == 'n') {
            if (s[5].unicode() == 'l') {
              if (s[6].unicode() == 'y') {
                return T_DOXY_MANONLY;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'p') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'c') {
        if (s[3].unicode() == 'k') {
          if (s[4].unicode() == 'a') {
            if (s[5].unicode() == 'g') {
              if (s[6].unicode() == 'e') {
                return T_DOXY_PACKAGE;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'r') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'l') {
        if (s[3].unicode() == 'a') {
          if (s[4].unicode() == 't') {
            if (s[5].unicode() == 'e') {
              if (s[6].unicode() == 's') {
                return T_DOXY_RELATES;
              }
            }
          }
        }
      }
      else if (s[2].unicode() == 'm') {
        if (s[3].unicode() == 'a') {
          if (s[4].unicode() == 'r') {
            if (s[5].unicode() == 'k') {
              if (s[6].unicode() == 's') {
                return T_DOXY_REMARKS;
              }
            }
          }
        }
      }
      else if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'u') {
          if (s[4].unicode() == 'r') {
            if (s[5].unicode() == 'n') {
              if (s[6].unicode() == 's') {
                return T_DOXY_RETURNS;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 's') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'c') {
        if (s[3].unicode() == 't') {
          if (s[4].unicode() == 'i') {
            if (s[5].unicode() == 'o') {
              if (s[6].unicode() == 'n') {
                return T_DOXY_SECTION;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 't') {
    if (s[1].unicode() == 'y') {
      if (s[2].unicode() == 'p') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'd') {
            if (s[5].unicode() == 'e') {
              if (s[6].unicode() == 'f') {
                return T_DOXY_TYPEDEF;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'v') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'r') {
        if (s[3].unicode() == 's') {
          if (s[4].unicode() == 'i') {
            if (s[5].unicode() == 'o') {
              if (s[6].unicode() == 'n') {
                return T_DOXY_VERSION;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'w') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'r') {
        if (s[3].unicode() == 'n') {
          if (s[4].unicode() == 'i') {
            if (s[5].unicode() == 'n') {
              if (s[6].unicode() == 'g') {
                return T_DOXY_WARNING;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'x') {
    if (s[1].unicode() == 'm') {
      if (s[2].unicode() == 'l') {
        if (s[3].unicode() == 'o') {
          if (s[4].unicode() == 'n') {
            if (s[5].unicode() == 'l') {
              if (s[6].unicode() == 'y') {
                return T_DOXY_XMLONLY;
              }
            }
          }
        }
      }
    }
  }
  return T_DOXY_IDENTIFIER;
}

static inline int classify8(const QChar *s) {
  if (s[0].unicode() == 'a') {
    if (s[1].unicode() == 'd') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'i') {
          if (s[4].unicode() == 'n') {
            if (s[5].unicode() == 'd') {
              if (s[6].unicode() == 'e') {
                if (s[7].unicode() == 'x') {
                  return T_DOXY_ADDINDEX;
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'd') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'f') {
        if (s[3].unicode() == 'g') {
          if (s[4].unicode() == 'r') {
            if (s[5].unicode() == 'o') {
              if (s[6].unicode() == 'u') {
                if (s[7].unicode() == 'p') {
                  return T_DOXY_DEFGROUP;
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'h') {
    if (s[1].unicode() == 't') {
      if (s[2].unicode() == 'm') {
        if (s[3].unicode() == 'l') {
          if (s[4].unicode() == 'o') {
            if (s[5].unicode() == 'n') {
              if (s[6].unicode() == 'l') {
                if (s[7].unicode() == 'y') {
                  return T_DOXY_HTMLONLY;
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'i') {
    if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'r') {
            if (s[5].unicode() == 'n') {
              if (s[6].unicode() == 'a') {
                if (s[7].unicode() == 'l') {
                  return T_DOXY_INTERNAL;
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'm') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'i') {
        if (s[3].unicode() == 'n') {
          if (s[4].unicode() == 'p') {
            if (s[5].unicode() == 'a') {
              if (s[6].unicode() == 'g') {
                if (s[7].unicode() == 'e') {
                  return T_DOXY_MAINPAGE;
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'o') {
    if (s[1].unicode() == 'v') {
      if (s[2].unicode() == 'e') {
        if (s[3].unicode() == 'r') {
          if (s[4].unicode() == 'l') {
            if (s[5].unicode() == 'o') {
              if (s[6].unicode() == 'a') {
                if (s[7].unicode() == 'd') {
                  return T_DOXY_OVERLOAD;
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 's') {
    if (s[1].unicode() == 'k') {
      if (s[2].unicode() == 'i') {
        if (s[3].unicode() == 'p') {
          if (s[4].unicode() == 'l') {
            if (s[5].unicode() == 'i') {
              if (s[6].unicode() == 'n') {
                if (s[7].unicode() == 'e') {
                  return T_DOXY_SKIPLINE;
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'v') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'r') {
        if (s[3].unicode() == 'b') {
          if (s[4].unicode() == 'a') {
            if (s[5].unicode() == 't') {
              if (s[6].unicode() == 'i') {
                if (s[7].unicode() == 'm') {
                  return T_DOXY_VERBATIM;
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'x') {
    if (s[1].unicode() == 'r') {
      if (s[2].unicode() == 'e') {
        if (s[3].unicode() == 'f') {
          if (s[4].unicode() == 'i') {
            if (s[5].unicode() == 't') {
              if (s[6].unicode() == 'e') {
                if (s[7].unicode() == 'm') {
                  return T_DOXY_XREFITEM;
                }
              }
            }
          }
        }
      }
    }
  }
  return T_DOXY_IDENTIFIER;
}

static inline int classify9(const QChar *s) {
  if (s[0].unicode() == 'a') {
    if (s[1].unicode() == 't') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'n') {
            if (s[5].unicode() == 't') {
              if (s[6].unicode() == 'i') {
                if (s[7].unicode() == 'o') {
                  if (s[8].unicode() == 'n') {
                    return T_DOXY_ATTENTION;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'c') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'l') {
        if (s[3].unicode() == 'l') {
          if (s[4].unicode() == 'g') {
            if (s[5].unicode() == 'r') {
              if (s[6].unicode() == 'a') {
                if (s[7].unicode() == 'p') {
                  if (s[8].unicode() == 'h') {
                    return T_DOXY_CALLGRAPH;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'e') {
    if (s[1].unicode() == 'x') {
      if (s[2].unicode() == 'c') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'p') {
            if (s[5].unicode() == 't') {
              if (s[6].unicode() == 'i') {
                if (s[7].unicode() == 'o') {
                  if (s[8].unicode() == 'n') {
                    return T_DOXY_EXCEPTION;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'i') {
    if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'r') {
            if (s[5].unicode() == 'f') {
              if (s[6].unicode() == 'a') {
                if (s[7].unicode() == 'c') {
                  if (s[8].unicode() == 'e') {
                    return T_DOXY_INTERFACE;
                  }
                }
              }
            }
          }
        }
      }
      else if (s[2].unicode() == 'v') {
        if (s[3].unicode() == 'a') {
          if (s[4].unicode() == 'r') {
            if (s[5].unicode() == 'i') {
              if (s[6].unicode() == 'a') {
                if (s[7].unicode() == 'n') {
                  if (s[8].unicode() == 't') {
                    return T_DOXY_INVARIANT;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'l') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'x') {
            if (s[5].unicode() == 'o') {
              if (s[6].unicode() == 'n') {
                if (s[7].unicode() == 'l') {
                  if (s[8].unicode() == 'y') {
                    return T_DOXY_LATEXONLY;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'n') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'm') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 's') {
            if (s[5].unicode() == 'p') {
              if (s[6].unicode() == 'a') {
                if (s[7].unicode() == 'c') {
                  if (s[8].unicode() == 'e') {
                    return T_DOXY_NAMESPACE;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'p') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'r') {
        if (s[3].unicode() == 'a') {
          if (s[4].unicode() == 'g') {
            if (s[5].unicode() == 'r') {
              if (s[6].unicode() == 'a') {
                if (s[7].unicode() == 'p') {
                  if (s[8].unicode() == 'h') {
                    return T_DOXY_PARAGRAPH;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'w') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'a') {
        if (s[3].unicode() == 'k') {
          if (s[4].unicode() == 'g') {
            if (s[5].unicode() == 'r') {
              if (s[6].unicode() == 'o') {
                if (s[7].unicode() == 'u') {
                  if (s[8].unicode() == 'p') {
                    return T_DOXY_WEAKGROUP;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return T_DOXY_IDENTIFIER;
}

static inline int classify10(const QChar *s) {
  if (s[0].unicode() == 'a') {
    if (s[1].unicode() == 'd') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 't') {
          if (s[4].unicode() == 'o') {
            if (s[5].unicode() == 'g') {
              if (s[6].unicode() == 'r') {
                if (s[7].unicode() == 'o') {
                  if (s[8].unicode() == 'u') {
                    if (s[9].unicode() == 'p') {
                      return T_DOXY_ADDTOGROUP;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'd') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'p') {
        if (s[3].unicode() == 'r') {
          if (s[4].unicode() == 'e') {
            if (s[5].unicode() == 'c') {
              if (s[6].unicode() == 'a') {
                if (s[7].unicode() == 't') {
                  if (s[8].unicode() == 'e') {
                    if (s[9].unicode() == 'd') {
                      return T_DOXY_DEPRECATED;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'e') {
    if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'm') {
          if (s[4].unicode() == 'a') {
            if (s[5].unicode() == 'n') {
              if (s[6].unicode() == 'o') {
                if (s[7].unicode() == 'n') {
                  if (s[8].unicode() == 'l') {
                    if (s[9].unicode() == 'y') {
                      return T_DOXY_ENDMANONLY;
                    }
                  }
                }
              }
            }
          }
        }
        else if (s[3].unicode() == 'x') {
          if (s[4].unicode() == 'm') {
            if (s[5].unicode() == 'l') {
              if (s[6].unicode() == 'o') {
                if (s[7].unicode() == 'n') {
                  if (s[8].unicode() == 'l') {
                    if (s[9].unicode() == 'y') {
                      return T_DOXY_ENDXMLONLY;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    else if (s[1].unicode() == 'x') {
      if (s[2].unicode() == 'c') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'p') {
            if (s[5].unicode() == 't') {
              if (s[6].unicode() == 'i') {
                if (s[7].unicode() == 'o') {
                  if (s[8].unicode() == 'n') {
                    if (s[9].unicode() == 's') {
                      return T_DOXY_EXCEPTIONS;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 's') {
    if (s[1].unicode() == 'u') {
      if (s[2].unicode() == 'b') {
        if (s[3].unicode() == 's') {
          if (s[4].unicode() == 'e') {
            if (s[5].unicode() == 'c') {
              if (s[6].unicode() == 't') {
                if (s[7].unicode() == 'i') {
                  if (s[8].unicode() == 'o') {
                    if (s[9].unicode() == 'n') {
                      return T_DOXY_SUBSECTION;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return T_DOXY_IDENTIFIER;
}

static inline int classify11(const QChar *s) {
  if (s[0].unicode() == 'd') {
    if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 'n') {
        if (s[3].unicode() == 't') {
          if (s[4].unicode() == 'i') {
            if (s[5].unicode() == 'n') {
              if (s[6].unicode() == 'c') {
                if (s[7].unicode() == 'l') {
                  if (s[8].unicode() == 'u') {
                    if (s[9].unicode() == 'd') {
                      if (s[10].unicode() == 'e') {
                        return T_DOXY_DONTINCLUDE;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'e') {
    if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'h') {
          if (s[4].unicode() == 't') {
            if (s[5].unicode() == 'm') {
              if (s[6].unicode() == 'l') {
                if (s[7].unicode() == 'o') {
                  if (s[8].unicode() == 'n') {
                    if (s[9].unicode() == 'l') {
                      if (s[10].unicode() == 'y') {
                        return T_DOXY_ENDHTMLONLY;
                      }
                    }
                  }
                }
              }
            }
          }
        }
        else if (s[3].unicode() == 'v') {
          if (s[4].unicode() == 'e') {
            if (s[5].unicode() == 'r') {
              if (s[6].unicode() == 'b') {
                if (s[7].unicode() == 'a') {
                  if (s[8].unicode() == 't') {
                    if (s[9].unicode() == 'i') {
                      if (s[10].unicode() == 'm') {
                        return T_DOXY_ENDVERBATIM;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'h') {
    if (s[1].unicode() == 't') {
      if (s[2].unicode() == 'm') {
        if (s[3].unicode() == 'l') {
          if (s[4].unicode() == 'i') {
            if (s[5].unicode() == 'n') {
              if (s[6].unicode() == 'c') {
                if (s[7].unicode() == 'l') {
                  if (s[8].unicode() == 'u') {
                    if (s[9].unicode() == 'd') {
                      if (s[10].unicode() == 'e') {
                        return T_DOXY_HTMLINCLUDE;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'r') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'l') {
        if (s[3].unicode() == 'a') {
          if (s[4].unicode() == 't') {
            if (s[5].unicode() == 'e') {
              if (s[6].unicode() == 's') {
                if (s[7].unicode() == 'a') {
                  if (s[8].unicode() == 'l') {
                    if (s[9].unicode() == 's') {
                      if (s[10].unicode() == 'o') {
                        return T_DOXY_RELATESALSO;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'v') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'r') {
        if (s[3].unicode() == 'b') {
          if (s[4].unicode() == 'i') {
            if (s[5].unicode() == 'n') {
              if (s[6].unicode() == 'c') {
                if (s[7].unicode() == 'l') {
                  if (s[8].unicode() == 'u') {
                    if (s[9].unicode() == 'd') {
                      if (s[10].unicode() == 'e') {
                        return T_DOXY_VERBINCLUDE;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return T_DOXY_IDENTIFIER;
}

static inline int classify12(const QChar *s) {
  if (s[0].unicode() == 'e') {
    if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'l') {
          if (s[4].unicode() == 'a') {
            if (s[5].unicode() == 't') {
              if (s[6].unicode() == 'e') {
                if (s[7].unicode() == 'x') {
                  if (s[8].unicode() == 'o') {
                    if (s[9].unicode() == 'n') {
                      if (s[10].unicode() == 'l') {
                        if (s[11].unicode() == 'y') {
                          return T_DOXY_ENDLATEXONLY;
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return T_DOXY_IDENTIFIER;
}

static inline int classify13(const QChar *s) {
  if (s[0].unicode() == 'n') {
    if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 's') {
        if (s[3].unicode() == 'u') {
          if (s[4].unicode() == 'b') {
            if (s[5].unicode() == 'g') {
              if (s[6].unicode() == 'r') {
                if (s[7].unicode() == 'o') {
                  if (s[8].unicode() == 'u') {
                    if (s[9].unicode() == 'p') {
                      if (s[10].unicode() == 'i') {
                        if (s[11].unicode() == 'n') {
                          if (s[12].unicode() == 'g') {
                            return T_DOXY_NOSUBGROUPING;
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 's') {
    if (s[1].unicode() == 'u') {
      if (s[2].unicode() == 'b') {
        if (s[3].unicode() == 's') {
          if (s[4].unicode() == 'u') {
            if (s[5].unicode() == 'b') {
              if (s[6].unicode() == 's') {
                if (s[7].unicode() == 'e') {
                  if (s[8].unicode() == 'c') {
                    if (s[9].unicode() == 't') {
                      if (s[10].unicode() == 'i') {
                        if (s[11].unicode() == 'o') {
                          if (s[12].unicode() == 'n') {
                            return T_DOXY_SUBSUBSECTION;
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return T_DOXY_IDENTIFIER;
}

static inline int classify15(const QChar *s) {
  if (s[0].unicode() == 'h') {
    if (s[1].unicode() == 'i') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'i') {
            if (s[5].unicode() == 'n') {
              if (s[6].unicode() == 'i') {
                if (s[7].unicode() == 't') {
                  if (s[8].unicode() == 'i') {
                    if (s[9].unicode() == 'a') {
                      if (s[10].unicode() == 'l') {
                        if (s[11].unicode() == 'i') {
                          if (s[12].unicode() == 'z') {
                            if (s[13].unicode() == 'e') {
                              if (s[14].unicode() == 'r') {
                                return T_DOXY_HIDEINITIALIZER;
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 's') {
    if (s[1].unicode() == 'h') {
      if (s[2].unicode() == 'o') {
        if (s[3].unicode() == 'w') {
          if (s[4].unicode() == 'i') {
            if (s[5].unicode() == 'n') {
              if (s[6].unicode() == 'i') {
                if (s[7].unicode() == 't') {
                  if (s[8].unicode() == 'i') {
                    if (s[9].unicode() == 'a') {
                      if (s[10].unicode() == 'l') {
                        if (s[11].unicode() == 'i') {
                          if (s[12].unicode() == 'z') {
                            if (s[13].unicode() == 'e') {
                              if (s[14].unicode() == 'r') {
                                return T_DOXY_SHOWINITIALIZER;
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return T_DOXY_IDENTIFIER;
}

const char *CppTools::doxygenTagSpell(int index)
{ return doxy_token_spell[index]; }

int CppTools::classifyDoxygenTag(const QChar *s, int n) {
  switch (n) {
    case 1: return classify1(s);
    case 2: return classify2(s);
    case 3: return classify3(s);
    case 4: return classify4(s);
    case 5: return classify5(s);
    case 6: return classify6(s);
    case 7: return classify7(s);
    case 8: return classify8(s);
    case 9: return classify9(s);
    case 10: return classify10(s);
    case 11: return classify11(s);
    case 12: return classify12(s);
    case 13: return classify13(s);
    case 15: return classify15(s);
    default: return T_DOXY_IDENTIFIER;
  } // switch
}

