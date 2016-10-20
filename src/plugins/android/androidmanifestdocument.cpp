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

bool AndroidManifestDocument::save(QString *errorString, const QString &fileName, bool autoSave)
{
    m_editorWidget->preSave();
    bool result = TextDocument::save(errorString, fileName, autoSave);
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
