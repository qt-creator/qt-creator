/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "qmt/infrastructure/uid.h"

#include <QObject>
#include <QHash>

namespace qmt {

class UndoController;
class ModelController;
class MElement;
class MObject;
class MDiagram;
class MRelation;
class DSelection;
class DContainer;
class DReferences;
class DElement;
class DRelation;

class QMT_EXPORT DiagramController : public QObject
{
    Q_OBJECT

public:
    enum UpdateAction {
        UpdateGeometry, // update only position and size of element
        UpdateMajor, // a major update of the element which will create a separate undo command
        UpdateMinor // a minor update of the element which may be merged with other minor updates in one undo command
    };

private:
    class Clone;
    class DiagramUndoCommand;
    class UpdateElementCommand;
    class AbstractAddRemCommand;
    class AddElementsCommand;
    class RemoveElementsCommand;
    class FindDiagramsVisitor;

public:
    explicit DiagramController(QObject *parent = nullptr);
    ~DiagramController() override;

signals:
    void beginResetAllDiagrams();
    void endResetAllDiagrams();
    void beginResetDiagram(const MDiagram *diagram);
    void endResetDiagram(const MDiagram *diagram);
    void beginUpdateElement(int row, const MDiagram *diagram);
    void endUpdateElement(int row, const MDiagram *diagram);
    void beginInsertElement(int row, const MDiagram *diagram);
    void endInsertElement(int row, const MDiagram *diagram);
    void beginRemoveElement(int row, const MDiagram *diagram);
    void endRemoveElement(int row, const MDiagram *diagram);
    void modified(const MDiagram *diagram);
    void diagramAboutToBeRemoved(const MDiagram *diagram);

public:
    ModelController *modelController() const { return m_modelController; }
    void setModelController(ModelController *modelController);
    UndoController *undoController() const { return m_undoController; }
    void setUndoController(UndoController *undoController);
    QList<MDiagram *> allDiagrams() const { return m_allDiagrams; }

private:
    MDiagram *findDiagram(const Uid &diagramKey) const;

public:
    void addElement(DElement *element, MDiagram *diagram);
    void removeElement(DElement *element, MDiagram *diagram);

    DElement *findElement(const Uid &key, const MDiagram *diagram) const;

    template<class T>
    T *findElement(const Uid &key, const MDiagram *diagram) const
    {
        return dynamic_cast<T *>(findElement(key, diagram));
    }

    bool hasDelegate(const MElement *modelElement, const MDiagram *diagram) const;
    DElement *findDelegate(const MElement *modelElement, const MDiagram *diagram) const;

    template<class T>
    T *findDelegate(const MElement *modelElement, const MDiagram *diagram) const
    {
        return dynamic_cast<T *>(findDelegate(modelElement, diagram));
    }

    void startUpdateElement(DElement *element, MDiagram *diagram, UpdateAction updateAction);
    void finishUpdateElement(DElement *element, MDiagram *diagram, bool cancelled);

    void breakUndoChain();

    DContainer cutElements(const DSelection &diagramSelection, MDiagram *diagram);
    DContainer copyElements(const DSelection &diagramSelection, const MDiagram *diagram);
    void pasteElements(const DContainer &diagramContainer, MDiagram *diagram);
    void deleteElements(const DSelection &diagramSelection, MDiagram *diagram);

private:
    void onBeginResetModel();
    void onEndResetModel();
    void onBeginUpdateObject(int row, const MObject *parent);
    void onEndUpdateObject(int row, const MObject *parent);
    void onBeginInsertObject(int row, const MObject *owner);
    void onEndInsertObject(int row, const MObject *owner);
    void onBeginRemoveObject(int row, const MObject *parent);
    void onEndRemoveObject(int row, const MObject *parent);
    void onBeginMoveObject(int formerRow, const MObject *formerOwner);
    void onEndMoveObject(int row, const MObject *owner);
    void onBeginUpdateRelation(int row, const MObject *owner);
    void onEndUpdateRelation(int row, const MObject *owner);
    void onBeginRemoveRelation(int row, const MObject *owner);
    void onEndRemoveRelation(int row, const MObject *owner);
    void onBeginMoveRelation(int formerRow, const MObject *formerOwner);
    void onEndMoveRelation(int row, const MObject *owner);

    void deleteElements(const DSelection &diagramSelection, MDiagram *diagram,
                        const QString &commandLabel);

    DElement *findElementOnAnyDiagram(const Uid &uid);

    void removeObjects(MObject *modelObject);
    void removeRelations(MRelation *modelRelation);
    void removeRelations(DElement *element, MDiagram *diagram);

    void renewElementKey(DElement *element, QHash<Uid, Uid> *renewedKeys);
    void updateRelationKeys(DRelation *relation, const QHash<Uid, Uid> &renewedKeys);
    void updateElementFromModel(DElement *element, const MDiagram *diagram, bool emitUpdateSignal);

    void diagramModified(MDiagram *diagram);

    DReferences simplify(const DSelection &diagramSelection, const MDiagram *diagram);

    MElement *delegatedElement(const DElement *element) const;
    bool isDelegatedElementOnDiagram(const DElement *element, const MDiagram *diagram) const;
    bool areRelationEndsOnDiagram(const DRelation *relation, const MDiagram *diagram) const;

    void updateAllDiagramsList();

    void verifyDiagramsIntegrity();
    void verifyDiagramIntegrity(const MDiagram *diagram);

    ModelController *m_modelController = nullptr;
    UndoController *m_undoController = nullptr;
    QList<MDiagram *> m_allDiagrams;
};

} // namespace qmt
