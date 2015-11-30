/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    connect(editorWidget, SIGNAL(guiChanged()),
            this, SIGNAL(changed()));
}

bool AndroidManifestDocument::save(QString *errorString, const QString &fileName, bool autoSave)
{
    m_editorWidget->preSave();
    bool result = TextDocument::save(errorString, fileName, autoSave);
    m_editorWidget->postSave();
    return result;
}

QString AndroidManifestDocument::defaultPath() const
{
    return filePath().toFileInfo().absolutePath();
}

QString AndroidManifestDocument::suggestedFileName() const
{
    return filePath().fileName();
}

bool AndroidManifestDocument::isModified() const
{
    return TextDocument::isModified() ||  m_editorWidget->isModified();
}

bool AndroidManifestDocument::isSaveAsAllowed() const
{
    return false;
}
