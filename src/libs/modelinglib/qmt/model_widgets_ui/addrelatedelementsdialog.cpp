// Copyright (C) 2018 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "addrelatedelementsdialog.h"

#include <qmt/tasks/diagramscenecontroller.h>
#include <qmt/diagram_controller/dselection.h>
#include <qmt/model/mdiagram.h>
#include <qmt/model/mrelation.h>
#include <qmt/model/mdependency.h>
#include <qmt/model/minheritance.h>
#include <qmt/model/massociation.h>
#include <qmt/model/mconnection.h>
#include <qmt/model_controller/modelcontroller.h>
#include <qmt/model_controller/mvoidvisitor.h>

#include "../../modelinglibtr.h"

#include <utils/layoutbuilder.h>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QStringListModel>

namespace qmt {

namespace {

enum class RelationType {
    Any,
    Dependency,
    Association,
    Inheritance,
    Connection
};

enum class RelationDirection {
    Any,
    Outgoing,
    Incoming,
    Bidirectional
};

enum class ElementType {
    Any,
    Package,
    Component,
    Class,
    Diagram,
    Item,
};

class Filter : public qmt::MVoidConstVisitor {
public:

    void setRelationType(RelationType newRelationType)
    {
        m_relationType = newRelationType;
    }

    void setRelationTypeId(const QString &newRelationTypeId)
    {
        m_relationTypeId = newRelationTypeId;
    }

    void setRelationDirection(RelationDirection newRelationDirection)
    {
        m_relationDirection = newRelationDirection;
    }

    void setRelationStereotypes(const QStringList &newRelationStereotypes)
    {
        m_relationStereotypes = newRelationStereotypes;
    }

    void setElementType(ElementType newElementType)
    {
        m_elementType = newElementType;
    }

    void setElementStereotypes(const QStringList &newElementStereotypes)
    {
        m_elementStereotypes = newElementStereotypes;
    }

    void setObject(const qmt::DObject *dobject, const qmt::MObject *mobject)
    {
        m_dobject = dobject;
        m_mobject = mobject;
    }

    void setRelation(const qmt::MRelation *relation)
    {
        m_relation = relation;
    }

    bool keep() const { return m_keep; }

    void reset()
    {
        m_dobject = nullptr;
        m_mobject = nullptr;
        m_relation = nullptr;
        m_keep = true;
    }

    // MConstVisitor interface
    void visitMObject(const qmt::MObject *object) override
    {
        if (!m_elementStereotypes.isEmpty()) {
            const QStringList stereotypes = object->stereotypes();
            bool containsElementStereotype = std::any_of(
                stereotypes.constBegin(), stereotypes.constEnd(),
                [&](const QString &s) { return m_elementStereotypes.contains(s); });
            if (!containsElementStereotype) {
                m_keep = false;
                return;
            }
        }
    }

    void visitMPackage(const qmt::MPackage *package) override
    {
        if (m_elementType == ElementType::Any || m_elementType == ElementType::Package)
            qmt::MVoidConstVisitor::visitMPackage(package);
        else
            m_keep = false;
    }

    void visitMClass(const qmt::MClass *klass) override
    {
        if (m_elementType == ElementType::Any || m_elementType == ElementType::Class)
            qmt::MVoidConstVisitor::visitMClass(klass);
        else
            m_keep = false;
    }

    void visitMComponent(const qmt::MComponent *component) override
    {
        if (m_elementType == ElementType::Any || m_elementType == ElementType::Component)
            qmt::MVoidConstVisitor::visitMComponent(component);
        else
            m_keep = false;
    }

    void visitMDiagram(const qmt::MDiagram *diagram) override
    {
        if (m_elementType == ElementType::Any || m_elementType == ElementType::Diagram)
            qmt::MVoidConstVisitor::visitMDiagram(diagram);
        else
            m_keep = false;
    }

    void visitMItem(const qmt::MItem *item) override
    {
        if (m_elementType == ElementType::Any || m_elementType == ElementType::Item)
            qmt::MVoidConstVisitor::visitMItem(item);
        else
            m_keep = false;
    }

    void visitMRelation(const qmt::MRelation *relation) override
    {
        if (!m_relationStereotypes.isEmpty()) {
            const QStringList relationStereotypes = relation->stereotypes();
            bool containsRelationStereotype = std::any_of(
                relationStereotypes.constBegin(), relationStereotypes.constEnd(),
                [&](const QString &s) { return m_relationStereotypes.contains(s); });
            if (!containsRelationStereotype) {
                m_keep = false;
                return;
            }
        }
    }

    void visitMDependency(const qmt::MDependency *dependency) override
    {
        if (m_relationDirection != RelationDirection::Any) {
            bool keep = false;
            if (m_relationDirection == RelationDirection::Outgoing) {
                if (dependency->direction() == qmt::MDependency::AToB && dependency->endAUid() == m_mobject->uid())
                    keep = true;
                else if (dependency->direction() == qmt::MDependency::BToA && dependency->endBUid() == m_mobject->uid())
                    keep = true;
            } else if (m_relationDirection == RelationDirection::Incoming) {
                if (dependency->direction() == qmt::MDependency::AToB && dependency->endBUid() == m_mobject->uid())
                    keep = true;
                else if (dependency->direction() == qmt::MDependency::BToA && dependency->endAUid() == m_mobject->uid())
                    keep = true;
            } else if (m_relationDirection == RelationDirection::Bidirectional) {
                if (dependency->direction() == qmt::MDependency::Bidirectional)
                    keep = true;
            }
            m_keep = keep;
            if (!keep)
                return;
        }
        if (m_relationType == RelationType::Any || m_relationType == RelationType::Dependency)
            qmt::MVoidConstVisitor::visitMDependency(dependency);
        else
            m_keep = false;
    }

    bool testDirection(const qmt::MRelation *relation)
    {
        if (m_relationDirection != RelationDirection::Any) {
            bool keep = false;
            if (m_relationDirection == RelationDirection::Outgoing) {
                if (relation->endAUid() == m_mobject->uid())
                    keep = true;
            } else if (m_relationDirection == RelationDirection::Incoming) {
                if (relation->endBUid() == m_mobject->uid())
                    keep = true;
            }
            m_keep = keep;
            if (!keep)
                return false;
        }
        return true;
    }

    void visitMInheritance(const qmt::MInheritance *inheritance) override
    {
        if (!testDirection(inheritance))
            return;
        if (m_relationType == RelationType::Any || m_relationType == RelationType::Inheritance)
            qmt::MVoidConstVisitor::visitMInheritance(inheritance);
        else
            m_keep = false;
    }

    void visitMAssociation(const qmt::MAssociation *association) override
    {
        if (!testDirection(association))
            return;
        if (m_relationType == RelationType::Any || m_relationType == RelationType::Association)
            qmt::MVoidConstVisitor::visitMAssociation(association);
        else
            m_keep = false;
    }

    void visitMConnection(const qmt::MConnection *connection) override
    {
        if (!testDirection(connection))
            return;
        if (m_relationType == RelationType::Any || m_relationType == RelationType::Connection)
            qmt::MVoidConstVisitor::visitMConnection(connection);
        else
            m_keep = false;
    }

private:
    RelationType m_relationType = RelationType::Any;
    QString m_relationTypeId;
    RelationDirection m_relationDirection = RelationDirection::Any;
    QStringList m_relationStereotypes;
    ElementType m_elementType = ElementType::Any;
    QStringList m_elementStereotypes;
    const qmt::DObject *m_dobject = nullptr;
    const qmt::MObject *m_mobject = nullptr;
    const qmt::MRelation *m_relation = nullptr;
    bool m_keep = true;
};
} // namespace

class AddRelatedElementsDialog::Private {
public:
    qmt::DiagramSceneController *m_diagramSceneController = nullptr;
    qmt::DSelection m_selection;
    qmt::Uid m_diagramUid;
    QStringListModel m_relationTypeModel;
    QStringListModel m_relationDirectionModel;
    QStringListModel m_relationStereotypesModel;
    QStringListModel m_elementTypeModel;
    QStringListModel m_elementStereotypesModel;
    Filter m_filter;

    QComboBox *RelationTypeCombobox;
    QComboBox *DirectionCombobox;
    QComboBox *StereotypesCombobox;
    QComboBox *ElementStereotypesCombobox;
    QComboBox *ElementTypeComboBox;
    QLabel *NumberOfMatchingElementsValue;
    QDialogButtonBox *buttonBox;
};

AddRelatedElementsDialog::AddRelatedElementsDialog(QWidget *parent) :
    QDialog(parent),
    d(new Private)
{
    setMinimumWidth(500);

    d->RelationTypeCombobox = new QComboBox;
    d->DirectionCombobox = new QComboBox;
    d->StereotypesCombobox = new QComboBox;
    d->StereotypesCombobox->setEditable(true);
    d->ElementTypeComboBox = new QComboBox;
    d->ElementStereotypesCombobox = new QComboBox;
    d->ElementStereotypesCombobox->setEditable(true);
    d->NumberOfMatchingElementsValue = new QLabel;
    d->buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    using namespace Layouting;
    Column {
        Group {
            title(Tr::tr("Relation Attributes")),
            Form {
                Tr::tr("Type"), d->RelationTypeCombobox, br,
                Tr::tr("Direction"), d->DirectionCombobox, br,
                Tr::tr("Stereotypes"), d->StereotypesCombobox, br,
            },
        },
        Group {
            title(Tr::tr("Other Element Attributes")),
            Form {
                Tr::tr("Type"), d->ElementTypeComboBox, br,
                Tr::tr("Stereotypes"), d->ElementStereotypesCombobox, br,
            },
        },
        Row {
            Tr::tr("Number of matching elements: "), d->NumberOfMatchingElementsValue, st,
        },
        st,
        d->buttonBox,
    }.attachTo(this);

    connect(d->RelationTypeCombobox, &QComboBox::currentIndexChanged, this, &AddRelatedElementsDialog::updateNumberOfElements);
    connect(d->DirectionCombobox, &QComboBox::currentIndexChanged, this, &AddRelatedElementsDialog::updateNumberOfElements);
    connect(d->StereotypesCombobox, &QComboBox::currentTextChanged, this, &AddRelatedElementsDialog::updateNumberOfElements);
    connect(d->ElementTypeComboBox, &QComboBox::currentIndexChanged, this, &AddRelatedElementsDialog::updateNumberOfElements);
    connect(d->ElementStereotypesCombobox, &QComboBox::currentTextChanged, this, &AddRelatedElementsDialog::updateNumberOfElements);
    connect(d->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(d->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(this, &QDialog::accepted, this, &AddRelatedElementsDialog::onAccepted);
}

AddRelatedElementsDialog::~AddRelatedElementsDialog()
{
    delete d;
}

void AddRelatedElementsDialog::setDiagramSceneController(qmt::DiagramSceneController *diagramSceneController)
{
    d->m_diagramSceneController = diagramSceneController;
}

void AddRelatedElementsDialog::setElements(const qmt::DSelection &selection, qmt::MDiagram *diagram)
{
    d->m_selection = selection;
    d->m_diagramUid = diagram->uid();
    QStringList relationTypes = {"Any", "Dependency", "Association", "Inheritance"};
    d->m_relationTypeModel.setStringList(relationTypes);
    d->RelationTypeCombobox->setModel(&d->m_relationTypeModel);
    QStringList relationDirections = {"Any", "Outgoing (->)", "Incoming (<-)", "Bidirectional (<->)"};
    d->m_relationDirectionModel.setStringList(relationDirections);
    d->DirectionCombobox->setModel(&d->m_relationDirectionModel);
    QStringList relationStereotypes = { };
    d->m_relationStereotypesModel.setStringList(relationStereotypes);
    d->StereotypesCombobox->setModel(&d->m_relationStereotypesModel);
    QStringList elementTypes = {"Any", "Package", "Component", "Class", "Diagram", "Item"};
    d->m_elementTypeModel.setStringList(elementTypes);
    d->ElementTypeComboBox->setModel(&d->m_elementTypeModel);
    QStringList elementStereotypes = { };
    d->m_elementStereotypesModel.setStringList(elementStereotypes);
    d->ElementStereotypesCombobox->setModel(&d->m_elementStereotypesModel);
    updateNumberOfElements();
}

void AddRelatedElementsDialog::onAccepted()
{
    qmt::MDiagram *diagram = d->m_diagramSceneController->modelController()->findElement<qmt::MDiagram>(d->m_diagramUid);
    if (diagram) {
        updateFilter();
        d->m_diagramSceneController->addRelatedElements(
            d->m_selection, diagram,
            [this](qmt::DObject *dobject, qmt::MObject *mobject, qmt::MRelation *relation) -> bool
            {
                return this->filter(dobject, mobject, relation);
            });
    }
}

void AddRelatedElementsDialog::updateFilter()
{
    d->m_filter.setRelationType((RelationType) d->RelationTypeCombobox->currentIndex());
    d->m_filter.setRelationDirection((RelationDirection) d->DirectionCombobox->currentIndex());
    d->m_filter.setRelationStereotypes(d->StereotypesCombobox->currentText().split(',', Qt::SkipEmptyParts));
    d->m_filter.setElementType((ElementType) d->ElementTypeComboBox->currentIndex());
    d->m_filter.setElementStereotypes(d->ElementStereotypesCombobox->currentText().split(',', Qt::SkipEmptyParts));
}

bool AddRelatedElementsDialog::filter(qmt::DObject *dobject, qmt::MObject *mobject, qmt::MRelation *relation)
{
    d->m_filter.reset();
    d->m_filter.setObject(dobject, mobject);
    d->m_filter.setRelation(relation);
    relation->accept(&d->m_filter);
    if (!d->m_filter.keep())
        return false;
    qmt::MObject *targetObject = nullptr;
    if (relation->endAUid() != mobject->uid())
        targetObject = d->m_diagramSceneController->modelController()->findObject(relation->endAUid());
    else if (relation->endBUid() != mobject->uid())
        targetObject = d->m_diagramSceneController->modelController()->findObject(relation->endBUid());
    if (!targetObject)
        return false;
    targetObject->accept(&d->m_filter);
    return d->m_filter.keep();
}

void AddRelatedElementsDialog::updateNumberOfElements()
{
    qmt::MDiagram *diagram = d->m_diagramSceneController->modelController()->findElement<qmt::MDiagram>(d->m_diagramUid);
    if (diagram) {
        updateFilter();
        d->NumberOfMatchingElementsValue->setText(QString::number(d->m_diagramSceneController->countRelatedElements(
            d->m_selection, diagram,
            [this](qmt::DObject *dobject, qmt::MObject *mobject, qmt::MRelation *relation) -> bool
            {
                return this->filter(dobject, mobject, relation);
            })));
    }
}

} // namespace qmt
