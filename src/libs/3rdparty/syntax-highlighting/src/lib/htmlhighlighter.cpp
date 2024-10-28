/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2018 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "htmlhighlighter.h"
#include "abstracthighlighter_p.h"
#include "definition.h"
#include "definition_p.h"
#include "format.h"
#include "ksyntaxhighlighting_logging.h"
#include "state.h"
#include "theme.h"

#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QTextStream>

using namespace KSyntaxHighlighting;

class KSyntaxHighlighting::HtmlHighlighterPrivate : public AbstractHighlighterPrivate
{
public:
    std::unique_ptr<QTextStream> out;
    std::unique_ptr<QFile> file;
    QString currentLine;
    std::vector<QString> htmlStyles;
    Theme::EditorColorRole bgRole = Theme::BackgroundColor;
};

HtmlHighlighter::HtmlHighlighter()
    : AbstractHighlighter(new HtmlHighlighterPrivate())
{
}

HtmlHighlighter::~HtmlHighlighter()
{
}

void HtmlHighlighter::setBackgroundRole(Theme::EditorColorRole bgRole)
{
    Q_D(HtmlHighlighter);
    d->bgRole = bgRole;
}

void HtmlHighlighter::setOutputFile(const QString &fileName)
{
    Q_D(HtmlHighlighter);
    d->file.reset(new QFile(fileName));
    if (!d->file->open(QFile::WriteOnly | QFile::Truncate)) {
        qCWarning(Log) << "Failed to open output file" << fileName << ":" << d->file->errorString();
        return;
    }
    d->out.reset(new QTextStream(d->file.get()));
    d->out->setEncoding(QStringConverter::Utf8);
}

void HtmlHighlighter::setOutputFile(FILE *fileHandle)
{
    Q_D(HtmlHighlighter);
    d->out.reset(new QTextStream(fileHandle, QIODevice::WriteOnly));
    d->out->setEncoding(QStringConverter::Utf8);
}

void HtmlHighlighter::highlightFile(const QString &fileName, const QString &title)
{
    QFile f(fileName);
    if (!f.open(QFile::ReadOnly)) {
        qCWarning(Log) << "Failed to open input file" << fileName << ":" << f.errorString();
        return;
    }

    if (title.isEmpty()) {
        QFileInfo fi(fileName);
        highlightData(&f, fi.fileName());
    } else {
        highlightData(&f, title);
    }
}

namespace
{
/**
 * @brief toHtmlRgba
 * Converts QRgb -> #RRGGBBAA if there is an alpha channel
 * otherwise it will just return the hexcode. This is because QColor
 * outputs #AARRGGBB, whereas browser support #RRGGBBAA.
 */
struct HtmlColor {
    HtmlColor(QRgb argb)
    {
        static const char16_t *digits = u"0123456789abcdef";

        hexcode[0] = u'#';
        hexcode[1] = digits[qRed(argb) >> 4];
        hexcode[2] = digits[qRed(argb) & 0xf];
        hexcode[3] = digits[qGreen(argb) >> 4];
        hexcode[4] = digits[qGreen(argb) & 0xf];
        hexcode[5] = digits[qBlue(argb) >> 4];
        hexcode[6] = digits[qBlue(argb) & 0xf];
        if (qAlpha(argb) == 0xff) {
            len = 7;
        } else {
            hexcode[7] = digits[qAlpha(argb) >> 4];
            hexcode[8] = digits[qAlpha(argb) & 0xf];
            len = 9;
        }
    }

    QStringView sv() const
    {
        return QStringView(hexcode, len);
    }

private:
    QChar hexcode[9];
    qsizetype len;
};
}

void HtmlHighlighter::highlightData(QIODevice *dev, const QString &title)
{
    Q_D(HtmlHighlighter);

    if (!d->out) {
        qCWarning(Log) << "No output stream defined!";
        return;
    }

    QString htmlTitle;
    if (title.isEmpty()) {
        htmlTitle = QStringLiteral("KSyntaxHighlighter");
    } else {
        htmlTitle = title.toHtmlEscaped();
    }

    const auto &theme = d->m_theme;
    const auto &definition = d->m_definition;
    const bool useSelectedText = d->bgRole == Theme::TextSelection;

    auto definitions = definition.includedDefinitions();
    definitions.append(definition);

    const auto mainTextColor = [&] {
        if (useSelectedText) {
            const auto fg = theme.selectedTextColor(Theme::Normal);
            if (fg) {
                return fg;
            }
        }
        return theme.textColor(Theme::Normal);
    }();
    const auto mainBgColor = theme.editorColor(d->bgRole);

    int maxId = 0;
    for (const auto &definition : std::as_const(definitions)) {
        for (const auto &format : std::as_const(DefinitionData::get(definition)->formats)) {
            maxId = qMax(maxId, format.id());
        }
    }
    d->htmlStyles.clear();
    // htmlStyles must not be empty for applyFormat to work even with a definition without any context
    d->htmlStyles.resize(maxId + 1);

    // initialize htmlStyles
    for (const auto &definition : std::as_const(definitions)) {
        for (const auto &format : std::as_const(DefinitionData::get(definition)->formats)) {
            auto &buffer = d->htmlStyles[format.id()];

            const auto textColor = useSelectedText ? format.selectedTextColor(theme).rgba() : format.textColor(theme).rgba();
            if (textColor && textColor != mainTextColor) {
                buffer += QStringLiteral("color:") + HtmlColor(textColor).sv() + u';';
            }
            const auto bgColor = useSelectedText ? format.selectedBackgroundColor(theme).rgba() : format.backgroundColor(theme).rgba();
            if (bgColor && bgColor != mainBgColor) {
                buffer += QStringLiteral("background-color:") + HtmlColor(bgColor).sv() + u';';
            }
            if (format.isBold(theme)) {
                buffer += QStringLiteral("font-weight:bold;");
            }
            if (format.isItalic(theme)) {
                buffer += QStringLiteral("font-style:italic;");
            }
            if (format.isUnderline(theme)) {
                buffer += QStringLiteral("text-decoration:underline;");
            }
            if (format.isStrikeThrough(theme)) {
                buffer += QStringLiteral("text-decoration:line-through;");
            }

            if (!buffer.isEmpty()) {
                buffer.insert(0, QStringLiteral("<span style=\""));
                // replace last ';'
                buffer.back() = u'"';
                buffer += u'>';
            }
        }
    }

    State state;
    *d->out << "<!DOCTYPE html>\n";
    *d->out << "<html><head>\n";
    *d->out << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/>\n";
    *d->out << "<title>" << htmlTitle << "</title>\n";
    *d->out << "<meta name=\"generator\" content=\"KF5::SyntaxHighlighting - Definition (" << definition.name() << ") - Theme (" << theme.name() << ")\"/>\n";
    *d->out << "</head><body";
    *d->out << " style=\"background-color:" << HtmlColor(mainBgColor).sv();
    *d->out << ";color:" << HtmlColor(mainTextColor).sv();
    *d->out << "\"><pre>\n";

    QTextStream in(dev);
    while (in.readLineInto(&d->currentLine)) {
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
    if (length == 0) {
        return;
    }

    Q_D(HtmlHighlighter);

    auto const &htmlStyle = d->htmlStyles[format.id()];

    if (!htmlStyle.isEmpty()) {
        *d->out << htmlStyle;
    }

    for (QChar ch : QStringView(d->currentLine).sliced(offset, length)) {
        if (ch == u'<')
            *d->out << QStringLiteral("&lt;");
        else if (ch == u'&')
            *d->out << QStringLiteral("&amp;");
        else
            *d->out << ch;
    }

    if (!htmlStyle.isEmpty()) {
        *d->out << QStringLiteral("</span>");
    }
}
