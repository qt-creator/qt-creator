/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "utils_global.h"
#include "fileutils.h"
#include "optional.h"
#include "outputformat.h"

#include <QObject>

#include <functional>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QRegularExpressionMatch;
class QTextCharFormat;
class QTextCursor;
QT_END_NAMESPACE

namespace Utils {
class FileInProjectFinder;
class FormattedText;

class QTCREATOR_UTILS_EXPORT OutputLineParser : public QObject
{
    Q_OBJECT
public:
    OutputLineParser();
    ~OutputLineParser() override;

    enum class Status { Done, InProgress, NotHandled };
    class LinkSpec {
    public:
        LinkSpec() = default;
        LinkSpec(int sp, int l, const QString &t) : startPos(sp), length(l), target(t) {}
        int startPos = -1;
        int length = -1;
        QString target;
    };
    using LinkSpecs = QList<LinkSpec>;
    class Result {
    public:
        Result(Status s, const LinkSpecs &l = {}, const optional<QString> &c = {})
            : status(s), linkSpecs(l), newContent(c) {}
        Status status;
        LinkSpecs linkSpecs;
        optional<QString> newContent; // Hard content override. Only to be used in extreme cases.
    };

    static bool isLinkTarget(const QString &target);
    static void parseLinkTarget(const QString &target, FilePath &filePath, int &line, int &column);

    void addSearchDir(const Utils::FilePath &dir);
    void dropSearchDir(const Utils::FilePath &dir);
    const FilePaths searchDirectories() const;

    void setFileFinder(Utils::FileInProjectFinder *finder);

    void setDemoteErrorsToWarnings(bool demote);
    bool demoteErrorsToWarnings() const;

    // line contains at most one line feed character, and if it does occur, it's the last character.
    // Either way, the input is to be considered "complete" for parsing purposes.
    virtual Result handleLine(const QString &line, OutputFormat format) = 0;

    virtual bool handleLink(const QString &href) { Q_UNUSED(href); return false; }
    virtual bool hasFatalErrors() const { return false; }
    virtual void flush() {}
    virtual void runPostPrintActions() {}

    void setRedirectionDetector(const OutputLineParser *detector);
    bool needsRedirection() const;
    virtual bool hasDetectedRedirection() const { return false; }

#ifdef WITH_TESTS
    void skipFileExistsCheck();
#endif

protected:
    static QString rightTrimmed(const QString &in);
    Utils::FilePath absoluteFilePath(const Utils::FilePath &filePath);
    static QString createLinkTarget(const FilePath &filePath, int line, int column);
    void addLinkSpecForAbsoluteFilePath(LinkSpecs &linkSpecs, const FilePath &filePath,
                                        int lineNo, int pos, int len);
    void addLinkSpecForAbsoluteFilePath(LinkSpecs &linkSpecs, const FilePath &filePath,
                                        int lineNo, const QRegularExpressionMatch &match,
                                        int capIndex);
    void addLinkSpecForAbsoluteFilePath(LinkSpecs &linkSpecs, const FilePath &filePath,
                                        int lineNo, const QRegularExpressionMatch &match,
                                        const QString &capName);

signals:
    void newSearchDir(const Utils::FilePath &dir);
    void searchDirExpired(const Utils::FilePath &dir);

private:
    class Private;
    Private * const d;
};

class QTCREATOR_UTILS_EXPORT OutputFormatter : public QObject
{
    Q_OBJECT
public:
    OutputFormatter();
    ~OutputFormatter() override;

    QPlainTextEdit *plainTextEdit() const;
    void setPlainTextEdit(QPlainTextEdit *plainText);

    // Forwards to line parsers. Add those before.
    void addSearchDir(const FilePath &dir);
    void dropSearchDir(const FilePath &dir);

    void setLineParsers(const QList<OutputLineParser *> &parsers); // Takes ownership
    void addLineParsers(const QList<OutputLineParser *> &parsers);
    void addLineParser(OutputLineParser *parser);

    void setFileFinder(const FileInProjectFinder &finder);
    void setDemoteErrorsToWarnings(bool demote);

    using PostPrintAction = std::function<void(OutputLineParser *)>;
    void overridePostPrintAction(const PostPrintAction &postPrintAction);

    void appendMessage(const QString &text, OutputFormat format);
    void flush(); // Flushes in-flight data.
    void clear(); // Clears the text edit, if there is one.
    void reset(); // Wipes everything except the text edit.

    bool handleFileLink(const QString &href);
    void handleLink(const QString &href);
    void setBoldFontEnabled(bool enabled);

    bool hasFatalErrors() const;

    static const QList<Utils::FormattedText> linkifiedText(const QList<FormattedText> &text,
            const OutputLineParser::LinkSpecs &linkSpecs);

#ifdef WITH_TESTS
    void overrideTextCharFormat(const QTextCharFormat &fmt);
    QList<OutputLineParser *> lineParsers() const;
#endif

#ifndef WITH_TESTS
private:
#endif
    QTextCharFormat charFormat(OutputFormat format) const;
    static QTextCharFormat linkFormat(const QTextCharFormat &inputFormat, const QString &href);

signals:
    void openInEditorRequested(const FilePath &filePath, int line, int column);

private:
    void doAppendMessage(const QString &text, OutputFormat format);

    OutputLineParser::Result handleMessage(const QString &text, OutputFormat format,
                                           QList<OutputLineParser *> &involvedParsers);

    void append(const QString &text, const QTextCharFormat &format);
    void initFormats();
    void flushIncompleteLine();
    void dumpIncompleteLine(const QString &line, OutputFormat format);
    void clearLastLine();
    QList<FormattedText> parseAnsi(const QString &text, const QTextCharFormat &format);
    OutputFormat outputTypeForParser(const OutputLineParser *parser, OutputFormat type) const;
    void setupLineParser(OutputLineParser *parser);

    class Private;
    Private * const d;
};


} // namespace Utils
