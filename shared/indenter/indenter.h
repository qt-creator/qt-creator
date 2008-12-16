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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef INDENTER_H
#define INDENTER_H

#include <QtCore/QString>
#include <QtCore/QStringList>

namespace SharedTools {
namespace IndenterInternal {
/* String constants and regexps required by the indenter. Separate for code cleanliness*/
struct Constants {
    Constants();
    const QString m_slashAster;
    const QString m_asterSlash;
    const QString m_slashSlash;
    const QString m_else;
    const QString m_qobject;
    const QString m_operators;
    const QString m_bracesSemicolon;
    const QString m_3dots;

    QRegExp m_literal;
    QRegExp m_label;
    QRegExp m_inlineCComment;
    QRegExp m_braceX;
    QRegExp m_iflikeKeyword;
    QRegExp m_caseLabel;
};

/*  The "linizer" is a group of functions and variables to iterate
 * through the source code of the program to indent. The program is
 * given as a list of strings, with the bottom line being the line to
 * indent. The actual program might contain extra lines, but those are
 * uninteresting and not passed over to us. */
template <class Iterator>
struct LinizerState {

    QString line;
    int braceDepth;
    bool leftBraceFollows;

    Iterator iter;
    bool inCComment;
    bool pendingRightBrace;
};
}

/* Indenter singleton as a template of a bidirectional input iterator
 * of a sequence of code lines represented as QString.
 * When setting the parameters, be careful to
 * specify the correct template parameters (best use a typedef). */
template <class Iterator>
class Indenter {
    Indenter(const Indenter&);
    Indenter &operator=(const Indenter&);
    Indenter();

public:

    ~Indenter();

    static Indenter &instance();

    void setIndentSize(int size);
    void setTabSize(int size );

    /* Return indentation for the last line of the sequence
     * based on the previous lines. */
    int indentForBottomLine(const Iterator &current,
                            const Iterator &programBegin,
                            const Iterator &programEnd,
                            QChar typedIn);

    // Helpers.
    static bool isOnlyWhiteSpace( const QString& t);
    static QChar firstNonWhiteSpace( const QString& t );

private:
    int columnForIndex( const QString& t, int index ) const;
    int indentOfLine( const QString& t ) const;
    QString trimmedCodeLine( const QString& t );
    bool readLine();
    void startLinizer();
    bool bottomLineStartsInCComment();
    int indentWhenBottomLineStartsInCComment() const;
    bool matchBracelessControlStatement();
    bool isUnfinishedLine();
    bool isContinuationLine();
    int indentForContinuationLine();
    int indentForStandaloneLine();

    IndenterInternal::Constants m_constants;
    int ppHardwareTabSize;
    int ppIndentSize;
    int ppContinuationIndentSize;

    Iterator yyProgramBegin;
    Iterator yyProgramEnd;

    typedef typename IndenterInternal::LinizerState<Iterator> LinizerState;

    LinizerState *yyLinizerState ;

    // shorthands
    const QString *yyLine;
    const int *yyBraceDepth;
    const bool *yyLeftBraceFollows;
};
}

#include "indenter_impl.h"

#endif // INDENTER_H
