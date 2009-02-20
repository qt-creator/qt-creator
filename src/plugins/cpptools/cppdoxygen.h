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

#include "cpptools_global.h"

namespace CppTools {

enum DoxygenReservedWord {
  T_DOXY_IDENTIFIER = 0,

  T_DOXY_ARG,
  T_DOXY_ATTENTION,
  T_DOXY_AUTHOR,
  T_DOXY_CALLGRAPH,
  T_DOXY_CODE,
  T_DOXY_DOT,
  T_DOXY_ELSE,
  T_DOXY_ENDCODE,
  T_DOXY_ENDCOND,
  T_DOXY_ENDDOT,
  T_DOXY_ENDHTMLONLY,
  T_DOXY_ENDIF,
  T_DOXY_ENDLATEXONLY,
  T_DOXY_ENDLINK,
  T_DOXY_ENDMANONLY,
  T_DOXY_ENDVERBATIM,
  T_DOXY_ENDXMLONLY,
  T_DOXY_HIDEINITIALIZER,
  T_DOXY_HTMLONLY,
  T_DOXY_INTERFACE,
  T_DOXY_INTERNAL,
  T_DOXY_INVARIANT,
  T_DOXY_LATEXONLY,
  T_DOXY_LI,
  T_DOXY_MANONLY,
  T_DOXY_N,
  T_DOXY_NOSUBGROUPING,
  T_DOXY_NOTE,
  T_DOXY_ONLY,
  T_DOXY_POST,
  T_DOXY_PRE,
  T_DOXY_REMARKS,
  T_DOXY_RETURN,
  T_DOXY_RETURNS,
  T_DOXY_SA,
  T_DOXY_SEE,
  T_DOXY_SHOWINITIALIZER,
  T_DOXY_SINCE,
  T_DOXY_TEST,
  T_DOXY_TODO,
  T_DOXY_VERBATIM,
  T_DOXY_WARNING,
  T_DOXY_XMLONLY,

  T_DOXY_A,
  T_DOXY_ADDTOGROUP,
  T_DOXY_ANCHOR,
  T_DOXY_B,
  T_DOXY_C,
  T_DOXY_CLASS,
  T_DOXY_COND,
  T_DOXY_COPYDOC,
  T_DOXY_DEF,
  T_DOXY_DONTINCLUDE,
  T_DOXY_DOTFILE,
  T_DOXY_E,
  T_DOXY_ELSEIF,
  T_DOXY_EM,
  T_DOXY_ENUM,
  T_DOXY_EXAMPLE,
  T_DOXY_EXCEPTION,
  T_DOXY_EXCEPTIONS,
  T_DOXY_FILE,
  T_DOXY_HTMLINCLUDE,
  T_DOXY_IF,
  T_DOXY_IFNOT,
  T_DOXY_INCLUDE,
  T_DOXY_LINK,
  T_DOXY_NAMESPACE,
  T_DOXY_P,
  T_DOXY_PACKAGE,
  T_DOXY_REF,
  T_DOXY_RELATES,
  T_DOXY_RELATESALSO,
  T_DOXY_RETVAL,
  T_DOXY_THROW,
  T_DOXY_THROWS,
  T_DOXY_VERBINCLUDE,
  T_DOXY_VERSION,
  T_DOXY_XREFITEM,

  T_DOXY_PARAM,

  T_DOXY_IMAGE,

  T_DOXY_DEFGROUP,
  T_DOXY_PAGE,
  T_DOXY_PARAGRAPH,
  T_DOXY_SECTION,
  T_DOXY_STRUCT,
  T_DOXY_SUBSECTION,
  T_DOXY_SUBSUBSECTION,
  T_DOXY_UNION,
  T_DOXY_WEAKGROUP,

  T_DOXY_ADDINDEX,
  T_DOXY_BRIEF,
  T_DOXY_BUG,
  T_DOXY_DATE,
  T_DOXY_DEPRECATED,
  T_DOXY_FN,
  T_DOXY_INGROUP,
  T_DOXY_LINE,
  T_DOXY_MAINPAGE,
  T_DOXY_NAME,
  T_DOXY_OVERLOAD,
  T_DOXY_PAR,
  T_DOXY_SHORT,
  T_DOXY_SKIP,
  T_DOXY_SKIPLINE,
  T_DOXY_TYPEDEF,
  T_DOXY_UNTIL,
  T_DOXY_VAR,

  T_DOXY_LAST_TAG

};

CPPTOOLS_EXPORT int classifyDoxygenTag(const QChar *s, int n);
CPPTOOLS_EXPORT const char *doxygenTagSpell(int index);

} // namespace CppEditor::Internal

