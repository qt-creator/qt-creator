/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkovic
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
#include "vcdocumentmodel.h"

#include <QDomDocument>
#include <QFile>
#include <QtXmlPatterns/QXmlSchema>
#include <QtXmlPatterns/QXmlSchemaValidator>

#include "vcprojectdocument.h"

namespace VcProjectManager {
namespace Internal {

VcDocumentModel::VcDocumentModel(const QString &filePath, VcDocConstants::DocumentVersion version)
    : m_vcProjectDocument(0)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return;

    QDomDocument document(filePath);
    if (!document.setContent(&file))
        return;

    m_vcProjectDocument = new VcProjectDocument(filePath, version);
    if (m_vcProjectDocument)
        m_vcProjectDocument->processNode(document);
}

VcDocumentModel::~VcDocumentModel()
{
    delete m_vcProjectDocument;
}

IVisualStudioProject *VcDocumentModel::vcProjectDocument() const
{
    return m_vcProjectDocument;
}

bool VcDocumentModel::saveToFile(const QString &filePath) const
{
    if (m_vcProjectDocument)
        return m_vcProjectDocument->saveToFile(filePath);

    return false;
}

} // namespace Internal
} // namespace VcProjectManager
