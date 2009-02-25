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

#ifndef INDENTER_C
#define INDENTER_C

/*
  This file is a self-contained interactive indenter for C++ and Qt
  Script.

  The general problem of indenting a C++ program is ill posed. On the
  one hand, an indenter has to analyze programs written in a
  free-form formal language that is best described in terms of
  tokens, not characters, not lines. On the other hand, indentation
  applies to lines and white space characters matter, and otherwise
  the programs to indent are formally invalid in general, as they are
  being edited.

  The approach taken here works line by line. We receive a program
  consisting of N lines or more, and we want to compute the
  indentation appropriate for the Nth line. Lines beyond the Nth
  lines are of no concern to us, so for simplicity we pretend the
  program has exactly N lines and we call the Nth line the "bottom
  line". Typically, we have to indent the bottom line when it's still
  empty, so we concentrate our analysis on the N - 1 lines that
  precede.

  By inspecting the (N - 1)-th line, the (N - 2)-th line, ...
  backwards, we determine the kind of the bottom line and indent it
  accordingly.

    * The bottom line is a comment line. See
      bottomLineStartsInCComment() and
      indentWhenBottomLineStartsInCComment().
    * The bottom line is a continuation line. See isContinuationLine()
      and indentForContinuationLine().
    * The bottom line is a standalone line. See
      indentForStandaloneLine().

  Certain tokens that influence the indentation, notably braces, are
  looked for in the lines. This is done by simple string comparison,
  without a real tokenizer. Confusing constructs such as comments and
  string literals are removed beforehand.
*/

namespace SharedTools {

/* qmake ignore Q_OBJECT */

/*
  The indenter avoids getting stuck in almost infinite loops by
  imposing arbitrary limits on the number of lines it analyzes when
  looking for a construct.

  For example, the indenter never considers more than BigRoof lines
  backwards when looking for the start of a C-style comment.
*/
namespace {
    enum { SmallRoof = 40, BigRoof = 400 };

/*
  The indenter supports a few parameters:

    * ppHardwareTabSize is the size of a '\t' in your favorite editor.
    * ppIndentSize is the size of an indentation, or software tab
      size.
    * ppContinuationIndentSize is the extra indent for a continuation
      line, when there is nothing to align against on the previous
      line.
    * ppCommentOffset is the indentation within a C-style comment,
      when it cannot be picked up.
*/


    enum { ppCommentOffset = 2 };
}

template <class Iterator>
Indenter<Iterator>::Indenter() :
    ppHardwareTabSize(8),
    ppIndentSize(4),
    ppContinuationIndentSize(8),
    yyLinizerState(new LinizerState),
    yyLine(0),
    yyBraceDepth(0),
    yyLeftBraceFollows(0)
{
}

template <class Iterator>
Indenter<Iterator>::~Indenter()
{
    delete yyLinizerState;
}

template <class Iterator>
Indenter<Iterator> &Indenter<Iterator>::instance()
{
    static Indenter rc;
    return rc;
}

template <class Iterator>
void Indenter<Iterator>::setIndentSize(int size)
{
    ppIndentSize = size;
    ppContinuationIndentSize = 2 * size;
}

template <class Iterator>
void Indenter<Iterator>::setTabSize(int size )
{
    ppHardwareTabSize = size;
}
/*
  Returns the first non-space character in the string t, or
  QChar::null if the string is made only of white space.
*/
template <class Iterator>
QChar Indenter<Iterator>::firstNonWhiteSpace( const QString& t )
{
    if (const int len = t.length())
	for ( int i = 0; i < len; i++)
	    if ( !t[i].isSpace() )
		return t[i];
    return QChar::Null;
}

/*
  Returns true if string t is made only of white space; otherwise
  returns false.
*/
template <class Iterator>
bool Indenter<Iterator>::isOnlyWhiteSpace( const QString& t )
{
    return t.isEmpty() || firstNonWhiteSpace( t ).isNull();
}

/*
  Assuming string t is a line, returns the column number of a given
  index. Column numbers and index are identical for strings that don't
  contain '\t's.
*/
template <class Iterator>
int Indenter<Iterator>::columnForIndex( const QString& t, int index ) const
{
    int col = 0;
    if ( index > t.length() )
	index = t.length();

    const  QChar tab = QLatin1Char('\t');

    for ( int i = 0; i < index; i++ ) {
	if ( t[i] == tab ) {
	    col = ( (col / ppHardwareTabSize) + 1 ) * ppHardwareTabSize;
	} else {
	    col++;
	}
    }
    return col;
}

/*
  Returns the indentation size of string t.
*/
template <class Iterator>
int Indenter<Iterator>::indentOfLine( const QString& t ) const
{
    return columnForIndex( t, t.indexOf(firstNonWhiteSpace(t)) );
}

/*
  Replaces t[k] by ch, unless t[k] is '\t'. Tab characters are better
  left alone since they break the "index equals column" rule. No
  provisions are taken against '\n' or '\r', which shouldn't occur in
  t anyway.
*/
static inline void eraseChar( QString& t, int k, QChar ch )
{
    if ( t[k] != QLatin1Char('\t') )
	t[k] = ch;
}

/*
  Removes some nefast constructs from a code line and returns the
  resulting line.
*/
template <class Iterator>
QString Indenter<Iterator>::trimmedCodeLine( const QString& t )
{
    QString trimmed = t;
    int k;

    const QChar capitalX = QLatin1Char('X');
    const QChar blank = QLatin1Char(' ');
    const QChar colon = QLatin1Char(':');
    const QChar semicolon = QLatin1Char(';');
    /*
      Replace character and string literals by X's, since they may
      contain confusing characters (such as '{' and ';'). "Hello!" is
      replaced by XXXXXXXX. The literals are rigourously of the same
      length before and after; otherwise, we would break alignment of
      continuation lines.
    */
    k = 0;
    while ( (k = m_constants.m_literal.indexIn(trimmed), k) != -1 ) {
	const int matchedLength =  m_constants.m_literal.matchedLength();
	for ( int i = 0; i < matchedLength ; i++ )
	    eraseChar( trimmed, k + i, capitalX );
	k += matchedLength;
    }

    /*
      Replace inline C-style comments by spaces. Other comments are
      handled elsewhere.
    */
    k = 0;
    while ( (k = m_constants.m_inlineCComment.indexIn(trimmed, k)) != -1 ) {
	const int matchedLength = m_constants.m_inlineCComment.matchedLength();
	for ( int i = 0; i < matchedLength; i++ )
	    eraseChar( trimmed, k + i, blank );
	k += matchedLength;
    }

    /*
      Replace goto and switch labels by whitespace, but be careful
      with this case:

	  foo1: bar1;
	      bar2;
    */
    while ( trimmed.lastIndexOf(colon ) != -1 && m_constants.m_label.indexIn(trimmed) != -1 ) {
	const QString cap1 = m_constants.m_label.cap( 1 );
	const int pos1 = m_constants.m_label.pos( 1 );
	int stop = cap1.length();

	if ( pos1 + stop < trimmed.length() && ppIndentSize < stop )
	    stop = ppIndentSize;

	int i = 0;
	while ( i < stop ) {
	    eraseChar( trimmed, pos1 + i, blank  );
	    i++;
	}
	while ( i <  cap1.length() ) {
	    eraseChar( trimmed, pos1 + i,semicolon );
	    i++;
	}
    }

    /*
      Remove C++-style comments.
    */
    k = trimmed.indexOf(m_constants.m_slashSlash );
    if ( k != -1 )
	trimmed.truncate( k );

    return trimmed;
}

/*
  Returns '(' if the last parenthesis is opening, ')' if it is
  closing, and QChar::null if there are no parentheses in t.
*/
static inline QChar lastParen( const QString& t )
{

    const QChar opening = QLatin1Char('(');
    const QChar closing = QLatin1Char(')');


    int i = t.length();
    while ( i > 0 ) {
	i--;
	const QChar c = t[i];
	if (c ==  opening || c == closing )
	    return c;
    }
    return QChar::Null;
}

/*
  Returns true if typedIn the same as okayCh or is null; otherwise
  returns false.
*/
static inline bool okay( QChar typedIn, QChar okayCh )
{
    return typedIn == QChar::Null || typedIn == okayCh;
}


/*
  Saves and restores the state of the global linizer. This enables
  backtracking.
*/
#define YY_SAVE() \
	LinizerState savedState = *yyLinizerState
#define YY_RESTORE() \
	*yyLinizerState = savedState

/*
  Advances to the previous line in yyProgram and update yyLine
  accordingly. yyLine is cleaned from comments and other damageable
  constructs. Empty lines are skipped.
*/
template <class Iterator>
bool Indenter<Iterator>::readLine()
{
    int k;

    const QChar openingBrace = QLatin1Char('{');
    const QChar closingBrace = QLatin1Char('}');
    const QChar blank =  QLatin1Char(' ');
    const QChar hash = QLatin1Char('#');

    yyLinizerState->leftBraceFollows =
	    ( firstNonWhiteSpace(yyLinizerState->line) == openingBrace  );

    do {
	if ( yyLinizerState->iter == yyProgramBegin ) {
	    yyLinizerState->line = QString::null;
	    return false;
	}

	--yyLinizerState->iter;
	yyLinizerState->line = *yyLinizerState->iter;

	yyLinizerState->line = trimmedCodeLine( yyLinizerState->line );

	/*
	  Remove C-style comments that span multiple lines. If the
	  bottom line starts in a C-style comment, we are not aware
	  of that and eventually yyLine will contain a slash-aster.

	  Notice that both if's can be executed, since
	  yyLinizerState->inCComment is potentially set to false in
	  the first if. The order of the if's is also important.
	*/

	if ( yyLinizerState->inCComment ) {

	    k = yyLinizerState->line.indexOf( m_constants.m_slashAster );
	    if ( k == -1 ) {
		yyLinizerState->line = QString::null;
	    } else {
		yyLinizerState->line.truncate( k );
		yyLinizerState->inCComment = false;
	    }
	}

	if ( !yyLinizerState->inCComment ) {
	    k = yyLinizerState->line.indexOf( m_constants.m_asterSlash );
	    if ( k != -1 ) {
		for ( int i = 0; i < k + 2; i++ )
		    eraseChar( yyLinizerState->line, i, blank );
		yyLinizerState->inCComment = true;
	    }
	}

	/*
	  Remove preprocessor directives.
	*/
	k = 0;
	while ( k <  yyLinizerState->line.length() ) {
	    QChar ch = yyLinizerState->line[k];
	    if ( ch == hash ) {
		yyLinizerState->line = QString::null;
	    } else if ( !ch.isSpace() ) {
		break;
	    }
	    k++;
	}

	/*
	  Remove trailing spaces.
	*/
	k = yyLinizerState->line.length();
	while ( k > 0 && yyLinizerState->line[k - 1].isSpace() )
	    k--;
	yyLinizerState->line.truncate( k );

	/*
	  '}' increment the brace depth and '{' decrements it and not
	  the other way around, as we are parsing backwards.
	*/
	yyLinizerState->braceDepth +=
            yyLinizerState->line.count( closingBrace  ) - yyLinizerState->line.count( openingBrace  );

	/*
	  We use a dirty trick for

	      } else ...

	  We don't count the '}' yet, so that it's more or less
	  equivalent to the friendly construct

	      }
	      else ...
	*/
	if ( yyLinizerState->pendingRightBrace )
	    yyLinizerState->braceDepth++;
	yyLinizerState->pendingRightBrace =
		( m_constants.m_braceX.indexIn(yyLinizerState->line) == 0 );
	if ( yyLinizerState->pendingRightBrace )
	    yyLinizerState->braceDepth--;
    } while ( yyLinizerState->line.isEmpty() );

    return true;
}

/*
  Resets the linizer to its initial state, with yyLine containing the
  line above the bottom line of the program.
*/
template <class Iterator>
void Indenter<Iterator>::startLinizer()
{
    yyLinizerState->braceDepth = 0;
    yyLinizerState->inCComment = false;
    yyLinizerState->pendingRightBrace = false;

    yyLine = &yyLinizerState->line;
    yyBraceDepth = &yyLinizerState->braceDepth;
    yyLeftBraceFollows = &yyLinizerState->leftBraceFollows;

    yyLinizerState->iter = yyProgramEnd;
    --yyLinizerState->iter;
    yyLinizerState->line = *yyLinizerState->iter;
    readLine();
}

/*
  Returns true if the start of the bottom line of yyProgram (and
  potentially the whole line) is part of a C-style comment; otherwise
  returns false.
*/
template <class Iterator>
bool Indenter<Iterator>::bottomLineStartsInCComment()
{
    /*
      We could use the linizer here, but that would slow us down
      terribly. We are better to trim only the code lines we need.
    */
    Iterator p = yyProgramEnd;
    --p; // skip bottom line

    for ( int i = 0; i < BigRoof; i++ ) {
	if ( p == yyProgramBegin )
	    return false;
	--p;

	if ( (*p).contains(m_constants.m_slashAster) || (*p).contains(m_constants.m_asterSlash) ) {
	    QString trimmed = trimmedCodeLine( *p );

	    if ( trimmed.contains(m_constants.m_slashAster) ) {
		return true;
	    } else if ( trimmed.contains(m_constants.m_asterSlash) ) {
		return false;
	    }
	}
    }
    return false;
}

/*
  Returns the recommended indent for the bottom line of yyProgram
  assuming that it starts in a C-style comment, a condition that is
  tested elsewhere.

  Essentially, we're trying to align against some text on the previous
  line.
*/
template <class Iterator>
int Indenter<Iterator>::indentWhenBottomLineStartsInCComment() const
{
    int k = yyLine->lastIndexOf(m_constants.m_slashAster );
    if ( k == -1 ) {
	/*
	  We found a normal text line in a comment. Align the
	  bottom line with the text on this line.
	*/
	return indentOfLine( *yyLine );
    } else {
	/*
	  The C-style comment starts on this line. If there is
	  text on the same line, align with it. Otherwise, align
	  with the slash-aster plus a given offset.
	*/
	const int indent = columnForIndex( *yyLine, k );
	k += 2;
	while ( k < yyLine->length() ) {
	    if ( !(*yyLine)[k].isSpace() )
		return columnForIndex( *yyLine, k );
	    k++;
	}
	return indent + ppCommentOffset;
    }
}

/*
  A function called match...() modifies the linizer state. If it
  returns true, yyLine is the top line of the matched construct;
  otherwise, the linizer is left in an unknown state.

  A function called is...() keeps the linizer state intact.
*/

/*
  Returns true if the current line (and upwards) forms a braceless
  control statement; otherwise returns false.

  The first line of the following example is a "braceless control
  statement":

      if ( x )
	  y;
*/
template <class Iterator>
bool Indenter<Iterator>::matchBracelessControlStatement()
{
    int delimDepth = 0;

    const QChar semicolon = QLatin1Char(';');


    if ( yyLine->endsWith(m_constants.m_else))
	return true;

    if ( !yyLine->endsWith(QLatin1Char(')')))
	return false;

    for ( int i = 0; i < SmallRoof; i++ ) {
	int j = yyLine->length();
	while ( j > 0 ) {
	    j--;
	    QChar ch = (*yyLine)[j];

	    switch ( ch.unicode() ) {
	    case ')':
		delimDepth++;
		break;
	    case '(':
		delimDepth--;
		if ( delimDepth == 0 ) {
		    if ( yyLine->contains(m_constants.m_iflikeKeyword) ) {
			/*
			  We have

			      if ( x )
				  y

			  "if ( x )" is not part of the statement
			  "y".
			*/
			return true;
		    }
		}
		if ( delimDepth == -1 ) {
		    /*
		      We have

			  if ( (1 +
				2)

		      and not

			  if ( 1 +
			       2 )
		    */
		    return false;
		}
		break;
	    case '{':
	    case '}':
	    case ';':
		/*
		  We met a statement separator, but not where we
		  expected it. What follows is probably a weird
		  continuation line. Be careful with ';' in for,
		  though.
		*/
		if ( ch != semicolon  || delimDepth == 0 )
		    return false;
	    }
	}

	if ( !readLine() )
	    break;
    }
    return false;
}

/*
  Returns true if yyLine is an unfinished line; otherwise returns
  false.

  In many places we'll use the terms "standalone line", "unfinished
  line" and "continuation line". The meaning of these should be
  evident from this code example:

      a = b;    // standalone line
      c = d +   // unfinished line
	  e +   // unfinished continuation line
	  f +   // unfinished continuation line
	  g;    // continuation line
*/
template <class Iterator>
bool  Indenter<Iterator>::isUnfinishedLine()
{
    bool unf = false;

    YY_SAVE();

    const QChar openingParenthesis = QLatin1Char('(');
    const QChar semicolon = QLatin1Char(';');

    if ( yyLine->isEmpty() )
	return false;

    const QChar lastCh = (*yyLine)[ yyLine->length() - 1];
    if ( ! m_constants.m_bracesSemicolon.contains(lastCh) && !yyLine->endsWith(m_constants.m_3dots) ) {
	/*
	  It doesn't end with ';' or similar. If it's neither
	  "Q_OBJECT" nor "if ( x )", it must be an unfinished line.
	*/
	unf = ( yyLine->contains(m_constants.m_qobject) == 0 &&
		!matchBracelessControlStatement() );
    } else if ( lastCh == semicolon ) {
	if ( lastParen(*yyLine) == openingParenthesis ) {
	    /*
	      Exception:

		  for ( int i = 1; i < 10;
	    */
	    unf = true;
	} else if ( readLine() && yyLine->endsWith(semicolon) &&
		    lastParen(*yyLine) == openingParenthesis ) {
	    /*
	      Exception:

		  for ( int i = 1;
			i < 10;
	    */
	    unf = true;
	}
    }

    YY_RESTORE();
    return unf;
}

/*
  Returns true if yyLine is a continuation line; otherwise returns
  false.
*/
template <class Iterator>
bool Indenter<Iterator>::isContinuationLine()
{
    YY_SAVE();
    const bool cont = readLine() && isUnfinishedLine();
    YY_RESTORE();
    return cont;
}

/*
  Returns the recommended indent for the bottom line of yyProgram,
  assuming it's a continuation line.

  We're trying to align the continuation line against some parenthesis
  or other bracked left opened on a previous line, or some interesting
  operator such as '='.
*/
template <class Iterator>
int Indenter<Iterator>::indentForContinuationLine()
{
    int braceDepth = 0;
    int delimDepth = 0;

    bool leftBraceFollowed = *yyLeftBraceFollows;

    const QChar equals = QLatin1Char('=');
    const QChar comma = QLatin1Char(',');
    const QChar openingParenthesis = QLatin1Char('(');
    const QChar closingParenthesis = QLatin1Char(')');

    for ( int i = 0; i < SmallRoof; i++ ) {
	int hook = -1;

	int j = yyLine->length();
	while ( j > 0 && hook < 0 ) {
	    j--;
	    QChar ch = (*yyLine)[j];

	    switch ( ch.unicode() ) {
	    case ')':
	    case ']':
		delimDepth++;
		break;
	    case '}':
		braceDepth++;
		break;
	    case '(':
	    case '[':
		delimDepth--;
		/*
		  An unclosed delimiter is a good place to align at,
		  at least for some styles (including Trolltech's).
		*/
		if ( delimDepth == -1 )
		    hook = j;
		break;
	    case '{':
		braceDepth--;
		/*
		  A left brace followed by other stuff on the same
		  line is typically for an enum or an initializer.
		  Such a brace must be treated just like the other
		  delimiters.
		*/
		if ( braceDepth == -1 ) {
		    if ( j <  yyLine->length() - 1 ) {
			hook = j;
		    } else {
			return 0; // shouldn't happen
		    }
		}
		break;
	    case '=':
		/*
		  An equal sign is a very natural alignment hook
		  because it's usually the operator with the lowest
		  precedence in statements it appears in. Case in
		  point:

		      int x = 1 +
			      2;

		  However, we have to beware of constructs such as
		  default arguments and explicit enum constant
		  values:

		      void foo( int x = 0,
				int y = 0 );

		  And not

		      void foo( int x = 0,
				      int y = 0 );

		  These constructs are caracterized by a ',' at the
		  end of the unfinished lines or by unbalanced
		  parentheses.
		*/
		if ( j > 0 && j < yyLine->length() - 1
                     && !m_constants.m_operators.contains((*yyLine)[j - 1])
		     && (*yyLine)[j + 1] != equals ) {
		    if ( braceDepth == 0 && delimDepth == 0 &&
			 !yyLine->endsWith(comma) &&
			 (yyLine->contains(openingParenthesis) == yyLine->contains(closingParenthesis)) )
			hook = j;
		}
	    }
	}

	if ( hook >= 0 ) {
	    /*
	      Yes, we have a delimiter or an operator to align
	      against! We don't really align against it, but rather
	      against the following token, if any. In this example,
	      the following token is "11":

		  int x = ( 11 +
			    2 );

	      If there is no such token, we use a continuation indent:

		  static QRegExp foo( QString(
			  "foo foo foo foo foo foo foo foo foo") );
	    */
	    hook++;
	    while ( hook <  yyLine->length() ) {
		if ( !(*yyLine)[hook].isSpace() )
		    return columnForIndex( *yyLine, hook );
		hook++;
	    }
	    return indentOfLine( *yyLine ) + ppContinuationIndentSize;
	}

	if ( braceDepth != 0 )
	    break;

	/*
	  The line's delimiters are balanced. It looks like a
	  continuation line or something.
	*/
	if ( delimDepth == 0 ) {
	    if ( leftBraceFollowed ) {
		/*
		  We have

		      int main()
		      {

		  or

		      Bar::Bar()
			  : Foo( x )
		      {

		  The "{" should be flush left.
		*/
		if ( !isContinuationLine() )
		    return indentOfLine( *yyLine );
	    } else if ( isContinuationLine() || yyLine->endsWith(comma)) {
		/*
		  We have

		      x = a +
			  b +
			  c;

		  or

		      int t[] = {
			  1, 2, 3,
			  4, 5, 6

		  The "c;" should fall right under the "b +", and the
		  "4, 5, 6" right under the "1, 2, 3,".
		*/
		return indentOfLine( *yyLine );
	    } else {
		/*
		  We have

		      stream << 1 +
			      2;

		  We could, but we don't, try to analyze which
		  operator has precedence over which and so on, to
		  obtain the excellent result

		      stream << 1 +
				2;

		  We do have a special trick above for the assignment
		  operator above, though.
		*/
		return indentOfLine( *yyLine ) + ppContinuationIndentSize;
	    }
	}

	if ( !readLine() )
	    break;
    }
    return 0;
}

/*
  Returns the recommended indent for the bottom line of yyProgram if
  that line is standalone (or should be indented likewise).

  Indenting a standalone line is tricky, mostly because of braceless
  control statements. Grossly, we are looking backwards for a special
  line, a "hook line", that we can use as a starting point to indent,
  and then modify the indentation level according to the braces met
  along the way to that hook.

  Let's consider a few examples. In all cases, we want to indent the
  bottom line.

  Example 1:

      x = 1;
      y = 2;

  The hook line is "x = 1;". We met 0 opening braces and 0 closing
  braces. Therefore, "y = 2;" inherits the indent of "x = 1;".

  Example 2:

      if ( x ) {
	  y;

  The hook line is "if ( x ) {". No matter what precedes it, "y;" has
  to be indented one level deeper than the hook line, since we met one
  opening brace along the way.

  Example 3:

      if ( a )
	  while ( b ) {
	      c;
	  }
      d;

  To indent "d;" correctly, we have to go as far as the "if ( a )".
  Compare with

      if ( a ) {
	  while ( b ) {
	      c;
	  }
	  d;

  Still, we're striving to go back as little as possible to accomodate
  people with irregular indentation schemes. A hook line near at hand
  is much more reliable than a remote one.
*/
template <class Iterator>
int  Indenter<Iterator>::indentForStandaloneLine()
{
    const QChar semicolon = QLatin1Char(';');
    const QChar openingBrace = QLatin1Char('{');

    for ( int i = 0; i < SmallRoof; i++ ) {
	if ( !*yyLeftBraceFollows ) {
	    YY_SAVE();

	    if ( matchBracelessControlStatement() ) {
		/*
		  The situation is this, and we want to indent "z;":

		      if ( x &&
			   y )
			  z;

		  yyLine is "if ( x &&".
		*/
		return indentOfLine( *yyLine ) + ppIndentSize;
	    }
	    YY_RESTORE();
	}

	if ( yyLine->endsWith(semicolon) || yyLine->count(openingBrace) > 0 ) {
	    /*
	      The situation is possibly this, and we want to indent
	      "z;":

		  while ( x )
		      y;
		  z;

	      We return the indent of "while ( x )". In place of "y;",
	      any arbitrarily complex compound statement can appear.
	    */

	    if ( *yyBraceDepth > 0 ) {
		do {
		    if ( !readLine() )
			break;
		} while ( *yyBraceDepth > 0 );
	    }

	    LinizerState hookState;

	    while ( isContinuationLine() )
		readLine();
	    hookState = *yyLinizerState;

	    readLine();
	    if ( *yyBraceDepth <= 0 ) {
		do {
		    if ( !matchBracelessControlStatement() )
			break;
		    hookState = *yyLinizerState;
		} while ( readLine() );
	    }

	    *yyLinizerState = hookState;

	    while ( isContinuationLine() )
		readLine();

	    /*
	      Never trust lines containing only '{' or '}', as some
	      people (Richard M. Stallman) format them weirdly.
	    */
	    if ( yyLine->trimmed().length() > 1 )
		return indentOfLine( *yyLine ) - *yyBraceDepth * ppIndentSize;
	}

	if ( !readLine() )
	    return -*yyBraceDepth * ppIndentSize;
    }
    return 0;
}

/*
  Returns the recommended indent for the bottom line of program.
  Unless null, typedIn stores the character of yyProgram that
  triggered reindentation.

  This function works better if typedIn is set properly; it is
  slightly more conservative if typedIn is completely wild, and
  slighly more liberal if typedIn is always null. The user might be
  annoyed by the liberal behavior.
*/
template <class Iterator>
int Indenter<Iterator>::indentForBottomLine(const Iterator &current,
                                            const Iterator &programBegin,
                                            const Iterator &programEnd,
                                            QChar typedIn )
{
    if ( programBegin == programEnd )
	return 0;

    yyProgramBegin = programBegin;
    yyProgramEnd = programEnd;

    startLinizer();

    Iterator lastIt = current;

    QString bottomLine = *lastIt;
    QChar firstCh = firstNonWhiteSpace( bottomLine );
    int indent;

    const QChar hash = QLatin1Char('#');
    const QChar closingBrace = QLatin1Char('}');
    const QChar colon =  QLatin1Char(':');

    if ( bottomLineStartsInCComment() ) {
	/*
	  The bottom line starts in a C-style comment. Indent it
	  smartly, unless the user has already played around with it,
	  in which case it's better to leave her stuff alone.
	*/
	if ( isOnlyWhiteSpace(bottomLine) ) {
	    indent = indentWhenBottomLineStartsInCComment();
	} else {
	    indent = indentOfLine( bottomLine );
	}
    } else if ( okay(typedIn, hash) && firstCh == hash ) {
	/*
	  Preprocessor directives go flush left.
	*/
	indent = 0;
    } else {
	if ( isUnfinishedLine() ) {
	    indent = indentForContinuationLine();
	} else {
	    indent = indentForStandaloneLine();
	}

	if ( okay(typedIn, closingBrace) && firstCh == closingBrace ) {
	    /*
	      A closing brace is one level more to the left than the
	      code it follows.
	    */
	    indent -= ppIndentSize;
	} else if ( okay(typedIn, colon) ) {
	    if ( m_constants.m_caseLabel.exactMatch(bottomLine) ) {
		/*
		  Move a case label (or the ':' in front of a
		  constructor initialization list) one level to the
		  left, but only if the user did not play around with
		  it yet. Some users have exotic tastes in the
		  matter, and most users probably are not patient
		  enough to wait for the final ':' to format their
		  code properly.

		  We don't attempt the same for goto labels, as the
		  user is probably the middle of "foo::bar". (Who
		  uses goto, anyway?)
		*/
		if ( indentOfLine(bottomLine) <= indent )
		    indent -= ppIndentSize;
		else
		    indent = indentOfLine( bottomLine );
	    }
	}
    }
    return qMax( 0, indent );
}

} // namespace SharedTools

#undef YY_SAVE
#undef YY_RESTORE

#endif // INDENTER_C
