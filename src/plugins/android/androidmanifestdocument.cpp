/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "androidmanifestdocument.h"
#include "androidmanifesteditorwidget.h"
#include "androidconstants.h"

#include <coreplugin/editormanager/ieditor.h>

#include <QDir>
#include <QDomDocument>

using namespace Android;
using namespace Android::Internal;


AndroidManifestDocument::AndroidManifestDocument(AndroidManifestEditorWidget *editorWidget)
    : TextEditor::BaseTextDocument(),
      m_editorWidget(editorWidget)
{
}

bool AndroidManifestDocument::save(QString *errorString, const QString &fileName, bool autoSave)
{
    m_editorWidget->preSave();
    return BaseTextDocument::save(errorString, fileName, autoSave);
}

QString AndroidManifestDocument::defaultPath() const
{
    QFileInfo fi(fileName());
    return fi.absolutePath();
}

QString AndroidManifestDocument::suggestedFileName() const
{
    QFileInfo fi(fileName());
    return fi.fileName();
}

bool AndroidManifestDocument::isModified() const
{
    return BaseTextDocument::isModified() ||  m_editorWidget->isModified();
}

bool AndroidManifestDocument::isSaveAsAllowed() const
{
    return false;
}
