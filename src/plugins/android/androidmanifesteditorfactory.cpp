// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "androidmanifesteditorfactory.h"
#include "androidconstants.h"
#include "androidmanifesteditorwidget.h"
#include "androidmanifesteditor.h"

#include <texteditor/texteditorsettings.h>

using namespace Android;
using namespace Android::Internal;

AndroidManifestEditorFactory::AndroidManifestEditorFactory()
    : m_actionHandler(Constants::ANDROID_MANIFEST_EDITOR_ID,
                      Constants::ANDROID_MANIFEST_EDITOR_CONTEXT,
                      TextEditor::TextEditorActionHandler::None,
                      [](Core::IEditor *editor) { return static_cast<AndroidManifestEditor *>(editor)->textEditor(); })
{
    setId(Constants::ANDROID_MANIFEST_EDITOR_ID);
    setDisplayName(AndroidManifestEditorWidget::tr("Android Manifest editor"));
    addMimeType(Constants::ANDROID_MANIFEST_MIME_TYPE);
    setEditorCreator([] {
        auto androidManifestEditorWidget = new AndroidManifestEditorWidget;
        return androidManifestEditorWidget->editor();
    });
}
