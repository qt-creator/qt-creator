/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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
