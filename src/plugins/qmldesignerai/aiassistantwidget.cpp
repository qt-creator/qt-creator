// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiassistantwidget.h"

#include <asset.h>
#include <designersettings.h>
#include <qmldesignerplugin.h>

#include <utils/filepath.h>

#include <QBuffer>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QTextEdit>

namespace QmlDesigner {

namespace {
QStringList getImageAssetsPaths()
{
    Utils::FilePath resourePath = DocumentManager::currentResourcePath();

    QStringList imagePaths;
    QStringList filters = Asset::supportedImageSuffixes();

    QDirIterator it(resourePath.toFSPathString(), filters, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        imagePaths << Utils::FilePath::fromString(it.next()).relativePathFrom(resourePath).toFSPathString();

    return imagePaths;
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
    if (obj == m_textInput.data() && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return && !(keyEvent->modifiers() & Qt::ShiftModifier)) {
            handleMessage();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

AiAssistantWidget::AiAssistantWidget()
    : m_manager(std::make_unique<QNetworkAccessManager>())
{
    setWindowTitle(tr("AI Assistant", "Title of Ai Assistant widget"));
    setMinimumWidth(220);
    setMinimumHeight(90);

    auto vLayout = new QVBoxLayout(this);
    vLayout->setContentsMargins(5, 5, 5, 5);

    QHBoxLayout *hLayout = new QHBoxLayout();
    hLayout->setSpacing(5);

    m_textInput = new QTextEdit(this);
    m_textInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_textInput->setStyleSheet("padding: 4px; font-size: 16px;");
    m_textInput->setPlaceholderText("Type your message...");
    m_textInput->installEventFilter(this);

    QPushButton *sendButton = new QPushButton(this);
    sendButton->setStyleSheet("font-size: 12px;");
    sendButton->setFixedWidth(32);
    sendButton->setFixedHeight(32);

    QIcon buttonIcon(":/AiAssistant/images/ai.png");
    sendButton->setIcon(buttonIcon);
    sendButton->setIconSize(QSize(20, 20));

    hLayout->addWidget(m_textInput.data());
    hLayout->addWidget(sendButton);
    hLayout->setAlignment(sendButton, Qt::AlignTop);

    hLayout->setStretch(0, 1);

    QPushButton *selectButton = new QPushButton("Attach", this);
    selectButton->setStyleSheet("padding: 4px; font-size: 14px;");

    m_imageLabel = new QLabel("", this);
    m_imageLabel->setStyleSheet("padding: 4px; font-size: 14px; color: gray;");
    m_imageLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    // Create a popup menu
    QMenu *imageMenu = new QMenu(this);

    connect(imageMenu, &QMenu::aboutToShow, this, [imageMenu, this]() {
        imageMenu->clear();
        m_imagePaths = QStringList("") << getImageAssetsPaths();
        for (const QString &option : std::as_const(m_imagePaths)) {
            QAction *action = imageMenu->addAction(option);
            connect(action, &QAction::triggered, m_imageLabel.data(), [option, this]() {
                m_imageLabel->setText(option);
            });
        }
    });

    // Connect the button to show the popup
    connect(selectButton, &QPushButton::clicked, this, [selectButton, imageMenu]() {
        imageMenu->exec(selectButton->mapToGlobal(QPoint(0, selectButton->height())));
    });

    QHBoxLayout *selectLayout = new QHBoxLayout();
    selectLayout->addWidget(selectButton);
    selectLayout->addWidget(m_imageLabel.data());
    selectLayout->addStretch();

    vLayout->addLayout(hLayout);
    vLayout->addLayout(selectLayout);

    connect(sendButton, &QPushButton::clicked, this, &AiAssistantWidget::handleMessage);
}

QSize AiAssistantWidget::sizeHint() const
{
    return {420, 20};
}

void AiAssistantWidget::handleMessage()
{
    QString prompt = m_textInput->toPlainText();
    if (prompt.isEmpty())
        return;

    m_textInput->clear();

    QString currentQml = QmlDesignerPlugin::instance()->currentDesignDocument()
                             ->plainTextEdit()->toPlainText();

    // Remove aux data (it seems to affect the result and make it generate 3D content)
    QString startMarker = "/*##^##";
    QString endMarker = "##^##*/";
    int startIndex = currentQml.indexOf(startMarker);
    int endIndex = currentQml.indexOf(endMarker, startIndex);
    if (startIndex != -1 && endIndex != -1) {
        int lengthToRemove = (endIndex + endMarker.length()) - startIndex;
        currentQml.remove(startIndex, lengthToRemove);
    }

    QByteArray groqApiKey = QmlDesignerPlugin::settings().value("GroqApiKey", "").toByteArray();

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
    if (isUiQml)
        systemTemplate += "11. **Declarative Only**: Write only declarative QML components. Do not include any JavaScript logic, signal handlers (like onClicked), or imperative code. Just define static UI structure, properties, and bindings—no behaviors or event handling.";

    QString userTemplate = R"(
Current QML:
%1

Request: %2
)";

    QJsonObject userJson;
    if (!m_imageLabel->text().isEmpty()) {
        Utils::FilePath imagePath = DocumentManager::currentResourcePath().pathAppended(m_imageLabel->text());
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

    connect(m_reply, &QNetworkReply::finished, this, [&]() {
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

            QmlDesignerPlugin::instance()->currentDesignDocument()->plainTextEdit()->setPlainText(content);
        }
        m_reply->deleteLater();
    });
}

} // namespace QmlDesigner
