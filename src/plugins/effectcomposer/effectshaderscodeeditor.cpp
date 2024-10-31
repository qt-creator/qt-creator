// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectshaderscodeeditor.h"
#include "effectcodeeditorwidget.h"

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <componentcore/designeractionmanager.h>
#include <componentcore/designericons.h>
#include <componentcore/theme.h>

#include <qmldesigner/qmldesignerplugin.h>
#include <qmljseditor/qmljseditor.h>
#include <qmljseditor/qmljseditordocument.h>

#include <qmldesignerplugin.h>

#include <QPlainTextEdit>
#include <QApplication>
#include <QSettings>
#include <QTabWidget>
#include <QToolBar>
#include <QVBoxLayout>

namespace {

using IconId = QmlDesigner::DesignerIcons::IconId;

inline constexpr char EFFECTCOMPOSER_LIVE_UPDATE_KEY[] = "EffectComposer/CodeEditor/LiveUpdate";

QIcon toolbarIcon(IconId iconId)
{
    return QmlDesigner::DesignerActionManager::instance().toolbarIcon(iconId);
};

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

    editorWidget->installEventFilter(this);

    return editorWidget;
}

void EffectShadersCodeEditor::setupUIComponents()
{
    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    QTabWidget *tabWidget = new QTabWidget(this);

    tabWidget->addTab(m_fragmentEditor, tr("Fragment Shader"));
    tabWidget->addTab(m_vertexEditor, tr("Vertex Shader"));

    verticalLayout->setContentsMargins(0, 0, 0, 0);
    verticalLayout->addWidget(createToolbar());
    verticalLayout->addWidget(tabWidget);

    connect(this, &EffectShadersCodeEditor::openedChanged, tabWidget, [this, tabWidget](bool opened) {
        if (!opened)
            return;

        QWidget *widgetToSelect = (m_vertexEditor->document()->isEmpty()
                                   && !m_fragmentEditor->document()->isEmpty())
                                      ? m_fragmentEditor.get()
                                      : m_vertexEditor.get();
        tabWidget->setCurrentWidget(widgetToSelect);
        widgetToSelect->setFocus();
        activateWindow();
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

    if (!liveUpdate())
        emit rebakeRequested();

    setOpened(false);
}

bool EffectShadersCodeEditor::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == event->FocusOut) {
        QFocusEvent *focusEvent = dynamic_cast<QFocusEvent *>(event);
        if (focusEvent && focusEvent->reason() == Qt::ActiveWindowFocusReason) {
            connect(
                qApp,
                &QApplication::focusWindowChanged,
                this,
                &EffectShadersCodeEditor::close,
                Qt::SingleShotConnection);
        }
    }

    return QWidget::eventFilter(watched, event);
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

QToolBar *EffectShadersCodeEditor::createToolbar()
{
    using QmlDesigner::Theme;

    QToolBar *toolbar = new QToolBar(this);

    toolbar->setFixedHeight(Theme::toolbarSize());
    toolbar->setFloatable(false);
    toolbar->setContentsMargins(0, 0, 0, 0);

    toolbar->setStyleSheet(Theme::replaceCssColors(
        QString::fromUtf8(Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css"))));

    QAction *liveUpdateButton
        = toolbar->addAction(toolbarIcon(IconId::LiveUpdateIcon), tr("Live Update"));
    liveUpdateButton->setCheckable(true);
    connect(liveUpdateButton, &QAction::toggled, this, &EffectShadersCodeEditor::setLiveUpdate);

    QAction *applyAction = toolbar->addAction(toolbarIcon(IconId::SyncIcon), tr("Apply"));
    connect(applyAction, &QAction::triggered, this, &EffectShadersCodeEditor::rebakeRequested);

    auto syncLive = [liveUpdateButton, applyAction](bool liveState) {
        liveUpdateButton->setChecked(liveState);
        applyAction->setDisabled(liveState);
    };

    connect(this, &EffectShadersCodeEditor::liveUpdateChanged, this, syncLive);
    syncLive(liveUpdate());

    toolbar->addAction(liveUpdateButton);
    toolbar->addAction(applyAction);

    return toolbar;
}

} // namespace EffectComposer
