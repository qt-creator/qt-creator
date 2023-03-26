// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "scxmleditorfactory.h"

#include "scxmleditorconstants.h"
#include "scxmleditordata.h"

#include <coreplugin/coreplugintr.h>
#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/fsengine/fileiconprovider.h>

#include <QGuiApplication>
#include <QFileInfo>

using namespace ScxmlEditor::Constants;
using namespace ScxmlEditor::Internal;

ScxmlEditorFactory::ScxmlEditorFactory()
{
    setId(K_SCXML_EDITOR_ID);
    setDisplayName(::Core::Tr::tr(C_SCXMLEDITOR_DISPLAY_NAME));
    addMimeType(ProjectExplorer::Constants::SCXML_MIMETYPE);

    Utils::FileIconProvider::registerIconOverlayForSuffix(":/projectexplorer/images/fileoverlay_scxml.png", "scxml");

    setEditorCreator([this] {
        if (!m_editorData) {
            m_editorData = new ScxmlEditorData;
            QGuiApplication::setOverrideCursor(Qt::WaitCursor);
            m_editorData->fullInit();
            QGuiApplication::restoreOverrideCursor();
        }
        return m_editorData->createEditor();
    });
}

ScxmlEditorFactory::~ScxmlEditorFactory()
{
    delete m_editorData;
}
