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

#include "differ.h"

#include <QList>
#include <QStringList>
#include <QMap>

namespace DiffEditor {

Diff::Diff() :
    command(Diff::Equal)
{
}

Diff::Diff(Command com, const QString &txt) :
    command(com),
    text(txt)
{
}

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

                lastDelete = QString();
                lastInsert = QString();
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

QList<Diff> Differ::decode(const QList<Diff> &diffList,
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

QList<Diff> Differ::merge(const QList<Diff> &diffList)
{
    QString lastDelete;
    QString lastInsert;
    QList<Diff> newDiffList;
    for (int i = 0; i <= diffList.count(); i++) {
        const Diff diff = i < diffList.count()
                  ? diffList.at(i)
                  : Diff(Diff::Equal, QString()); // dummy, ensure we process to the end even when diffList doesn't end with equality
        if (diff.command == Diff::Delete) {
            lastDelete += diff.text;
        } else if (diff.command == Diff::Insert) {
            lastInsert += diff.text;
        } else { // Diff::Equal
            if (lastDelete.count() || lastInsert.count()) {
                // common prefix and suffix?

                if (lastDelete.count())
                    newDiffList.append(Diff(Diff::Delete, lastDelete));
                if (lastInsert.count())
                    newDiffList.append(Diff(Diff::Insert, lastInsert));
                if (diff.text.count())
                    newDiffList.append(diff);
                lastDelete = QString();
                lastInsert = QString();
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

QList<Diff> Differ::squashEqualities(const QList<Diff> &diffList)
{
    if (diffList.count() <= 3) // we need at least 3 items
        return diffList;
    QList<Diff> squashedDiffList;
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
                squashedDiffList.append(prevDiff);
            } else {
                squashedDiffList.append(prevDiff);
            }
        } else {
            squashedDiffList.append(prevDiff);
        }
        prevDiff = thisDiff;
        thisDiff = nextDiff;
        i++;
        if (i < diffList.count())
            nextDiff = diffList.at(i);
    }
    squashedDiffList.append(prevDiff);
    if (i == diffList.count())
        squashedDiffList.append(thisDiff);
    return squashedDiffList;
}

int Differ::commonPrefix(const QString &text1, const QString &text2) const
{
    int i = 0;
    const int minCount = qMin(text1.count(), text2.count());
    while (i < minCount) {
        if (text1.at(i) != text2.at(i))
            break;
        i++;
    }
    return i;
}

int Differ::commonSuffix(const QString &text1, const QString &text2) const
{
    int i = 0;
    const int text1Count = text1.count();
    const int text2Count = text2.count();
    const int minCount = qMin(text1.count(), text2.count());
    while (i < minCount) {
        if (text1.at(text1Count - i - 1) != text2.at(text2Count - i - 1))
            break;
        i++;
    }
    return i;
}

} // namespace DiffEditor
