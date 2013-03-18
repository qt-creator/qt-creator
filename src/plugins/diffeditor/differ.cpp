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

/*
The main algorithm "diffMyers()" is based on "An O(ND) Difference Algorithm
and Its Variations" by Eugene W. Myers: http://www.xmailserver.org/diff2.pdf

Preprocessing and postprocessing functions inspired by "Diff Strategies"
publication by Neil Fraser: http://neil.fraser.name/writing/diff/
*/

#include "differ.h"

#include <QList>
#include <QStringList>
#include <QMap>
#include <QCoreApplication>

namespace DiffEditor {

static int commonPrefix(const QString &text1, const QString &text2)
{
    int i = 0;
    const int text1Count = text1.count();
    const int text2Count = text2.count();
    const int maxCount = qMin(text1Count, text2Count);
    while (i < maxCount) {
        if (text1.at(i) != text2.at(i))
            break;
        i++;
    }
    return i;
}

static int commonSuffix(const QString &text1, const QString &text2)
{
    int i = 0;
    const int text1Count = text1.count();
    const int text2Count = text2.count();
    const int maxCount = qMin(text1Count, text2Count);
    while (i < maxCount) {
        if (text1.at(text1Count - i - 1) != text2.at(text2Count - i - 1))
            break;
        i++;
    }
    return i;
}

static int commonOverlap(const QString &text1, const QString &text2)
{
    int i = 0;
    const int text1Count = text1.count();
    const int text2Count = text2.count();
    const int maxCount = qMin(text1Count, text2Count);
    while (i < maxCount) {
        if (text1.midRef(text1Count - maxCount + i) == text2.leftRef(maxCount - i))
            return maxCount - i;
        i++;
    }
    return 0;
}

static QList<Diff> decode(const QList<Diff> &diffList,
                                  const QStringList &lines)
{
    QList<Diff> newDiffList;
    for (int i = 0; i < diffList.count(); i++) {
        Diff diff = diffList.at(i);
        QString text;
        for (int j = 0; j < diff.text.count(); j++) {
            const int idx = static_cast<ushort>(diff.text.at(j).unicode());
            text += lines.value(idx);
        }
        diff.text = text;
        newDiffList.append(diff);
    }
    return newDiffList;
}

static QList<Diff> squashEqualities(const QList<Diff> &diffList)
{
    if (diffList.count() < 3) // we need at least 3 items
        return diffList;

    QList<Diff> newDiffList;
    Diff prevDiff = diffList.at(0);
    Diff thisDiff = diffList.at(1);
    Diff nextDiff = diffList.at(2);
    int i = 2;
    while (i < diffList.count()) {
        if (prevDiff.command == Diff::Equal
                && nextDiff.command == Diff::Equal) {
            if (thisDiff.text.endsWith(prevDiff.text)) {
                thisDiff.text = prevDiff.text
                        + thisDiff.text.left(thisDiff.text.count()
                        - prevDiff.text.count());
                nextDiff.text = prevDiff.text + nextDiff.text;
            } else if (thisDiff.text.startsWith(nextDiff.text)) {
                prevDiff.text += nextDiff.text;
                thisDiff.text = thisDiff.text.mid(nextDiff.text.count())
                        + nextDiff.text;
                i++;
                if (i < diffList.count())
                    nextDiff = diffList.at(i);
                newDiffList.append(prevDiff);
            } else {
                newDiffList.append(prevDiff);
            }
        } else {
            newDiffList.append(prevDiff);
        }
        prevDiff = thisDiff;
        thisDiff = nextDiff;
        i++;
        if (i < diffList.count())
            nextDiff = diffList.at(i);
    }
    newDiffList.append(prevDiff);
    if (i == diffList.count())
        newDiffList.append(thisDiff);
    return newDiffList;
}

static QList<Diff> cleanupOverlaps(const QList<Diff> &diffList)
{
    // Find overlaps between deletions and insetions.
    // The "diffList" already contains at most one deletion and
    // one insertion between two equalities, in this order.
    // Eliminate overlaps, e.g.:
    // DEL(ABCXXXX), INS(XXXXDEF) -> DEL(ABC), EQ(XXXX), INS(DEF)
    // DEL(XXXXABC), INS(DEFXXXX) -> INS(DEF), EQ(XXXX), DEL(ABC)
    QList<Diff> newDiffList;
    int i = 0;
    while (i < diffList.count()) {
        Diff thisDiff = diffList.at(i);
        Diff nextDiff = i < diffList.count() - 1
                ? diffList.at(i + 1)
                : Diff(Diff::Equal, QString());
        if (thisDiff.command == Diff::Delete
                && nextDiff.command == Diff::Insert) {
            const int delInsOverlap = commonOverlap(thisDiff.text, nextDiff.text);
            const int insDelOverlap = commonOverlap(nextDiff.text, thisDiff.text);
            if (delInsOverlap >= insDelOverlap) {
                if (delInsOverlap > thisDiff.text.count() / 2
                        || delInsOverlap > nextDiff.text.count() / 2) {
                    thisDiff.text = thisDiff.text.left(thisDiff.text.count() - delInsOverlap);
                    Diff equality = Diff(Diff::Equal, nextDiff.text.left(delInsOverlap));
                    nextDiff.text = nextDiff.text.mid(delInsOverlap);
                    newDiffList.append(thisDiff);
                    newDiffList.append(equality);
                    newDiffList.append(nextDiff);
                } else {
                    newDiffList.append(thisDiff);
                    newDiffList.append(nextDiff);
                }
            } else {
                if (insDelOverlap > thisDiff.text.count() / 2
                        || insDelOverlap > nextDiff.text.count() / 2) {
                    nextDiff.text = nextDiff.text.left(nextDiff.text.count() - insDelOverlap);
                    Diff equality = Diff(Diff::Equal, thisDiff.text.left(insDelOverlap));
                    thisDiff.text = thisDiff.text.mid(insDelOverlap);
                    newDiffList.append(nextDiff);
                    newDiffList.append(equality);
                    newDiffList.append(thisDiff);
                } else {
                    newDiffList.append(thisDiff);
                    newDiffList.append(nextDiff);
                }
            }
            i += 2;
        } else {
            newDiffList.append(thisDiff);
            i++;
        }
    }
    return newDiffList;
}

static int cleanupSemanticsScore(const QString &text1, const QString &text2)
{
    static QRegExp blankLineEnd = QRegExp(QLatin1String("\\n\\r?\\n$"));
    static QRegExp blankLineStart = QRegExp(QLatin1String("^\\r?\\n\\r?\\n"));
    static QRegExp sentenceEnd = QRegExp(QLatin1String("\\. $"));

    if (!text1.count() || !text2.count()) // Edges
        return 6;

    QChar char1 = text1[text1.count() - 1];
    QChar char2 = text2[0];
    bool nonAlphaNumeric1 = !char1.isLetterOrNumber();
    bool nonAlphaNumeric2 = !char2.isLetterOrNumber();
    bool whitespace1 = nonAlphaNumeric1 && char1.isSpace();
    bool whitespace2 = nonAlphaNumeric2 && char2.isSpace();
    bool lineBreak1 = whitespace1 && char1.category() == QChar::Other_Control;
    bool lineBreak2 = whitespace2 && char2.category() == QChar::Other_Control;
    bool blankLine1 = lineBreak1 && blankLineEnd.indexIn(text1) != -1;
    bool blankLine2 = lineBreak2 && blankLineStart.indexIn(text2) != -1;

    if (blankLine1 || blankLine2) // Blank lines
      return 5;
    if (lineBreak1 || lineBreak2) // Line breaks
      return 4;
    if (sentenceEnd.indexIn(text1) != -1) // End of sentence
      return 3;
    if (whitespace1 || whitespace2) // Whitespaces
      return 2;
    if (nonAlphaNumeric1 || nonAlphaNumeric2) // Non-alphanumerics
      return 1;

    return 0;
}

///////////////


Diff::Diff() :
    command(Diff::Equal)
{
}

Diff::Diff(Command com, const QString &txt) :
    command(com),
    text(txt)
{
}

bool Diff::operator==(const Diff &other) const
{
     return command == other.command && text == other.text;
}

bool Diff::operator!=(const Diff &other) const
{
     return !(operator == (other));
}

QString Diff::commandString(Command com)
{
    if (com == Delete)
        return QCoreApplication::translate("Diff", "Delete");
    else if (com == Insert)
        return QCoreApplication::translate("Diff", "Insert");
    return QCoreApplication::translate("Diff", "Equal");
}

QString Diff::toString() const
{
    QString prettyText = text;
    // Replace linebreaks with pretty char
    prettyText.replace(QLatin1Char('\n'), QLatin1Char(L'\u00b6'));
    return commandString(command) + QLatin1String(" \"")
            + prettyText + QLatin1String("\"");
}

///////////////

Differ::Differ()
    : m_diffMode(Differ::LineMode),
      m_currentDiffMode(Differ::LineMode)
{

}

QList<Diff> Differ::diff(const QString &text1, const QString &text2)
{
    m_currentDiffMode = m_diffMode;
    return merge(preprocess1AndDiff(text1, text2));
}

void Differ::setDiffMode(Differ::DiffMode mode)
{
    m_diffMode = mode;
}

bool Differ::diffMode() const
{
    return m_diffMode;
}

QList<Diff> Differ::preprocess1AndDiff(const QString &text1, const QString &text2)
{
    if (text1.isNull() && text2.isNull())
        return QList<Diff>();

    if (text1 == text2) {
        QList<Diff> diffList;
        if (!text1.isEmpty())
            diffList.append(Diff(Diff::Equal, text1));
        return diffList;
    }

    QString newText1 = text1;
    QString newText2 = text2;
    QString prefix;
    QString suffix;
    const int prefixCount = commonPrefix(text1, text2);
    if (prefixCount) {
        prefix = text1.left(prefixCount);
        newText1 = text1.mid(prefixCount);
        newText2 = text2.mid(prefixCount);
    }
    const int suffixCount = commonSuffix(newText1, newText2);
    if (suffixCount) {
        suffix = newText1.right(suffixCount);
        newText1 = newText1.left(newText1.count() - suffixCount);
        newText2 = newText2.left(newText2.count() - suffixCount);
    }
    QList<Diff> diffList = preprocess2AndDiff(newText1, newText2);
    if (prefixCount)
        diffList.prepend(Diff(Diff::Equal, prefix));
    if (suffixCount)
        diffList.append(Diff(Diff::Equal, suffix));
    return diffList;
}

QList<Diff> Differ::preprocess2AndDiff(const QString &text1, const QString &text2)
{
    QList<Diff> diffList;

    if (text1.isEmpty()) {
        diffList.append(Diff(Diff::Insert, text2));
        return diffList;
    }

    if (text2.isEmpty()) {
        diffList.append(Diff(Diff::Delete, text1));
        return diffList;
    }

    if (text1.count() != text2.count())
    {
        const QString longtext = text1.count() > text2.count() ? text1 : text2;
        const QString shorttext = text1.count() > text2.count() ? text2 : text1;
        const int i = longtext.indexOf(shorttext);
        if (i != -1) {
            const Diff::Command command = (text1.count() > text2.count()) ? Diff::Delete : Diff::Insert;
            diffList.append(Diff(command, longtext.left(i)));
            diffList.append(Diff(Diff::Equal, shorttext));
            diffList.append(Diff(command, longtext.mid(i + shorttext.count())));
            return diffList;
        }

        if (shorttext.count() == 1) {
            diffList.append(Diff(Diff::Delete, text1));
            diffList.append(Diff(Diff::Insert, text2));
            return diffList;
        }
    }

    if (m_currentDiffMode != Differ::CharMode && text1.count() > 80 && text2.count() > 80)
        return diffNonCharMode(text1, text2);

    return diffMyers(text1, text2);
}

QList<Diff> Differ::diffMyers(const QString &text1, const QString &text2)
{
    const int n = text1.count();
    const int m = text2.count();
    const bool odd = (n + m) % 2;
    const int D = odd ? (n + m) / 2 + 1 : (n + m) / 2;
    const int delta = n - m;
    const int vShift = D;
    int *forwardV = new int[2 * D + 1]; // free me
    int *reverseV = new int[2 * D + 1]; // free me
    for (int i = 0; i <= 2 * D; i++) {
        forwardV[i] = -1;
        reverseV[i] = -1;
    }
    forwardV[vShift + 1] = 0;
    reverseV[vShift + 1] = 0;
    int kMinForward = -D;
    int kMaxForward = D;
    int kMinReverse = -D;
    int kMaxReverse = D;
    for (int d = 0; d <= D; d++) {
        // going forward
        for (int k = qMax(-d, kMinForward + qAbs(d + kMinForward) % 2);
             k <= qMin(d, kMaxForward - qAbs(d + kMaxForward) % 2);
             k = k + 2) {
            int x;
            if (k == -d || (k < d && forwardV[k + vShift - 1] < forwardV[k + vShift + 1])) {
                x = forwardV[k + vShift + 1]; // copy vertically from diagonal k + 1, y increases, y may exceed the graph
            } else {
                x = forwardV[k + vShift - 1] + 1; // copy horizontally from diagonal k - 1, x increases, x may exceed the graph
            }
            int y = x - k;

            if (x > n) {
                kMaxForward = k - 1; // we are beyond the graph (right border), don't check diagonals >= current k anymore
            } else if (y > m) {
                kMinForward = k + 1; // we are beyond the graph (bottom border), don't check diagonals <= current k anymore
            } else {
                // find snake
                while (x < n && y < m) {
                    if (text1.at(x) != text2.at(y))
                        break;
                    x++;
                    y++;
                }
                forwardV[k + vShift] = x;
                if (odd) { // check if overlap
                    if (k >= delta - (d - 1) && k <= delta + (d - 1)) {
                        if (n - reverseV[delta - k + vShift] <= x) {
                            delete [] forwardV;
                            delete [] reverseV;
                            return diffMyersSplit(text1, x, text2, y);
                        }
                    }
                }
            }
        }
        // in reverse direction
        for (int k = qMax(-d, kMinReverse + qAbs(d + kMinReverse) % 2);
             k <= qMin(d, kMaxReverse - qAbs(d + kMaxReverse) % 2);
             k = k + 2) {
            int x;
            if (k == -d || (k < d && reverseV[k + vShift - 1] < reverseV[k + vShift + 1])) {
                x = reverseV[k + vShift + 1];
            } else {
                x = reverseV[k + vShift - 1] + 1;
            }
            int y = x - k;

            if (x > n) {
                kMaxReverse = k - 1; // we are beyond the graph (right border), don't check diagonals >= current k anymore
            } else if (y > m) {
                kMinReverse = k + 1; // we are beyond the graph (bottom border), don't check diagonals <= current k anymore
            } else {
                // find snake
                while (x < n && y < m) {
                    if (text1.at(n - x - 1) != text2.at(m - y - 1))
                        break;
                    x++;
                    y++;
                }
                reverseV[k + vShift] = x;
                if (!odd) { // check if overlap
                    if (k >= delta - d && k <= delta + d) {
                        if (n - forwardV[delta - k + vShift] <= x) {
                            delete [] forwardV;
                            delete [] reverseV;
                            return diffMyersSplit(text1, n - x, text2, m - x + k);
                        }
                    }
                }
            }
        }
    }
    delete [] forwardV;
    delete [] reverseV;

    // Completely different
    QList<Diff> diffList;
    diffList.append(Diff(Diff::Delete, text1));
    diffList.append(Diff(Diff::Insert, text2));
    return diffList;
}

QList<Diff> Differ::diffMyersSplit(
        const QString &text1, int x,
        const QString &text2, int y)
{
    const QString text11 = text1.left(x);
    const QString text12 = text1.mid(x);
    const QString text21 = text2.left(y);
    const QString text22 = text2.mid(y);

    QList<Diff> diffList1 = preprocess1AndDiff(text11, text21);
    QList<Diff> diffList2 = preprocess1AndDiff(text12, text22);
    return diffList1 + diffList2;
}

QList<Diff> Differ::diffNonCharMode(const QString text1, const QString text2)
{
    QString encodedText1;
    QString encodedText2;
    QStringList subtexts = encode(text1, text2, &encodedText1, &encodedText2);

    DiffMode diffMode = m_currentDiffMode;
    m_currentDiffMode = CharMode;

    // Each different subtext is a separate symbol
    // process these symbols as text with bigger alphabet
    QList<Diff> diffList = preprocess1AndDiff(encodedText1, encodedText2);

    diffList = decode(diffList, subtexts);

    QString lastDelete;
    QString lastInsert;
    QList<Diff> newDiffList;
    for (int i = 0; i <= diffList.count(); i++) {
        const Diff diffItem = i < diffList.count()
                  ? diffList.at(i)
                  : Diff(Diff::Equal, QLatin1String("")); // dummy, ensure we process to the end even when diffList doesn't end with equality
        if (diffItem.command == Diff::Delete) {
            lastDelete += diffItem.text;
        } else if (diffItem.command == Diff::Insert) {
            lastInsert += diffItem.text;
        } else { // Diff::Equal
            if (lastDelete.count() || lastInsert.count()) {
                // Rediff here on char basis
                newDiffList += preprocess1AndDiff(lastDelete, lastInsert);

                lastDelete.clear();
                lastInsert.clear();
            }
            newDiffList.append(diffItem);
        }
    }

    m_currentDiffMode = diffMode;
    return newDiffList;
}

QStringList Differ::encode(const QString &text1,
                                  const QString &text2,
                                  QString *encodedText1,
                                  QString *encodedText2)
{
    QStringList lines;
    lines.append(QLatin1String("")); // don't use code: 0
    QMap<QString, int> lineToCode;

    *encodedText1 = encode(text1, &lines, &lineToCode);
    *encodedText2 = encode(text2, &lines, &lineToCode);

    return lines;
}

int Differ::findSubtextEnd(const QString &text,
                                  int subtextStart)
{
    if (m_currentDiffMode == Differ::LineMode) {
        int subtextEnd = text.indexOf(QLatin1Char('\n'), subtextStart);
        if (subtextEnd == -1) {
            subtextEnd = text.count() - 1;
        }
        return ++subtextEnd;
    } else if (m_currentDiffMode == Differ::WordMode) {
        if (!text.at(subtextStart).isLetter())
            return subtextStart + 1;
        int i = subtextStart + 1;

        const int count = text.count();
        while (i < count && text.at(i).isLetter())
            i++;
        return i;
    }
    return subtextStart + 1; // CharMode
}

QString Differ::encode(const QString &text,
                              QStringList *lines,
                              QMap<QString, int> *lineToCode)
{
    int subtextStart = 0;
    int subtextEnd = -1;
    QString codes;
    while (subtextEnd < text.count()) {
        subtextEnd = findSubtextEnd(text, subtextStart);
        const QString line = text.mid(subtextStart, subtextEnd - subtextStart);
        subtextStart = subtextEnd;

        if (lineToCode->contains(line)) {
            int code = lineToCode->value(line);
            codes += QChar(static_cast<ushort>(code));
        } else {
            lines->append(line);
            lineToCode->insert(line, lines->count() - 1);
            codes += QChar(static_cast<ushort>(lines->count() - 1));
        }
    }
    return codes;
}

QList<Diff> Differ::merge(const QList<Diff> &diffList)
{
    QString lastDelete;
    QString lastInsert;
    QList<Diff> newDiffList;
    for (int i = 0; i <= diffList.count(); i++) {
        Diff diff = i < diffList.count()
                  ? diffList.at(i)
                  : Diff(Diff::Equal, QString()); // dummy, ensure we process to the end even when diffList doesn't end with equality
        if (diff.command == Diff::Delete) {
            lastDelete += diff.text;
        } else if (diff.command == Diff::Insert) {
            lastInsert += diff.text;
        } else { // Diff::Equal
            if (lastDelete.count() || lastInsert.count()) {

                // common prefix
                const int prefixCount = commonPrefix(lastDelete, lastInsert);
                if (prefixCount) {
                    const QString prefix = lastDelete.left(prefixCount);
                    lastDelete = lastDelete.mid(prefixCount);
                    lastInsert = lastInsert.mid(prefixCount);

                    if (newDiffList.count()
                            && newDiffList.last().command == Diff::Equal) {
                        newDiffList.last().text += prefix;
                    } else {
                        newDiffList.append(Diff(Diff::Equal, prefix));
                    }
                }

                // common suffix
                const int suffixCount = commonSuffix(lastDelete, lastInsert);
                if (suffixCount) {
                    const QString suffix = lastDelete.right(suffixCount);
                    lastDelete = lastDelete.left(lastDelete.count() - suffixCount);
                    lastInsert = lastInsert.left(lastInsert.count() - suffixCount);

                    diff.text.prepend(suffix);
                }

                // append delete / insert / equal
                if (lastDelete.count())
                    newDiffList.append(Diff(Diff::Delete, lastDelete));
                if (lastInsert.count())
                    newDiffList.append(Diff(Diff::Insert, lastInsert));
                if (diff.text.count())
                    newDiffList.append(diff);
                lastDelete.clear();
                lastInsert.clear();
            } else { // join with last equal diff
                if (newDiffList.count()
                        && newDiffList.last().command == Diff::Equal) {
                    newDiffList.last().text += diff.text;
                } else {
                    if (diff.text.count())
                        newDiffList.append(diff);
                }
            }
        }
    }

    QList<Diff> squashedDiffList = squashEqualities(newDiffList);
    if (squashedDiffList.count() != newDiffList.count())
        return merge(squashedDiffList);

    return squashedDiffList;
}

struct EqualityData
{
    int equalityIndex;
    int textCount;
    int deletesBefore;
    int insertsBefore;
    int deletesAfter;
    int insertsAfter;
};

QList<Diff> Differ::cleanupSemantics(const QList<Diff> &diffList)
{
    int deletes = 0;
    int inserts = 0;
    // equality index, equality data
    QList<EqualityData> equalities;
    for (int i = 0; i <= diffList.count(); i++) {
        Diff diff = i < diffList.count()
                  ? diffList.at(i)
                  : Diff(Diff::Equal, QString()); // dummy, ensure we process to the end even when diffList doesn't end with equality
        if (diff.command == Diff::Equal) {
            if (!equalities.isEmpty()) {
                EqualityData &previousData = equalities.last();
                previousData.deletesAfter = deletes;
                previousData.insertsAfter = inserts;
            }
            if (i < diffList.count()) { // don't insert dummy
                EqualityData data;
                data.equalityIndex = i;
                data.textCount = diff.text.count();
                data.deletesBefore = deletes;
                data.insertsBefore = inserts;
                equalities.append(data);

                deletes = 0;
                inserts = 0;
            }
        } else {
            if (diff.command == Diff::Delete)
                deletes += diff.text.count();
            else if (diff.command == Diff::Insert)
                inserts += diff.text.count();
        }
    }

    QMap<int, bool> equalitiesToBeSplit;
    int i = 0;
    while (i < equalities.count()) {
        const EqualityData data = equalities.at(i);
        if (data.textCount <= qMax(data.deletesBefore, data.insertsBefore)
                && data.textCount <= qMax(data.deletesAfter, data.insertsAfter)) {
            if (i > 0) {
                EqualityData &previousData = equalities[i - 1];
                previousData.deletesAfter += data.textCount + data.deletesAfter;
                previousData.insertsAfter += data.textCount + data.insertsAfter;
            }
            if (i < equalities.count() - 1) {
                EqualityData &nextData = equalities[i + 1];
                nextData.deletesBefore += data.textCount + data.deletesBefore;
                nextData.insertsBefore += data.textCount + data.insertsBefore;
            }
            equalitiesToBeSplit.insert(data.equalityIndex, true);
            equalities.removeAt(i);
            if (i > 0) {
                i--; // reexamine previous equality
            }
        } else {
            i++;
        }
    }

    QList<Diff> newDiffList;
    for (int i = 0; i < diffList.count(); i++) {
        const Diff &diff = diffList.at(i);
        if (equalitiesToBeSplit.contains(i)) {
            newDiffList.append(Diff(Diff::Delete, diff.text));
            newDiffList.append(Diff(Diff::Insert, diff.text));
        } else {
            newDiffList.append(diff);
        }
    }

    return cleanupOverlaps(cleanupSemanticsLossless(merge(newDiffList)));
}

QList<Diff> Differ::cleanupSemanticsLossless(const QList<Diff> &diffList)
{
    if (diffList.count() < 3) // we need at least 3 items
        return diffList;

    QList<Diff> newDiffList;
    Diff prevDiff = diffList.at(0);
    Diff thisDiff = diffList.at(1);
    Diff nextDiff = diffList.at(2);
    int i = 2;
    while (i < diffList.count()) {
        if (prevDiff.command == Diff::Equal
                && nextDiff.command == Diff::Equal) {

            // Single edit surrounded by equalities
            QString equality1 = prevDiff.text;
            QString edit = thisDiff.text;
            QString equality2 = nextDiff.text;

            // Shift the edit as far left as possible
            const int suffixCount = commonSuffix(equality1, edit);
            if (suffixCount) {
                const QString commonString = edit.mid(edit.count() - suffixCount);
                equality1 = equality1.left(equality1.count() - suffixCount);
                edit = commonString + edit.left(edit.count() - suffixCount);
                equality2 = commonString + equality2;
            }

            // Step char by char right, looking for the best score
            QString bestEquality1 = equality1;
            QString bestEdit = edit;
            QString bestEquality2 = equality2;
            int bestScore = cleanupSemanticsScore(equality1, edit)
                    + cleanupSemanticsScore(edit, equality2);

            while (!edit.isEmpty() && !equality2.isEmpty()
                   && edit[0] == equality2[0]) {
                equality1 += edit[0];
                edit = edit.mid(1) + equality2[0];
                equality2 = equality2.mid(1);
                const int score = cleanupSemanticsScore(equality1, edit)
                        + cleanupSemanticsScore(edit, equality2);
                if (score >= bestScore) {
                    bestEquality1 = equality1;
                    bestEdit = edit;
                    bestEquality2 = equality2;
                    bestScore = score;
                }
            }
            prevDiff.text = bestEquality1;
            thisDiff.text = bestEdit;
            nextDiff.text = bestEquality2;

            if (!bestEquality1.isEmpty()) {
                newDiffList.append(prevDiff); // append modified equality1
            }
            if (bestEquality2.isEmpty()) {
                i++;
                if (i < diffList.count())
                    nextDiff = diffList.at(i); // omit equality2
            }
        } else {
            newDiffList.append(prevDiff); // append prevDiff
        }
        prevDiff = thisDiff;
        thisDiff = nextDiff;
        i++;
        if (i < diffList.count())
            nextDiff = diffList.at(i);
    }
    newDiffList.append(prevDiff);
    if (i == diffList.count())
        newDiffList.append(thisDiff);
    return newDiffList;
}

} // namespace DiffEditor
