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

#include "androidmanifesteditorfactory.h"
#include "androidconstants.h"
#include "androidmanifesteditorwidget.h"
#include "androidmanifesteditor.h"

#include <coreplugin/id.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorsettings.h>

using namespace Android;
using namespace Android::Internal;

class AndroidTextEditorActionHandler : public TextEditor::TextEditorActionHandler
{
public:
    explicit AndroidTextEditorActionHandler(QObject *parent)
        : TextEditorActionHandler(parent, Constants::ANDROID_MANIFEST_EDITOR_CONTEXT)
    {}
private:
    TextEditor::TextEditorWidget *resolveTextEditorWidget(Core::IEditor *editor) const
    {
        AndroidManifestEditor *androidManifestEditor = static_cast<AndroidManifestEditor *>(editor);
        return androidManifestEditor->textEditor();
    }
};

AndroidManifestEditorFactory::AndroidManifestEditorFactory(QObject *parent)
    : Core::IEditorFactory(parent)
{
    setId(Constants::ANDROID_MANIFEST_EDITOR_ID);
    setDisplayName(tr("Android Manifest editor"));
    addMimeType(Constants::ANDROID_MANIFEST_MIME_TYPE);
    new AndroidTextEditorActionHandler(this);
}

Core::IEditor *AndroidManifestEditorFactory::createEditor()
{
    AndroidManifestEditorWidget *androidManifestEditorWidget = new AndroidManifestEditorWidget();
    return androidManifestEditorWidget->editor();
}
