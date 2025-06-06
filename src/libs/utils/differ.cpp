// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

/*
The main algorithm "diffMyers()" is based on "An O(ND) Difference Algorithm
and Its Variations" by Eugene W. Myers: http://www.xmailserver.org/diff2.pdf

Preprocessing and postprocessing functions inspired by "Diff Strategies"
publication by Neil Fraser: http://neil.fraser.name/writing/diff/
*/

#include "differ.h"

#include "utilstr.h"

#include <QMap>
#include <QPair>
#include <QRegularExpression>
#include <QStringList>

namespace Utils {

static int commonPrefix(const QString &text1, const QString &text2)
{
    int i = 0;
    const int text1Count = text1.size();
    const int text2Count = text2.size();
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
    const int text1Count = text1.size();
    const int text2Count = text2.size();
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
    const int text1Count = text1.size();
    const int text2Count = text2.size();
    const int maxCount = qMin(text1Count, text2Count);
    while (i < maxCount) {
        if (QStringView(text1).mid(text1Count - maxCount + i) == QStringView(text2).left(maxCount - i))
            return maxCount - i;
        i++;
    }
    return 0;
}

static QList<Diff> decode(const QList<Diff> &diffList, const QStringList &lines)
{
    QList<Diff> newDiffList;
    newDiffList.reserve(diffList.size());
    for (const Diff &diff : diffList) {
        QString text;
        for (QChar c : diff.text) {
            const int idx = static_cast<ushort>(c.unicode());
            text += lines.value(idx);
        }
        newDiffList.append({diff.command, text});
    }
    return newDiffList;
}

static QList<Diff> squashEqualities(const QList<Diff> &diffList)
{
    if (diffList.size() < 3) // we need at least 3 items
        return diffList;

    QList<Diff> newDiffList;
    Diff prevDiff = diffList.at(0);
    Diff thisDiff = diffList.at(1);
    Diff nextDiff = diffList.at(2);
    int i = 2;
    while (i < diffList.size()) {
        if (prevDiff.command == Diff::Equal
                && nextDiff.command == Diff::Equal) {
            if (thisDiff.text.endsWith(prevDiff.text)) {
                thisDiff.text = prevDiff.text
                        + thisDiff.text.left(thisDiff.text.size()
                        - prevDiff.text.size());
                nextDiff.text = prevDiff.text + nextDiff.text;
            } else if (thisDiff.text.startsWith(nextDiff.text)) {
                prevDiff.text += nextDiff.text;
                thisDiff.text = thisDiff.text.mid(nextDiff.text.size())
                        + nextDiff.text;
                i++;
                if (i < diffList.size())
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
        if (i < diffList.size())
            nextDiff = diffList.at(i);
    }
    newDiffList.append(prevDiff);
    if (i == diffList.size())
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
    while (i < diffList.size()) {
        Diff thisDiff = diffList.at(i);
        Diff nextDiff = i < diffList.size() - 1
                ? diffList.at(i + 1)
                : Diff(Diff::Equal);
        if (thisDiff.command == Diff::Delete
                && nextDiff.command == Diff::Insert) {
            const int delInsOverlap = commonOverlap(thisDiff.text, nextDiff.text);
            const int insDelOverlap = commonOverlap(nextDiff.text, thisDiff.text);
            if (delInsOverlap >= insDelOverlap) {
                if (delInsOverlap > thisDiff.text.size() / 2
                        || delInsOverlap > nextDiff.text.size() / 2) {
                    thisDiff.text = thisDiff.text.left(thisDiff.text.size() - delInsOverlap);
                    const Diff equality(Diff::Equal, nextDiff.text.left(delInsOverlap));
                    nextDiff.text = nextDiff.text.mid(delInsOverlap);
                    newDiffList.append(thisDiff);
                    newDiffList.append(equality);
                    newDiffList.append(nextDiff);
                } else {
                    newDiffList.append(thisDiff);
                    newDiffList.append(nextDiff);
                }
            } else {
                if (insDelOverlap > thisDiff.text.size() / 2
                        || insDelOverlap > nextDiff.text.size() / 2) {
                    nextDiff.text = nextDiff.text.left(nextDiff.text.size() - insDelOverlap);
                    const Diff equality(Diff::Equal, thisDiff.text.left(insDelOverlap));
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
    static const QRegularExpression blankLineEnd("\\n\\r?\\n$");
    static const QRegularExpression blankLineStart("^\\r?\\n\\r?\\n");
    static const QRegularExpression sentenceEnd("\\. $");

    if (text1.isEmpty() || text2.isEmpty()) // Edges
        return 6;

    const QChar char1 = text1[text1.size() - 1];
    const QChar char2 = text2[0];
    const bool nonAlphaNumeric1 = !char1.isLetterOrNumber();
    const bool nonAlphaNumeric2 = !char2.isLetterOrNumber();
    const bool whitespace1 = nonAlphaNumeric1 && char1.isSpace();
    const bool whitespace2 = nonAlphaNumeric2 && char2.isSpace();
    const bool lineBreak1 = whitespace1 && char1.category() == QChar::Other_Control;
    const bool lineBreak2 = whitespace2 && char2.category() == QChar::Other_Control;
    const bool blankLine1 = lineBreak1 && blankLineEnd.match(text1).hasMatch();
    const bool blankLine2 = lineBreak2 && blankLineStart.match(text2).hasMatch();

    if (blankLine1 || blankLine2) // Blank lines
      return 5;
    if (lineBreak1 || lineBreak2) // Line breaks
      return 4;
    if (sentenceEnd.match(text1).hasMatch()) // End of sentence
      return 3;
    if (whitespace1 || whitespace2) // Whitespaces
      return 2;
    if (nonAlphaNumeric1 || nonAlphaNumeric2) // Non-alphanumerics
      return 1;

    return 0;
}

static bool isWhitespace(const QChar &c)
{
    return c == ' ' || c == '\t';
}

static bool isNewLine(const QChar &c)
{
    return c == '\n';
}

/*
 * Splits the diffList into left and right diff lists.
 * The left diff list contains the original (insertions removed),
 * while the right diff list contains the destination (deletions removed).
 */
void Differ::splitDiffList(const QList<Diff> &diffList,
                          QList<Diff> *leftDiffList,
                          QList<Diff> *rightDiffList)
{
    if (!leftDiffList || !rightDiffList)
        return;

    leftDiffList->clear();
    rightDiffList->clear();

    for (const Diff &diff : diffList) {
        if (diff.command != Diff::Delete)
            rightDiffList->append(diff);
        if (diff.command != Diff::Insert)
            leftDiffList->append(diff);
    }
}

/*
 * Prerequisites:
 * input should be only the left or right list of diffs, not a mix of both.
 *
 * Moves whitespace characters from Diff::Delete or Diff::Insert into
 * surrounding Diff::Equal, if possible.
 * It may happen, that some Diff::Delete of Diff::Insert will disappear.
 */
QList<Diff> Differ::moveWhitespaceIntoEqualities(const QList<Diff> &input)
{
    QList<Diff> output = input;

    for (int i = 0; i < output.size(); i++) {
        Diff diff = output[i];

        if (diff.command != Diff::Equal) {
            if (i > 0) { // check previous equality
                Diff &previousDiff = output[i - 1];
                const int previousDiffCount = previousDiff.text.size();
                if (previousDiff.command == Diff::Equal
                        && previousDiffCount
                        && isWhitespace(previousDiff.text.at(previousDiffCount - 1))) {
                    // previous diff ends with whitespace
                    int j = 0;
                    while (j < diff.text.size()) {
                        if (!isWhitespace(diff.text.at(j)))
                            break;
                        ++j;
                    }
                    if (j > 0) {
                        // diff starts with j whitespaces, move them to the previous diff
                        previousDiff.text.append(diff.text.left(j));
                        diff.text = diff.text.mid(j);
                    }
                }
            }
            if (i < output.size() - 1) { // check next equality
                const int diffCount = diff.text.size();
                Diff &nextDiff = output[i + 1];
                const int nextDiffCount = nextDiff.text.size();
                if (nextDiff.command == Diff::Equal
                        && nextDiffCount
                        && (isWhitespace(nextDiff.text.at(0)) || isNewLine(nextDiff.text.at(0)))) {
                    // next diff starts with whitespace or with newline
                    int j = 0;
                    while (j < diffCount) {
                        if (!isWhitespace(diff.text.at(diffCount - j - 1)))
                            break;
                        ++j;
                    }
                    if (j > 0) {
                        // diff ends with j whitespaces, move them to the next diff
                        nextDiff.text.prepend(diff.text.mid(diffCount - j));
                        diff.text = diff.text.left(diffCount - j);
                    }
                }
            }
            // remove diff if empty
            if (diff.text.isEmpty()) {
                output.removeAt(i);
                --i;
            } else {
                output[i] = diff;
            }
        }
    }
    return output;
}

/*
 * Encodes any sentence of whitespaces with one simple space.
 *
 * The mapping is returned by codeMap argument, which contains
 * the position in output string of encoded whitespace character
 * and it's corresponding original sequence of whitespace characters.
 *
 * The returned string contains encoded version of the input string.
 */
static QString encodeReducedWhitespace(const QString &input,
                                       QMap<int, QString> *codeMap)
{
    QString output;
    if (!codeMap)
        return output;

    int inputIndex = 0;
    int outputIndex = 0;
    while (inputIndex < input.size()) {
        QChar c = input.at(inputIndex);

        if (isWhitespace(c)) {
            output.append(' ');
            codeMap->insert(outputIndex, QString(c));
            ++inputIndex;

            while (inputIndex < input.size()) {
                QChar reducedChar = input.at(inputIndex);

                if (!isWhitespace(reducedChar))
                    break;

                (*codeMap)[outputIndex].append(reducedChar);
                ++inputIndex;
            }
        } else {
            output.append(c);
            ++inputIndex;
        }
        ++outputIndex;
    }
    return output;
}

/*
 * This is corresponding function to encodeReducedWhitespace().
 *
 * The input argument contains version encoded with codeMap,
 * the returned value contains decoded diff list.
 */
static QList<Diff> decodeReducedWhitespace(const QList<Diff> &input,
                                           const QMap<int, QString> &codeMap)
{
    QList<Diff> output;

    int counter = 0;
    auto it = codeMap.constBegin();
    const auto itEnd = codeMap.constEnd();
    for (Diff diff : input) {
        const int diffCount = diff.text.size();
        while ((it != itEnd) && (it.key() < counter + diffCount)) {
            const int reversePosition = diffCount + counter - it.key();
            const int updatedDiffCount = diff.text.size();
            diff.text.replace(updatedDiffCount - reversePosition, 1, it.value());
            ++it;
        }
        output.append(diff);
        counter += diffCount;
    }
    return output;
}

/*
 * Prerequisites:
 * leftDiff is expected to be Diff::Delete and rightDiff is expected to be Diff::Insert.
 *
 * Encode any sentence of whitespaces with simple space (inside leftDiff and rightDiff),
 * diff without cleanup,
 * split diffs,
 * decode.
 */
void Differ::diffWithWhitespaceReduced(const QString &leftInput,
                                       const QString &rightInput,
                                       QList<Diff> *leftOutput,
                                       QList<Diff> *rightOutput)
{
    if (!leftOutput || !rightOutput)
        return;

    leftOutput->clear();
    rightOutput->clear();

    QMap<int, QString> leftCodeMap;
    QMap<int, QString> rightCodeMap;
    const QString &leftString = encodeReducedWhitespace(leftInput, &leftCodeMap);
    const QString &rightString = encodeReducedWhitespace(rightInput, &rightCodeMap);

    Differ differ;
    QList<Diff> diffList = differ.diff(leftString, rightString);

    QList<Diff> leftDiffList;
    QList<Diff> rightDiffList;
    Differ::splitDiffList(diffList, &leftDiffList, &rightDiffList);

    *leftOutput = decodeReducedWhitespace(leftDiffList, leftCodeMap);
    *rightOutput = decodeReducedWhitespace(rightDiffList, rightCodeMap);
}

/*
 * Prerequisites:
 * leftDiff is expected to be Diff::Delete and rightDiff is expected to be Diff::Insert.
 *
 * Encode any sentence of whitespaces with simple space (inside leftDiff and rightDiff),
 * diff without cleanup,
 * split diffs,
 * decode.
 */
void Differ::unifiedDiffWithWhitespaceReduced(const QString &leftInput,
                                       const QString &rightInput,
                                       QList<Diff> *leftOutput,
                                       QList<Diff> *rightOutput)
{
    if (!leftOutput || !rightOutput)
        return;

    leftOutput->clear();
    rightOutput->clear();

    QMap<int, QString> leftCodeMap;
    QMap<int, QString> rightCodeMap;
    const QString &leftString = encodeReducedWhitespace(leftInput, &leftCodeMap);
    const QString &rightString = encodeReducedWhitespace(rightInput, &rightCodeMap);

    Differ differ;
    const QList<Diff> &diffList = differ.unifiedDiff(leftString, rightString);

    QList<Diff> leftDiffList;
    QList<Diff> rightDiffList;
    Differ::splitDiffList(diffList, &leftDiffList, &rightDiffList);

    *leftOutput = decodeReducedWhitespace(leftDiffList, leftCodeMap);
    *rightOutput = decodeReducedWhitespace(rightDiffList, rightCodeMap);
}

/*
 * Prerequisites:
 * leftEquality and rightEquality needs to be equal. They may differ only with
 * whitespaces character (count and kind).
 *
 * Replaces any corresponding sentence of whitespaces inside left and right equality
 * with space characters. The number of space characters inside
 * replaced sequence depends on the longest sequence of whitespace characters
 * either in left or right equlity.
 *
 * E.g., when:
 * leftEquality:   "a   b     c" (3 whitespace characters, 5 whitespace characters)
 * rightEquality:  "a /tb  /t    c" (2 whitespace characters, 7 whitespace characters)
 * then:
 * returned value: "a   b       c" (3 space characters, 7 space characters)
 *
 * The returned code maps contains the info about the encoding done.
 * The key on the map is the position of encoding inside the output string,
 * and the value, which is a pair of int and string,
 * describes how many characters were encoded in the output string
 * and what was the original whitespace sequence in the original
 * For the above example it would be:
 *
 * leftCodeMap:  <1, <3, "   "> >
 *               <5, <7, "     "> >
 * rightCodeMap: <1, <3, " /t"> >
 *               <5, <7, "  /t    "> >
 *
 */
static QString encodeExpandedWhitespace(const QString &leftEquality,
                                        const QString &rightEquality,
                                        QMap<int, QPair<int, QString>> *leftCodeMap,
                                        QMap<int, QPair<int, QString>> *rightCodeMap,
                                        bool *ok)
{
    if (ok)
        *ok = false;

    if (!leftCodeMap || !rightCodeMap)
        return {};

    leftCodeMap->clear();
    rightCodeMap->clear();
    QString output;

    const int leftCount = leftEquality.size();
    const int rightCount = rightEquality.size();
    int leftIndex = 0;
    int rightIndex = 0;
    while (leftIndex < leftCount && rightIndex < rightCount) {
        QString leftWhitespaces;
        QString rightWhitespaces;
        while (leftIndex < leftCount && isWhitespace(leftEquality.at(leftIndex))) {
            leftWhitespaces.append(leftEquality.at(leftIndex));
            ++leftIndex;
        }
        while (rightIndex < rightCount && isWhitespace(rightEquality.at(rightIndex))) {
            rightWhitespaces.append(rightEquality.at(rightIndex));
            ++rightIndex;
        }

        if (leftIndex < leftCount && rightIndex < rightCount) {
            if (leftEquality.at(leftIndex) != rightEquality.at(rightIndex))
                return {}; // equalities broken

        } else if (leftIndex == leftCount && rightIndex == rightCount) {
            ; // do nothing, the last iteration
        } else {
            return {}; // equalities broken
        }

        if (leftWhitespaces.isEmpty() != rightWhitespaces.isEmpty()) {
            // there must be at least 1 corresponding whitespace, equalities broken
            return {};
        }

        if (!leftWhitespaces.isEmpty() && !rightWhitespaces.isEmpty()) {
            const int replacementPosition = output.size();
            const int replacementSize = qMax(leftWhitespaces.size(), rightWhitespaces.size());
            const QString replacement(replacementSize, ' ');
            leftCodeMap->insert(replacementPosition, {replacementSize, leftWhitespaces});
            rightCodeMap->insert(replacementPosition, {replacementSize, rightWhitespaces});
            output.append(replacement);
        }

        if (leftIndex < leftCount)
            output.append(leftEquality.at(leftIndex)); // add common character

        ++leftIndex;
        ++rightIndex;
    }

    if (ok)
        *ok = true;

    return output;
}

/*
 * This is corresponding function to encodeExpandedWhitespace().
 *
 * The input argument contains version encoded with codeMap,
 * the returned value contains decoded diff list.
 */
static QList<Diff> decodeExpandedWhitespace(const QList<Diff> &input,
                                            const QMap<int, QPair<int, QString>> &codeMap,
                                            bool *ok)
{
    if (ok)
        *ok = false;

    QList<Diff> output;

    int counter = 0;
    auto it = codeMap.constBegin();
    const auto itEnd = codeMap.constEnd();
    for (Diff diff : input) {
        const int diffCount = diff.text.size();
        while ((it != itEnd) && (it.key() < counter + diffCount)) {
            const int replacementSize = it.value().first;
            const int reversePosition = diffCount + counter - it.key();
            if (reversePosition < replacementSize)
                return {}; // replacement exceeds one Diff
            const QString replacement = it.value().second;
            const int updatedDiffCount = diff.text.size();
            diff.text.replace(updatedDiffCount - reversePosition,
                              replacementSize, replacement);
            ++it;
        }
        output.append(diff);
        counter += diffCount;
    }

    if (ok)
        *ok = true;

    return output;
}

/*
 * Prerequisites:
 * leftInput and rightInput should contain the same number of equalities,
 * equalities should differ only in whitespaces.
 *
 * Encodes any sentence of whitespace characters in equalities only
 * with the maximal number of corresponding whitespace characters
 * (inside leftInput and rightInput), so that the leftInput and rightInput
 * can be merged together again,
 * diff merged sequence with cleanup,
 * decode.
 */
static bool diffWithWhitespaceExpandedInEqualities(const QList<Diff> &leftInput,
                                                   const QList<Diff> &rightInput,
                                                   QList<Diff> *leftOutput,
                                                   QList<Diff> *rightOutput)
{
    if (!leftOutput || !rightOutput)
        return false;

    leftOutput->clear();
    rightOutput->clear();

    const int leftCount = leftInput.size();
    const int rightCount = rightInput.size();
    int l = 0;
    int r = 0;

    QString leftText;
    QString rightText;

    QMap<int, QPair<int, QString>> commonLeftCodeMap;
    QMap<int, QPair<int, QString>> commonRightCodeMap;

    while (l <= leftCount && r <= rightCount) {
        const Diff leftDiff = l < leftCount ? leftInput.at(l) : Diff(Diff::Equal);
        const Diff rightDiff = r < rightCount ? rightInput.at(r) : Diff(Diff::Equal);

        if (leftDiff.command == Diff::Equal && rightDiff.command == Diff::Equal) {
            QMap<int, QPair<int, QString>> leftCodeMap;
            QMap<int, QPair<int, QString>> rightCodeMap;

            bool ok = false;
            const QString &commonEquality = encodeExpandedWhitespace(leftDiff.text,
                                          rightDiff.text,
                                          &leftCodeMap,
                                          &rightCodeMap,
                                          &ok);
            if (!ok)
                return false;

            // join code map positions with common maps
            for (auto it = leftCodeMap.cbegin(), end = leftCodeMap.cend(); it != end; ++it)
                commonLeftCodeMap.insert(leftText.size() + it.key(), it.value());

            for (auto it = rightCodeMap.cbegin(), end = rightCodeMap.cend(); it != end; ++it)
                commonRightCodeMap.insert(rightText.size() + it.key(), it.value());

            leftText.append(commonEquality);
            rightText.append(commonEquality);

            ++l;
            ++r;
        }

        if (leftDiff.command != Diff::Equal) {
            leftText.append(leftDiff.text);
            ++l;
        }
        if (rightDiff.command != Diff::Equal) {
            rightText.append(rightDiff.text);
            ++r;
        }
    }

    Differ differ;
    const QList<Diff> &diffList = Differ::cleanupSemantics(
                differ.diff(leftText, rightText));

    QList<Diff> leftDiffList;
    QList<Diff> rightDiffList;
    Differ::splitDiffList(diffList, &leftDiffList, &rightDiffList);

    leftDiffList = Differ::moveWhitespaceIntoEqualities(leftDiffList);
    rightDiffList = Differ::moveWhitespaceIntoEqualities(rightDiffList);

    bool ok = false;
    *leftOutput = decodeExpandedWhitespace(leftDiffList,
                                           commonLeftCodeMap, &ok);
    if (!ok)
        return false;
    *rightOutput = decodeExpandedWhitespace(rightDiffList,
                                            commonRightCodeMap, &ok);
    return ok;
}

static void appendWithEqualitiesSquashed(const QList<Diff> &leftInput,
                             const QList<Diff> &rightInput,
                             QList<Diff> *leftOutput,
                             QList<Diff> *rightOutput)
{
    if (!leftInput.isEmpty()
            && !rightInput.isEmpty()
            && !leftOutput->isEmpty()
            && !rightOutput->isEmpty()
            && leftInput.first().command == Diff::Equal
            && rightInput.first().command == Diff::Equal
            && leftOutput->last().command == Diff::Equal
            && rightOutput->last().command == Diff::Equal) {
        leftOutput->last().text += leftInput.first().text;
        rightOutput->last().text += rightInput.first().text;
        leftOutput->append(leftInput.mid(1));
        rightOutput->append(rightInput.mid(1));
        return;
    }
    leftOutput->append(leftInput);
    rightOutput->append(rightInput);
}

/*
 * Prerequisites:
 * leftInput cannot contain insertions, while right input cannot contain deletions.
 * The number of equalities on leftInput and rightInput lists should be the same.
 * Deletions and insertions need to be merged.
 *
 * For each corresponding insertion / deletion pair:
 * - diffWithWhitespaceReduced(): rediff them separately with whitespace reduced
 *                            (new equalities may appear)
 * - moveWhitespaceIntoEqualities(): move whitespace into new equalities
 * - diffWithWhitespaceExpandedInEqualities(): expand whitespace inside new
 *                            equalities only and rediff with cleanup
 */
void Differ::ignoreWhitespaceBetweenEqualities(const QList<Diff> &leftInput,
                                  const QList<Diff> &rightInput,
                                  QList<Diff> *leftOutput,
                                  QList<Diff> *rightOutput)
{
    if (!leftOutput || !rightOutput)
        return;

    leftOutput->clear();
    rightOutput->clear();

    const int leftCount = leftInput.size();
    const int rightCount = rightInput.size();
    int l = 0;
    int r = 0;

    while (l <= leftCount && r <= rightCount) {
        const Diff leftDiff = l < leftCount
                ? leftInput.at(l)
                : Diff(Diff::Equal);
        const Diff rightDiff = r < rightCount
                ? rightInput.at(r)
                : Diff(Diff::Equal);

        if (leftDiff.command == Diff::Equal && rightDiff.command == Diff::Equal) {
            const Diff previousLeftDiff = l > 0 ? leftInput.at(l - 1) : Diff(Diff::Equal);
            const Diff previousRightDiff = r > 0 ? rightInput.at(r - 1) : Diff(Diff::Equal);

            if (previousLeftDiff.command == Diff::Delete
                    && previousRightDiff.command == Diff::Insert) {
                QList<Diff> outputLeftDiffList;
                QList<Diff> outputRightDiffList;

                QList<Diff> reducedLeftDiffList;
                QList<Diff> reducedRightDiffList;
                diffWithWhitespaceReduced(previousLeftDiff.text,
                                           previousRightDiff.text,
                                           &reducedLeftDiffList,
                                           &reducedRightDiffList);

                reducedLeftDiffList = moveWhitespaceIntoEqualities(reducedLeftDiffList);
                reducedRightDiffList = moveWhitespaceIntoEqualities(reducedRightDiffList);

                QList<Diff> cleanedLeftDiffList;
                QList<Diff> cleanedRightDiffList;
                if (diffWithWhitespaceExpandedInEqualities(reducedLeftDiffList,
                                                           reducedRightDiffList,
                                                           &cleanedLeftDiffList,
                                                           &cleanedRightDiffList)) {
                    outputLeftDiffList = cleanedLeftDiffList;
                    outputRightDiffList = cleanedRightDiffList;
                } else {
                    outputLeftDiffList = reducedLeftDiffList;
                    outputRightDiffList = reducedRightDiffList;
                }

                appendWithEqualitiesSquashed(outputLeftDiffList,
                                             outputRightDiffList,
                                             leftOutput,
                                             rightOutput);
            } else if (previousLeftDiff.command == Diff::Delete) {
                leftOutput->append(previousLeftDiff);
            } else if (previousRightDiff.command == Diff::Insert) {
                rightOutput->append(previousRightDiff);
            }

            QList<Diff> leftEquality;
            QList<Diff> rightEquality;
            if (l < leftCount)
                leftEquality.append(leftDiff);
            if (r < rightCount)
                rightEquality.append(rightDiff);

            appendWithEqualitiesSquashed(leftEquality,
                                         rightEquality,
                                         leftOutput,
                                         rightOutput);

            ++l;
            ++r;
        }

        if (leftDiff.command != Diff::Equal)
            ++l;
        if (rightDiff.command != Diff::Equal)
            ++r;
    }
}

/*
 * Prerequisites:
 * leftInput cannot contain insertions, while right input cannot contain deletions.
 * The number of equalities on leftInput and rightInput lists should be the same.
 * Deletions and insertions need to be merged.
 *
 * For each corresponding insertion / deletion pair do just diff and merge equalities
 */
void Differ::diffBetweenEqualities(const QList<Diff> &leftInput,
                                   const QList<Diff> &rightInput,
                                   QList<Diff> *leftOutput,
                                   QList<Diff> *rightOutput)
{
    if (!leftOutput || !rightOutput)
        return;

    leftOutput->clear();
    rightOutput->clear();

    const int leftCount = leftInput.size();
    const int rightCount = rightInput.size();
    int l = 0;
    int r = 0;

    while (l <= leftCount && r <= rightCount) {
        const Diff leftDiff = l < leftCount
                ? leftInput.at(l)
                : Diff(Diff::Equal);
        const Diff rightDiff = r < rightCount
                ? rightInput.at(r)
                : Diff(Diff::Equal);

        if (leftDiff.command == Diff::Equal && rightDiff.command == Diff::Equal) {
            const Diff previousLeftDiff = l > 0 ? leftInput.at(l - 1) : Diff(Diff::Equal);
            const Diff previousRightDiff = r > 0 ? rightInput.at(r - 1) : Diff(Diff::Equal);

            if (previousLeftDiff.command == Diff::Delete
                    && previousRightDiff.command == Diff::Insert) {
                Differ differ;
                differ.setDiffMode(Differ::CharMode);
                const QList<Diff> commonOutput = cleanupSemantics(
                            differ.diff(previousLeftDiff.text, previousRightDiff.text));

                QList<Diff> outputLeftDiffList;
                QList<Diff> outputRightDiffList;

                Differ::splitDiffList(commonOutput, &outputLeftDiffList,
                                      &outputRightDiffList);

                appendWithEqualitiesSquashed(outputLeftDiffList,
                                             outputRightDiffList,
                                             leftOutput,
                                             rightOutput);
            } else if (previousLeftDiff.command == Diff::Delete) {
                leftOutput->append(previousLeftDiff);
            } else if (previousRightDiff.command == Diff::Insert) {
                rightOutput->append(previousRightDiff);
            }

            QList<Diff> leftEquality;
            QList<Diff> rightEquality;
            if (l < leftCount)
                leftEquality.append(leftDiff);
            if (r < rightCount)
                rightEquality.append(rightDiff);

            appendWithEqualitiesSquashed(leftEquality,
                                         rightEquality,
                                         leftOutput,
                                         rightOutput);

            ++l;
            ++r;
        }

        if (leftDiff.command != Diff::Equal)
            ++l;
        if (rightDiff.command != Diff::Equal)
            ++r;
    }
}

///////////////

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
        return ::Utils::Tr::tr("Delete");
    else if (com == Insert)
        return ::Utils::Tr::tr("Insert");
    return ::Utils::Tr::tr("Equal");
}

QString Diff::toString() const
{
    QString prettyText = text;
    // Replace linebreaks with pretty char
    prettyText.replace('\n', '\xb6');
    return commandString(command) + " \"" + prettyText + "\"";
}

///////////////

Differ::Differ(const std::optional<QFuture<void>> &future)
    : m_future(future)
{
}

QList<Diff> Differ::diff(const QString &text1, const QString &text2)
{
    m_currentDiffMode = m_diffMode;
    return merge(preprocess1AndDiff(text1, text2));
}

QList<Diff> Differ::unifiedDiff(const QString &text1, const QString &text2)
{
    QString encodedText1;
    QString encodedText2;
    QStringList subtexts = encode(text1, text2, &encodedText1, &encodedText2);

    const DiffMode diffMode = m_currentDiffMode;
    m_currentDiffMode = CharMode;

    // Each different subtext is a separate symbol
    // process these symbols as text with bigger alphabet
    QList<Diff> diffList = merge(preprocess1AndDiff(encodedText1, encodedText2));

    diffList = decode(diffList, subtexts);
    m_currentDiffMode = diffMode;
    return diffList;
}

void Differ::setDiffMode(Differ::DiffMode mode)
{
    m_diffMode = mode;
}

Differ::DiffMode Differ::diffMode() const
{
    return m_diffMode;
}

QList<Diff> Differ::preprocess1AndDiff(const QString &text1, const QString &text2)
{
    if (text1.isNull() && text2.isNull())
        return {};

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
        newText1 = newText1.left(newText1.size() - suffixCount);
        newText2 = newText2.left(newText2.size() - suffixCount);
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

    if (text1.size() != text2.size()) {
        const QString longtext = text1.size() > text2.size() ? text1 : text2;
        const QString shorttext = text1.size() > text2.size() ? text2 : text1;
        const int i = longtext.indexOf(shorttext);
        if (i != -1) {
            const Diff::Command command = (text1.size() > text2.size())
                    ? Diff::Delete : Diff::Insert;
            diffList.append(Diff(command, longtext.left(i)));
            diffList.append(Diff(Diff::Equal, shorttext));
            diffList.append(Diff(command, longtext.mid(i + shorttext.size())));
            return diffList;
        }

        if (shorttext.size() == 1) {
            diffList.append(Diff(Diff::Delete, text1));
            diffList.append(Diff(Diff::Insert, text2));
            return diffList;
        }
    }

    if (m_currentDiffMode != Differ::CharMode && text1.size() > 80 && text2.size() > 80)
        return diffNonCharMode(text1, text2);

    return diffMyers(text1, text2);
}

QList<Diff> Differ::diffMyers(const QString &text1, const QString &text2)
{
    const int n = text1.size();
    const int m = text2.size();
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
        if (m_future && m_future->isCanceled()) {
            delete [] forwardV;
            delete [] reverseV;
            return {};
        }
        // going forward
        for (int k = qMax(-d, kMinForward + qAbs(d + kMinForward) % 2);
             k <= qMin(d, kMaxForward - qAbs(d + kMaxForward) % 2);
             k = k + 2) {
            int x;
            if (k == -d || (k < d && forwardV[k + vShift - 1] < forwardV[k + vShift + 1]))
                x = forwardV[k + vShift + 1]; // copy vertically from diagonal k + 1, y increases, y may exceed the graph
            else
                x = forwardV[k + vShift - 1] + 1; // copy horizontally from diagonal k - 1, x increases, x may exceed the graph
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
            if (k == -d || (k < d && reverseV[k + vShift - 1] < reverseV[k + vShift + 1]))
                x = reverseV[k + vShift + 1];
            else
                x = reverseV[k + vShift - 1] + 1;
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

    const QList<Diff> &diffList1 = preprocess1AndDiff(text11, text21);
    const QList<Diff> &diffList2 = preprocess1AndDiff(text12, text22);
    return diffList1 + diffList2;
}

QList<Diff> Differ::diffNonCharMode(const QString &text1, const QString &text2)
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
    for (int i = 0; i <= diffList.size(); i++) {
        if (m_future && m_future->isCanceled()) {
            m_currentDiffMode = diffMode;
            return {};
        }
        const Diff diffItem = i < diffList.size()
                  ? diffList.at(i)
                  : Diff(Diff::Equal); // dummy, ensure we process to the end
                                       // even when diffList doesn't end with equality
        if (diffItem.command == Diff::Delete) {
            lastDelete += diffItem.text;
        } else if (diffItem.command == Diff::Insert) {
            lastInsert += diffItem.text;
        } else { // Diff::Equal
            if (!(lastDelete.isEmpty() && lastInsert.isEmpty())) {
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
    QStringList lines{{}}; // don't use code: 0
    QMap<QString, int> lineToCode;

    *encodedText1 = encode(text1, &lines, &lineToCode);
    *encodedText2 = encode(text2, &lines, &lineToCode);

    return lines;
}

int Differ::findSubtextEnd(const QString &text,
                                  int subtextStart)
{
    if (m_currentDiffMode == Differ::LineMode) {
        int subtextEnd = text.indexOf('\n', subtextStart);
        if (subtextEnd == -1)
            subtextEnd = text.size() - 1;
        return ++subtextEnd;
    } else if (m_currentDiffMode == Differ::WordMode) {
        if (!text.at(subtextStart).isLetter())
            return subtextStart + 1;
        int i = subtextStart + 1;

        const int count = text.size();
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
    while (subtextEnd < text.size()) {
        subtextEnd = findSubtextEnd(text, subtextStart);
        const QString line = text.mid(subtextStart, subtextEnd - subtextStart);
        subtextStart = subtextEnd;

        if (lineToCode->contains(line)) {
            codes += QChar(static_cast<ushort>(lineToCode->value(line)));
        } else {
            lines->append(line);
            lineToCode->insert(line, lines->size() - 1);
            codes += QChar(static_cast<ushort>(lines->size() - 1));
        }
    }
    return codes;
}

QList<Diff> Differ::merge(const QList<Diff> &diffList)
{
    QString lastDelete;
    QString lastInsert;
    QList<Diff> newDiffList;
    for (int i = 0; i <= diffList.size(); i++) {
        Diff diff = i < diffList.size()
                  ? diffList.at(i)
                  : Diff(Diff::Equal); // dummy, ensure we process to the end
                                       // even when diffList doesn't end with equality
        if (diff.command == Diff::Delete) {
            lastDelete += diff.text;
        } else if (diff.command == Diff::Insert) {
            lastInsert += diff.text;
        } else { // Diff::Equal
            if (!(lastDelete.isEmpty() && lastInsert.isEmpty())) {

                // common prefix
                const int prefixCount = commonPrefix(lastDelete, lastInsert);
                if (prefixCount) {
                    const QString prefix = lastDelete.left(prefixCount);
                    lastDelete = lastDelete.mid(prefixCount);
                    lastInsert = lastInsert.mid(prefixCount);

                    if (!newDiffList.isEmpty()
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
                    lastDelete = lastDelete.left(lastDelete.size() - suffixCount);
                    lastInsert = lastInsert.left(lastInsert.size() - suffixCount);

                    diff.text.prepend(suffix);
                }

                // append delete / insert / equal
                if (!lastDelete.isEmpty())
                    newDiffList.append(Diff(Diff::Delete, lastDelete));
                if (!lastInsert.isEmpty())
                    newDiffList.append(Diff(Diff::Insert, lastInsert));
                if (!diff.text.isEmpty())
                    newDiffList.append(diff);
                lastDelete.clear();
                lastInsert.clear();
            } else { // join with last equal diff
                if (!newDiffList.isEmpty()
                        && newDiffList.last().command == Diff::Equal) {
                    newDiffList.last().text += diff.text;
                } else {
                    if (!diff.text.isEmpty())
                        newDiffList.append(diff);
                }
            }
        }
    }

    QList<Diff> squashedDiffList = squashEqualities(newDiffList);
    if (squashedDiffList.size() != newDiffList.size())
        return merge(squashedDiffList);

    return squashedDiffList;
}

QList<Diff> Differ::cleanupSemantics(const QList<Diff> &diffList)
{
    struct EqualityData
    {
        int equalityIndex = 0;
        int textCount = 0;
        int deletesBefore = 0;
        int insertsBefore = 0;
        int deletesAfter = 0;
        int insertsAfter = 0;
    };
    int deletes = 0;
    int inserts = 0;
    // equality index, equality data
    QList<EqualityData> equalities;
    for (int i = 0; i <= diffList.size(); i++) {
        const Diff diff = i < diffList.size()
                  ? diffList.at(i)
                  : Diff(Diff::Equal); // dummy, ensure we process to the end
                                       // even when diffList doesn't end with equality
        if (diff.command == Diff::Equal) {
            if (!equalities.isEmpty()) {
                EqualityData &previousData = equalities.last();
                previousData.deletesAfter = deletes;
                previousData.insertsAfter = inserts;
            }
            if (i < diffList.size()) { // don't insert dummy
                EqualityData data;
                data.equalityIndex = i;
                data.textCount = diff.text.size();
                data.deletesBefore = deletes;
                data.insertsBefore = inserts;
                equalities.append(data);
                deletes = 0;
                inserts = 0;
            }
        } else {
            if (diff.command == Diff::Delete)
                deletes += diff.text.size();
            else if (diff.command == Diff::Insert)
                inserts += diff.text.size();
        }
    }

    QMap<int, bool> equalitiesToBeSplit;
    int i = 0;
    while (i < equalities.size()) {
        const EqualityData &data = equalities.at(i);
        if (data.textCount <= qMax(data.deletesBefore, data.insertsBefore)
                && data.textCount <= qMax(data.deletesAfter, data.insertsAfter)) {
            if (i > 0) {
                EqualityData &previousData = equalities[i - 1];
                previousData.deletesAfter += data.textCount + data.deletesAfter;
                previousData.insertsAfter += data.textCount + data.insertsAfter;
            }
            if (i < equalities.size() - 1) {
                EqualityData &nextData = equalities[i + 1];
                nextData.deletesBefore += data.textCount + data.deletesBefore;
                nextData.insertsBefore += data.textCount + data.insertsBefore;
            }
            equalitiesToBeSplit.insert(data.equalityIndex, true);
            equalities.removeAt(i);
            if (i > 0)
                i--; // reexamine previous equality
        } else {
            i++;
        }
    }

    QList<Diff> newDiffList;
    for (int i = 0; i < diffList.size(); i++) {
        const Diff &diff = diffList.at(i);
        if (equalitiesToBeSplit.contains(i)) {
            newDiffList.append(Diff(Diff::Delete, diff.text));
            newDiffList.append(Diff(Diff::Insert, diff.text));
        } else {
            newDiffList.append(diff);
        }
    }

    return cleanupOverlaps(merge(cleanupSemanticsLossless(merge(newDiffList))));
}

QList<Diff> Differ::cleanupSemanticsLossless(const QList<Diff> &diffList)
{
    if (diffList.size() < 3) // we need at least 3 items
        return diffList;

    QList<Diff> newDiffList;
    Diff prevDiff = diffList.at(0);
    Diff thisDiff = diffList.at(1);
    Diff nextDiff = diffList.at(2);
    int i = 2;
    while (i < diffList.size()) {
        if (prevDiff.command == Diff::Equal
                && nextDiff.command == Diff::Equal) {

            // Single edit surrounded by equalities
            QString equality1 = prevDiff.text;
            QString edit = thisDiff.text;
            QString equality2 = nextDiff.text;

            // Shift the edit as far left as possible
            const int suffixCount = commonSuffix(equality1, edit);
            if (suffixCount) {
                const QString commonString = edit.mid(edit.size() - suffixCount);
                equality1 = equality1.left(equality1.size() - suffixCount);
                edit = commonString + edit.left(edit.size() - suffixCount);
                equality2 = commonString + equality2;
            }

            // Step char by char right, looking for the best score
            QString bestEquality1 = equality1;
            QString bestEdit = edit;
            QString bestEquality2 = equality2;
            int bestScore = cleanupSemanticsScore(equality1, edit)
                    + cleanupSemanticsScore(edit, equality2);

            while (!edit.isEmpty() && !equality2.isEmpty()
                   && edit.at(0) == equality2.at(0)) {
                equality1 += edit.at(0);
                edit = edit.mid(1) + equality2.at(0);
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

            if (!bestEquality1.isEmpty())
                newDiffList.append(prevDiff); // append modified equality1
            if (bestEquality2.isEmpty()) {
                i++;
                if (i < diffList.size())
                    nextDiff = diffList.at(i); // omit equality2
            }
        } else {
            newDiffList.append(prevDiff); // append prevDiff
        }
        prevDiff = thisDiff;
        thisDiff = nextDiff;
        i++;
        if (i < diffList.size())
            nextDiff = diffList.at(i);
    }
    newDiffList.append(prevDiff);
    if (i == diffList.size())
        newDiffList.append(thisDiff);
    return newDiffList;
}

} // namespace Utils
