// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QBrush>
#include <QList>
#include <QString>
#include <QSyntaxHighlighter>

#include <functional>

QT_BEGIN_NAMESPACE
class QJsonValue;
QT_END_NAMESPACE

namespace Utils {

class FilePath;

// Create a usable settings key from a category,
// for example Editor|C++ -> Editor_C__
QTCREATOR_UTILS_EXPORT QString settingsKey(const QString &category);

// Return the common prefix part of a string list:
// "C:\foo\bar1" "C:\foo\bar2"  -> "C:\foo\bar"
QTCREATOR_UTILS_EXPORT QString commonPrefix(const QStringList &strings);

// Removes first unescaped ampersand in text
QTCREATOR_UTILS_EXPORT QString stripAccelerator(const QString &text);
// Quotes all ampersands
QTCREATOR_UTILS_EXPORT QString quoteAmpersands(const QString &text);

QTCREATOR_UTILS_EXPORT bool readMultiLineString(const QJsonValue &value, QString *out);

// Compare case insensitive and use case sensitive comparison in case of that being equal.
QTCREATOR_UTILS_EXPORT int caseFriendlyCompare(const QString &a, const QString &b);

class QTCREATOR_UTILS_EXPORT AbstractMacroExpander
{
public:
    virtual ~AbstractMacroExpander() {}
    // Not const, as it may change the state of the expander.
    //! Find an expando to replace and provide a replacement string.
    //! \param str The string to scan
    //! \param pos Position to start scan on input, found position on output
    //! \param ret Replacement string on output
    //! \return Length of string part to replace, zero if no (further) matches found
    virtual int findMacro(const QString &str, int *pos, QString *ret);
    //! Provide a replacement string for an expando
    //! \param name The name of the expando
    //! \param ret Replacement string on output
    //! \return True if the expando was found
    virtual bool resolveMacro(const QString &name, QString *ret, QSet<AbstractMacroExpander *> &seen) = 0;
private:
    bool expandNestedMacros(const QString &str, int *pos, QString *ret);
};

QTCREATOR_UTILS_EXPORT void expandMacros(QString *str, AbstractMacroExpander *mx);
QTCREATOR_UTILS_EXPORT QString expandMacros(const QString &str, AbstractMacroExpander *mx);

QTCREATOR_UTILS_EXPORT int parseUsedPortFromNetstatOutput(const QByteArray &line);

QTCREATOR_UTILS_EXPORT QString appendHelper(const QString &base, int n);
QTCREATOR_UTILS_EXPORT FilePath appendHelper(const FilePath &base, int n);

template<typename T>
T makeUniquelyNumbered(const T &preferred, const std::function<bool(const T &)> &isOk)
{
    if (isOk(preferred))
        return preferred;
    int i = 2;
    T tryName = appendHelper(preferred, i);
    while (!isOk(tryName))
        tryName = appendHelper(preferred, ++i);
    return tryName;
}

template<typename T, typename Container>
T makeUniquelyNumbered(const T &preferred, const Container &reserved)
{
    const std::function<bool(const T &)> isOk
            = [&reserved](const T &v) { return !reserved.contains(v); };
    return makeUniquelyNumbered(preferred, isOk);
}

QTCREATOR_UTILS_EXPORT QString formatElapsedTime(qint64 elapsed);

/* This function is only necessary if you need to match the wildcard expression against a
 * string that might contain path separators - otherwise
 * QRegularExpression::wildcardToRegularExpression() can be used.
 * Working around QRegularExpression::wildcardToRegularExpression() taking native separators
 * into account and handling them to disallow matching a wildcard characters.
 */
QTCREATOR_UTILS_EXPORT QString wildcardToRegularExpression(const QString &original);

QTCREATOR_UTILS_EXPORT QString languageNameFromLanguageCode(const QString &languageCode);


#ifdef QT_WIDGETS_LIB

// Feeds the global clipboard and, when present, the primary selection
QTCREATOR_UTILS_EXPORT void setClipboardAndSelection(const QString &text);

#endif

QTCREATOR_UTILS_EXPORT QString chopIfEndsWith(QString str, QChar c);
QTCREATOR_UTILS_EXPORT QStringView chopIfEndsWith(QStringView str, QChar c);

QTCREATOR_UTILS_EXPORT QString normalizeNewlines(const QString &text);

// Skips empty parts - see QTBUG-110900
QTCREATOR_UTILS_EXPORT QString joinStrings(const QStringList &strings, QChar separator);
QTCREATOR_UTILS_EXPORT QString trimFront(const QString &string, QChar ch);
QTCREATOR_UTILS_EXPORT QString trimBack(const QString &string, QChar ch);
QTCREATOR_UTILS_EXPORT QString trim(const QString &string, QChar ch);

QTCREATOR_UTILS_EXPORT QPair<QStringView, QStringView> splitAtFirst(const QString &string, QChar ch);
QTCREATOR_UTILS_EXPORT QPair<QStringView, QStringView> splitAtFirst(const QStringView &stringView,
                                                                    QChar ch);

QTCREATOR_UTILS_EXPORT int endOfNextWord(const QString &string, int position = 0);

class QTCREATOR_UTILS_EXPORT MarkdownHighlighter : public QSyntaxHighlighter
{
public:
    MarkdownHighlighter(QTextDocument *parent);
    void highlightBlock(const QString &text);

private:
    QBrush codeBgBrush();

    QBrush h2Brush;
    QBrush m_codeBgBrush;
};

} // namespace Utils
