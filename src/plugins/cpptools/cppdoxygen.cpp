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

#include "cppdoxygen.h"

#include <QString>

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
    "var",
    "abstract",
    "badcode",
    "basename",
    "bold",
    "caption",
    "chapter",
    "codeline",
    "dots",
    "endabstract",
    "endchapter",
    "endfootnote",
    "endlegalese",
    "endlist",
    "endomit",
    "endpart",
    "endquotation",
    "endraw",
    "endsection1",
    "endsection2",
    "endsection3",
    "endsection4",
    "endsidebar",
    "endtable",
    "expire",
    "footnote",
    "generatelist",
    "granularity",
    "header",
    "i",
    "index",
    "inlineimage",
    "keyword",
    "l",
    "legalese",
    "list",
    "meta",
    "newcode",
    "o",
    "oldcode",
    "omit",
    "omitvalue",
    "part",
    "printline",
    "printto",
    "printuntil",
    "quotation",
    "quotefile",
    "quotefromfile",
    "quotefunction",
    "raw",
    "row",
    "section1",
    "section2",
    "section3",
    "section4",
    "sidebar",
    "skipto",
    "skipuntil",
    "snippet",
    "sub",
    "sup",
    "table",
    "tableofcontents",
    "target",
    "tt",
    "underline",
    "unicode",
    "value",
    "contentspage",
    "externalpage",
    "group",
    "headerfile",
    "indexpage",
    "inheaderfile",
    "macro",
    "module",
    "nextpage",
    "previouspage",
    "property",
    "reimp",
    "service",
    "startpage",
    "variable",
    "compat",
    "inmodule",
    "mainclass",
    "nonreentrant",
    "obsolete",
    "preliminary",
    "inpublicgroup",
    "reentrant",
    "subtitle",
    "threadsafe",
    "title",
    "corelib",
    "uitools",
    "gui",
    "network",
    "opengl",
    "qt3support",
    "svg",
    "sql",
    "qtestlib",
    "webkit",
    "xml"
};

const char *CppTools::doxygenTagSpell(int index)
{ return doxy_token_spell[index]; }

static inline int classify1(const QChar *s) {
  if (s[0].unicode() == 'a')
    return T_DOXY_A;
  else if (s[0].unicode() == 'b')
    return T_DOXY_B;
  else if (s[0].unicode() == 'c')
    return T_DOXY_C;
  else if (s[0].unicode() == 'e')
    return T_DOXY_E;
  else if (s[0].unicode() == 'i')
    return T_DOXY_I;
  else if (s[0].unicode() == 'l')
    return T_DOXY_L;
  else if (s[0].unicode() == 'n')
    return T_DOXY_N;
  else if (s[0].unicode() == 'o')
    return T_DOXY_O;
  else if (s[0].unicode() == 'p')
    return T_DOXY_P;
  return T_DOXY_IDENTIFIER;
}

static inline int classify2(const QChar *s) {
  if (s[0].unicode() == 'e') {
    if (s[1].unicode() == 'm')
      return T_DOXY_EM;
  }
  else if (s[0].unicode() == 'f') {
    if (s[1].unicode() == 'n')
      return T_DOXY_FN;
  }
  else if (s[0].unicode() == 'i') {
    if (s[1].unicode() == 'f')
      return T_DOXY_IF;
  }
  else if (s[0].unicode() == 'l') {
    if (s[1].unicode() == 'i')
      return T_DOXY_LI;
  }
  else if (s[0].unicode() == 's') {
    if (s[1].unicode() == 'a')
      return T_DOXY_SA;
  }
  else if (s[0].unicode() == 't') {
    if (s[1].unicode() == 't')
      return T_DOXY_TT;
  }
  return T_DOXY_IDENTIFIER;
}

static inline int classify3(const QChar *s) {
  if (s[0].unicode() == 'a') {
    if (s[1].unicode() == 'r') {
      if (s[2].unicode() == 'g')
        return T_DOXY_ARG;
    }
  }
  else if (s[0].unicode() == 'b') {
    if (s[1].unicode() == 'u') {
      if (s[2].unicode() == 'g')
        return T_DOXY_BUG;
    }
  }
  else if (s[0].unicode() == 'd') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'f')
        return T_DOXY_DEF;
    }
    else if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 't')
        return T_DOXY_DOT;
    }
  }
  else if (s[0].unicode() == 'g') {
    if (s[1].unicode() == 'u') {
      if (s[2].unicode() == 'i')
        return T_DOXY_GUI;
    }
  }
  else if (s[0].unicode() == 'p') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'r')
        return T_DOXY_PAR;
    }
    else if (s[1].unicode() == 'r') {
      if (s[2].unicode() == 'e')
        return T_DOXY_PRE;
    }
  }
  else if (s[0].unicode() == 'r') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'w')
        return T_DOXY_RAW;
    }
    else if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'f')
        return T_DOXY_REF;
    }
    else if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 'w')
        return T_DOXY_ROW;
    }
  }
  else if (s[0].unicode() == 's') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'e')
        return T_DOXY_SEE;
    }
    else if (s[1].unicode() == 'q') {
      if (s[2].unicode() == 'l')
        return T_DOXY_SQL;
    }
    else if (s[1].unicode() == 'u') {
      if (s[2].unicode() == 'b')
        return T_DOXY_SUB;
      else if (s[2].unicode() == 'p')
        return T_DOXY_SUP;
    }
    else if (s[1].unicode() == 'v') {
      if (s[2].unicode() == 'g')
        return T_DOXY_SVG;
    }
  }
  else if (s[0].unicode() == 'v') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'r')
        return T_DOXY_VAR;
    }
  }
  else if (s[0].unicode() == 'x') {
    if (s[1].unicode() == 'm') {
      if (s[2].unicode() == 'l')
        return T_DOXY_XML;
    }
  }
  return T_DOXY_IDENTIFIER;
}

static inline int classify4(const QChar *s) {
  if (s[0].unicode() == 'b') {
    if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 'l') {
        if (s[3].unicode() == 'd')
          return T_DOXY_BOLD;
      }
    }
  }
  else if (s[0].unicode() == 'c') {
    if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'e')
          return T_DOXY_CODE;
      }
      else if (s[2].unicode() == 'n') {
        if (s[3].unicode() == 'd')
          return T_DOXY_COND;
      }
    }
  }
  else if (s[0].unicode() == 'd') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'e')
          return T_DOXY_DATE;
      }
    }
    else if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 's')
          return T_DOXY_DOTS;
      }
    }
  }
  else if (s[0].unicode() == 'e') {
    if (s[1].unicode() == 'l') {
      if (s[2].unicode() == 's') {
        if (s[3].unicode() == 'e')
          return T_DOXY_ELSE;
      }
    }
    else if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'u') {
        if (s[3].unicode() == 'm')
          return T_DOXY_ENUM;
      }
    }
  }
  else if (s[0].unicode() == 'f') {
    if (s[1].unicode() == 'i') {
      if (s[2].unicode() == 'l') {
        if (s[3].unicode() == 'e')
          return T_DOXY_FILE;
      }
    }
  }
  else if (s[0].unicode() == 'l') {
    if (s[1].unicode() == 'i') {
      if (s[2].unicode() == 'n') {
        if (s[3].unicode() == 'e')
          return T_DOXY_LINE;
        else if (s[3].unicode() == 'k')
          return T_DOXY_LINK;
      }
      else if (s[2].unicode() == 's') {
        if (s[3].unicode() == 't')
          return T_DOXY_LIST;
      }
    }
  }
  else if (s[0].unicode() == 'm') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'a')
          return T_DOXY_META;
      }
    }
  }
  else if (s[0].unicode() == 'n') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'm') {
        if (s[3].unicode() == 'e')
          return T_DOXY_NAME;
      }
    }
    else if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'e')
          return T_DOXY_NOTE;
      }
    }
  }
  else if (s[0].unicode() == 'o') {
    if (s[1].unicode() == 'm') {
      if (s[2].unicode() == 'i') {
        if (s[3].unicode() == 't')
          return T_DOXY_OMIT;
      }
    }
    else if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'l') {
        if (s[3].unicode() == 'y')
          return T_DOXY_ONLY;
      }
    }
  }
  else if (s[0].unicode() == 'p') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'g') {
        if (s[3].unicode() == 'e')
          return T_DOXY_PAGE;
      }
      else if (s[2].unicode() == 'r') {
        if (s[3].unicode() == 't')
          return T_DOXY_PART;
      }
    }
    else if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 's') {
        if (s[3].unicode() == 't')
          return T_DOXY_POST;
      }
    }
  }
  else if (s[0].unicode() == 's') {
    if (s[1].unicode() == 'k') {
      if (s[2].unicode() == 'i') {
        if (s[3].unicode() == 'p')
          return T_DOXY_SKIP;
      }
    }
  }
  else if (s[0].unicode() == 't') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 's') {
        if (s[3].unicode() == 't')
          return T_DOXY_TEST;
      }
    }
    else if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'o')
          return T_DOXY_TODO;
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
          if (s[4].unicode() == 'f')
            return T_DOXY_BRIEF;
        }
      }
    }
  }
  else if (s[0].unicode() == 'c') {
    if (s[1].unicode() == 'l') {
      if (s[2].unicode() == 'a') {
        if (s[3].unicode() == 's') {
          if (s[4].unicode() == 's')
            return T_DOXY_CLASS;
        }
      }
    }
  }
  else if (s[0].unicode() == 'e') {
    if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'i') {
          if (s[4].unicode() == 'f')
            return T_DOXY_ENDIF;
        }
      }
    }
  }
  else if (s[0].unicode() == 'g') {
    if (s[1].unicode() == 'r') {
      if (s[2].unicode() == 'o') {
        if (s[3].unicode() == 'u') {
          if (s[4].unicode() == 'p')
            return T_DOXY_GROUP;
        }
      }
    }
  }
  else if (s[0].unicode() == 'i') {
    if (s[1].unicode() == 'f') {
      if (s[2].unicode() == 'n') {
        if (s[3].unicode() == 'o') {
          if (s[4].unicode() == 't')
            return T_DOXY_IFNOT;
        }
      }
    }
    else if (s[1].unicode() == 'm') {
      if (s[2].unicode() == 'a') {
        if (s[3].unicode() == 'g') {
          if (s[4].unicode() == 'e')
            return T_DOXY_IMAGE;
        }
      }
    }
    else if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'x')
            return T_DOXY_INDEX;
        }
      }
    }
  }
  else if (s[0].unicode() == 'm') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'c') {
        if (s[3].unicode() == 'r') {
          if (s[4].unicode() == 'o')
            return T_DOXY_MACRO;
        }
      }
    }
  }
  else if (s[0].unicode() == 'p') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'r') {
        if (s[3].unicode() == 'a') {
          if (s[4].unicode() == 'm')
            return T_DOXY_PARAM;
        }
      }
    }
  }
  else if (s[0].unicode() == 'r') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'i') {
        if (s[3].unicode() == 'm') {
          if (s[4].unicode() == 'p')
            return T_DOXY_REIMP;
        }
      }
    }
  }
  else if (s[0].unicode() == 's') {
    if (s[1].unicode() == 'h') {
      if (s[2].unicode() == 'o') {
        if (s[3].unicode() == 'r') {
          if (s[4].unicode() == 't')
            return T_DOXY_SHORT;
        }
      }
    }
    else if (s[1].unicode() == 'i') {
      if (s[2].unicode() == 'n') {
        if (s[3].unicode() == 'c') {
          if (s[4].unicode() == 'e')
            return T_DOXY_SINCE;
        }
      }
    }
  }
  else if (s[0].unicode() == 't') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'b') {
        if (s[3].unicode() == 'l') {
          if (s[4].unicode() == 'e')
            return T_DOXY_TABLE;
        }
      }
    }
    else if (s[1].unicode() == 'h') {
      if (s[2].unicode() == 'r') {
        if (s[3].unicode() == 'o') {
          if (s[4].unicode() == 'w')
            return T_DOXY_THROW;
        }
      }
    }
    else if (s[1].unicode() == 'i') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'l') {
          if (s[4].unicode() == 'e')
            return T_DOXY_TITLE;
        }
      }
    }
  }
  else if (s[0].unicode() == 'u') {
    if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'i') {
        if (s[3].unicode() == 'o') {
          if (s[4].unicode() == 'n')
            return T_DOXY_UNION;
        }
      }
      else if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'i') {
          if (s[4].unicode() == 'l')
            return T_DOXY_UNTIL;
        }
      }
    }
  }
  else if (s[0].unicode() == 'v') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'l') {
        if (s[3].unicode() == 'u') {
          if (s[4].unicode() == 'e')
            return T_DOXY_VALUE;
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
            if (s[5].unicode() == 'r')
              return T_DOXY_ANCHOR;
          }
        }
      }
    }
    else if (s[1].unicode() == 'u') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'h') {
          if (s[4].unicode() == 'o') {
            if (s[5].unicode() == 'r')
              return T_DOXY_AUTHOR;
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'c') {
    if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 'm') {
        if (s[3].unicode() == 'p') {
          if (s[4].unicode() == 'a') {
            if (s[5].unicode() == 't')
              return T_DOXY_COMPAT;
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
            if (s[5].unicode() == 'f')
              return T_DOXY_ELSEIF;
          }
        }
      }
    }
    else if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'd') {
          if (s[4].unicode() == 'o') {
            if (s[5].unicode() == 't')
              return T_DOXY_ENDDOT;
          }
        }
        else if (s[3].unicode() == 'r') {
          if (s[4].unicode() == 'a') {
            if (s[5].unicode() == 'w')
              return T_DOXY_ENDRAW;
          }
        }
      }
    }
    else if (s[1].unicode() == 'x') {
      if (s[2].unicode() == 'p') {
        if (s[3].unicode() == 'i') {
          if (s[4].unicode() == 'r') {
            if (s[5].unicode() == 'e')
              return T_DOXY_EXPIRE;
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'h') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'a') {
        if (s[3].unicode() == 'd') {
          if (s[4].unicode() == 'e') {
            if (s[5].unicode() == 'r')
              return T_DOXY_HEADER;
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'm') {
    if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'u') {
          if (s[4].unicode() == 'l') {
            if (s[5].unicode() == 'e')
              return T_DOXY_MODULE;
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'o') {
    if (s[1].unicode() == 'p') {
      if (s[2].unicode() == 'e') {
        if (s[3].unicode() == 'n') {
          if (s[4].unicode() == 'g') {
            if (s[5].unicode() == 'l')
              return T_DOXY_OPENGL;
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
            if (s[5].unicode() == 'n')
              return T_DOXY_RETURN;
          }
        }
        else if (s[3].unicode() == 'v') {
          if (s[4].unicode() == 'a') {
            if (s[5].unicode() == 'l')
              return T_DOXY_RETVAL;
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 's') {
    if (s[1].unicode() == 'k') {
      if (s[2].unicode() == 'i') {
        if (s[3].unicode() == 'p') {
          if (s[4].unicode() == 't') {
            if (s[5].unicode() == 'o')
              return T_DOXY_SKIPTO;
          }
        }
      }
    }
    else if (s[1].unicode() == 't') {
      if (s[2].unicode() == 'r') {
        if (s[3].unicode() == 'u') {
          if (s[4].unicode() == 'c') {
            if (s[5].unicode() == 't')
              return T_DOXY_STRUCT;
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 't') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'r') {
        if (s[3].unicode() == 'g') {
          if (s[4].unicode() == 'e') {
            if (s[5].unicode() == 't')
              return T_DOXY_TARGET;
          }
        }
      }
    }
    else if (s[1].unicode() == 'h') {
      if (s[2].unicode() == 'r') {
        if (s[3].unicode() == 'o') {
          if (s[4].unicode() == 'w') {
            if (s[5].unicode() == 's')
              return T_DOXY_THROWS;
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'w') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'b') {
        if (s[3].unicode() == 'k') {
          if (s[4].unicode() == 'i') {
            if (s[5].unicode() == 't')
              return T_DOXY_WEBKIT;
          }
        }
      }
    }
  }
  return T_DOXY_IDENTIFIER;
}

static inline int classify7(const QChar *s) {
  if (s[0].unicode() == 'b') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'c') {
          if (s[4].unicode() == 'o') {
            if (s[5].unicode() == 'd') {
              if (s[6].unicode() == 'e')
                return T_DOXY_BADCODE;
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'c') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'p') {
        if (s[3].unicode() == 't') {
          if (s[4].unicode() == 'i') {
            if (s[5].unicode() == 'o') {
              if (s[6].unicode() == 'n')
                return T_DOXY_CAPTION;
            }
          }
        }
      }
    }
    else if (s[1].unicode() == 'h') {
      if (s[2].unicode() == 'a') {
        if (s[3].unicode() == 'p') {
          if (s[4].unicode() == 't') {
            if (s[5].unicode() == 'e') {
              if (s[6].unicode() == 'r')
                return T_DOXY_CHAPTER;
            }
          }
        }
      }
    }
    else if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 'p') {
        if (s[3].unicode() == 'y') {
          if (s[4].unicode() == 'd') {
            if (s[5].unicode() == 'o') {
              if (s[6].unicode() == 'c')
                return T_DOXY_COPYDOC;
            }
          }
        }
      }
      else if (s[2].unicode() == 'r') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'l') {
            if (s[5].unicode() == 'i') {
              if (s[6].unicode() == 'b')
                return T_DOXY_CORELIB;
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
              if (s[6].unicode() == 'e')
                return T_DOXY_DOTFILE;
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
              if (s[6].unicode() == 'e')
                return T_DOXY_ENDCODE;
            }
            else if (s[5].unicode() == 'n') {
              if (s[6].unicode() == 'd')
                return T_DOXY_ENDCOND;
            }
          }
        }
        else if (s[3].unicode() == 'l') {
          if (s[4].unicode() == 'i') {
            if (s[5].unicode() == 'n') {
              if (s[6].unicode() == 'k')
                return T_DOXY_ENDLINK;
            }
            else if (s[5].unicode() == 's') {
              if (s[6].unicode() == 't')
                return T_DOXY_ENDLIST;
            }
          }
        }
        else if (s[3].unicode() == 'o') {
          if (s[4].unicode() == 'm') {
            if (s[5].unicode() == 'i') {
              if (s[6].unicode() == 't')
                return T_DOXY_ENDOMIT;
            }
          }
        }
        else if (s[3].unicode() == 'p') {
          if (s[4].unicode() == 'a') {
            if (s[5].unicode() == 'r') {
              if (s[6].unicode() == 't')
                return T_DOXY_ENDPART;
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
              if (s[6].unicode() == 'e')
                return T_DOXY_EXAMPLE;
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
              if (s[6].unicode() == 'e')
                return T_DOXY_INCLUDE;
            }
          }
        }
      }
      else if (s[2].unicode() == 'g') {
        if (s[3].unicode() == 'r') {
          if (s[4].unicode() == 'o') {
            if (s[5].unicode() == 'u') {
              if (s[6].unicode() == 'p')
                return T_DOXY_INGROUP;
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'k') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'y') {
        if (s[3].unicode() == 'w') {
          if (s[4].unicode() == 'o') {
            if (s[5].unicode() == 'r') {
              if (s[6].unicode() == 'd')
                return T_DOXY_KEYWORD;
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
              if (s[6].unicode() == 'y')
                return T_DOXY_MANONLY;
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'n') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'w') {
          if (s[4].unicode() == 'o') {
            if (s[5].unicode() == 'r') {
              if (s[6].unicode() == 'k')
                return T_DOXY_NETWORK;
            }
          }
        }
      }
      else if (s[2].unicode() == 'w') {
        if (s[3].unicode() == 'c') {
          if (s[4].unicode() == 'o') {
            if (s[5].unicode() == 'd') {
              if (s[6].unicode() == 'e')
                return T_DOXY_NEWCODE;
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'o') {
    if (s[1].unicode() == 'l') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'c') {
          if (s[4].unicode() == 'o') {
            if (s[5].unicode() == 'd') {
              if (s[6].unicode() == 'e')
                return T_DOXY_OLDCODE;
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
              if (s[6].unicode() == 'e')
                return T_DOXY_PACKAGE;
            }
          }
        }
      }
    }
    else if (s[1].unicode() == 'r') {
      if (s[2].unicode() == 'i') {
        if (s[3].unicode() == 'n') {
          if (s[4].unicode() == 't') {
            if (s[5].unicode() == 't') {
              if (s[6].unicode() == 'o')
                return T_DOXY_PRINTTO;
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
              if (s[6].unicode() == 's')
                return T_DOXY_RELATES;
            }
          }
        }
      }
      else if (s[2].unicode() == 'm') {
        if (s[3].unicode() == 'a') {
          if (s[4].unicode() == 'r') {
            if (s[5].unicode() == 'k') {
              if (s[6].unicode() == 's')
                return T_DOXY_REMARKS;
            }
          }
        }
      }
      else if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'u') {
          if (s[4].unicode() == 'r') {
            if (s[5].unicode() == 'n') {
              if (s[6].unicode() == 's')
                return T_DOXY_RETURNS;
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
              if (s[6].unicode() == 'n')
                return T_DOXY_SECTION;
            }
          }
        }
      }
      else if (s[2].unicode() == 'r') {
        if (s[3].unicode() == 'v') {
          if (s[4].unicode() == 'i') {
            if (s[5].unicode() == 'c') {
              if (s[6].unicode() == 'e')
                return T_DOXY_SERVICE;
            }
          }
        }
      }
    }
    else if (s[1].unicode() == 'i') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'b') {
            if (s[5].unicode() == 'a') {
              if (s[6].unicode() == 'r')
                return T_DOXY_SIDEBAR;
            }
          }
        }
      }
    }
    else if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'i') {
        if (s[3].unicode() == 'p') {
          if (s[4].unicode() == 'p') {
            if (s[5].unicode() == 'e') {
              if (s[6].unicode() == 't')
                return T_DOXY_SNIPPET;
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
              if (s[6].unicode() == 'f')
                return T_DOXY_TYPEDEF;
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'u') {
    if (s[1].unicode() == 'i') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'o') {
          if (s[4].unicode() == 'o') {
            if (s[5].unicode() == 'l') {
              if (s[6].unicode() == 's')
                return T_DOXY_UITOOLS;
            }
          }
        }
      }
    }
    else if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'i') {
        if (s[3].unicode() == 'c') {
          if (s[4].unicode() == 'o') {
            if (s[5].unicode() == 'd') {
              if (s[6].unicode() == 'e')
                return T_DOXY_UNICODE;
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
              if (s[6].unicode() == 'n')
                return T_DOXY_VERSION;
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
              if (s[6].unicode() == 'g')
                return T_DOXY_WARNING;
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
              if (s[6].unicode() == 'y')
                return T_DOXY_XMLONLY;
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
    if (s[1].unicode() == 'b') {
      if (s[2].unicode() == 's') {
        if (s[3].unicode() == 't') {
          if (s[4].unicode() == 'r') {
            if (s[5].unicode() == 'a') {
              if (s[6].unicode() == 'c') {
                if (s[7].unicode() == 't')
                  return T_DOXY_ABSTRACT;
              }
            }
          }
        }
      }
    }
    else if (s[1].unicode() == 'd') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'i') {
          if (s[4].unicode() == 'n') {
            if (s[5].unicode() == 'd') {
              if (s[6].unicode() == 'e') {
                if (s[7].unicode() == 'x')
                  return T_DOXY_ADDINDEX;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'b') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 's') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'n') {
            if (s[5].unicode() == 'a') {
              if (s[6].unicode() == 'm') {
                if (s[7].unicode() == 'e')
                  return T_DOXY_BASENAME;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'c') {
    if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'l') {
            if (s[5].unicode() == 'i') {
              if (s[6].unicode() == 'n') {
                if (s[7].unicode() == 'e')
                  return T_DOXY_CODELINE;
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
                if (s[7].unicode() == 'p')
                  return T_DOXY_DEFGROUP;
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
        if (s[3].unicode() == 't') {
          if (s[4].unicode() == 'a') {
            if (s[5].unicode() == 'b') {
              if (s[6].unicode() == 'l') {
                if (s[7].unicode() == 'e')
                  return T_DOXY_ENDTABLE;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'f') {
    if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 'o') {
        if (s[3].unicode() == 't') {
          if (s[4].unicode() == 'n') {
            if (s[5].unicode() == 'o') {
              if (s[6].unicode() == 't') {
                if (s[7].unicode() == 'e')
                  return T_DOXY_FOOTNOTE;
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
                if (s[7].unicode() == 'y')
                  return T_DOXY_HTMLONLY;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'i') {
    if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'm') {
        if (s[3].unicode() == 'o') {
          if (s[4].unicode() == 'd') {
            if (s[5].unicode() == 'u') {
              if (s[6].unicode() == 'l') {
                if (s[7].unicode() == 'e')
                  return T_DOXY_INMODULE;
              }
            }
          }
        }
      }
      else if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'r') {
            if (s[5].unicode() == 'n') {
              if (s[6].unicode() == 'a') {
                if (s[7].unicode() == 'l')
                  return T_DOXY_INTERNAL;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'l') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'g') {
        if (s[3].unicode() == 'a') {
          if (s[4].unicode() == 'l') {
            if (s[5].unicode() == 'e') {
              if (s[6].unicode() == 's') {
                if (s[7].unicode() == 'e')
                  return T_DOXY_LEGALESE;
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
                if (s[7].unicode() == 'e')
                  return T_DOXY_MAINPAGE;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'n') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'x') {
        if (s[3].unicode() == 't') {
          if (s[4].unicode() == 'p') {
            if (s[5].unicode() == 'a') {
              if (s[6].unicode() == 'g') {
                if (s[7].unicode() == 'e')
                  return T_DOXY_NEXTPAGE;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'o') {
    if (s[1].unicode() == 'b') {
      if (s[2].unicode() == 's') {
        if (s[3].unicode() == 'o') {
          if (s[4].unicode() == 'l') {
            if (s[5].unicode() == 'e') {
              if (s[6].unicode() == 't') {
                if (s[7].unicode() == 'e')
                  return T_DOXY_OBSOLETE;
              }
            }
          }
        }
      }
    }
    else if (s[1].unicode() == 'v') {
      if (s[2].unicode() == 'e') {
        if (s[3].unicode() == 'r') {
          if (s[4].unicode() == 'l') {
            if (s[5].unicode() == 'o') {
              if (s[6].unicode() == 'a') {
                if (s[7].unicode() == 'd')
                  return T_DOXY_OVERLOAD;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'p') {
    if (s[1].unicode() == 'r') {
      if (s[2].unicode() == 'o') {
        if (s[3].unicode() == 'p') {
          if (s[4].unicode() == 'e') {
            if (s[5].unicode() == 'r') {
              if (s[6].unicode() == 't') {
                if (s[7].unicode() == 'y')
                  return T_DOXY_PROPERTY;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'q') {
    if (s[1].unicode() == 't') {
      if (s[2].unicode() == 'e') {
        if (s[3].unicode() == 's') {
          if (s[4].unicode() == 't') {
            if (s[5].unicode() == 'l') {
              if (s[6].unicode() == 'i') {
                if (s[7].unicode() == 'b')
                  return T_DOXY_QTESTLIB;
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
                if (s[7].unicode() == '1')
                  return T_DOXY_SECTION1;
                else if (s[7].unicode() == '2')
                  return T_DOXY_SECTION2;
                else if (s[7].unicode() == '3')
                  return T_DOXY_SECTION3;
                else if (s[7].unicode() == '4')
                  return T_DOXY_SECTION4;
              }
            }
          }
        }
      }
    }
    else if (s[1].unicode() == 'k') {
      if (s[2].unicode() == 'i') {
        if (s[3].unicode() == 'p') {
          if (s[4].unicode() == 'l') {
            if (s[5].unicode() == 'i') {
              if (s[6].unicode() == 'n') {
                if (s[7].unicode() == 'e')
                  return T_DOXY_SKIPLINE;
              }
            }
          }
        }
      }
    }
    else if (s[1].unicode() == 'u') {
      if (s[2].unicode() == 'b') {
        if (s[3].unicode() == 't') {
          if (s[4].unicode() == 'i') {
            if (s[5].unicode() == 't') {
              if (s[6].unicode() == 'l') {
                if (s[7].unicode() == 'e')
                  return T_DOXY_SUBTITLE;
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'v') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'r') {
        if (s[3].unicode() == 'i') {
          if (s[4].unicode() == 'a') {
            if (s[5].unicode() == 'b') {
              if (s[6].unicode() == 'l') {
                if (s[7].unicode() == 'e')
                  return T_DOXY_VARIABLE;
              }
            }
          }
        }
      }
    }
    else if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'r') {
        if (s[3].unicode() == 'b') {
          if (s[4].unicode() == 'a') {
            if (s[5].unicode() == 't') {
              if (s[6].unicode() == 'i') {
                if (s[7].unicode() == 'm')
                  return T_DOXY_VERBATIM;
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
                if (s[7].unicode() == 'm')
                  return T_DOXY_XREFITEM;
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
                  if (s[8].unicode() == 'n')
                    return T_DOXY_ATTENTION;
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
                  if (s[8].unicode() == 'h')
                    return T_DOXY_CALLGRAPH;
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
                  if (s[8].unicode() == 'n')
                    return T_DOXY_EXCEPTION;
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
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'x') {
            if (s[5].unicode() == 'p') {
              if (s[6].unicode() == 'a') {
                if (s[7].unicode() == 'g') {
                  if (s[8].unicode() == 'e')
                    return T_DOXY_INDEXPAGE;
                }
              }
            }
          }
        }
      }
      else if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'r') {
            if (s[5].unicode() == 'f') {
              if (s[6].unicode() == 'a') {
                if (s[7].unicode() == 'c') {
                  if (s[8].unicode() == 'e')
                    return T_DOXY_INTERFACE;
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
                  if (s[8].unicode() == 't')
                    return T_DOXY_INVARIANT;
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
                  if (s[8].unicode() == 'y')
                    return T_DOXY_LATEXONLY;
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
          if (s[4].unicode() == 'c') {
            if (s[5].unicode() == 'l') {
              if (s[6].unicode() == 'a') {
                if (s[7].unicode() == 's') {
                  if (s[8].unicode() == 's')
                    return T_DOXY_MAINCLASS;
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
                  if (s[8].unicode() == 'e')
                    return T_DOXY_NAMESPACE;
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'o') {
    if (s[1].unicode() == 'm') {
      if (s[2].unicode() == 'i') {
        if (s[3].unicode() == 't') {
          if (s[4].unicode() == 'v') {
            if (s[5].unicode() == 'a') {
              if (s[6].unicode() == 'l') {
                if (s[7].unicode() == 'u') {
                  if (s[8].unicode() == 'e')
                    return T_DOXY_OMITVALUE;
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
                  if (s[8].unicode() == 'h')
                    return T_DOXY_PARAGRAPH;
                }
              }
            }
          }
        }
      }
    }
    else if (s[1].unicode() == 'r') {
      if (s[2].unicode() == 'i') {
        if (s[3].unicode() == 'n') {
          if (s[4].unicode() == 't') {
            if (s[5].unicode() == 'l') {
              if (s[6].unicode() == 'i') {
                if (s[7].unicode() == 'n') {
                  if (s[8].unicode() == 'e')
                    return T_DOXY_PRINTLINE;
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'q') {
    if (s[1].unicode() == 'u') {
      if (s[2].unicode() == 'o') {
        if (s[3].unicode() == 't') {
          if (s[4].unicode() == 'a') {
            if (s[5].unicode() == 't') {
              if (s[6].unicode() == 'i') {
                if (s[7].unicode() == 'o') {
                  if (s[8].unicode() == 'n')
                    return T_DOXY_QUOTATION;
                }
              }
            }
          }
          else if (s[4].unicode() == 'e') {
            if (s[5].unicode() == 'f') {
              if (s[6].unicode() == 'i') {
                if (s[7].unicode() == 'l') {
                  if (s[8].unicode() == 'e')
                    return T_DOXY_QUOTEFILE;
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
      if (s[2].unicode() == 'e') {
        if (s[3].unicode() == 'n') {
          if (s[4].unicode() == 't') {
            if (s[5].unicode() == 'r') {
              if (s[6].unicode() == 'a') {
                if (s[7].unicode() == 'n') {
                  if (s[8].unicode() == 't')
                    return T_DOXY_REENTRANT;
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
          if (s[4].unicode() == 'u') {
            if (s[5].unicode() == 'n') {
              if (s[6].unicode() == 't') {
                if (s[7].unicode() == 'i') {
                  if (s[8].unicode() == 'l')
                    return T_DOXY_SKIPUNTIL;
                }
              }
            }
          }
        }
      }
    }
    else if (s[1].unicode() == 't') {
      if (s[2].unicode() == 'a') {
        if (s[3].unicode() == 'r') {
          if (s[4].unicode() == 't') {
            if (s[5].unicode() == 'p') {
              if (s[6].unicode() == 'a') {
                if (s[7].unicode() == 'g') {
                  if (s[8].unicode() == 'e')
                    return T_DOXY_STARTPAGE;
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'u') {
    if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'r') {
            if (s[5].unicode() == 'l') {
              if (s[6].unicode() == 'i') {
                if (s[7].unicode() == 'n') {
                  if (s[8].unicode() == 'e')
                    return T_DOXY_UNDERLINE;
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
                  if (s[8].unicode() == 'p')
                    return T_DOXY_WEAKGROUP;
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
                    if (s[9].unicode() == 'p')
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
  else if (s[0].unicode() == 'd') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'p') {
        if (s[3].unicode() == 'r') {
          if (s[4].unicode() == 'e') {
            if (s[5].unicode() == 'c') {
              if (s[6].unicode() == 'a') {
                if (s[7].unicode() == 't') {
                  if (s[8].unicode() == 'e') {
                    if (s[9].unicode() == 'd')
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
  else if (s[0].unicode() == 'e') {
    if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'c') {
          if (s[4].unicode() == 'h') {
            if (s[5].unicode() == 'a') {
              if (s[6].unicode() == 'p') {
                if (s[7].unicode() == 't') {
                  if (s[8].unicode() == 'e') {
                    if (s[9].unicode() == 'r')
                      return T_DOXY_ENDCHAPTER;
                  }
                }
              }
            }
          }
        }
        else if (s[3].unicode() == 'm') {
          if (s[4].unicode() == 'a') {
            if (s[5].unicode() == 'n') {
              if (s[6].unicode() == 'o') {
                if (s[7].unicode() == 'n') {
                  if (s[8].unicode() == 'l') {
                    if (s[9].unicode() == 'y')
                      return T_DOXY_ENDMANONLY;
                  }
                }
              }
            }
          }
        }
        else if (s[3].unicode() == 's') {
          if (s[4].unicode() == 'i') {
            if (s[5].unicode() == 'd') {
              if (s[6].unicode() == 'e') {
                if (s[7].unicode() == 'b') {
                  if (s[8].unicode() == 'a') {
                    if (s[9].unicode() == 'r')
                      return T_DOXY_ENDSIDEBAR;
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
                    if (s[9].unicode() == 'y')
                      return T_DOXY_ENDXMLONLY;
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
                    if (s[9].unicode() == 's')
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
  else if (s[0].unicode() == 'h') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'a') {
        if (s[3].unicode() == 'd') {
          if (s[4].unicode() == 'e') {
            if (s[5].unicode() == 'r') {
              if (s[6].unicode() == 'f') {
                if (s[7].unicode() == 'i') {
                  if (s[8].unicode() == 'l') {
                    if (s[9].unicode() == 'e')
                      return T_DOXY_HEADERFILE;
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
    if (s[1].unicode() == 'r') {
      if (s[2].unicode() == 'i') {
        if (s[3].unicode() == 'n') {
          if (s[4].unicode() == 't') {
            if (s[5].unicode() == 'u') {
              if (s[6].unicode() == 'n') {
                if (s[7].unicode() == 't') {
                  if (s[8].unicode() == 'i') {
                    if (s[9].unicode() == 'l')
                      return T_DOXY_PRINTUNTIL;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0].unicode() == 'q') {
    if (s[1].unicode() == 't') {
      if (s[2].unicode() == '3') {
        if (s[3].unicode() == 's') {
          if (s[4].unicode() == 'u') {
            if (s[5].unicode() == 'p') {
              if (s[6].unicode() == 'p') {
                if (s[7].unicode() == 'o') {
                  if (s[8].unicode() == 'r') {
                    if (s[9].unicode() == 't')
                      return T_DOXY_QT3SUPPORT;
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
                    if (s[9].unicode() == 'n')
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
  else if (s[0].unicode() == 't') {
    if (s[1].unicode() == 'h') {
      if (s[2].unicode() == 'r') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'a') {
            if (s[5].unicode() == 'd') {
              if (s[6].unicode() == 's') {
                if (s[7].unicode() == 'a') {
                  if (s[8].unicode() == 'f') {
                    if (s[9].unicode() == 'e')
                      return T_DOXY_THREADSAFE;
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
                      if (s[10].unicode() == 'e')
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
  else if (s[0].unicode() == 'e') {
    if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'd') {
        if (s[3].unicode() == 'a') {
          if (s[4].unicode() == 'b') {
            if (s[5].unicode() == 's') {
              if (s[6].unicode() == 't') {
                if (s[7].unicode() == 'r') {
                  if (s[8].unicode() == 'a') {
                    if (s[9].unicode() == 'c') {
                      if (s[10].unicode() == 't')
                        return T_DOXY_ENDABSTRACT;
                    }
                  }
                }
              }
            }
          }
        }
        else if (s[3].unicode() == 'f') {
          if (s[4].unicode() == 'o') {
            if (s[5].unicode() == 'o') {
              if (s[6].unicode() == 't') {
                if (s[7].unicode() == 'n') {
                  if (s[8].unicode() == 'o') {
                    if (s[9].unicode() == 't') {
                      if (s[10].unicode() == 'e')
                        return T_DOXY_ENDFOOTNOTE;
                    }
                  }
                }
              }
            }
          }
        }
        else if (s[3].unicode() == 'h') {
          if (s[4].unicode() == 't') {
            if (s[5].unicode() == 'm') {
              if (s[6].unicode() == 'l') {
                if (s[7].unicode() == 'o') {
                  if (s[8].unicode() == 'n') {
                    if (s[9].unicode() == 'l') {
                      if (s[10].unicode() == 'y')
                        return T_DOXY_ENDHTMLONLY;
                    }
                  }
                }
              }
            }
          }
        }
        else if (s[3].unicode() == 'l') {
          if (s[4].unicode() == 'e') {
            if (s[5].unicode() == 'g') {
              if (s[6].unicode() == 'a') {
                if (s[7].unicode() == 'l') {
                  if (s[8].unicode() == 'e') {
                    if (s[9].unicode() == 's') {
                      if (s[10].unicode() == 'e')
                        return T_DOXY_ENDLEGALESE;
                    }
                  }
                }
              }
            }
          }
        }
        else if (s[3].unicode() == 's') {
          if (s[4].unicode() == 'e') {
            if (s[5].unicode() == 'c') {
              if (s[6].unicode() == 't') {
                if (s[7].unicode() == 'i') {
                  if (s[8].unicode() == 'o') {
                    if (s[9].unicode() == 'n') {
                      if (s[10].unicode() == '1')
                        return T_DOXY_ENDSECTION1;
                      else if (s[10].unicode() == '2')
                        return T_DOXY_ENDSECTION2;
                      else if (s[10].unicode() == '3')
                        return T_DOXY_ENDSECTION3;
                      else if (s[10].unicode() == '4')
                        return T_DOXY_ENDSECTION4;
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
                      if (s[10].unicode() == 'm')
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
  else if (s[0].unicode() == 'g') {
    if (s[1].unicode() == 'r') {
      if (s[2].unicode() == 'a') {
        if (s[3].unicode() == 'n') {
          if (s[4].unicode() == 'u') {
            if (s[5].unicode() == 'l') {
              if (s[6].unicode() == 'a') {
                if (s[7].unicode() == 'r') {
                  if (s[8].unicode() == 'i') {
                    if (s[9].unicode() == 't') {
                      if (s[10].unicode() == 'y')
                        return T_DOXY_GRANULARITY;
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
                      if (s[10].unicode() == 'e')
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
  else if (s[0].unicode() == 'i') {
    if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'l') {
        if (s[3].unicode() == 'i') {
          if (s[4].unicode() == 'n') {
            if (s[5].unicode() == 'e') {
              if (s[6].unicode() == 'i') {
                if (s[7].unicode() == 'm') {
                  if (s[8].unicode() == 'a') {
                    if (s[9].unicode() == 'g') {
                      if (s[10].unicode() == 'e')
                        return T_DOXY_INLINEIMAGE;
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
  else if (s[0].unicode() == 'p') {
    if (s[1].unicode() == 'r') {
      if (s[2].unicode() == 'e') {
        if (s[3].unicode() == 'l') {
          if (s[4].unicode() == 'i') {
            if (s[5].unicode() == 'm') {
              if (s[6].unicode() == 'i') {
                if (s[7].unicode() == 'n') {
                  if (s[8].unicode() == 'a') {
                    if (s[9].unicode() == 'r') {
                      if (s[10].unicode() == 'y')
                        return T_DOXY_PRELIMINARY;
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
                      if (s[10].unicode() == 'o')
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
                      if (s[10].unicode() == 'e')
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
  return T_DOXY_IDENTIFIER;
}

static inline int classify12(const QChar *s) {
  if (s[0].unicode() == 'c') {
    if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 'n') {
        if (s[3].unicode() == 't') {
          if (s[4].unicode() == 'e') {
            if (s[5].unicode() == 'n') {
              if (s[6].unicode() == 't') {
                if (s[7].unicode() == 's') {
                  if (s[8].unicode() == 'p') {
                    if (s[9].unicode() == 'a') {
                      if (s[10].unicode() == 'g') {
                        if (s[11].unicode() == 'e')
                          return T_DOXY_CONTENTSPAGE;
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
        if (s[3].unicode() == 'l') {
          if (s[4].unicode() == 'a') {
            if (s[5].unicode() == 't') {
              if (s[6].unicode() == 'e') {
                if (s[7].unicode() == 'x') {
                  if (s[8].unicode() == 'o') {
                    if (s[9].unicode() == 'n') {
                      if (s[10].unicode() == 'l') {
                        if (s[11].unicode() == 'y')
                          return T_DOXY_ENDLATEXONLY;
                      }
                    }
                  }
                }
              }
            }
          }
        }
        else if (s[3].unicode() == 'q') {
          if (s[4].unicode() == 'u') {
            if (s[5].unicode() == 'o') {
              if (s[6].unicode() == 't') {
                if (s[7].unicode() == 'a') {
                  if (s[8].unicode() == 't') {
                    if (s[9].unicode() == 'i') {
                      if (s[10].unicode() == 'o') {
                        if (s[11].unicode() == 'n')
                          return T_DOXY_ENDQUOTATION;
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
    else if (s[1].unicode() == 'x') {
      if (s[2].unicode() == 't') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'r') {
            if (s[5].unicode() == 'n') {
              if (s[6].unicode() == 'a') {
                if (s[7].unicode() == 'l') {
                  if (s[8].unicode() == 'p') {
                    if (s[9].unicode() == 'a') {
                      if (s[10].unicode() == 'g') {
                        if (s[11].unicode() == 'e')
                          return T_DOXY_EXTERNALPAGE;
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
  else if (s[0].unicode() == 'g') {
    if (s[1].unicode() == 'e') {
      if (s[2].unicode() == 'n') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'r') {
            if (s[5].unicode() == 'a') {
              if (s[6].unicode() == 't') {
                if (s[7].unicode() == 'e') {
                  if (s[8].unicode() == 'l') {
                    if (s[9].unicode() == 'i') {
                      if (s[10].unicode() == 's') {
                        if (s[11].unicode() == 't')
                          return T_DOXY_GENERATELIST;
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
  else if (s[0].unicode() == 'i') {
    if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'h') {
        if (s[3].unicode() == 'e') {
          if (s[4].unicode() == 'a') {
            if (s[5].unicode() == 'd') {
              if (s[6].unicode() == 'e') {
                if (s[7].unicode() == 'r') {
                  if (s[8].unicode() == 'f') {
                    if (s[9].unicode() == 'i') {
                      if (s[10].unicode() == 'l') {
                        if (s[11].unicode() == 'e')
                          return T_DOXY_INHEADERFILE;
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
  else if (s[0].unicode() == 'n') {
    if (s[1].unicode() == 'o') {
      if (s[2].unicode() == 'n') {
        if (s[3].unicode() == 'r') {
          if (s[4].unicode() == 'e') {
            if (s[5].unicode() == 'e') {
              if (s[6].unicode() == 'n') {
                if (s[7].unicode() == 't') {
                  if (s[8].unicode() == 'r') {
                    if (s[9].unicode() == 'a') {
                      if (s[10].unicode() == 'n') {
                        if (s[11].unicode() == 't')
                          return T_DOXY_NONREENTRANT;
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
  else if (s[0].unicode() == 'p') {
    if (s[1].unicode() == 'r') {
      if (s[2].unicode() == 'e') {
        if (s[3].unicode() == 'v') {
          if (s[4].unicode() == 'i') {
            if (s[5].unicode() == 'o') {
              if (s[6].unicode() == 'u') {
                if (s[7].unicode() == 's') {
                  if (s[8].unicode() == 'p') {
                    if (s[9].unicode() == 'a') {
                      if (s[10].unicode() == 'g') {
                        if (s[11].unicode() == 'e')
                          return T_DOXY_PREVIOUSPAGE;
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
  if (s[0].unicode() == 'i') {
    if (s[1].unicode() == 'n') {
      if (s[2].unicode() == 'p') {
        if (s[3].unicode() == 'u') {
          if (s[4].unicode() == 'b') {
            if (s[5].unicode() == 'l') {
              if (s[6].unicode() == 'i') {
                if (s[7].unicode() == 'c') {
                  if (s[8].unicode() == 'g') {
                    if (s[9].unicode() == 'r') {
                      if (s[10].unicode() == 'o') {
                        if (s[11].unicode() == 'u') {
                          if (s[12].unicode() == 'p')
                            return T_DOXY_INPUBLICGROUP;
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
  else if (s[0].unicode() == 'n') {
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
                          if (s[12].unicode() == 'g')
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
  else if (s[0].unicode() == 'q') {
    if (s[1].unicode() == 'u') {
      if (s[2].unicode() == 'o') {
        if (s[3].unicode() == 't') {
          if (s[4].unicode() == 'e') {
            if (s[5].unicode() == 'f') {
              if (s[6].unicode() == 'r') {
                if (s[7].unicode() == 'o') {
                  if (s[8].unicode() == 'm') {
                    if (s[9].unicode() == 'f') {
                      if (s[10].unicode() == 'i') {
                        if (s[11].unicode() == 'l') {
                          if (s[12].unicode() == 'e')
                            return T_DOXY_QUOTEFROMFILE;
                        }
                      }
                    }
                  }
                }
              }
              else if (s[6].unicode() == 'u') {
                if (s[7].unicode() == 'n') {
                  if (s[8].unicode() == 'c') {
                    if (s[9].unicode() == 't') {
                      if (s[10].unicode() == 'i') {
                        if (s[11].unicode() == 'o') {
                          if (s[12].unicode() == 'n')
                            return T_DOXY_QUOTEFUNCTION;
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
                          if (s[12].unicode() == 'n')
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
                              if (s[14].unicode() == 'r')
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
                              if (s[14].unicode() == 'r')
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
  else if (s[0].unicode() == 't') {
    if (s[1].unicode() == 'a') {
      if (s[2].unicode() == 'b') {
        if (s[3].unicode() == 'l') {
          if (s[4].unicode() == 'e') {
            if (s[5].unicode() == 'o') {
              if (s[6].unicode() == 'f') {
                if (s[7].unicode() == 'c') {
                  if (s[8].unicode() == 'o') {
                    if (s[9].unicode() == 'n') {
                      if (s[10].unicode() == 't') {
                        if (s[11].unicode() == 'e') {
                          if (s[12].unicode() == 'n') {
                            if (s[13].unicode() == 't') {
                              if (s[14].unicode() == 's')
                                return T_DOXY_TABLEOFCONTENTS;
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

