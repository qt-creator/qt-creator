// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiassistantwidget.h"

#include "aiassistantconstants.h"
#include "aiassistanttermsdialog.h"
#include "aiassistantutils.h"
#include "aiassistantview.h"
#include "aimodelsmodel.h"
#include "airesponse.h"
#include "mcpconfigloader.h"
#include "mcp/agenticrequestmanager.h"
#include "mcp/conversationmanager.h"
#include "mcp/mcphost.h"
#include "mcp/toolregistry.h"

#include <asset.h>
#include <designersettings.h>
#include <studioquickwidget.h>
#include <theme.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>

#include <coreplugin/icore.h>
#include <utils/filepath.h>
#include <utils/environment.h>

#include <QApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QVBoxLayout>

namespace QmlDesigner {

namespace {

bool showAgenticDebug = false;

QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
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

DesignDocument *currentDesignDocument()
{
    return QmlDesignerPlugin::instance()->currentDesignDocument();
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

AiAssistantWidget::AiAssistantWidget(AiAssistantView *view)
    : m_quickWidget(Utils::makeUniqueObjectPtr<StudioQuickWidget>())
    , m_modelsModel(Utils::makeUniqueObjectPtr<AiModelsModel>())
    , m_mcpHost(Utils::makeUniqueObjectPtr<McpHost>())
    , m_toolRegistry(Utils::makeUniqueObjectPtr<ToolRegistry>())
    , m_conversation(std::make_unique<ConversationManager>())
    , m_requestManager(Utils::makeUniqueObjectPtr<AgenticRequestManager>(
        m_mcpHost.get(),
        m_toolRegistry.get(),
        m_conversation.get()))
    , m_view(view)
    , m_termsAccepted(
          Core::ICore::settings()->value(Constants::aiAssistantTermsAcceptedKey, false).toBool())
{
    setWindowTitle(tr("AI Assistant", "Title of AI Assistant widget"));
    setMinimumWidth(240);
    setMinimumHeight(200);

    connectRequestManager();

    auto vLayout = new QVBoxLayout(this);
    vLayout->setContentsMargins(0, 0, 0, 0);

    m_quickWidget->quickWidget()->setObjectName(Constants::OBJECT_NAME_AI_ASSISTANT);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quickWidget->setMinimumHeight(minimumHeight() - 5);
    m_quickWidget->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    m_quickWidget->quickWidget()->installEventFilter(this);
    m_quickWidget->setClearColor(Theme::getColor(Theme::Color::DSpanelBackground));

    auto map = m_quickWidget->registerPropertyMap("AiAssistantBackend");
    map->setProperties({
        {"rootView", QVariant::fromValue(this)},
    });

    vLayout->addWidget(m_quickWidget.get());

    setFocusProxy(m_quickWidget->quickWidget());
    QmlDesignerPlugin::trackWidgetFocus(this, Constants::EVENT_AIASSISTANT);

    reloadQmlSource();
    updateModelConfig();
}

AiAssistantWidget::~AiAssistantWidget() = default;

void AiAssistantWidget::clear()
{
    setAttachedImageSource("");
    m_lastTransaction = AiTransaction();
    emit removeFeedbackPopup();
}

void AiAssistantWidget::initManifest()
{
    const QString extension = currentDesignDocument()->fileName().suffix().toLower();
    if (extension.endsWith("qml")) {
        m_manifest = Manifest::fromJsonResource(":/AiAssistant/manifests/ai.manifest.json");

        if (extension == "ui.qml")
            m_manifest.addManifest(Manifest::fromJsonResource(":/AiAssistant/manifests/ai_ui.manifest.json"));
    } else {
        m_manifest = {};
    }
}

void AiAssistantWidget::updateModelConfig()
{
    m_modelsModel->update();
    setHasValidModel(m_modelsModel->rowCount() > 0);
}

void AiAssistantWidget::removeMissingAttachedImage()
{
    const QUrl imageUrl = fullImageUrl(attachedImageSource());
    if (imageUrl.isEmpty())
        return;

    if (Utils::FilePath file = Utils::FilePath::fromUrl(imageUrl); !file.exists())
        setAttachedImageSource({});
}

void AiAssistantWidget::setProjectPath(const QString &projectPath)
{
    if (m_projectPath == projectPath)
        return;
    m_projectPath = projectPath;

    initializeMcp();
}

QSize AiAssistantWidget::sizeHint() const
{
    return {420, 20};
}

QStringList AiAssistantWidget::getImageAssetsPaths() const
{
    Utils::FilePath resourePath = DocumentManager::currentResourcePath();

    QStringList imagePaths;
    const QStringList filters = {
        "*.jpg",
        "*.jpeg",
        "*.jfif",
        "*.png",
        "*.webp",
        "*.gif",
        "*.bmp",
        "*.tif",
        "*.tiff",
    };

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
    m_manifest.setTagsMap({
        {"image_assets", getImageAssetsPaths().join('\n')},
    });

    m_inputHistory.append(prompt);
    m_historyIndex = m_inputHistory.size();

    const AiModelInfo modelInfo = m_modelsModel->selectedInfo();
    if (!modelInfo.isValid()) {
        qWarning() << __FUNCTION__ << "No model configuration";
        return; // TODO: Notify error
    }

    RequestData reqData {
        .manifest = m_manifest.toString(),
        .userPrompt = prompt,
        .currentQml = AiAssistantUtils::currentQmlText(),
        .currentFilePath = QmlDesignerPlugin::instance()->currentDesignDocument()->fileName()
                               .relativePathFromDir(DocumentManager::currentResourcePath())
                               .toFSPathString(), // Current file's path relative to resouces dir
        .projectStructure = m_projectStructure,
        .attachedImageUrl = fullImageUrl(attachedImageSource()),
    };

    m_requestManager->request(reqData, modelInfo);
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

void AiAssistantWidget::retryLastPrompt()
{
    QTC_ASSERT(!m_inputHistory.isEmpty(), return);

    QString lastPrompt = m_inputHistory.last();
    handleMessage(lastPrompt);
}

void AiAssistantWidget::applyLastGeneratedQml()
{
    m_lastTransaction.commit();
}

void AiAssistantWidget::undoLastChange()
{
    m_lastTransaction.rollback();
}

void AiAssistantWidget::sendThumbFeedback(bool up)
{
    QmlDesignerPlugin::instance()->sendStatisticsFeedback(m_view->widgetInfo().feedbackDisplayName,
                                                          m_inputHistory.last(),
                                                          up ? 1 : -1);
}

void AiAssistantWidget::openModelSettings()
{
    Core::ICore::showOptionsDialog(Constants::aiAssistantSettingsPageId);
}

void AiAssistantWidget::openTermsDialog()
{
    AiAssistantTermsDialog dialog;
    if (dialog.exec() == QDialog::Accepted) {
        Core::ICore::settings()->setValue(Constants::aiAssistantTermsAcceptedKey, true);
        m_termsAccepted = true;
        emit termsAcceptedChanged();
    }
}

void AiAssistantWidget::initializeMcp()
{
    m_projectStructure.clear();

    // Load server configs
    McpConfigLoader loader;
    loader.setResolveMap({{"${PROJECT_PATH}", m_projectPath}});
    bool success = loader.loadFile(":/AiAssistant/mcpconfig.json");
    QTC_ASSERT(success, return);

    m_mcpHost->addServers(loader.getConfigs());

    // Wire resource signals before starting so we don't miss the first fetch
    connectMcpResourceSignals();

    m_mcpHost->restart();

    // Register tools
    const QStringList serverConfigs = m_mcpHost->servers();
    for (const QString &serverName : serverConfigs)
        m_toolRegistry->registerServer(serverName, m_mcpHost->client(serverName));
}

void AiAssistantWidget::connectMcpResourceSignals()
{
    // Fetch the project structure as soon as the QML server is ready
    connect(m_mcpHost.get(), &McpHost::serverStarted, this,
            [this](const QString &serverName, const McpServerInfo &) {
                if (serverName == QLatin1String(Constants::qmlServerName))
                    fetchProjectStructure();
            });

    // Store the structure text when the read completes
    connect(m_mcpHost.get(), &McpHost::resourceReadSucceeded, this,
            [this](const QString &serverName, const QJsonObject &result, qint64) {
                if (serverName != QLatin1String(Constants::qmlServerName))
                    return;

                // The MCP resource/read response wraps content in a "contents" array.
                // Each entry has a "text" field for text/plain resources.
                const QJsonArray contents = result.value("contents").toArray();
                if (!contents.isEmpty())
                    m_projectStructure = contents.first().toObject().value("text").toString();
                else
                    m_projectStructure = result.value("text").toString(); // fallback

                if (showAgenticDebug) {
                    qDebug() << "[Agentic] Log" << QString("[MCP] Project structure updated (%1 chars)")
                                        .arg(m_projectStructure.size());
                }
            });

    // Re-fetch structure after any tool call that changes the file structure
    connect(m_mcpHost.get(), &McpHost::toolCallSucceeded, this,
            [this](const QString &serverName, const QJsonObject &, qint64) {
                Q_UNUSED(serverName)
                // We check the last tool that was reported as started by the request manager
                // via the toolCallStarted signal.
                if (m_pendingStructureRefresh && serverName == Constants::qmlServerName)
                    fetchProjectStructure();
            });
}

void AiAssistantWidget::fetchProjectStructure()
{
    m_mcpHost->readResource(Constants::qmlServerName, Constants::structureResourceUri);
}

void AiAssistantWidget::connectRequestManager()
{
    AgenticRequestManager *reqManager = m_requestManager.get();

    // Lifecycle signals
    connect(reqManager, &AgenticRequestManager::started,
            this, [this]() {
                setIsGenerating(true);
            });

    connect(reqManager, &AgenticRequestManager::finished,
            this, [this]() {
                setIsGenerating(false);
            });

    // Error handling
    connect(reqManager, &AgenticRequestManager::errorOccurred,
            this, [this](const QString &error) {
                emit notifyAIResponseError(error);
            });

    // Success - handle response
    connect(reqManager, &AgenticRequestManager::responseReady,
            this, [this](const AiResponse &response) {
                handleAiResponse(response);
            });

    // Progress signals
    connect(reqManager, &AgenticRequestManager::iterationStarted,
            this, [](int iteration, int max) {
                if (showAgenticDebug)
                    qDebug() << "[Agentic] Iteration" << iteration << "of" << max;
            });

    // Track which tool is being called so we know whether to re-fetch structure on success
    connect(reqManager, &AgenticRequestManager::toolCallStarted,
            this, [this](const QString &server, const QString &tool) {
                if (showAgenticDebug)
                    qDebug() << "[Agentic] Calling tool:" << tool << "on" << server;

                static const QSet<QString> structureMutatingTools = {
                    "create_qml",
                    "delete_qml",
                    "move_qml",
                };
                m_pendingStructureRefresh = structureMutatingTools.contains(tool);
            });

    connect(reqManager, &AgenticRequestManager::toolCallFinished,
            this, [](const QString &server, const QString &tool, bool success) {
                if (showAgenticDebug)
                    qDebug() << "[Agentic] Tool" << tool << (success ? "succeeded" : "failed");
            });

    connect(reqManager, &AgenticRequestManager::logMessage,
            this, [](const QString &msg) {
                if (showAgenticDebug)
                    qDebug() << "[Agentic] Log" << msg;
            });
}

void AiAssistantWidget::reloadQmlSource()
{
    const QString aiAssistantQmlPath = qmlSourcesPath() + "/AiAssistantView.qml";
    QTC_ASSERT(QFileInfo::exists(aiAssistantQmlPath), return);
    m_quickWidget->setSource(QUrl::fromLocalFile(aiAssistantQmlPath));
}

void AiAssistantWidget::setIsGenerating(bool val)
{
    if (m_isGenerating != val) {
        m_isGenerating = val;
        emit isGeneratingChanged();
    }
}

void AiAssistantWidget::setHasValidModel(bool val)
{
    if (m_hasValidModel != val) {
        m_hasValidModel = val;
        emit hasValidModelChanged();
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

QAbstractItemModel *AiAssistantWidget::modelsModel() const
{
    return m_modelsModel.get();
}

void AiAssistantWidget::handleAiResponse(const AiResponse &response)
{
    using namespace Qt::StringLiterals;
    using Utils::FilePath;

    if (response.error() != AiResponse::Error::NoError) {
        emit notifyAIResponseError(response.errorString());
        return;
    }

    m_lastTransaction = AiTransaction(response);

    if (m_lastTransaction.producesQmlError()) {
        emit notifyAIResponseInvalidQml();
    } else {
        m_lastTransaction.commit();
        emit notifyAIResponseSuccess();
    }
}

} // namespace QmlDesigner
