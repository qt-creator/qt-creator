/*
    Copyright (C) 2016 Volker Krause <vkrause@kde.org>
    Copyright (C) 2018 Christoph Cullmann <cullmann@kde.org>

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "htmlhighlighter.h"
#include "definition.h"
#include "format.h"
#include "ksyntaxhighlighting_logging.h"
#include "state.h"
#include "theme.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QVarLengthArray>

using namespace KSyntaxHighlighting;

class KSyntaxHighlighting::HtmlHighlighterPrivate
{
public:
    std::unique_ptr<QTextStream> out;
    std::unique_ptr<QFile> file;
    QString currentLine;
};

HtmlHighlighter::HtmlHighlighter()
    : d(new HtmlHighlighterPrivate())
{
}

HtmlHighlighter::~HtmlHighlighter()
{
}

void HtmlHighlighter::setOutputFile(const QString &fileName)
{
    d->file.reset(new QFile(fileName));
    if (!d->file->open(QFile::WriteOnly | QFile::Truncate)) {
        qCWarning(Log) << "Failed to open output file" << fileName << ":" << d->file->errorString();
        return;
    }
    d->out.reset(new QTextStream(d->file.get()));
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    d->out->setEncoding(QStringConverter::Utf8);
#else
    d->out->setCodec("UTF-8");
#endif
}

void HtmlHighlighter::setOutputFile(FILE *fileHandle)
{
    d->out.reset(new QTextStream(fileHandle, QIODevice::WriteOnly));
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    d->out->setEncoding(QStringConverter::Utf8);
#else
    d->out->setCodec("UTF-8");
#endif
}

void HtmlHighlighter::highlightFile(const QString &fileName, const QString &title)
{
    QFileInfo fi(fileName);
    QFile f(fileName);
    if (!f.open(QFile::ReadOnly)) {
        qCWarning(Log) << "Failed to open input file" << fileName << ":" << f.errorString();
        return;
    }

    if (title.isEmpty())
        highlightData(&f, fi.fileName());
    else
        highlightData(&f, title);
}

void HtmlHighlighter::highlightData(QIODevice *dev, const QString &title)
{
    if (!d->out) {
        qCWarning(Log) << "No output stream defined!";
        return;
    }

    QString htmlTitle;
    if (title.isEmpty())
        htmlTitle = QStringLiteral("Kate Syntax Highlighter");
    else
        htmlTitle = title.toHtmlEscaped();

    State state;
    *d->out << "<!DOCTYPE html>\n";
    *d->out << "<html><head>\n";
    *d->out << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/>\n";
    *d->out << "<title>" << htmlTitle << "</title>\n";
    *d->out << "<meta name=\"generator\" content=\"KF5::SyntaxHighlighting (" << definition().name() << ")\"/>\n";
    *d->out << "</head><body";
    if (theme().textColor(Theme::Normal))
        *d->out << " style=\"color:" << QColor(theme().textColor(Theme::Normal)).name() << "\"";
    *d->out << "><pre>\n";

    QTextStream in(dev);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    in.setEncoding(QStringConverter::Utf8);
#else
    in.setCodec("UTF-8");
#endif
    while (!in.atEnd()) {
        d->currentLine = in.readLine();
        state = highlightLine(d->currentLine, state);
        *d->out << "\n";
    }

    *d->out << "</pre></body></html>\n";
    d->out->flush();

    d->out.reset();
    d->file.reset();
}

void HtmlHighlighter::applyFormat(int offset, int length, const Format &format)
{
    if (length == 0)
        return;

    // collect potential output, cheaper than thinking about "is there any?"
    QVarLengthArray<QString, 16> formatOutput;
    if (format.hasTextColor(theme()))
        formatOutput << QStringLiteral("color:") << format.textColor(theme()).name() << QStringLiteral(";");
    if (format.hasBackgroundColor(theme()))
        formatOutput << QStringLiteral("background-color:") << format.backgroundColor(theme()).name() << QStringLiteral(";");
    if (format.isBold(theme()))
        formatOutput << QStringLiteral("font-weight:bold;");
    if (format.isItalic(theme()))
        formatOutput << QStringLiteral("font-style:italic;");
    if (format.isUnderline(theme()))
        formatOutput << QStringLiteral("text-decoration:underline;");
    if (format.isStrikeThrough(theme()))
        formatOutput << QStringLiteral("text-decoration:line-through;");

    if (!formatOutput.isEmpty()) {
        *d->out << "<span style=\"";
        for (const auto &out : qAsConst(formatOutput)) {
            *d->out << out;
        }
        *d->out << "\">";
    }

    *d->out << d->currentLine.mid(offset, length).toHtmlEscaped();

    if (!formatOutput.isEmpty()) {
        *d->out << "</span>";
    }
}
