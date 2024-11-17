// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectshaderscodeeditor.h"

#include "effectcodeeditorwidget.h"
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
{
    setWindowFlag(Qt::Tool, true);
    setWindowFlag(Qt::WindowStaysOnTopHint);
    setWindowTitle(title);

    m_fragmentEditor = createJSEditor();
    m_vertexEditor = createJSEditor();

    connect(
        m_fragmentEditor,
        &QPlainTextEdit::textChanged,
        this,
        &EffectShadersCodeEditor::fragmentValueChanged);
    connect(
        m_vertexEditor,
        &QPlainTextEdit::textChanged,
        this,
        &EffectShadersCodeEditor::vertexValueChanged);

    setupUIComponents();
}

EffectShadersCodeEditor::~EffectShadersCodeEditor()
{
    if (isOpened())
        close();
    m_fragmentEditor->deleteLater();
    m_vertexEditor->deleteLater();
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

QString EffectShadersCodeEditor::fragmentValue() const
{
    if (!m_fragmentEditor)
        return {};

    return m_fragmentEditor->document()->toPlainText();
}

void EffectShadersCodeEditor::setFragmentValue(const QString &text)
{
    if (m_fragmentEditor)
        m_fragmentEditor->setEditorTextWithIndentation(text);
}

QString EffectShadersCodeEditor::vertexValue() const
{
    if (!m_vertexEditor)
        return {};

    return m_vertexEditor->document()->toPlainText();
}

void EffectShadersCodeEditor::setVertexValue(const QString &text)
{
    if (m_vertexEditor)
        m_vertexEditor->setEditorTextWithIndentation(text);
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

void EffectShadersCodeEditor::setUniformsModel(EffectComposerUniformsModel *uniforms)
{
    std::function<QStringList()> uniformNames = [uniforms]() -> QStringList {
        if (!uniforms)
            return {};
        int count = uniforms->rowCount();
        QStringList result;
        for (int i = 0; i < count; ++i) {
            const QModelIndex mIndex = uniforms->index(i, 0);
            result.append(mIndex.data(EffectComposerUniformsModel::NameRole).toString());
        }
        return result;
    };
    m_fragmentEditor->setUniformsCallback(uniformNames);
    m_vertexEditor->setUniformsCallback(uniformNames);

    if (m_headerWidget && uniforms) {
        m_uniformsTableModel
            = Utils::makeUniqueObjectLatePtr<EffectComposerUniformsTableModel>(uniforms, this);
        EffectComposerUniformsTableModel *uniformsTable = m_uniformsTableModel.get();

        m_headerWidget->rootContext()
            ->setContextProperty("uniformsTableModel", QVariant::fromValue(uniformsTable));
    }
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
    QTabWidget *tabWidget = new QTabWidget(this);

    splitter->setOrientation(Qt::Vertical);

    createHeader();

    tabWidget->addTab(m_fragmentEditor, tr("Fragment Shader"));
    tabWidget->addTab(m_vertexEditor, tr("Vertex Shader"));

    verticalLayout->setContentsMargins(0, 0, 0, 0);
    verticalLayout->addWidget(splitter);
    splitter->addWidget(m_headerWidget.get());
    splitter->addWidget(tabWidget);

    splitter->setCollapsible(0, false);
    splitter->setCollapsible(1, false);

    connect(this, &EffectShadersCodeEditor::openedChanged, tabWidget, [this, tabWidget](bool opened) {
        if (!opened)
            return;

        QWidget *widgetToSelect = (m_vertexEditor->document()->isEmpty()
                                   && !m_fragmentEditor->document()->isEmpty())
                                      ? m_fragmentEditor.get()
                                      : m_vertexEditor.get();
        tabWidget->setCurrentWidget(widgetToSelect);
        widgetToSelect->setFocus();
    });

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

} // namespace EffectComposer
