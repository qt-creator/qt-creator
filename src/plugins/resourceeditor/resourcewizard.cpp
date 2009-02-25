/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "resourcewizard.h"
#include "resourceeditorw.h"
#include "resourceeditorconstants.h"

using namespace ResourceEditor;
using namespace ResourceEditor::Internal;

ResourceWizard::ResourceWizard(const BaseFileWizardParameters &parameters, QObject *parent)
  : Core::StandardFileWizard(parameters, parent)
{
}

Core::GeneratedFiles
ResourceWizard::generateFilesFromPath(const QString &path,
                                      const QString &name,
                                      QString *errorMessage) const
{
    Q_UNUSED(errorMessage);
    const QString suffix = preferredSuffix(QLatin1String(Constants::C_RESOURCE_MIMETYPE));
    const QString fileName = Core::BaseFileWizard::buildFileName(path, name, suffix);
    Core::GeneratedFile file(fileName);
    file.setContents(QLatin1String("<RCC/>"));
    file.setEditorKind(QLatin1String(Constants::C_RESOURCEEDITOR));
    return Core::GeneratedFiles() << file;
}
