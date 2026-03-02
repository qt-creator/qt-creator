// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiassistantutils.h"

#include <astcheck/astcheck.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsreformatter.h>
#include <qmljs/qmljsstaticanalysismessage.h>

#include <qmldesignerplugin.h>

#include <utils/filepath.h>
#include <utils/qtcassert.h>

#include <QBuffer>
#include <QImage>
#include <QRegularExpression>
#include <QTextDocument>

namespace QmlDesigner::AiAssistantUtils {

DesignDocument *currentDesignDocument()
{
    return QmlDesignerPlugin::instance()->currentDesignDocument();
}

RewriterView *rewriterView()
{
    auto designDocument = currentDesignDocument();
    QTC_ASSERT(designDocument, return nullptr);

    return designDocument->rewriterView();
}

QString currentQmlText()
{
    constexpr QStringView annotationsStart{u"/*##^##"};
    constexpr QStringView annotationsEnd{u"##^##*/"};

    auto designDocument = currentDesignDocument();
    QTC_ASSERT(designDocument, return nullptr);

    QString pureQml = designDocument->plainTextEdit()->toPlainText();
    int startIndex = pureQml.indexOf(annotationsStart);
    int endIndex = pureQml.indexOf(annotationsEnd, startIndex);
    if (startIndex != -1 && endIndex != -1) {
        int lengthToRemove = (endIndex + annotationsEnd.length()) - startIndex;
        pureQml.remove(startIndex, lengthToRemove);
    }
    return pureQml;
}

QString reformatQml(const QString &content)
{
    auto document = QmlJS::Document::create({}, QmlJS::Dialect::QmlQtQuick2);
    document->setSource(content);
    document->parseQml();
    if (document->isParsedCorrectly())
        return QmlJS::reformat(document);

    return content;
}

void setQml(const QString &qml)
{
    auto view = rewriterView();
    QTC_ASSERT(view, return);

    auto textModifier = view->textModifier();
    textModifier->replace(0, textModifier->text().size(), qml);
}

bool isValidQmlCode(const QString &qmlCode)
{
    using namespace QmlJS;

    Document::MutablePtr doc
        = Document::create(Utils::FilePath::fromString("<internal>"), Dialect::Qml);
    doc->setSource(qmlCode);
    bool success = doc->parseQml();

    AstCheck check(doc);
    QList<StaticAnalysis::Message> messages = check();

    // TODO: add check for correct Qml types (property names)

    success &= Utils::allOf(messages, [](const StaticAnalysis::Message &message) {
        return message.severity != Severity::Error;
    });

    return success;
}

QString toBase64Image(const QImage &image, const QByteArray &format)
{
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    image.save(&buffer, format);
    return QString::fromLatin1(byteArray.toBase64());
}

QString markdownToHtml(const QString &markdown)
{
    if (markdown.isEmpty())
        return {};

    QTextDocument doc;
    doc.setMarkdown(markdown, QTextDocument::MarkdownDialectCommonMark);

    QString html = doc.toHtml();

    // Strip everything up to and including the opening <body...> tag.
    const int bodyStart = html.indexOf("<body");
    if (bodyStart != -1) {
        const int bodyTagEnd = html.indexOf('>', bodyStart);
        if (bodyTagEnd != -1)
            html = html.mid(bodyTagEnd + 1);
    }

    // Strip the closing </body></html>.
    const int bodyClose = html.lastIndexOf("</body>");
    if (bodyClose != -1)
        html = html.left(bodyClose);

    // QTextDocument wraps everything in a <p style="margin-top:0px; …"> with hard-coded
    // pixel sizes and font families.  Remove inline styles so the QML theme fonts apply.
    static QRegularExpression inlineStyle(R"( style="[^"]*")");
    html.remove(inlineStyle);

    return html.trimmed();
}

} // namespace QmlDesigner::AiAssistantUtils
