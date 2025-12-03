// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiassistantwidget.h"

#include <asset.h>
#include <designersettings.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>

#include <coreplugin/icore.h>
#include <utils/filepath.h>

#include <QApplication>
#include <QBuffer>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QQmlContext>
#include <QQmlEngine>
#include <QTextEdit>
#include <QtQuick/QQuickItem>

namespace QmlDesigner {

namespace {

QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (::Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toUrlishString();
}

QString qmlSourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/aiAssistantQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/aiAssistantQmlSources").toUrlishString();
}

QString getContent(const QJsonObject &responseObject)
{
    if (!responseObject.contains("choices") || !responseObject["choices"].isArray()) {
        qWarning() << __FUNCTION__ << "Missing or invalid 'choices' array";
        return {};
    }

    QJsonArray choicesArray = responseObject["choices"].toArray();
    if (choicesArray.isEmpty()) {
        qWarning() << __FUNCTION__ << "'choices' array is empty";
        return {};
    }

    QJsonObject firstChoice = choicesArray.first().toObject();
    if (!firstChoice.contains("message") || !firstChoice["message"].isObject()) {
        qWarning() << __FUNCTION__ << "Missing or invalid 'message' object in first choice";
        return {};
    }

    QJsonObject messageObject = firstChoice["message"].toObject();
    if (!messageObject.contains("content") || !messageObject["content"].isString()) {
        qWarning() << __FUNCTION__ << "Missing or invalid 'content' string in message";
        return {};
    }

    return messageObject["content"].toString();
}

} // namespace

bool AiAssistantWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::EnabledChange) {
        // Only focus the AI Assitant widget if no other view has taken focus through user interaction
        if (m_quickWidget->quickWidget()->isEnabled() && !QApplication::focusWidget())
            m_quickWidget->quickWidget()->setFocus();
    }

    return QWidget::eventFilter(obj, event);
}

AiAssistantWidget::AiAssistantWidget()
    : m_manager(std::make_unique<QNetworkAccessManager>())
    , m_quickWidget(Utils::makeUniqueObjectPtr<StudioQuickWidget>())
{
    setWindowTitle(tr("AI Assistant", "Title of Ai Assistant widget"));
    setMinimumWidth(220);
    setMinimumHeight(82);

    auto vLayout = new QVBoxLayout(this);
    vLayout->setContentsMargins(5, 5, 5, 5);

    m_quickWidget->setContentsMargins({0, 0, 0, 0});
    m_quickWidget->quickWidget()->setObjectName(Constants::OBJECT_NAME_AI_ASSISTANT);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quickWidget->setMinimumHeight(minimumHeight() - 5);
    m_quickWidget->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    m_quickWidget->quickWidget()->installEventFilter(this);

    auto map = m_quickWidget->registerPropertyMap("AiAssistantBackend");
    map->setProperties({
        {"rootView", QVariant::fromValue(this)},
    });

    vLayout->addWidget(m_quickWidget.get());
    reloadQmlSource();
}

void AiAssistantWidget::clearAttachedImage()
{
    m_quickWidget->rootObject()->setProperty("attachedImageSource", "");
}

QSize AiAssistantWidget::sizeHint() const
{
    return {420, 20};
}

QStringList AiAssistantWidget::getImageAssetsPaths() const
{
    Utils::FilePath resourePath = DocumentManager::currentResourcePath();

    QStringList imagePaths;
    QStringList filters = Asset::supportedImageSuffixes();

    QDirIterator it(resourePath.toFSPathString(), filters, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        imagePaths << Utils::FilePath::fromString(it.next())
                          .relativePathFromDir(resourePath);

    return imagePaths;
}

void AiAssistantWidget::handleMessage(const QString &prompt)
{
    if (prompt.isEmpty())
        return;

    m_inputHistory.append(prompt);
    m_historyIndex = m_inputHistory.size();

    QString currentQml = QmlDesignerPlugin::instance()->currentDesignDocument()
                             ->textEditorWidget()->toPlainText();

    // Remove aux data (it seems to affect the result and make it generate 3D content)
    QString startMarker = "/*##^##";
    QString endMarker = "##^##*/";
    int startIndex = currentQml.indexOf(startMarker);
    int endIndex = currentQml.indexOf(endMarker, startIndex);
    if (startIndex != -1 && endIndex != -1) {
        int lengthToRemove = (endIndex + endMarker.length()) - startIndex;
        currentQml.remove(startIndex, lengthToRemove);
    }

    QByteArray groqApiKey = designerSettings().value("GroqApiKey", "").toByteArray();

    QNetworkRequest request(QUrl("https://api.groq.com/openai/v1/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QByteArray("Bearer ").append(groqApiKey));

    QString systemTemplate = R"(
You are a QML expert assistant specializing in Qt 6.4+ code generation. Follow these rules strictly:
1. **Output Format**: Reply ONLY with raw, runnable QML code. No explanations, markdown, or extra text.
2. **Imports**: Do not remove any `import` statement. Add missing ones if needed (e.g., `QtQuick`, `QtQuick3D`).
3. **Identifiers**: IDs must start with lowercase and avoid reserved words (e.g., `text1` instead of `text`).
4. **Property Binding**: Don't modify any assigmed property bindings (e.g. color: Constants.backgroundColor).
5. **Scope**: Generate a SINGLE QML file. Use inline `Component` blocks if reusable parts are needed.
6. **QtQuick**: Only use built-in QtQuick/QtQuick3D components. No custom components, external images, or C++ integrations.
7. **Completeness**: Ensure the code is runnable. Include required properties (e.g., `width`, `height`) for root objects.
8. **Modifications**: If the user requests changes to existing code, retain the original structure and only modify requested parts.
9. **Root Item**: Never change the root item type, always keep it as is, in the current QML.
10. **Image Assets**: You have access to the following project image assets: %1. Use them in your generations when appropriate.
)";

    bool isUiQml = QmlDesignerPlugin::instance()->currentDesignDocument()->fileName().endsWith(".ui.qml");
    if (isUiQml) {
        systemTemplate += "11. **Declarative Only**: Write only declarative QML components. Do not include any JavaScript logic, signal handlers (like onClicked), or imperative code. Just define static UI structure, properties, and bindings—no behaviors or event handling.";
        systemTemplate += "12. **States**: The `states` property must ALWAYS be declared on the root object only. If the user requests states for a child object, define the state on the root and use the `PropertyChanges { target: <childId> ... }` pattern to apply changes to that object.";
    }

    QString userTemplate = R"(
Current QML:
%1

Request: %2
)";

    QJsonObject userJson;
    if (const QUrl &imageUrl = fullImageUrl(attachedImageSource()); !imageUrl.isEmpty()) {
        Utils::FilePath imagePath = Utils::FilePath::fromUrl(imageUrl);
        if (imagePath.exists()) {
            QImage image(imagePath.toFSPathString());
            QByteArray byteArray;
            QBuffer buffer(&byteArray);

            image.save(&buffer, imagePath.suffix().toLatin1());

            QString base64Image = "data:image/" + imagePath.suffix() + ";base64,"
                                  + QString::fromLatin1(byteArray.toBase64());
            userJson  = {
                {"role", "user"},
                {"content", QJsonArray{
                    QJsonObject{{"type", "text"}, {"text", userTemplate.arg(currentQml, prompt)}},
                    QJsonObject{
                        {"type", "image_url"},
                        {"image_url", QJsonObject{{"url", base64Image}}}
                    }
                }}
            };
        }
    }

    if (userJson.isEmpty())
        userJson = QJsonObject{{"role", "user"}, {"content", userTemplate.arg(currentQml, prompt)}};

    const QString imagePaths = getImageAssetsPaths().join('\n');

    QJsonObject json;
    json["model"] = "meta-llama/llama-4-maverick-17b-128e-instruct";
    json["messages"] = QJsonArray {
        QJsonObject{{"role", "system"}, {"content", systemTemplate.arg(imagePaths)}},
        userJson,
    };

    m_reply = m_manager->post(request, QJsonDocument(json).toJson());
    setIsGenerating(true);

    connect(m_reply, &QNetworkReply::finished, this, [&]() {
        setIsGenerating(false);
        if (m_reply->error() != QNetworkReply::NoError) {
            qWarning() << "AI Assistant Request failed: " << m_reply->errorString();
            // TODO: notify the user that the request failed (+ maybe show the error message)
        } else {
            QJsonDocument doc = QJsonDocument::fromJson(m_reply->readAll());
            QJsonObject responseObject = doc.object();
            QString content = getContent(responseObject);

            QTC_ASSERT(!content.isEmpty(), m_reply->deleteLater(); return);

            // remove <think> block if exists
            if (content.startsWith("<think>")) {
                int endPos = content.indexOf("</think>");
                if (endPos != -1)
                    content.remove(0, endPos + 8);
                else // If no closing tag, remove the opening tag only
                    content.remove(0, 7); // 7 is length of "<think>"
            }

            content = content.trimmed();

            // remove the start/end sentence and ``` if exists
            if (content.startsWith("```qml"))
                content.remove(0, 6);
            else if (content.startsWith("```"))
                content.remove(0, 3);
            if (content.endsWith("```"))
                content.chop(3);

            auto textModifier = QmlDesignerPlugin::instance()->currentDesignDocument()
                                    ->rewriterView()->textModifier();
            textModifier->replace(0, textModifier->text().size(), content);
        }
        m_reply->deleteLater();
    });
}

QString AiAssistantWidget::getPreviousCommand()
{
    if (m_historyIndex > 0)
        --m_historyIndex;

    if (m_inputHistory.size() > m_historyIndex)
        return m_inputHistory.at(m_historyIndex);

    return {};
}

QString AiAssistantWidget::getNextCommand()
{
    if (int newIdx = m_historyIndex + 1; newIdx < m_inputHistory.size())
        m_historyIndex = newIdx;

    if (m_inputHistory.size() > m_historyIndex)
        return m_inputHistory.at(m_historyIndex);

    return {};
}

QUrl AiAssistantWidget::fullImageUrl(const QString &path) const
{
    return path.isEmpty() ? QUrl()
                          : DocumentManager::currentResourcePath().pathAppended(path).toUrl();
}

void AiAssistantWidget::reloadQmlSource()
{
    const QString itemLibraryQmlPath = qmlSourcesPath() + "/AiAssistantView.qml";
    QTC_ASSERT(QFileInfo::exists(itemLibraryQmlPath), return);
    m_quickWidget->setSource(QUrl::fromLocalFile(itemLibraryQmlPath));
}

void AiAssistantWidget::setIsGenerating(bool val)
{
    if (m_isGenerating != val) {
        m_isGenerating = val;
        emit isGeneratingChanged();
    }
}

QString AiAssistantWidget::attachedImageSource() const
{
    return m_quickWidget->rootObject()->property("attachedImageSource").toString();
}

} // namespace QmlDesigner
