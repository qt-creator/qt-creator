// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidmanifestdocument.h"
#include "androidmanifesteditorwidget.h"
#include "androidconstants.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/idocument.h>

#include <utils/fileutils.h>

#include <QFileInfo>

using namespace Android;
using namespace Android::Internal;

AndroidManifestDocument::AndroidManifestDocument(AndroidManifestEditorWidget *editorWidget)
    : m_editorWidget(editorWidget)
{
    setId(Constants::ANDROID_MANIFEST_EDITOR_ID);
    setMimeType(QLatin1String(Constants::ANDROID_MANIFEST_MIME_TYPE));
    setSuspendAllowed(false);
    connect(editorWidget, &AndroidManifestEditorWidget::guiChanged,
            this, &Core::IDocument::changed);
}

bool AndroidManifestDocument::saveImpl(QString *errorString,
                                       const Utils::FilePath &filePath,
                                       bool autoSave)
{
    m_editorWidget->preSave();
    bool result = TextDocument::saveImpl(errorString, filePath, autoSave);
    m_editorWidget->postSave();
    return result;
}

bool AndroidManifestDocument::isModified() const
{
    return TextDocument::isModified() ||  m_editorWidget->isModified();
}

bool AndroidManifestDocument::isSaveAsAllowed() const
{
    return false;
}
