// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "scxmleditordocument.h"
#include "mainwidget.h"
#include "scxmleditorconstants.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <qtsupport/qtkitaspect.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QGuiApplication>
#include <QTextCodec>
#include <QTextDocument>

using namespace Utils;

using namespace ScxmlEditor::Common;
using namespace ScxmlEditor::Internal;

ScxmlEditorDocument::ScxmlEditorDocument(MainWidget *designWidget, QObject *parent)
    : m_designWidget(designWidget)
{
    setMimeType(QLatin1String(ProjectExplorer::Constants::SCXML_MIMETYPE));
    setParent(parent);
    setId(Utils::Id(ScxmlEditor::Constants::K_SCXML_EDITOR_ID));

    // Designer needs UTF-8 regardless of settings.
    setCodec(QTextCodec::codecForName("UTF-8"));
    connect(m_designWidget.data(), &Common::MainWidget::dirtyChanged, this, [this]{
        emit changed();
    });
}

Core::IDocument::OpenResult ScxmlEditorDocument::open(QString *errorString,
                                                      const Utils::FilePath &filePath,
                                                      const Utils::FilePath &realFilePath)
{
    Q_UNUSED(realFilePath)

    if (filePath.isEmpty())
        return OpenResult::ReadError;

    if (!m_designWidget)
        return OpenResult::ReadError;

    const FilePath &absoluteFilePath = filePath.absoluteFilePath();
    if (!m_designWidget->load(absoluteFilePath.toString())) {
        *errorString = m_designWidget->errorMessage();
        return OpenResult::ReadError;
    }

    setFilePath(absoluteFilePath);

    return OpenResult::Success;
}

bool ScxmlEditorDocument::saveImpl(QString *errorString, const FilePath &filePath, bool autoSave)
{
    if (filePath.isEmpty())
        return false;
    bool dirty = m_designWidget->isDirty();

    m_designWidget->setFileName(filePath.toString());
    if (!m_designWidget->save()) {
        *errorString = m_designWidget->errorMessage();
        m_designWidget->setFileName(this->filePath().toString());
        return false;
    }

    if (autoSave) {
        m_designWidget->setFileName(this->filePath().toString());
        m_designWidget->save();
        return true;
    }

    setFilePath(filePath);

    if (dirty != m_designWidget->isDirty())
        emit changed();

    return true;
}

void ScxmlEditorDocument::setFilePath(const FilePath &newName)
{
    m_designWidget->setFileName(newName.toString());
    IDocument::setFilePath(newName);
}

bool ScxmlEditorDocument::shouldAutoSave() const
{
    return false;
}

bool ScxmlEditorDocument::isSaveAsAllowed() const
{
    return true;
}

MainWidget *ScxmlEditorDocument::designWidget() const
{
    return m_designWidget;
}

bool ScxmlEditorDocument::isModified() const
{
    return m_designWidget && m_designWidget->isDirty();
}

bool ScxmlEditorDocument::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(type)
    if (flag == FlagIgnore)
        return true;
    emit aboutToReload();
    emit reloadRequested(errorString, filePath().toString());
    const bool success = errorString->isEmpty();
    emit reloadFinished(success);
    return success;
}

bool ScxmlEditorDocument::supportsCodec(const QTextCodec *codec) const
{
    return codec == QTextCodec::codecForName("UTF-8");
}

QString ScxmlEditorDocument::designWidgetContents() const
{
    return m_designWidget->contents();
}

void ScxmlEditorDocument::syncXmlFromDesignWidget()
{
    document()->setPlainText(designWidgetContents());
}
