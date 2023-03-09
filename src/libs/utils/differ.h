// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QFuture>
#include <QString>

#include <optional>

QT_BEGIN_NAMESPACE
template <class K, class T>
class QMap;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT Diff
{
public:
    enum Command {
        Delete,
        Insert,
        Equal
    };
    Command command = Equal;
    QString text;
    Diff() = default;
    Diff(Command com, const QString &txt = {});
    bool operator==(const Diff &other) const;
    bool operator!=(const Diff &other) const;
    QString toString() const;
    static QString commandString(Command com);
};

class QTCREATOR_UTILS_EXPORT Differ
{
public:
    enum DiffMode
    {
        CharMode,
        WordMode,
        LineMode
    };
    Differ(const std::optional<QFuture<void>> &future = {});
    QList<Diff> diff(const QString &text1, const QString &text2);
    QList<Diff> unifiedDiff(const QString &text1, const QString &text2);
    void setDiffMode(DiffMode mode);
    DiffMode diffMode() const;
    static QList<Diff> merge(const QList<Diff> &diffList);
    static QList<Diff> cleanupSemantics(const QList<Diff> &diffList);
    static QList<Diff> cleanupSemanticsLossless(const QList<Diff> &diffList);

    static void splitDiffList(const QList<Diff> &diffList,
                              QList<Diff> *leftDiffList,
                              QList<Diff> *rightDiffList);
    static QList<Diff> moveWhitespaceIntoEqualities(const QList<Diff> &input);
    static void diffWithWhitespaceReduced(const QString &leftInput,
                                          const QString &rightInput,
                                          QList<Diff> *leftOutput,
                                          QList<Diff> *rightOutput);
    static void unifiedDiffWithWhitespaceReduced(const QString &leftInput,
                                          const QString &rightInput,
                                          QList<Diff> *leftOutput,
                                          QList<Diff> *rightOutput);
    static void ignoreWhitespaceBetweenEqualities(const QList<Diff> &leftInput,
                                      const QList<Diff> &rightInput,
                                      QList<Diff> *leftOutput,
                                      QList<Diff> *rightOutput);
    static void diffBetweenEqualities(const QList<Diff> &leftInput,
                                      const QList<Diff> &rightInput,
                                      QList<Diff> *leftOutput,
                                      QList<Diff> *rightOutput);

private:
    QList<Diff> preprocess1AndDiff(const QString &text1, const QString &text2);
    QList<Diff> preprocess2AndDiff(const QString &text1, const QString &text2);
    QList<Diff> diffMyers(const QString &text1, const QString &text2);
    QList<Diff> diffMyersSplit(const QString &text1, int x,
                               const QString &text2, int y);
    QList<Diff> diffNonCharMode(const QString &text1, const QString &text2);
    QStringList encode(const QString &text1,
                       const QString &text2,
                       QString *encodedText1,
                       QString *encodedText2);
    QString encode(const QString &text,
                   QStringList *lines,
                   QMap<QString, int> *lineToCode);
    int findSubtextEnd(const QString &text,
                       int subTextStart);
    DiffMode m_diffMode = Differ::LineMode;
    DiffMode m_currentDiffMode = Differ::LineMode;
    std::optional<QFuture<void>> m_future;
};

} // namespace Utils
