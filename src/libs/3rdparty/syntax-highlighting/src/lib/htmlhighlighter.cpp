/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2018 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
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

/**
 * @brief toHtmlRgba
 * Converts QColor -> rgba(r, g, b, a) if there is an alpha channel
 * otherwise it will just return the hexcode. This is because QColor
 * outputs #AARRGGBB, whereas browser support #RRGGBBAA.
 *
 * @param color
 * @return
 */
static QString toHtmlRgbaString(const QColor &color)
{
    if (color.alpha() == 0xFF)
        return color.name();

    QString rgba = QStringLiteral("rgba(");
    rgba.append(QString::number(color.red()));
    rgba.append(QLatin1Char(','));
    rgba.append(QString::number(color.green()));
    rgba.append(QLatin1Char(','));
    rgba.append(QString::number(color.blue()));
    rgba.append(QLatin1Char(','));
    // this must be alphaF
    rgba.append(QString::number(color.alphaF()));
    rgba.append(QLatin1Char(')'));
    return rgba;
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
    *d->out << "<meta name=\"generator\" content=\"KF5::SyntaxHighlighting - Definition (" << definition().name() << ") - Theme (" << theme().name()
            << ")\"/>\n";
    *d->out << "</head><body";
    *d->out << " style=\"background-color:" << toHtmlRgbaString(QColor::fromRgba(theme().editorColor(Theme::BackgroundColor)));
    if (theme().textColor(Theme::Normal))
        *d->out << ";color:" << toHtmlRgbaString(QColor::fromRgba(theme().textColor(Theme::Normal)));
    *d->out << "\"><pre>\n";

    QTextStream in(dev);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
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
        formatOutput << QStringLiteral("color:") << toHtmlRgbaString(format.textColor(theme())) << QStringLiteral(";");
    if (format.hasBackgroundColor(theme()))
        formatOutput << QStringLiteral("background-color:") << toHtmlRgbaString(format.backgroundColor(theme())) << QStringLiteral(";");
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
