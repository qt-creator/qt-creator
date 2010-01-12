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

#ifndef SCALEMANIPULATOR_H
#define SCALEMANIPULATOR_H

#include <QPointF>
#include <QList>



class QGraphicsLineItem;

namespace QmlDesigner {

class LayerItem;
class FormEditorItem;

class ScaleManipulator
{
public:
    ScaleManipulator(LayerItem *layerItem, FormEditorItem *formEditorItem);
    virtual ~ScaleManipulator();


    void begin(const QPointF& beginPoint);
    void update(const QPointF& updatePoint);
    void reparentTo(FormEditorItem *newParent);
    void end(const QPointF& updatePoint);

private:
  LayerItem *m_layerItem;
  FormEditorItem *m_formEditorItem;
};

}
#endif // SCALEMANIPULATOR_H
