/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#ifndef QNX_INTERNAL_BARDESCRIPTORDOCUMENT_H
#define QNX_INTERNAL_BARDESCRIPTORDOCUMENT_H

#include <coreplugin/textdocument.h>

#include <QDomNode>

namespace Qnx {
namespace Internal {

struct BarDescriptorAsset {
    QString source;
    QString destination;
    bool entry;
};

class BarDescriptorEditorWidget;
class BarDescriptorDocumentAbstractNodeHandler;

class BarDescriptorDocument : public Core::TextDocument
{
    Q_OBJECT
public:
    explicit BarDescriptorDocument(BarDescriptorEditorWidget *editorWidget);
    ~BarDescriptorDocument();

    bool open(QString *errorString, const QString &fileName);
    bool save(QString *errorString, const QString &fileName = QString(), bool autoSave = false);
    QString fileName() const;

    QString defaultPath() const;
    QString suggestedFileName() const;
    QString mimeType() const;

    bool shouldAutoSave() const;
    bool isModified() const;
    bool isSaveAsAllowed() const;

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);
    void rename(const QString &newName);

    QString xmlSource() const;
    bool loadContent(const QString &xmlSource, QString *errorMessage = 0, int *errorLine = 0);

private:
    void registerNodeHandler(BarDescriptorDocumentAbstractNodeHandler *nodeHandler);
    BarDescriptorDocumentAbstractNodeHandler *nodeHandlerForDomNode(const QDomNode &node);
    void removeUnknownNodeHandlers();

    QList<BarDescriptorDocumentAbstractNodeHandler *> m_nodeHandlers;

    QString m_fileName;

    BarDescriptorEditorWidget *m_editorWidget;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_BARDESCRIPTORDOCUMENT_H
