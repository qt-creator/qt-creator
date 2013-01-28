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

#ifndef PASTER_H
#define PASTER_H

#include <QList>
#include <QMimeData>
#include <QSet>
#include <QString>

#include "import.h"
#include "internalnode_p.h"
#include "modeltotextmerger.h"
#include "textlocation.h"

namespace QmlDesigner {
namespace Internal {

class ASTExtractor;

class Paster
{
    friend class ASTExtractor;

public:
    Paster(QMimeData *transferData, const InternalNode::Pointer &intoNode);

    bool canPaste();
    bool doPaste(ModelToTextMerger &modelToTextMerger);

protected:
    void addImports(const Import &imports);
    void addNode(const QString &nodeText);
    void addNodeState(const QString &stateName, const QString &propertyChanges);

private:
    QList<TextLocation> extractPasteData(QMimeData *transferData);

private:
    InternalNode::Pointer m_targetNode;
    QList<TextLocation> m_pastedLocations;
    QString m_pastedSource;
    ModelToTextMerger *m_modelToTextMerger;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // PASTER_H
