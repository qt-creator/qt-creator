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
#include <QMultiHash>

namespace qmt {

class UndoController;
class MElement;
class MObject;
class MPackage;
class MDiagram;
class MRelation;
class MSelection;
class MContainer;
class MReferences;

class QMT_EXPORT ModelController : public QObject
{
    Q_OBJECT

    enum ElementType {
        TypeUnknown,
        TypeObject,
        TypeRelation };

    class Clone;
    class UpdateObjectCommand;
    class UpdateRelationCommand;
    class AddElementsCommand;
    class RemoveElementsCommand;
    class MoveObjectCommand;
    class MoveRelationCommand;

public:
    explicit ModelController(QObject *parent = nullptr);
    ~ModelController() override;

signals:
    void beginResetModel();
    void endResetModel();
    void beginUpdateObject(int row, const MObject *owner);
    void endUpdateObject(int row, const MObject *owner);
    void beginInsertObject(int row, const MObject *owner);
    void endInsertObject(int row, const MObject *owner);
    void beginRemoveObject(int row, const MObject *owner);
    void endRemoveObject(int row, const MObject *owner);
    void beginMoveObject(int formerRow, const MObject *formerOwner);
    void endMoveObject(int newRow, const MObject *newOwner);
    void beginUpdateRelation(int row, const MObject *owner);
    void endUpdateRelation(int row, const MObject *owner);
    void beginInsertRelation(int row, const MObject *owner);
    void endInsertRelation(int row, const MObject *owner);
    void beginRemoveRelation(int row, const MObject *owner);
    void endRemoveRelation(int row, const MObject *owner);
    void beginMoveRelation(int formerRow, const MObject *formerOwner);
    void endMoveRelation(int newRow, const MObject *newOwner);
    void packageNameChanged(MPackage *package, const QString &oldPackageName);
    void relationEndChanged(MRelation *relation, MObject *endObject);
    void modified();

public:
    MPackage *rootPackage() const { return m_rootPackage; }
    void setRootPackage(MPackage *rootPackage);
    UndoController *undoController() const { return m_undoController; }
    void setUndoController(UndoController *undoController);

    Uid ownerKey(const MElement *element) const;
    MElement *findElement(const Uid &key);
    template<class T>
    T *findElement(const Uid &key) { return dynamic_cast<T *>(findElement(key)); }

    void startResetModel();
    void finishResetModel(bool modified);

    MObject *object(int row, const MObject *owner) const;
    MObject *findObject(const Uid &key) const;
    template<class T>
    T *findObject(const Uid &key) const { return dynamic_cast<T *>(findObject(key)); }
    void addObject(MPackage *parentPackage, MObject *object);
    void removeObject(MObject *object);
    void startUpdateObject(MObject *object);
    void finishUpdateObject(MObject *object, bool cancelled);
    void moveObject(MPackage *newOwner, MObject *object);

    MRelation *findRelation(const Uid &key) const;
    template<class T>
    T *findRelation(const Uid &key) const { return dynamic_cast<T *>(findRelation(key)); }
    void addRelation(MObject *owner, MRelation *relation);
    void removeRelation(MRelation *relation);
    void startUpdateRelation(MRelation *relation);
    void finishUpdateRelation(MRelation *relation, bool cancelled);
    void moveRelation(MObject *newOwner, MRelation *relation);

    QList<MRelation *> findRelationsOfObject(const MObject *object) const;

    MContainer cutElements(const MSelection &modelSelection);
    MContainer copyElements(const MSelection &modelSelection);
    void pasteElements(MObject *owner, const MContainer &modelContainer);
    void deleteElements(const MSelection &modelSelection);

private:
    void deleteElements(const MSelection &modelSelection, const QString &commandLabel);
    void removeRelatedRelations(MObject *object);

public:
    void unloadPackage(MPackage *package);
    void loadPackage(MPackage *package);

private:
    void renewElementKey(MElement *element, QHash<Uid, Uid> *renewedKeys);
    void updateRelationKeys(MElement *element, const QHash<Uid, Uid> &renewedKeys);
    void updateRelationEndKeys(MRelation *relation, const QHash<Uid, Uid> &renewedKeys);
    void mapObject(MObject *object);
    void unmapObject(MObject *object);
    void mapRelation(MRelation *relation);
    void unmapRelation(MRelation *relation);

    MReferences simplify(const MSelection &modelSelection);

    void verifyModelIntegrity() const;
    void verifyModelIntegrity(const MObject *object, QHash<Uid, const MObject *> *objectsMap,
                              QHash<Uid, const MRelation *> *relationsMap,
                              QMultiHash<Uid, MRelation *> *objectRelationsMap) const;

    MPackage *m_rootPackage = nullptr;
    UndoController *m_undoController = nullptr;
    QHash<Uid, MObject *> m_objectsMap;
    QHash<Uid, MRelation *> m_relationsMap;
    QMultiHash<Uid, MRelation *> m_objectRelationsMap;
    bool m_isResettingModel = false;
    QString m_oldPackageName;
};

} // namespace qmt
