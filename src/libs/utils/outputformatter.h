// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"
#include "outputformat.h"

#include <QObject>

#include <functional>
#include <optional>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QRegularExpressionMatch;
class QTextCharFormat;
class QTextCursor;
QT_END_NAMESPACE

namespace Utils {
class FileInProjectFinder;
class FormattedText;
class Link;

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
        Result(Status s,
               const LinkSpecs &l = {},
               const std::optional<QString> &c = {},
               const std::optional<OutputFormat> &f = {})
            : status(s)
            , linkSpecs(l)
            , newContent(c)
            , formatOverride(f)
        {}
        Status status;
        LinkSpecs linkSpecs;
        std::optional<QString> newContent; // Hard content override. Only to be used in extreme cases.
        std::optional<OutputFormat> formatOverride;
    };

    static bool isLinkTarget(const QString &target);
    static Utils::Link parseLinkTarget(const QString &target);

    void addSearchDir(const Utils::FilePath &dir);
    void dropSearchDir(const Utils::FilePath &dir);
    const FilePaths searchDirectories() const;

    void setFileFinder(Utils::FileInProjectFinder *finder);

    void setDemoteErrorsToWarnings(bool demote);
    bool demoteErrorsToWarnings() const;

    // Represents a single line, without a trailing line feed character.
    // The input is to be considered "complete" for parsing purposes.
    virtual Result handleLine(const QString &line, OutputFormat format) = 0;

    virtual bool handleLink(const QString &href) { Q_UNUSED(href); return false; }
    virtual bool hasFatalErrors() const { return false; }
    virtual void flush() {}
    virtual void runPostPrintActions(QPlainTextEdit *) {}

    void setRedirectionDetector(const OutputLineParser *detector);
    bool needsRedirection() const;
    virtual bool hasDetectedRedirection() const { return false; }

#ifdef WITH_TESTS
    void skipFileExistsCheck();
#endif

protected:
    static QString rightTrimmed(const QString &in);
    Utils::FilePath absoluteFilePath(const Utils::FilePath &filePath) const;
    static QString createLinkTarget(const FilePath &filePath, int line, int column);
    static void addLinkSpecForAbsoluteFilePath(LinkSpecs &linkSpecs, const FilePath &filePath,
                                               int lineNo, int pos, int len);
    static void addLinkSpecForAbsoluteFilePath(LinkSpecs &linkSpecs, const FilePath &filePath,
                                               int lineNo, const QRegularExpressionMatch &match,
                                               int capIndex);
    static void addLinkSpecForAbsoluteFilePath(LinkSpecs &linkSpecs, const FilePath &filePath,
                                               int lineNo, const QRegularExpressionMatch &match,
                                               const QString &capName);
    bool fileExists(const Utils::FilePath &fp) const;

signals:
    void newSearchDirFound(const Utils::FilePath &dir);
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
    void setForwardStdOutToStdError(bool enabled);

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
    void openInEditorRequested(const Utils::Link &link);

private:
    void doAppendMessage(const QString &text, OutputFormat format);

    OutputLineParser::Result handleMessage(const QString &text, OutputFormat format,
                                           QList<OutputLineParser *> &involvedParsers);

    void append(const QString &text, const QTextCharFormat &format);
    void initFormats();
    void flushIncompleteLine();
    void flushTrailingNewline();
    void dumpIncompleteLine(const QString &line, OutputFormat format);
    void clearLastLine();
    QList<FormattedText> parseAnsi(const QString &text, const QTextCharFormat &format);
    OutputFormat outputTypeForParser(const OutputLineParser *parser, OutputFormat type) const;
    void setupLineParser(OutputLineParser *parser);

    class Private;
    Private * const d;
};


} // namespace Utils
