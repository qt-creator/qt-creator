// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "findinfilesripgrep.h"

#include "ripgrepparser.h"
#include "ripgreptr.h"

#include <coreplugin/messagemanager.h>

#include <texteditor/findinfiles.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/environment.h>
#include <utils/filesearch.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QLineEdit>

using namespace TextEditor;
using namespace Utils;

namespace Ripgrep {

const char s_searchOptionsString[] = "SearchOptionsString";

// A line break of a document is the CRLF of a file with Windows line endings, too.
static const char lineBreak[] = "\\r?\\n";

// The text of a plain text search as a regular expression, which is the only way to give ripgrep
// a line break.
static QString escapedForRipgrep(const QString &text)
{
    static const QString metaCharacters("\\.+*?()|[]{}^$#&-~");
    QString result;
    for (const QChar c : text) {
        if (c == '\n')
            result += lineBreak;
        else if (c == '\r')
            continue; // Only the one of a CRLF in the text, and the line break has it
        else if (metaCharacters.contains(c))
            result += QString("\\") + c;
        else
            result += c;
    }
    return result;
}

// What ripgrep's regular expressions do not know: a literal line break and \R. A line break in
// the pattern stands for the line break of a document, which can be a CRLF in the file, unless
// it is in a character class.
static QString regularExpressionForRipgrep(const QString &pattern)
{
    QString result;
    bool inClass = false;
    for (qsizetype i = 0; i < pattern.size(); ++i) {
        const QChar c = pattern.at(i);
        if (c == '\\' && i + 1 < pattern.size()) {
            const QChar next = pattern.at(i + 1);
            if (next == 'R' && !inClass) {
                result += QString("(?:") + lineBreak + ")";
                ++i;
            } else if (next == 'r' && !inClass && pattern.mid(i + 2, 2) == "\\n") {
                result += lineBreak;
                i += 3;
            } else if (next == 'n' && !inClass) {
                result += lineBreak;
                ++i;
            } else {
                result += c;
                result += next;
                ++i;
            }
            continue;
        }
        if (c == '[')
            inClass = true;
        else if (c == ']')
            inClass = false;
        if (c == '\n')
            result += inClass ? QString("\\n") : QString(lineBreak);
        else if (c == '\r')
            result += "\\r";
        else
            result += c;
    }
    return result;
}

// Qt Creator matches the filters of a search with wildcards in which * also matches a /, with
// or without regard to the case, and against the whole path and the file name. For ripgrep, a
// glob that contains a / is anchored and * does not cross it.
static QString globForRipgrep(QString filter)
{
    if (filter.startsWith("*/"))
        filter.insert(1, '*');
    if (filter.endsWith("/*"))
        filter.append('*');
    return filter;
}

// Whether globForRipgrep() makes the glob of ripgrep match the same paths as the filter. In a
// glob of ripgrep a * never crosses a /, and a glob with a / is anchored to the search directory.
// So a * is only safe at the start of the name of a file, or right after the */ that is taken for
// any directory.
static bool isGlobForRipgrepEquivalent(QString filter)
{
    const bool anyDepth = filter.startsWith("*/");
    if (anyDepth)
        filter.remove(0, 2);
    const bool directory = filter.endsWith("/*");
    if (directory)
        filter.chop(2);
    if (filter.contains('/') || filter.contains('?') || filter.contains('['))
        return false;
    const qsizetype star = filter.lastIndexOf('*');
    return star < 0 || (star == 0 && (anyDepth || !directory));
}

// Whether the glob of ripgrep for the exclusion \a filter cannot exclude a file that the filter
// keeps. ripgrep does not descend into a directory that the glob matches, while Qt Creator only
// matches the filter against the path and the name of a file. So "src*" leaves out all of the
// directory "src" for ripgrep, but not "src/app.cpp" for Qt Creator. A filter that starts and
// ends with a *, like "*/build/*", matches the path of every file in such a directory, though,
// and a * of ripgrep never crosses a /. Character classes and alternatives, which a glob of
// ripgrep and a filter do not spell the same, are left to Qt Creator.
static bool isExclusionForRipgrepSafe(const QString &filter)
{
    return filter.startsWith('*') && filter.endsWith('*') && !filter.contains('[')
           && !filter.contains('{') && !filter.contains('\\');
}

static void runRipgrep(QPromise<SearchResultItems> &promise, const FileFindParameters &parameters,
                       const QString &searchOptions)
{
    // A search without some of the options would find what the user asked to leave out
    ProcessArgs::SplitError splitError = ProcessArgs::SplitOk;
    const QStringList options = ProcessArgs::splitArgs(searchOptions, HostOsInfo::hostOs(), false,
                                                       &splitError);
    if (splitError != ProcessArgs::SplitOk) {
        Core::MessageManager::writeFlashing(
            Tr::tr("Ripgrep did not search: The quotes in the search options \"%1\" do not match.")
                .arg(searchOptions));
        return;
    }

    const auto setupProcess = [&promise, parameters, options](Process &process) {
        QStringList arguments = {"--json", "--no-config", "--color", "never"};

        arguments << (parameters.flags & FindCaseSensitively ? "-s" : "-i");

        if (parameters.flags & FindWholeWords)
            arguments << "-w";

        // Only an expression that names a line break can match across lines
        const bool acrossLines = findExpressionSpansLines(
            findRegularExpression(parameters.text, parameters.flags));
        if (acrossLines)
            arguments << "--multiline";

        // A line break of the text is the one of a document, a CRLF in it is a single LF
        QString pattern = parameters.text;
        pattern.replace("\r\n", "\n");
        if (parameters.flags & FindRegularExpression) {
            // The default engine of ripgrep does not know look-arounds and back-references,
            // which QRegularExpression has
            arguments << "--engine" << "auto";
            pattern = regularExpressionForRipgrep(pattern);
        } else if (acrossLines) {
            pattern = escapedForRipgrep(pattern);
        } else {
            arguments << "-F";
        }

        // The filters are only given to ripgrep where its globs cannot leave out a file that the
        // filters include. The results are filtered again, but that cannot bring back what
        // ripgrep left out. A name filter must say the same, an exclusion filter must not
        // exclude more.
        if (Utils::allOf(parameters.nameFilters, isGlobForRipgrepEquivalent)) {
            for (const QString &filter : std::as_const(parameters.nameFilters))
                arguments << "--iglob" << globForRipgrep(filter);
        }
        for (const QString &filter : std::as_const(parameters.exclusionFilters)) {
            if (isExclusionForRipgrepSafe(filter))
                arguments << "--iglob" << "!" + globForRipgrep(filter);
        }

        arguments << options;

        arguments << "-e" << pattern << "--"
                  << parameters.searchDir.normalizedPathName().toUrlishString();
        process.setCommand({"rg", arguments});
        process.setUtf8StdOutCodec(); // What rg --json always writes
        process.setEnvironment(Environment::systemEnvironment());

        // ripgrep exits with 1 if nothing was found, and with 2 on an error, which it only
        // explains on stderr. That is an invalid expression or option, before anything is
        // searched, or a file that could not be read, which does not keep it from finding
        // matches in the others.
        QObject::connect(&process, &Process::done, &process, [&promise, &process] {
            if (promise.isCanceled())
                return;
            if (process.result() == ProcessResult::StartFailed) {
                Core::MessageManager::writeFlashing(process.exitMessage());
            } else if (process.exitStatus() == ProcessExitStatus::NormalExit
                       && process.exitCode() > 1) {
                const QString message = Tr::tr("Ripgrep reported an error: %1")
                                            .arg(process.cleanedStdErr().trimmed());
                if (promise.future().resultCount() == 0)
                    Core::MessageManager::writeFlashing(message);
                else
                    Core::MessageManager::writeSilently(message);
            }
        });
    };

    // The filters are only a hint for ripgrep, the results are the ones of Qt Creator's.
    const FilterFilesFunction filterFiles = filterFilesFunction(parameters.nameFilters,
                                                                parameters.exclusionFilters);
    QHash<FilePath, bool> isIncluded;
    QString remainder;
    const auto outputParser = [&](const QFuture<void> &future, const QString &input,
                                  const std::optional<QRegularExpression> &regExp) {
        SearchResultItems items = Ripgrep::parse(future, input, regExp, &remainder);
        Utils::erase(items, [&](const SearchResultItem &item) {
            const FilePath filePath = item.filePath().value_or(FilePath());
            auto it = isIncluded.find(filePath);
            if (it == isIncluded.end())
                it = isIncluded.insert(filePath, !filterFiles({filePath}).isEmpty());
            return !it.value();
        });
        return items;
    };

    TextEditor::searchInProcessOutput(promise, parameters, setupProcess, outputParser);
}

QFuture<SearchResultItems> findWithRipgrep(const FileFindParameters &parameters,
                                           const QString &searchOptions)
{
    return Utils::asyncRun(runRipgrep, parameters, searchOptions);
}

class FindInFilesRipgrep final : public SearchEngine
{
public:
    FindInFilesRipgrep()
    {
        m_widget = new QWidget;
        auto layout = new QHBoxLayout(m_widget);
        layout->setContentsMargins(0, 0, 0, 0);
        m_searchOptionsLineEdit = new QLineEdit;
        m_searchOptionsLineEdit->setPlaceholderText(Tr::tr("Search Options (optional)"));
        layout->addWidget(m_searchOptionsLineEdit);

        FindInFiles *findInFiles = FindInFiles::instance();
        QTC_ASSERT(findInFiles, return);
        findInFiles->addSearchEngine(this);

        // Starting a process can take long, so the engine is enabled once rg turns out to run
        setEnabled(false);
        auto label = new QLabel(Tr::tr("Ripgrep is not available on the system."));
        label->setStyleSheet("QLabel { color : red; }");
        label->setVisible(false);
        layout->addWidget(label);

        auto process = new Process(m_widget);
        process->setCommand({"rg", {"--version"}});
        process->setEnvironment(Environment::systemEnvironment());
        connect(process, &Process::done, this, [this, process, label] {
            const bool available = process->result() == ProcessResult::FinishedWithSuccess
                                   && process->cleanedStdOut().contains("ripgrep");
            setEnabled(available);
            label->setVisible(!available);
            process->deleteLater();
        });
        process->start();
    }

    QString title() const final { return "Ripgrep"; }
    QString toolTip() const final { return {}; }
    QWidget *widget() const final { return m_widget; }

    void readSettings(const Store &s) final
    {
        m_searchOptionsLineEdit->setText(s.value(s_searchOptionsString).toString());
    }

    void writeSettings(Store &s) const final
    {
        if (!m_searchOptionsLineEdit->text().isEmpty())
            s.insert(s_searchOptionsString, m_searchOptionsLineEdit->text());
    }

    SearchExecutor searchExecutor() const final
    {
        return [searchOptions = m_searchOptionsLineEdit->text()](const FileFindParameters &parameters) {
            return findWithRipgrep(parameters, searchOptions);
        };
    }

private:
    QPointer<QWidget> m_widget;
    QPointer<QLineEdit> m_searchOptionsLineEdit;
};

static FindInFilesRipgrep &theFindInFilesRipgrep()
{
    static FindInFilesRipgrep engine;
    return engine;
}

void setupFindInFilesRipgrep()
{
    theFindInFilesRipgrep();
}

#ifdef WITH_TESTS
SearchEngine *ripgrepSearchEngine()
{
    return &theFindInFilesRipgrep();
}
#endif

} // namespace Ripgrep
