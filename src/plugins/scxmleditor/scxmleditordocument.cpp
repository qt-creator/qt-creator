/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "scxmleditordocument.h"
#include "mainwidget.h"
#include "scxmleditorconstants.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>
#include <qtsupport/qtkitinformation.h>
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
    setId(Core::Id(ScxmlEditor::Constants::K_SCXML_EDITOR_ID));

    // Designer needs UTF-8 regardless of settings.
    setCodec(QTextCodec::codecForName("UTF-8"));
    connect(m_designWidget.data(), &Common::MainWidget::dirtyChanged, this, [this]{
        emit changed();
    });
}

Core::IDocument::OpenResult ScxmlEditorDocument::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    Q_UNUSED(realFileName)

    if (fileName.isEmpty())
        return OpenResult::ReadError;

    if (!m_designWidget)
        return OpenResult::ReadError;

    const QFileInfo fi(fileName);
    const QString absfileName = fi.absoluteFilePath();
    if (!m_designWidget->load(absfileName)) {
        *errorString = m_designWidget->errorMessage();
        return OpenResult::ReadError;
    }

    setFilePath(Utils::FileName::fromString(absfileName));

    return OpenResult::Success;
}

bool ScxmlEditorDocument::save(QString *errorString, const QString &name, bool autoSave)
{
    const FileName oldFileName = filePath();
    const FileName actualName = name.isEmpty() ? oldFileName : FileName::fromString(name);
    if (actualName.isEmpty())
        return false;
    bool dirty = m_designWidget->isDirty();

    m_designWidget->setFileName(actualName.toString());
    if (!m_designWidget->save()) {
        *errorString = m_designWidget->errorMessage();
        m_designWidget->setFileName(oldFileName.toString());
        return false;
    }

    if (autoSave) {
        m_designWidget->setFileName(oldFileName.toString());
        m_designWidget->save();
        return true;
    }

    setFilePath(actualName);

    if (dirty != m_designWidget->isDirty())
        emit changed();

    return true;
}

void ScxmlEditorDocument::setFilePath(const FileName &newName)
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
    if (flag == FlagIgnore) {
        return true;
    } if (type == TypePermissions) {
        emit changed();
    } else {
        emit aboutToReload();
        emit reloadRequested(errorString, filePath().toString());
        const bool success = errorString->isEmpty();
        emit reloadFinished(success);
        return success;
    }

    return true;
}

QString ScxmlEditorDocument::designWidgetContents() const
{
    return m_designWidget->contents();
}

void ScxmlEditorDocument::syncXmlFromDesignWidget()
{
    document()->setPlainText(designWidgetContents());
}
