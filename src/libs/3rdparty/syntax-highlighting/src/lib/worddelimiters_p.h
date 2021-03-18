/*
    SPDX-FileCopyrightText: 2020 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_WORDDELIMITERS_P_H
#define KSYNTAXHIGHLIGHTING_WORDDELIMITERS_P_H

#include <QString>

namespace KSyntaxHighlighting
{
/**
 * Repesents a list of character that separates 2 words.
 *
 * Default delimiters are .():!+*,-<=>%&/;?[]^{|}~\, space (' ') and tabulator ('\t').
 *
 * @see Rule
 * @since 5.74
 */
class WordDelimiters
{
public:
    WordDelimiters();

    /**
     * Returns @c true if @p c is a word delimiter; otherwise returns @c false.
     */
    bool contains(QChar c) const;

    /**
     * Appends each character of @p s to word delimiters.
     */
    void append(QStringView s);

    /**
     * Removes each character of @p s from word delimiters.
     */
    void remove(QStringView c);

private:
    /**
     * An array which represents ascii characters for very fast lookup.
     * The character is used as an index and the value @c true indicates a word delimiter.
     */
    bool asciiDelimiters[128];

    /**
     * Contains characters that are not ascii and is empty for most syntax definition.
     */
    QString notAsciiDelimiters;
};
}

#endif
