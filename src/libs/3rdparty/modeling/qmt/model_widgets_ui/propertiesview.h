/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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

#ifndef QMT_PROPERTIESVIEW_H
#define QMT_PROPERTIESVIEW_H

#include <QObject>

#include "qmt/infrastructure/qmt_global.h"

#include <QScopedPointer>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace qmt {

class ModelController;
class DiagramController;
class MElement;
class MObject;
class MDiagram;
class MRelation;
class DElement;
class StereotypeController;
class StyleController;

class QMT_EXPORT PropertiesView : public QObject
{
    Q_OBJECT
    class MView;

public:
    explicit PropertiesView(QObject *parent = 0);
    ~PropertiesView() override;

    ModelController *modelController() const { return m_modelController; }
    void setModelController(ModelController *modelController);
    DiagramController *diagramController() const { return m_diagramController; }
    void setDiagramController(DiagramController *diagramController);
    StereotypeController *stereotypeController() const { return m_stereotypeController; }
    void setStereotypeController(StereotypeController *stereotypeController);
    StyleController *styleController() const { return m_styleController; }
    void setStyleController(StyleController *styleController);

    QList<MElement *> selectedModelElements() const { return m_selectedModelElements; }
    void setSelectedModelElements(const QList<MElement *> &modelElements);
    QList<DElement *> selectedDiagramElements() const { return m_selectedDiagramElements; }
    MDiagram *selectedDiagram() const { return m_selectedDiagram; }
    void setSelectedDiagramElements(const QList<DElement *> &diagramElements, MDiagram *diagram);
    void clearSelection();

    QWidget *widget() const;

    void editSelectedElement();

private:
    void onBeginResetModel();
    void onEndResetModel();
    void onBeginUpdateObject(int row, const MObject *parent);
    void onEndUpdateObject(int row, const MObject *parent);
    void onBeginInsertObject(int row, const MObject *parent);
    void onEndInsertObject(int row, const MObject *parent);
    void onBeginRemoveObject(int row, const MObject *parent);
    void onEndRemoveObject(int row, const MObject *parent);
    void onBeginMoveObject(int formerRow, const MObject *formerOwner);
    void onEndMoveObject(int row, const MObject *owner);
    void onBeginUpdateRelation(int row, const MObject *parent);
    void onEndUpdateRelation(int row, const MObject *parent);
    void onBeginInsertRelation(int row, const MObject *parent);
    void onEndInsertRelation(int row, const MObject *parent);
    void onBeginRemoveRelation(int row, const MObject *parent);
    void onEndRemoveRelation(int row, const MObject *parent);
    void onBeginMoveRelation(int formerRow, const MObject *formerOwner);
    void onEndMoveRelation(int row, const MObject *owner);
    void onRelationEndChanged(MRelation *relation, MObject *endObject);

    void onBeginResetAllDiagrams();
    void onEndResetAllDiagrams();
    void onBeginResetDiagram(const MDiagram *diagram);
    void onEndResetDiagram(const MDiagram *diagram);
    void onBeginUpdateElement(int row, const MDiagram *diagram);
    void onEndUpdateElement(int row, const MDiagram *diagram);
    void onBeginInsertElement(int row, const MDiagram *diagram);
    void onEndInsertElement(int row, const MDiagram *diagram);
    void onBeginRemoveElement(int row, const MDiagram *diagram);
    void onEndRemoveElement(int row, const MDiagram *diagram);

    void beginUpdate(MElement *modelElement);
    void endUpdate(MElement *modelElement, bool cancelled);
    void beginUpdate(DElement *diagramElement);
    void endUpdate(DElement *diagramElement, bool cancelled);

    ModelController *m_modelController;
    DiagramController *m_diagramController;
    StereotypeController *m_stereotypeController;
    StyleController *m_styleController;
    QList<MElement *> m_selectedModelElements;
    QList<DElement *> m_selectedDiagramElements;
    MDiagram *m_selectedDiagram;
    QScopedPointer<MView> m_mview;
    QWidget *m_widget;
};

} // namespace qmt

#endif // QMT_PROPERTIESVIEW_H
