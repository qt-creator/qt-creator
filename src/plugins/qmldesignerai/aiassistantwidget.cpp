// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiassistantwidget.h"

#include "airesponse.h"

#include <asset.h>
#include <designersettings.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <studioquickwidget.h>

#include <coreplugin/icore.h>
#include <utils/filepath.h>

#include <QApplication>
#include <QBuffer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QQmlContext>
#include <QQmlEngine>
#include <QVBoxLayout>

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

QString toBase64Image(const Utils::FilePath &imagePath)
{
    using namespace Qt::StringLiterals;
    QImage image(imagePath.toFSPathString());
    QByteArray byteArray;
    QBuffer buffer(&byteArray);

    image.save(&buffer, imagePath.suffix().toLatin1());

    return "data:image/%1;base64,%2"_L1.arg(imagePath.suffix(), byteArray.toBase64());
}

QJsonObject getUserJson(
    const QUrl &imageUrl, const QString &filePath, const QString &currentQml, const QString &prompt)
{
    using namespace Qt::StringLiterals;
    using Utils::FilePath;

    QJsonArray jsonContent;

    jsonContent << QJsonObject{
        {"type", "text"},
        {"text", "filePath: %1"_L1.arg(filePath)},
    };

    jsonContent << QJsonObject{
        {"type", "text"},
        {"text", "currentQml:\n```qml\n%1\n```"_L1.arg(currentQml)},
    };

    jsonContent << QJsonObject{
        {"type", "text"},
        {"text", "request: %1"_L1.arg(prompt)},
    };

    if (!imageUrl.isEmpty()) {
        FilePath imagePath = FilePath::fromUrl(imageUrl);
        if (imagePath.exists()) {
            jsonContent << QJsonObject{
                {"type", "image_url"},
                {"image_url", QJsonObject{{"url", toBase64Image(imagePath)}}},
            };
        }
    }

    return {
        {"role", "user"},
        {"content", jsonContent},
    };
}

DesignDocument *currentDesignDocument()
{
    return QmlDesignerPlugin::instance()->currentDesignDocument();
}

RewriterView *rewriterView()
{
    return currentDesignDocument()->rewriterView();
}

// AUX data excluded
QString currentQmlText()
{
    constexpr QStringView annotationsStart{u"/*##^##"};
    constexpr QStringView annotationsEnd{u"##^##*/"};

    QString pureQml = currentDesignDocument()->plainTextEdit()->toPlainText();
    int startIndex = pureQml.indexOf(annotationsStart);
    int endIndex = pureQml.indexOf(annotationsEnd, startIndex);
    if (startIndex != -1 && endIndex != -1) {
        int lengthToRemove = (endIndex + annotationsEnd.length()) - startIndex;
        pureQml.remove(startIndex, lengthToRemove);
    }
    return pureQml;
}

QString relativePathFromProject(const Utils::FilePath &file)
{
    return file.relativePathFromDir(DocumentManager::currentProjectDirPath()).toFSPathString();
}

void selectIds(const QStringList &ids)
{
    if (ids.isEmpty())
        return;

    Model *model = rewriterView()->model();
    ModelNodes selectedNodes;
    selectedNodes.reserve(ids.size());

    for (const QString &id : ids)
        selectedNodes.append(rewriterView()->modelNodeForId(id));

    if (!selectedNodes.isEmpty())
        model->setSelectedModelNodes(selectedNodes);
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
    : m_manager(Utils::makeUniqueObjectPtr<QNetworkAccessManager>())
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

AiAssistantWidget::~AiAssistantWidget() = default;

void AiAssistantWidget::clearAttachedImage()
{
    setAttachedImageSource("");
}

void AiAssistantWidget::initManifest()
{
    const QString extension = currentDesignDocument()->fileName().suffix().toLower();
    if (extension == "qml")
        m_manifest = Manifest::fromJsonResource(":/AiAssistant/manifests/ai.manifest.json");
    else if (extension == "ui.qml")
        m_manifest = Manifest::fromJsonResource(":/AiAssistant/manifests/ai_ui.manifest.json");
    else
        m_manifest = {};
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
                          .relativePathFromDir(resourePath)
                          .toFSPathString();

    return imagePaths;
}

void AiAssistantWidget::handleMessage(const QString &prompt)
{
    if (prompt.isEmpty())
        return;

    if (m_manifest.hasNoRules()) {
        qWarning() << __FUNCTION__ << "Manifest has no rules";
        return;
    }

    m_inputHistory.append(prompt);
    m_historyIndex = m_inputHistory.size();

    QByteArray groqApiKey = QmlDesignerPlugin::settings().value("GroqApiKey", "").toByteArray();

    QNetworkRequest request(QUrl("https://api.groq.com/openai/v1/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QByteArray("Bearer ").append(groqApiKey));

    const Utils::FilePath qmlFile = currentDesignDocument()->fileName();
    const QString imagePaths = getImageAssetsPaths().join('\n');
    m_manifest.setTagsMap({
        {"image_assets", imagePaths},
    });

    QJsonObject userJson = getUserJson(
        fullImageUrl(attachedImageSource()),
        relativePathFromProject(qmlFile),
        currentQmlText(),
        prompt);

    QJsonObject json;
    json["model"] = "meta-llama/llama-4-maverick-17b-128e-instruct";
    json["messages"] = QJsonArray{
        QJsonObject{{"role", "system"}, {"content", m_manifest.toJsonContent()}},
        userJson,
    };

    QNetworkReply *reply = m_manager->post(request, QJsonDocument(json).toJson());
    setIsGenerating(true);

    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        setIsGenerating(false);
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "AI Assistant Request failed: " << reply->errorString() << reply->error();
            // TODO: notify the user that the request failed (+ maybe show the error message)
        } else {
            handleAiResponse(reply->readAll());
        }
        reply->deleteLater();
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
    return m_attachedImageSource;
}

void AiAssistantWidget::setAttachedImageSource(const QString &source)
{
    if (source == m_attachedImageSource)
        return;

    m_attachedImageSource = source;
    emit attachedImageSourceChanged();
}

void AiAssistantWidget::handleAiResponse(const AiResponse &response)
{
    using namespace Qt::StringLiterals;
    using Utils::FilePath;

    if (response.error() != AiResponse::Error::NoError) {
        qWarning() << __FUNCTION__ << "Response Error:" << response.errorString();
        return;
    }

    const AiResponseFile file = response.file();
    if (file.isValid()) {
        const FilePath currentFile = currentDesignDocument()->fileName();
        const QString currentRelativeFilePath = relativePathFromProject(currentFile);
        if (currentRelativeFilePath != file.filePath()
            && currentFile.fileName() != file.filePath()) {
            qWarning() << __FUNCTION__
                       << "Invalid file path '%1' Current filePath is '%2'"_L1
                              .arg(file.filePath(), currentRelativeFilePath);
            return;
        }

        const QString aiFileContent = file.content();
        const QString currentQml = currentQmlText();
        if (!aiFileContent.isEmpty() && currentQml != aiFileContent) {
            auto textModifier = rewriterView()->textModifier();
            textModifier->replace(0, textModifier->text().size(), aiFileContent);
        }
    }

    selectIds(response.selectedIds());
}

} // namespace QmlDesigner
