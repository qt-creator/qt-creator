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

#ifndef SINGLESELECTIONMANIPULATOR_H
#define SINGLESELECTIONMANIPULATOR_H

#include "formeditorview.h"

#include <QPointF>

namespace QmlDesigner {

class SingleSelectionManipulator
{
public:
    SingleSelectionManipulator(FormEditorView *editorView);

//    void setItems(const QList<FormEditorItem*> &itemList);

    enum SelectionType {
        ReplaceSelection,
        AddToSelection,
        RemoveFromSelection,
        InvertSelection
    };

    void begin(const QPointF& beginPoint);
    void update(const QPointF& updatePoint);
    void end(const QPointF& updatePoint);

    void select(SelectionType selectionType);

    void clear();

    QPointF beginPoint() const;

    bool isActive() const;

private:
    QList<QmlItemNode> m_oldSelectionList;
    QPointF m_beginPoint;
    FormEditorView *m_editorView;
    bool m_isActive;
};

}
#endif // SINGLESELECTIONMANIPULATOR_H
