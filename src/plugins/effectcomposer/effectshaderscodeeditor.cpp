// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectshaderscodeeditor.h"

#include "effectcomposereditablenodesmodel.h"
#include "effectcomposermodel.h"
#include "effectcomposertr.h"
#include "effectcomposeruniformsmodel.h"
#include "effectcomposeruniformstablemodel.h"
#include "effectcomposerwidget.h"
#include "effectutils.h"

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <coreplugin/icore.h>

#include <componentcore/theme.h>

#include <qmldesigner/qmldesignerplugin.h>
#include <qmldesignerbase/studio/studioquickwidget.h>

#include <qmljseditor/qmljseditor.h>
#include <qmljseditor/qmljseditordocument.h>

#include <QApplication>
#include <QClipboard>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QPlainTextEdit>
#include <QSettings>
#include <QSplitter>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {

inline constexpr char EFFECTCOMPOSER_LIVE_UPDATE_KEY[] = "EffectComposer/CodeEditor/LiveUpdate";
inline constexpr char EFFECTCOMPOSER_SHADER_EDITOR_GEO_KEY[] = "EffectComposer/CodeEditor/Geometry";
inline constexpr char EFFECTCOMPOSER_SHADER_EDITOR_SPLITTER_KEY[]
    = "EffectComposer/CodeEditor/SplitterSizes";
inline constexpr char OBJECT_NAME_EFFECTCOMPOSER_SHADER_HEADER[]
    = "QQuickWidgetEffectComposerCodeEditorHeader";
inline constexpr char OBJECT_NAME_EFFECTCOMPOSER_SHADER_EDITOR_TABS[]
    = "QQuickWidgetEffectComposerCodeEditorTabs";
inline constexpr char OBJECT_NAME_EFFECTCOMPOSER_SHADER_EDITOR_FOOTER[]
    = "QQuickWidgetEffectComposerCodeEditorFooter";

inline constexpr char EFFECTCOMPOSER_VERTEX_ID[] = "VERTEX";
inline constexpr char EFFECTCOMPOSER_FRAGMENT_ID[] = "FRAGMENT";

QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toUrlishString();
}

template<typename T>
QByteArray serializeList(const QList<T> &list)
{
    QJsonDocument doc;
    QJsonArray jsonArray;
    for (const T &value : list)
        jsonArray.push_back(value);
    doc.setArray(jsonArray);
    return doc.toJson();
}

template<typename T>
QList<T> deserializeList(const QByteArray &serialData)
{
    const QJsonDocument doc = QJsonDocument::fromJson(serialData);
    if (!doc.isArray())
        return {};

    QList<T> result;
    const QJsonArray jsonArray = doc.array();
    for (const QJsonValue &val : jsonArray)
        result.append(val.toVariant().value<T>());

    return result;
}

void resetDocumentRevisions(TextEditor::TextDocumentPtr textDoc)
{
    QTextDocument *doc = textDoc->document();
    const int blockCount = doc->blockCount();
    const int docRevision = doc->revision();

    for (int i = 0; i < blockCount; ++i) {
        QTextBlock block = doc->findBlockByNumber(i);
        block.setRevision(docRevision);
    }
}

} // namespace

namespace EffectComposer {

EffectShadersCodeEditor::EffectShadersCodeEditor(const QString &title, QWidget *parent)
    : QWidget(parent)
    , m_settings(new QSettings(qApp->organizationName(), qApp->applicationName(), this))
    , m_defaultTableModel(new EffectComposerUniformsTableModel(nullptr, this))
    , m_editableNodesModel(new EffectComposerEditableNodesModel(this))
{
    setWindowFlag(Qt::Tool, true);
    setWindowModality(Qt::WindowModality::NonModal);
    setWindowTitle(title);

    setupUIComponents();
    setUniformsModel(nullptr);
    loadQml();
}

EffectShadersCodeEditor::~EffectShadersCodeEditor()
{
    if (isOpened())
        close();

    m_headerWidget->setSource({});
    m_qmlTabWidget->setSource({});
    m_qmlFooterWidget->setSource({});
}

void EffectShadersCodeEditor::showWidget()
{
    readAndApplyLiveUpdateSettings();
    setParent(Core::ICore::dialogParent());
    show();
    raise();
    setOpened(true);
}

void EffectShadersCodeEditor::showWidget(int x, int y)
{
    showWidget();
    move(QPoint(x, y));
}

bool EffectShadersCodeEditor::liveUpdate() const
{
    return m_liveUpdate;
}

void EffectShadersCodeEditor::setLiveUpdate(bool liveUpdate)
{
    if (m_liveUpdate == liveUpdate)
        return;

    m_liveUpdate = liveUpdate;
    writeLiveUpdateSettings();

    emit liveUpdateChanged(m_liveUpdate);

    if (m_liveUpdate)
        emit rebakeRequested();
}

bool EffectShadersCodeEditor::isOpened() const
{
    return m_opened;
}

void EffectShadersCodeEditor::setCompositionsModel(EffectComposerModel *compositionsModel)
{
    m_editableNodesModel->setSourceModel(compositionsModel);
}

void EffectShadersCodeEditor::setupShader(ShaderEditorData *data)
{
    if (m_currentEditorData == data)
        return;

    auto oldEditorData = m_currentEditorData;
    m_currentEditorData = data;

    if (data) {
        m_stackedWidget->addWidget(data->fragmentEditor.get());
        m_stackedWidget->addWidget(data->vertexEditor.get());

        selectNonEmptyShader(data);
        setUniformsModel(data->tableModel);
    } else {
        setUniformsModel(nullptr);
    }

    if (oldEditorData) {
        m_stackedWidget->removeWidget(oldEditorData->fragmentEditor.get());
        m_stackedWidget->removeWidget(oldEditorData->vertexEditor.get());
    }
}

void EffectShadersCodeEditor::cleanFromData(ShaderEditorData *data)
{
    if (m_currentEditorData == data)
        setupShader(nullptr);
}

void EffectShadersCodeEditor::selectShader(const QString &shaderName)
{
    using namespace Qt::StringLiterals;
    if (!m_currentEditorData)
        return;
    EffectCodeEditorWidget *editor = nullptr;
    if (shaderName == EFFECTCOMPOSER_FRAGMENT_ID)
        editor = m_currentEditorData->fragmentEditor.get();
    else if (shaderName == EFFECTCOMPOSER_VERTEX_ID)
        editor = m_currentEditorData->vertexEditor.get();

    m_stackedWidget->setCurrentWidget(editor);
}

ShaderEditorData *EffectShadersCodeEditor::createEditorData(
    const QString &fragmentDocument,
    const QString &vertexDocument,
    EffectComposerUniformsModel *uniforms)
{
    ShaderEditorData *result = new ShaderEditorData;
    result->fragmentEditor.reset(createJSEditor());
    result->vertexEditor.reset(createJSEditor());

    result->fragmentEditor->setPlainText(fragmentDocument);
    result->vertexEditor->setPlainText(vertexDocument);

    result->fragmentDocument = result->fragmentEditor->textDocumentPtr();
    result->vertexDocument = result->vertexEditor->textDocumentPtr();

    ::resetDocumentRevisions(result->fragmentDocument);
    ::resetDocumentRevisions(result->vertexDocument);

    if (uniforms) {
        result->tableModel = new EffectComposerUniformsTableModel(uniforms, uniforms);
        std::function<QStringList()> uniformNames =
            [uniformsTable = result->tableModel]() -> QStringList {
            if (!uniformsTable)
                return {};

            auto uniformsModel = uniformsTable->sourceModel();
            if (!uniformsModel)
                return {};

            return uniformsModel->uniformNames();
        };

        result->fragmentEditor->setUniformsCallback(uniformNames);
        result->vertexEditor->setUniformsCallback(uniformNames);
    }

    return result;
}

void EffectShadersCodeEditor::copyText(const QString &text)
{
    qApp->clipboard()->setText(text);
}

void EffectShadersCodeEditor::insertTextToCursorPosition(const QString &text)
{
    auto editor = currentEditor();
    if (!editor)
        return;

    editor->textCursor().insertText(text);
    editor->setFocus();
}

EffectShadersCodeEditor *EffectShadersCodeEditor::instance()
{
    static EffectShadersCodeEditor *editorInstance
        = new EffectShadersCodeEditor(Tr::tr("Shaders Code Editor"), Core::ICore::dialogParent());
    return editorInstance;
}

EffectCodeEditorWidget *EffectShadersCodeEditor::createJSEditor()
{
    static EffectCodeEditorFactory f;
    TextEditor::BaseTextEditor *editor = qobject_cast<TextEditor::BaseTextEditor *>(
        f.createEditor());
    Q_ASSERT(editor);

    editor->setParent(this);

    EffectCodeEditorWidget *editorWidget = qobject_cast<EffectCodeEditorWidget *>(
        editor->editorWidget());
    Q_ASSERT(editorWidget);

    f.decorateEditor(editorWidget);
    editorWidget->unregisterAutoCompletion();
    editorWidget->setParent(this);
    editorWidget->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

    return editorWidget;
}

void EffectShadersCodeEditor::setupUIComponents()
{
    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    m_splitter = new QSplitter(this);
    QWidget *tabComplexWidget = new QWidget(this);
    QVBoxLayout *tabsLayout = new QVBoxLayout(tabComplexWidget);
    m_stackedWidget = new QStackedWidget(tabComplexWidget);

    m_splitter->setOrientation(Qt::Vertical);

    createHeader();
    createQmlTabs();
    createQmlFooter();

    verticalLayout->setContentsMargins(0, 0, 0, 0);
    verticalLayout->addWidget(m_splitter);
    tabsLayout->setContentsMargins(0, 0, 0, 0);
    tabsLayout->setSpacing(0);
    tabsLayout->addWidget(m_qmlTabWidget);
    tabsLayout->addWidget(m_stackedWidget);
    tabsLayout->addWidget(m_qmlFooterWidget);

    m_splitter->addWidget(m_headerWidget.get());
    m_splitter->addWidget(tabComplexWidget);

    m_splitter->setCollapsible(0, false);
    m_splitter->setCollapsible(1, false);

    connect(
        m_stackedWidget.get(),
        &QStackedWidget::currentChanged,
        this,
        &EffectShadersCodeEditor::onEditorWidgetChanged);

    setMinimumSize(660, 240);
    resize(900, 600);
}

void EffectShadersCodeEditor::setOpened(bool value)
{
    if (m_opened == value)
        return;

    m_opened = value;
    emit openedChanged(m_opened);
    onOpenStateChanged();
}

void EffectShadersCodeEditor::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);

    emit rebakeRequested();

    setOpened(false);
}

void EffectShadersCodeEditor::writeLiveUpdateSettings()
{
    m_settings->setValue(EFFECTCOMPOSER_LIVE_UPDATE_KEY, m_liveUpdate);
}

void EffectShadersCodeEditor::readAndApplyLiveUpdateSettings()
{
    bool liveUpdateStatus = m_settings->value(EFFECTCOMPOSER_LIVE_UPDATE_KEY, true).toBool();

    setLiveUpdate(liveUpdateStatus);
}

void EffectShadersCodeEditor::writeGeometrySettings()
{
    const QByteArray &splitterSizeData = ::serializeList(m_splitter->sizes());
    m_settings->setValue(EFFECTCOMPOSER_SHADER_EDITOR_GEO_KEY, saveGeometry());
    m_settings->setValue(EFFECTCOMPOSER_SHADER_EDITOR_SPLITTER_KEY, splitterSizeData);
}

void EffectShadersCodeEditor::readAndApplyGeometrySettings()
{
    if (m_settings->contains(EFFECTCOMPOSER_SHADER_EDITOR_GEO_KEY))
        restoreGeometry(m_settings->value(EFFECTCOMPOSER_SHADER_EDITOR_GEO_KEY).toByteArray());

    if (m_settings->contains(EFFECTCOMPOSER_SHADER_EDITOR_SPLITTER_KEY)) {
        const QByteArray &splitterSizeData
            = m_settings->value(EFFECTCOMPOSER_SHADER_EDITOR_SPLITTER_KEY).toByteArray();
        m_splitter->setSizes(::deserializeList<int>(splitterSizeData));
    }
}

void EffectShadersCodeEditor::createHeader()
{
    m_headerWidget = new StudioQuickWidget(this);
    m_headerWidget->quickWidget()->setObjectName(OBJECT_NAME_EFFECTCOMPOSER_SHADER_HEADER);
    m_headerWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    QmlDesigner::Theme::setupTheme(m_headerWidget->engine());
    m_headerWidget->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    m_headerWidget->engine()->addImportPath(EffectUtils::nodesSourcesPath() + "/common");
    m_headerWidget->setClearColor(QmlDesigner::Theme::getColor(
        QmlDesigner::Theme::Color::QmlDesigner_BackgroundColorDarkAlternate));
    m_headerWidget->rootContext()->setContextProperty("shaderEditor", QVariant::fromValue(this));
    m_headerWidget->rootContext()->setContextProperty(
        "editableCompositionsModel", QVariant::fromValue(m_editableNodesModel.get()));
}

void EffectShadersCodeEditor::createQmlTabs()
{
    m_qmlTabWidget = new StudioQuickWidget(this);
    m_qmlTabWidget->quickWidget()->setObjectName(OBJECT_NAME_EFFECTCOMPOSER_SHADER_EDITOR_TABS);
    m_qmlTabWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    QmlDesigner::Theme::setupTheme(m_qmlTabWidget->engine());
    m_qmlTabWidget->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    m_qmlTabWidget->engine()->addImportPath(EffectUtils::nodesSourcesPath() + "/common");
    m_qmlTabWidget->setClearColor(QmlDesigner::Theme::getColor(
        QmlDesigner::Theme::Color::QmlDesigner_BackgroundColorDarkAlternate));
    m_qmlTabWidget->rootContext()->setContextProperty("shaderEditor", QVariant::fromValue(this));
    m_qmlTabWidget->setFixedHeight(37);
}

void EffectShadersCodeEditor::createQmlFooter()
{
    m_qmlFooterWidget = new StudioQuickWidget(this);
    m_qmlFooterWidget->quickWidget()->setObjectName(OBJECT_NAME_EFFECTCOMPOSER_SHADER_EDITOR_FOOTER);
    m_qmlFooterWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    QmlDesigner::Theme::setupTheme(m_qmlFooterWidget->engine());
    m_qmlFooterWidget->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    m_qmlFooterWidget->engine()->addImportPath(EffectUtils::nodesSourcesPath() + "/common");
    m_qmlFooterWidget->setClearColor(QmlDesigner::Theme::getColor(
        QmlDesigner::Theme::Color::QmlDesigner_BackgroundColorDarkAlternate));
    m_qmlFooterWidget->rootContext()->setContextProperty("shaderEditor", QVariant::fromValue(this));
    m_qmlFooterWidget->setFixedHeight(40);
}

void EffectShadersCodeEditor::loadQml()
{
    const QString headerQmlPath = EffectComposerWidget::qmlSourcesPath() + "/CodeEditorHeader.qml";
    QTC_ASSERT(QFileInfo::exists(headerQmlPath), return);
    m_headerWidget->setSource(QUrl::fromLocalFile(headerQmlPath));

    const QString editorTabsQmlPath = EffectComposerWidget::qmlSourcesPath()
                                      + "/CodeEditorTabs.qml";
    QTC_ASSERT(QFileInfo::exists(editorTabsQmlPath), return);
    m_qmlTabWidget->setSource(QUrl::fromLocalFile(editorTabsQmlPath));

    const QString footerQmlPath = EffectComposerWidget::qmlSourcesPath() + "/CodeEditorFooter.qml";
    QTC_ASSERT(QFileInfo::exists(footerQmlPath), return);
    m_qmlFooterWidget->setSource(QUrl::fromLocalFile(footerQmlPath));
}

void EffectShadersCodeEditor::setUniformsModel(EffectComposerUniformsTableModel *uniformsTable)
{
    if (!uniformsTable)
        uniformsTable = m_defaultTableModel;

    m_headerWidget->rootContext()
        ->setContextProperty("uniformsTableModel", QVariant::fromValue(uniformsTable));
}

void EffectShadersCodeEditor::selectNonEmptyShader(ShaderEditorData *data)
{
    auto vertexDoc = data->vertexDocument->document();
    auto fragmentDoc = data->fragmentDocument->document();

    QWidget *widgetToSelect = (fragmentDoc->isEmpty() && !vertexDoc->isEmpty())
                                  ? data->vertexEditor.get()
                                  : data->fragmentEditor.get();

    m_stackedWidget->setCurrentWidget(widgetToSelect);
    widgetToSelect->setFocus();
}

void EffectShadersCodeEditor::setSelectedShaderName(const QString &shaderName)
{
    if (m_selectedShaderName == shaderName)
        return;
    m_selectedShaderName = shaderName;
    emit selectedShaderChanged(m_selectedShaderName);
}

void EffectShadersCodeEditor::onEditorWidgetChanged()
{
    QWidget *currentWidget = m_stackedWidget->currentWidget();
    if (!m_currentEditorData || !currentWidget) {
        setSelectedShaderName({});
        return;
    }

    if (currentWidget == m_currentEditorData->fragmentEditor.get())
        setSelectedShaderName(EFFECTCOMPOSER_FRAGMENT_ID);
    else if (currentWidget == m_currentEditorData->vertexEditor.get())
        setSelectedShaderName(EFFECTCOMPOSER_VERTEX_ID);
    else
        setSelectedShaderName({});
}

void EffectShadersCodeEditor::onOpenStateChanged()
{
    if (isOpened())
        readAndApplyGeometrySettings();
    else
        writeGeometrySettings();
}

EffectCodeEditorWidget *EffectShadersCodeEditor::currentEditor() const
{
    QWidget *currentTab = m_stackedWidget->currentWidget();
    if (!m_currentEditorData || !currentTab)
        return nullptr;

    if (currentTab == m_currentEditorData->fragmentEditor.get())
        return m_currentEditorData->fragmentEditor.get();
    if (currentTab == m_currentEditorData->vertexEditor.get())
        return m_currentEditorData->vertexEditor.get();

    return nullptr;
}

} // namespace EffectComposer
