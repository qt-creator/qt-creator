// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectshaderscodeeditor.h"

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
#include <QPlainTextEdit>
#include <QSettings>
#include <QSplitter>
#include <QTabWidget>
#include <QVBoxLayout>

namespace {

inline constexpr char EFFECTCOMPOSER_LIVE_UPDATE_KEY[] = "EffectComposer/CodeEditor/LiveUpdate";
inline constexpr char OBJECT_NAME_EFFECTCOMPOSER_SHADER_HEADER[]
    = "QQuickWidgetEffectComposerCodeEditorHeader";

QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toString();
}

} // namespace

namespace EffectComposer {

EffectShadersCodeEditor::EffectShadersCodeEditor(const QString &title, QWidget *parent)
    : QWidget(parent)
    , m_settings(new QSettings(qApp->organizationName(), qApp->applicationName(), this))
    , m_defaultTableModel(new EffectComposerUniformsTableModel(nullptr, this))
{
    setWindowFlag(Qt::Tool, true);
    setWindowFlag(Qt::WindowStaysOnTopHint);
    setWindowTitle(title);

    setupUIComponents();
    setUniformsModel(nullptr);
}

EffectShadersCodeEditor::~EffectShadersCodeEditor()
{
    if (isOpened())
        close();
}

void EffectShadersCodeEditor::showWidget()
{
    readAndApplyLiveUpdateSettings();
    reloadQml();
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

void EffectShadersCodeEditor::setupShader(ShaderEditorData *data)
{
    if (m_currentEditorData == data)
        return;

    while (m_tabWidget->count())
        m_tabWidget->removeTab(0);

    if (data) {
        m_tabWidget->addTab(data->fragmentEditor.get(), tr("Fragment Shader"));
        m_tabWidget->addTab(data->vertexEditor.get(), tr("Vertex Shader"));

        selectNonEmptyShader(data);
        setUniformsModel(data->tableModel);
    } else {
        setUniformsModel(nullptr);
    }

    m_currentEditorData = data;
}

void EffectShadersCodeEditor::cleanFromData(ShaderEditorData *data)
{
    if (m_currentEditorData == data)
        setupShader(nullptr);
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

EffectShadersCodeEditor *EffectShadersCodeEditor::instance()
{
    static EffectShadersCodeEditor *editorInstance = new EffectShadersCodeEditor(
        tr("Shaders Code Editor"));
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

    editorWidget->setLineNumbersVisible(false);
    editorWidget->setMarksVisible(false);
    editorWidget->setCodeFoldingSupported(false);
    editorWidget->setTabChangesFocus(true);
    editorWidget->unregisterAutoCompletion();
    editorWidget->setParent(this);
    editorWidget->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

    return editorWidget;
}

void EffectShadersCodeEditor::setupUIComponents()
{
    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    QSplitter *splitter = new QSplitter(this);
    m_tabWidget = new QTabWidget(this);

    splitter->setOrientation(Qt::Vertical);

    createHeader();

    verticalLayout->setContentsMargins(0, 0, 0, 0);
    verticalLayout->addWidget(splitter);
    splitter->addWidget(m_headerWidget.get());
    splitter->addWidget(m_tabWidget);

    splitter->setCollapsible(0, false);
    splitter->setCollapsible(1, false);

    this->resize(660, 240);
}

void EffectShadersCodeEditor::setOpened(bool value)
{
    if (m_opened == value)
        return;

    m_opened = value;
    emit openedChanged(m_opened);
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
    bool liveUpdateStatus = m_settings->value(EFFECTCOMPOSER_LIVE_UPDATE_KEY, false).toBool();

    setLiveUpdate(liveUpdateStatus);
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
}

void EffectShadersCodeEditor::reloadQml()
{
    const QString headerQmlPath = EffectComposerWidget::qmlSourcesPath() + "/CodeEditorHeader.qml";
    QTC_ASSERT(QFileInfo::exists(headerQmlPath), return);
    m_headerWidget->setSource(QUrl::fromLocalFile(headerQmlPath));
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

    QWidget *widgetToSelect = (vertexDoc->isEmpty() && !fragmentDoc->isEmpty())
                                  ? data->fragmentEditor.get()
                                  : data->vertexEditor.get();

    m_tabWidget->setCurrentWidget(widgetToSelect);
    widgetToSelect->setFocus();
}

} // namespace EffectComposer
