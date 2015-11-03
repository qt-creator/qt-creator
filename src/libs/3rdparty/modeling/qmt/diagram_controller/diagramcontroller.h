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

#ifndef QMT_DIAGRAMCONTROLLER_H
#define QMT_DIAGRAMCONTROLLER_H

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


class QMT_EXPORT DiagramController :
        public QObject
{
    Q_OBJECT

public:

    enum UpdateAction {
        UPDATE_GEOMETRY, // update only position and size of element
        UPDATE_MAJOR, // a major update of the element which will create a separate undo command
        UPDATE_MINOR // a minor update of the element which may be merged with other minor updates in one undo command
    };

private:

    struct Clone;

    class DiagramUndoCommand;
    class UpdateElementCommand;
    class AbstractAddRemCommand;
    class AddElementsCommand;
    class RemoveElementsCommand;
    class FindDiagramsVisitor;

public:

    explicit DiagramController(QObject *parent = 0);

    ~DiagramController();

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

    ModelController *getModelController() const { return m_modelController; }

    void setModelController(ModelController *model_controller);

    UndoController *getUndoController() const { return m_undoController; }

    void setUndoController(UndoController *undo_controller);

private:

    MDiagram *findDiagram(const Uid &diagram_key) const;

public:

    void addElement(DElement *element, MDiagram *diagram);

    void removeElement(DElement *element, MDiagram *diagram);

    DElement *findElement(const Uid &key, const MDiagram *diagram) const;

    template<class T>
    T *findElement(const Uid &key, const MDiagram *diagram) const { return dynamic_cast<T *>(findElement(key, diagram)); }

    bool hasDelegate(const MElement *model_element, const MDiagram *diagram) const;

    DElement *findDelegate(const MElement *model_element, const MDiagram *diagram) const;

    template<class T>
    T *findDelegate(const MElement *model_element, const MDiagram *diagram) const
    {
        return dynamic_cast<T *>(findDelegate(model_element, diagram));
    }

    void startUpdateElement(DElement *element, MDiagram *diagram, UpdateAction update_action);

    void finishUpdateElement(DElement *element, MDiagram *diagram, bool cancelled);

    void breakUndoChain();

public:

    DContainer cutElements(const DSelection &diagram_selection, MDiagram *diagram);

    DContainer copyElements(const DSelection &diagram_selection, const MDiagram *diagram);

    void pasteElements(const DContainer &diagram_container, MDiagram *diagram);

    void deleteElements(const DSelection &diagram_selection, MDiagram *diagram);

private slots:

    void onBeginResetModel();

    void onEndResetModel();

    void onBeginUpdateObject(int row, const MObject *parent);

    void onEndUpdateObject(int row, const MObject *parent);

    void onBeginInsertObject(int row, const MObject *owner);

    void onEndInsertObject(int row, const MObject *owner);

    void onBeginRemoveObject(int row, const MObject *parent);

    void onEndRemoveObject(int row, const MObject *parent);

    void onBeginMoveObject(int former_row, const MObject *former_owner);

    void onEndMoveObject(int row, const MObject *owner);

    void onBeginUpdateRelation(int row, const MObject *owner);

    void onEndUpdateRelation(int row, const MObject *owner);

    void onBeginRemoveRelation(int row, const MObject *owner);

    void onEndRemoveRelation(int row, const MObject *owner);

    void onBeginMoveRelation(int former_row, const MObject *former_owner);

    void onEndMoveRelation(int row, const MObject *owner);

private:

    void deleteElements(const DSelection &diagram_selection, MDiagram *diagram, const QString &command_label);

    DElement *findElementOnAnyDiagram(const Uid &uid);

    void removeObjects(MObject *model_object);

    void removeRelations(MRelation *model_relation);

    void removeRelations(DElement *element, MDiagram *diagram);

    void renewElementKey(DElement *element, QHash<Uid, Uid> *renewed_keys);

    void updateRelationKeys(DRelation *relation, const QHash<Uid, Uid> &renewed_keys);

    void updateElementFromModel(DElement *element, const MDiagram *diagram, bool emit_update_signal);

    void diagramModified(MDiagram *diagram);

    DReferences simplify(const DSelection &diagram_selection, const MDiagram *diagram);

    MElement *getDelegatedElement(const DElement *element) const;

    bool isDelegatedElementOnDiagram(const DElement *element, const MDiagram *diagram) const;

    bool areRelationEndsOnDiagram(const DRelation *relation, const MDiagram *diagram) const;

    void updateAllDiagramsList();

private:

    ModelController *m_modelController;

    UndoController *m_undoController;

    QList<MDiagram *> m_allDiagrams;
};

}

#endif // DIAGRAMCONTROLLER_H
