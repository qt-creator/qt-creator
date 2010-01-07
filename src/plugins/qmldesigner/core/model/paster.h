/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef PASTER_H
#define PASTER_H

#include <QtCore/QList>
#include <QtCore/QMimeData>
#include <QtCore/QSet>
#include <QtCore/QString>

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
