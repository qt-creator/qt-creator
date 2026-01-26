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

#include <ranges>

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

QStringList currentSelectedIds()
{
    auto view = rewriterView();
    QTC_ASSERT(view, return {});

    const ModelNodes selectedNodes = view->selectedModelNodes();
    auto currentSelectedIdxsView
        = selectedNodes
          | std::views::filter([](const ModelNode &node) { return !node.id().isEmpty(); })
          | std::views::transform([](const ModelNode &node) { return node.id(); });
    return QStringList(currentSelectedIdxsView.begin(), currentSelectedIdxsView.end());
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

void selectIds(const QStringList &ids)
{
    if (ids.isEmpty())
        return;

    auto view = rewriterView();
    QTC_ASSERT(view, return);

    Model *model = view->model();
    ModelNodes selectedNodes;
    selectedNodes.reserve(ids.size());

    for (const QString &id : ids)
        selectedNodes.append(view->modelNodeForId(id));

    if (!selectedNodes.isEmpty())
        model->setSelectedModelNodes(selectedNodes);
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

} // namespace QmlDesigner::AiAssistantUtils
