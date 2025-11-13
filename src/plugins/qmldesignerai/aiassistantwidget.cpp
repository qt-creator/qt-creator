// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiassistantwidget.h"

#include "aiapimanager.h"
#include "aiassistantconstants.h"
#include "aiassistanttermsdialog.h"
#include "aiassistantview.h"
#include "aimodelsmodel.h"
#include "airesponse.h"

#include <astcheck/astcheck.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsreformatter.h>
#include <qmljs/qmljsstaticanalysismessage.h>

#include <asset.h>
#include <designersettings.h>
#include <studioquickwidget.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>

#include <coreplugin/icore.h>
#include <utils/filepath.h>

#include <QApplication>
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

QString reformatQml(const QString &content)
{
    auto document = QmlJS::Document::create({}, QmlJS::Dialect::QmlQtQuick2);
    document->setSource(content);
    document->parseQml();
    if (document->isParsedCorrectly())
        return QmlJS::reformat(document);

    return content;
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

AiAssistantWidget::AiAssistantWidget(AiAssistantView *view)
    : m_apiManager(Utils::makeUniqueObjectPtr<AiApiManager>())
    , m_quickWidget(Utils::makeUniqueObjectPtr<StudioQuickWidget>())
    , m_modelsModel(Utils::makeUniqueObjectPtr<AiModelsModel>())
    , m_view(view)
    , m_termsAccepted(
          Core::ICore::settings()->value(Constants::aiAssistantTermsAcceptedKey, false).toBool())
{
    setWindowTitle(tr("AI Assistant", "Title of AI Assistant widget"));
    setMinimumWidth(240);
    setMinimumHeight(200);

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

    connectApiManager();
    reloadQmlSource();
    updateModelConfig();
}

AiAssistantWidget::~AiAssistantWidget() = default;

void AiAssistantWidget::clear()
{
    setAttachedImageSource("");
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

    m_apiManager->request(
        {
            .manifest = m_manifest.toString(),
            .qml = currentQmlText(),
            .userPrompt = prompt,
            .attachedImage = fullImageUrl(attachedImageSource()),
        },
        modelInfo);
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
    auto textModifier = rewriterView()->textModifier();
    textModifier->replace(0, textModifier->text().size(), m_lastGeneratedQml);
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

void AiAssistantWidget::connectApiManager()
{
    AiApiManager *apiManager = m_apiManager.get();

    connect(
        apiManager,
        &AiApiManager::started,
        this,
        std::bind_front(&AiAssistantWidget::setIsGenerating, this, true));

    connect(
        apiManager,
        &AiApiManager::finished,
        this,
        std::bind_front(&AiAssistantWidget::setIsGenerating, this, false));

    connect(
        apiManager,
        &AiApiManager::responseError,
        this,
        [this](const auto &, const auto &, const QString &error) {
            emit notifyAIResponseError(error);
        });

    connect(
        apiManager,
        &AiApiManager::responseReady,
        this,
        [this](const auto &, const auto &, const AiResponse &response) {
            handleAiResponse(response);
        });
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

    QStringList selectedIds = response.selectedIds();
    if (!selectedIds.isEmpty()) {
        selectIds(selectedIds);
    } else {
        const QString newQml = reformatQml(response.qml());
        const QString currentQml = currentQmlText();

        if (!newQml.isEmpty() && currentQml != newQml) {
            m_lastGeneratedQml = newQml;
            if (isValidQmlCode(newQml)) {
                auto textModifier = rewriterView()->textModifier();
                textModifier->replace(0, textModifier->text().size(), newQml);
                emit notifyAIResponseSuccess();
            } else {
                emit notifyAIResponseInvalidQml();
            }
        }
    }
}

bool AiAssistantWidget::isValidQmlCode(const QString &qmlCode) const
{
    using namespace QmlJS;

    Document::MutablePtr doc = Document::create(Utils::FilePath::fromString("<internal>"), Dialect::Qml);
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

} // namespace QmlDesigner
