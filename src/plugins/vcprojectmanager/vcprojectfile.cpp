/**************************************************************************
**
** Copyright (c) 2014 Bojan Petrovic
** Copyright (c) 2014 Radovan Zivkovic
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
#include "vcprojectfile.h"
#include "vcprojectmanagerconstants.h"
#include "vcprojectmodel/vcdocprojectnodes.h"
#include "vcprojectmodel/vcdocumentmodel.h"
#include "vcprojectmodel/vcprojectdocument.h"

namespace VcProjectManager {
namespace Internal {

VcProjectFile::VcProjectFile(const QString &filePath, VcDocConstants::DocumentVersion docVersion)
{
    setFilePath(filePath);
    m_documentModel = new VcProjectManager::Internal::VcDocumentModel(filePath, docVersion);
}

VcProjectFile::~VcProjectFile()
{
    delete m_documentModel;
}

bool VcProjectFile::save(QString *errorString, const QString &fileName, bool autoSave)
{
    Q_UNUSED(errorString)
    Q_UNUSED(fileName)
    Q_UNUSED(autoSave)
    // TODO: obvious
    return false;
}

QString VcProjectFile::defaultPath() const
{
    // TODO: what's this for?
    return QString();
}

QString VcProjectFile::suggestedFileName() const
{
    // TODO: what's this for?
    return QString();
}

QString VcProjectFile::mimeType() const
{
    return QLatin1String(Constants::VCPROJ_MIMETYPE);
}

bool VcProjectFile::isModified() const
{
    // TODO: obvious
    return false;
}

bool VcProjectFile::isSaveAsAllowed() const
{
    return false;
}

bool VcProjectFile::reload(QString *errorString, Core::IDocument::ReloadFlag flag, Core::IDocument::ChangeType type)
{
    Q_UNUSED(errorString);
    Q_UNUSED(flag);
    Q_UNUSED(type);

    // TODO: what's this for?
    return false;
}

void VcProjectFile::rename(const QString &newName)
{
    Q_UNUSED(newName);

    // TODO: obvious
}

VcDocProjectNode *VcProjectFile::createVcDocNode() const
{
    if (m_documentModel)
        return new VcDocProjectNode(m_documentModel->vcProjectDocument());
    return 0;
}

void VcProjectFile::reloadVcDoc()
{
    VcDocConstants::DocumentVersion docVersion = m_documentModel->vcProjectDocument()->documentVersion();
    delete m_documentModel;
    m_documentModel = new VcDocumentModel(filePath(), docVersion);
}

VcDocumentModel *VcProjectFile::documentModel() const
{
    return m_documentModel;
}

} // namespace Internal
} // namespace VcProjectManager
