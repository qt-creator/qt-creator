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

#include "propertiesviewmview.h"

#include "classmembersedit.h"
#include "palettebox.h"

#include "qmt/model_ui/stereotypescontroller.h"

#include "qmt/model_controller/modelcontroller.h"

#include "qmt/model/melement.h"
#include "qmt/model/mobject.h"
#include "qmt/model/mpackage.h"
#include "qmt/model/mclass.h"
#include "qmt/model/mcomponent.h"
#include "qmt/model/mdiagram.h"
#include "qmt/model/mcanvasdiagram.h"
#include "qmt/model/mitem.h"
#include "qmt/model/mrelation.h"
#include "qmt/model/mdependency.h"
#include "qmt/model/minheritance.h"
#include "qmt/model/massociation.h"

#include "qmt/diagram/delement.h"
#include "qmt/diagram/dobject.h"
#include "qmt/diagram/dpackage.h"
#include "qmt/diagram/dclass.h"
#include "qmt/diagram/dcomponent.h"
#include "qmt/diagram/ddiagram.h"
#include "qmt/diagram/ditem.h"
#include "qmt/diagram/drelation.h"
#include "qmt/diagram/dinheritance.h"
#include "qmt/diagram/ddependency.h"
#include "qmt/diagram/dassociation.h"
#include "qmt/diagram/dannotation.h"
#include "qmt/diagram/dboundary.h"

// TODO move into better place
#include "qmt/diagram_scene/items/stereotypedisplayvisitor.h"

#include "qmt/stereotype/stereotypecontroller.h"

#include "qmt/style/stylecontroller.h"
#include "qmt/style/style.h"
#include "qmt/style/objectvisuals.h"

#include <QWidget>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QPainter>


//#define SHOW_DEBUG_PROPERTIES

namespace qmt {

static int translateDirectionToIndex(MDependency::Direction direction)
{
    switch (direction) {
    case MDependency::AToB:
        return 0;
    case MDependency::BToA:
        return 1;
    case MDependency::Bidirectional:
        return 2;
    }
    return 0;
}

static bool isValidDirectionIndex(int index)
{
    return index >= 0 && index <= 2;
}

static MDependency::Direction translateIndexToDirection(int index)
{
    static const MDependency::Direction map[] = {
        MDependency::AToB, MDependency::BToA, MDependency::Bidirectional
    };
    QMT_CHECK(isValidDirectionIndex(index));
    return map[index];
}

static int translateAssociationKindToIndex(MAssociationEnd::Kind kind)
{
    switch (kind) {
    case MAssociationEnd::Association:
        return 0;
    case MAssociationEnd::Aggregation:
        return 1;
    case MAssociationEnd::Composition:
        return 2;
    }
    return 0;
}

static bool isValidAssociationKindIndex(int index)
{
    return index >= 0 && index <= 2;
}

static MAssociationEnd::Kind translateIndexToAssociationKind(int index)
{
    static const MAssociationEnd::Kind map[] = {
        MAssociationEnd::Association, MAssociationEnd::Aggregation, MAssociationEnd::Composition
    };
    QMT_CHECK(isValidAssociationKindIndex(index));
    return map[index];
}

static int translateVisualPrimaryRoleToIndex(DObject::VisualPrimaryRole visualRole)
{
    switch (visualRole) {
    case DObject::PrimaryRoleNormal:
        return 0;
    case DObject::PrimaryRoleCustom1:
        return 1;
    case DObject::PrimaryRoleCustom2:
        return 2;
    case DObject::PrimaryRoleCustom3:
        return 3;
    case DObject::PrimaryRoleCustom4:
        return 4;
    case DObject::PrimaryRoleCustom5:
        return 5;
    case DObject::DeprecatedPrimaryRoleLighter:
    case DObject::DeprecatedPrimaryRoleDarker:
    case DObject::DeprecatedPrimaryRoleSoften:
    case DObject::DeprecatedPrimaryRoleOutline:
        return 0;
    }
    return 0;
}

static DObject::VisualPrimaryRole translateIndexToVisualPrimaryRole(int index)
{
    static const DObject::VisualPrimaryRole map[] = {
        DObject::PrimaryRoleNormal,
        DObject::PrimaryRoleCustom1, DObject::PrimaryRoleCustom2, DObject::PrimaryRoleCustom3,
        DObject::PrimaryRoleCustom4, DObject::PrimaryRoleCustom5
    };
    QMT_CHECK(index >= 0 && index <= 5);
    return map[index];
}

static int translateVisualSecondaryRoleToIndex(DObject::VisualSecondaryRole visualRole)
{
    switch (visualRole) {
    case DObject::SecondaryRoleNone:
        return 0;
    case DObject::SecondaryRoleLighter:
        return 1;
    case DObject::SecondaryRoleDarker:
        return 2;
    case DObject::SecondaryRoleSoften:
        return 3;
    case DObject::SecondaryRoleOutline:
        return 4;
    }
    return 0;
}

static DObject::VisualSecondaryRole translateIndexToVisualSecondaryRole(int index)
{
    static const DObject::VisualSecondaryRole map[] = {
        DObject::SecondaryRoleNone,
        DObject::SecondaryRoleLighter, DObject::SecondaryRoleDarker,
        DObject::SecondaryRoleSoften, DObject::SecondaryRoleOutline
    };
    QMT_CHECK(index >= 0 && index <= 4);
    return map[index];
}

static int translateStereotypeDisplayToIndex(DObject::StereotypeDisplay stereotypeDisplay)
{
    switch (stereotypeDisplay) {
    case DObject::StereotypeNone:
        return 1;
    case DObject::StereotypeLabel:
        return 2;
    case DObject::StereotypeDecoration:
        return 3;
    case DObject::StereotypeIcon:
        return 4;
    case DObject::StereotypeSmart:
        return 0;
    }
    return 0;
}

static DObject::StereotypeDisplay translateIndexToStereotypeDisplay(int index)
{
    static const DObject::StereotypeDisplay map[] = {
        DObject::StereotypeSmart,
        DObject::StereotypeNone,
        DObject::StereotypeLabel,
        DObject::StereotypeDecoration,
        DObject::StereotypeIcon
    };
    QMT_CHECK(index >= 0 && index <= 4);
    return map[index];
}

static int translateTemplateDisplayToIndex(DClass::TemplateDisplay templateDisplay)
{
    switch (templateDisplay) {
    case DClass::TemplateSmart:
        return 0;
    case DClass::TemplateBox:
        return 1;
    case DClass::TemplateName:
        return 2;
    }
    return 0;
}

static DClass::TemplateDisplay translateIndexToTemplateDisplay(int index)
{
    static const DClass::TemplateDisplay map[] = {
        DClass::TemplateSmart,
        DClass::TemplateBox,
        DClass::TemplateName
    };
    QMT_CHECK(index >= 0 && index <= 2);
    return map[index];
}

static int translateAnnotationVisualRoleToIndex(DAnnotation::VisualRole visualRole)
{
    switch (visualRole) {
    case DAnnotation::RoleNormal:
        return 0;
    case DAnnotation::RoleTitle:
        return 1;
    case DAnnotation::RoleSubtitle:
        return 2;
    case DAnnotation::RoleEmphasized:
        return 3;
    case DAnnotation::RoleSoften:
        return 4;
    case DAnnotation::RoleFootnote:
        return 5;
    }
    return 0;
}

static DAnnotation::VisualRole translateIndexToAnnotationVisualRole(int index)
{
    static const DAnnotation::VisualRole map[] = {
        DAnnotation::RoleNormal, DAnnotation::RoleTitle, DAnnotation::RoleSubtitle,
        DAnnotation::RoleEmphasized, DAnnotation::RoleSoften, DAnnotation::RoleFootnote
    };
    QMT_CHECK(index >= 0 && index <= 5);
    return map[index];
}


PropertiesView::MView::MView(PropertiesView *propertiesView)
    : m_propertiesView(propertiesView),
      m_diagram(0),
      m_stereotypesController(new StereotypesController(this)),
      m_topWidget(0),
      m_topLayout(0),
      m_stereotypeElement(StereotypeIcon::ElementAny),
      m_classNameLabel(0),
      m_stereotypeComboBox(0),
      m_reverseEngineeredLabel(0),
      m_elementNameLineEdit(0),
      m_childrenLabel(0),
      m_relationsLabel(0),
      m_namespaceLineEdit(0),
      m_templateParametersLineEdit(0),
      m_classMembersStatusLabel(0),
      m_classMembersParseButton(0),
      m_classMembersEdit(0),
      m_diagramsLabel(0),
      m_itemVarietyEdit(0),
      m_endALabel(0),
      m_endBLabel(0),
      m_directionSelector(0),
      m_endAEndName(0),
      m_endACardinality(0),
      m_endANavigable(0),
      m_endAKind(0),
      m_endBEndName(0),
      m_endBCardinality(0),
      m_endBNavigable(0),
      m_endBKind(0),
      m_separatorLine(0),
      m_styleElementType(StyleEngine::TypeOther),
      m_posRectLabel(0),
      m_autoSizedCheckbox(0),
      m_visualPrimaryRoleSelector(0),
      m_visualSecondaryRoleSelector(0),
      m_visualEmphasizedCheckbox(0),
      m_stereotypeDisplaySelector(0),
      m_depthLabel(0),
      m_templateDisplaySelector(0),
      m_showAllMembersCheckbox(0),
      m_plainShapeCheckbox(0),
      m_itemShapeEdit(0),
      m_annotationAutoWidthCheckbox(0),
      m_annotationVisualRoleSelector(0)
{
}

PropertiesView::MView::~MView()
{
}

void PropertiesView::MView::update(QList<MElement *> &modelElements)
{
    QMT_CHECK(modelElements.size() > 0);

    m_modelElements = modelElements;
    m_diagramElements.clear();
    m_diagram = 0;
    modelElements.at(0)->accept(this);
}

void PropertiesView::MView::update(QList<DElement *> &diagramElements, MDiagram *diagram)
{
    QMT_CHECK(diagramElements.size() > 0);
    QMT_CHECK(diagram);

    m_diagramElements = diagramElements;
    m_diagram = diagram;
    m_modelElements.clear();
    foreach (DElement *delement, diagramElements) {
        bool appendedMelement = false;
        if (delement->modelUid().isValid()) {
            MElement *melement = m_propertiesView->modelController()->findElement(delement->modelUid());
            if (melement) {
                m_modelElements.append(melement);
                appendedMelement = true;
            }
        }
        if (!appendedMelement) {
            // ensure that indices within m_diagramElements match indices into m_modelElements
            m_modelElements.append(0);
        }
    }
    diagramElements.at(0)->accept(this);
}

void PropertiesView::MView::edit()
{
    if (m_elementNameLineEdit) {
        m_elementNameLineEdit->setFocus();
        m_elementNameLineEdit->selectAll();
    }
}

void PropertiesView::MView::visitMElement(const MElement *element)
{
    Q_UNUSED(element);

    prepare();
    if (m_stereotypeComboBox == 0) {
        m_stereotypeComboBox = new QComboBox(m_topWidget);
        m_stereotypeComboBox->setEditable(true);
        m_stereotypeComboBox->setInsertPolicy(QComboBox::NoInsert);
        m_topLayout->addRow(tr("Stereotypes:"), m_stereotypeComboBox);
        m_stereotypeComboBox->addItems(m_propertiesView->stereotypeController()->knownStereotypes(m_stereotypeElement));
        connect(m_stereotypeComboBox->lineEdit(), SIGNAL(textEdited(QString)), this, SLOT(onStereotypesChanged(QString)));
        connect(m_stereotypeComboBox, SIGNAL(activated(QString)), this, SLOT(onStereotypesChanged(QString)));
    }
    if (!m_stereotypeComboBox->hasFocus()) {
        QList<QString> stereotypeList;
        if (haveSameValue(m_modelElements, &MElement::stereotypes, &stereotypeList)) {
            QString stereotypes = m_stereotypesController->toString(stereotypeList);
            m_stereotypeComboBox->setEnabled(true);
            if (stereotypes != m_stereotypeComboBox->currentText()) {
                m_stereotypeComboBox->setCurrentText(stereotypes);
            }
        } else {
            m_stereotypeComboBox->clear();
            m_stereotypeComboBox->setEnabled(false);
        }
    }
#ifdef SHOW_DEBUG_PROPERTIES
    if (m_reverseEngineeredLabel == 0) {
        m_reverseEngineeredLabel = new QLabel(m_topWidget);
        m_topLayout->addRow(tr("Reverese engineered:"), m_reverseEngineeredLabel);
    }
    QString text = element->flags().testFlag(MElement::ReverseEngineered) ? tr("Yes") : tr("No");
    m_reverseEngineeredLabel->setText(text);
#endif
}

void PropertiesView::MView::visitMObject(const MObject *object)
{
    visitMElement(object);
    QList<MObject *> selection = filter<MObject>(m_modelElements);
    bool isSingleSelection = selection.size() == 1;
    if (m_elementNameLineEdit == 0) {
        m_elementNameLineEdit = new QLineEdit(m_topWidget);
        m_topLayout->addRow(tr("Name:"), m_elementNameLineEdit);
        connect(m_elementNameLineEdit, SIGNAL(textChanged(QString)), this, SLOT(onObjectNameChanged(QString)));
    }
    if (isSingleSelection) {
        if (object->name() != m_elementNameLineEdit->text() && !m_elementNameLineEdit->hasFocus()) {
            m_elementNameLineEdit->setText(object->name());
        }
    } else {
        m_elementNameLineEdit->clear();
    }
    if (m_elementNameLineEdit->isEnabled() != isSingleSelection) {
        m_elementNameLineEdit->setEnabled(isSingleSelection);
    }

#ifdef SHOW_DEBUG_PROPERTIES
    if (m_childrenLabel == 0) {
        m_childrenLabel = new QLabel(m_topWidget);
        m_topLayout->addRow(tr("Children:"), m_childrenLabel);
    }
    m_childrenLabel->setText(QString::number(object->children().size()));
    if (m_relationsLabel == 0) {
        m_relationsLabel = new QLabel(m_topWidget);
        m_topLayout->addRow(tr("Relations:"), m_relationsLabel);
    }
    m_relationsLabel->setText(QString::number(object->relations().size()));
#endif
}

void PropertiesView::MView::visitMPackage(const MPackage *package)
{
    if (m_modelElements.size() == 1 && !package->owner()) {
        setTitle<MPackage>(m_modelElements, tr("Model"), tr("Models"));
    } else {
        setTitle<MPackage>(m_modelElements, tr("Package"), tr("Packages"));
    }
    visitMObject(package);
}

void PropertiesView::MView::visitMClass(const MClass *klass)
{
    setTitle<MClass>(m_modelElements, tr("Class"), tr("Classes"));
    visitMObject(klass);
    QList<MClass *> selection = filter<MClass>(m_modelElements);
    bool isSingleSelection = selection.size() == 1;
    if (m_namespaceLineEdit == 0) {
        m_namespaceLineEdit = new QLineEdit(m_topWidget);
        m_topLayout->addRow(tr("Namespace:"), m_namespaceLineEdit);
        connect(m_namespaceLineEdit, SIGNAL(textEdited(QString)), this, SLOT(onNamespaceChanged(QString)));
    }
    if (!m_namespaceLineEdit->hasFocus()) {
        QString nameSpace;
        if (haveSameValue(m_modelElements, &MClass::nameSpace, &nameSpace)) {
            m_namespaceLineEdit->setEnabled(true);
            if (nameSpace != m_namespaceLineEdit->text()) {
                m_namespaceLineEdit->setText(nameSpace);
            }
        } else {
            m_namespaceLineEdit->clear();
            m_namespaceLineEdit->setEnabled(false);
        }
    }
    if (m_templateParametersLineEdit == 0) {
        m_templateParametersLineEdit = new QLineEdit(m_topWidget);
        m_topLayout->addRow(tr("Template:"), m_templateParametersLineEdit);
        connect(m_templateParametersLineEdit, SIGNAL(textChanged(QString)), this, SLOT(onTemplateParametersChanged(QString)));
    }
    if (isSingleSelection) {
        if (!m_templateParametersLineEdit->hasFocus()) {
            QList<QString> templateParameters = splitTemplateParameters(m_templateParametersLineEdit->text());
            if (klass->templateParameters() != templateParameters) {
                m_templateParametersLineEdit->setText(formatTemplateParameters(klass->templateParameters()));
            }
        }
    } else {
        m_templateParametersLineEdit->clear();
    }
    if (m_templateParametersLineEdit->isEnabled() != isSingleSelection) {
        m_templateParametersLineEdit->setEnabled(isSingleSelection);
    }
    if (m_classMembersStatusLabel == 0) {
        QMT_CHECK(!m_classMembersParseButton);
        m_classMembersStatusLabel = new QLabel(m_topWidget);
        m_classMembersParseButton = new QPushButton(QStringLiteral("Clean Up"), m_topWidget);
        QHBoxLayout *layout = new QHBoxLayout();
        layout->addWidget(m_classMembersStatusLabel);
        layout->addWidget(m_classMembersParseButton);
        layout->setStretch(0, 1);
        layout->setStretch(1, 0);
        m_topLayout->addRow(QStringLiteral(""), layout);
        connect(m_classMembersParseButton, SIGNAL(clicked()), this, SLOT(onParseClassMembers()));
    }
    if (m_classMembersParseButton->isEnabled() != isSingleSelection) {
        m_classMembersParseButton->setEnabled(isSingleSelection);
    }
    if (m_classMembersEdit == 0) {
        m_classMembersEdit = new ClassMembersEdit(m_topWidget);
        m_classMembersEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
        m_topLayout->addRow(tr("Members:"), m_classMembersEdit);
        connect(m_classMembersEdit, SIGNAL(membersChanged(QList<MClassMember>&)), this, SLOT(onClassMembersChanged(QList<MClassMember>&)));
        connect(m_classMembersEdit, SIGNAL(statusChanged(bool)), this, SLOT(onClassMembersStatusChanged(bool)));
    }
    if (isSingleSelection) {
        if (klass->members() != m_classMembersEdit->members() && !m_classMembersEdit->hasFocus()) {
            m_classMembersEdit->setMembers(klass->members());
        }
    } else {
        m_classMembersEdit->clear();
    }
    if (m_classMembersEdit->isEnabled() != isSingleSelection) {
        m_classMembersEdit->setEnabled(isSingleSelection);
    }
}

void PropertiesView::MView::visitMComponent(const MComponent *component)
{
    setTitle<MComponent>(m_modelElements, tr("Component"), tr("Components"));
    visitMObject(component);
}

void PropertiesView::MView::visitMDiagram(const MDiagram *diagram)
{
    setTitle<MDiagram>(m_modelElements, tr("Diagram"), tr("Diagrams"));
    visitMObject(diagram);
#ifdef SHOW_DEBUG_PROPERTIES
    if (m_diagramsLabel == 0) {
        m_diagramsLabel = new QLabel(m_topWidget);
        m_topLayout->addRow(tr("Elements:"), m_diagramsLabel);
    }
    m_diagramsLabel->setText(QString::number(diagram->diagramElements().size()));
#endif
}

void PropertiesView::MView::visitMCanvasDiagram(const MCanvasDiagram *diagram)
{
    setTitle<MCanvasDiagram>(m_modelElements, tr("Canvas Diagram"), tr("Canvas Diagrams"));
    visitMDiagram(diagram);
}

void PropertiesView::MView::visitMItem(const MItem *item)
{
    setTitle<MItem>(item, m_modelElements, tr("Item"), tr("Items"));
    visitMObject(item);
    QList<MItem *> selection = filter<MItem>(m_modelElements);
    bool isSingleSelection = selection.size() == 1;
    if (item->isVarietyEditable()) {
        if (m_itemVarietyEdit == 0) {
            m_itemVarietyEdit = new QLineEdit(m_topWidget);
            m_topLayout->addRow(tr("Variety:"), m_itemVarietyEdit);
            connect(m_itemVarietyEdit, SIGNAL(textChanged(QString)), this, SLOT(onItemVarietyChanged(QString)));
        }
        if (isSingleSelection) {
            if (item->variety() != m_itemVarietyEdit->text() && !m_itemVarietyEdit->hasFocus()) {
                m_itemVarietyEdit->setText(item->variety());
            }
        } else {
            m_itemVarietyEdit->clear();
        }
        if (m_itemVarietyEdit->isEnabled() != isSingleSelection) {
            m_itemVarietyEdit->setEnabled(isSingleSelection);
        }
    }
}

void PropertiesView::MView::visitMRelation(const MRelation *relation)
{
    visitMElement(relation);
    QList<MRelation *> selection = filter<MRelation>(m_modelElements);
    bool isSingleSelection = selection.size() == 1;
    if (m_elementNameLineEdit == 0) {
        m_elementNameLineEdit = new QLineEdit(m_topWidget);
        m_topLayout->addRow(tr("Name:"), m_elementNameLineEdit);
        connect(m_elementNameLineEdit, SIGNAL(textChanged(QString)), this, SLOT(onRelationNameChanged(QString)));
    }
    if (isSingleSelection) {
        if (relation->name() != m_elementNameLineEdit->text() && !m_elementNameLineEdit->hasFocus()) {
            m_elementNameLineEdit->setText(relation->name());
        }
    } else {
        m_elementNameLineEdit->clear();
    }
    if (m_elementNameLineEdit->isEnabled() != isSingleSelection) {
        m_elementNameLineEdit->setEnabled(isSingleSelection);
    }
    MObject *endAObject = m_propertiesView->modelController()->findObject(relation->endAUid());
    QMT_CHECK(endAObject);
    setEndAName(tr("End A: %1").arg(endAObject->name()));
    MObject *endBObject = m_propertiesView->modelController()->findObject(relation->endBUid());
    QMT_CHECK(endBObject);
    setEndBName(tr("End B: %1").arg(endBObject->name()));
}

void PropertiesView::MView::visitMDependency(const MDependency *dependency)
{
    setTitle<MDependency>(m_modelElements, tr("Dependency"), tr("Dependencies"));
    visitMRelation(dependency);
    QList<MDependency *> selection = filter<MDependency>(m_modelElements);
    bool isSingleSelection = selection.size() == 1;
    if (m_directionSelector == 0) {
        m_directionSelector = new QComboBox(m_topWidget);
        m_directionSelector->addItems(QStringList() << QStringLiteral("->") << QStringLiteral("<-") << QStringLiteral("<->"));
        m_topLayout->addRow(tr("Direction:"), m_directionSelector);
        connect(m_directionSelector, SIGNAL(activated(int)), this, SLOT(onDependencyDirectionChanged(int)));
    }
    if (isSingleSelection) {
        if ((!isValidDirectionIndex(m_directionSelector->currentIndex()) || dependency->direction() != translateIndexToDirection(m_directionSelector->currentIndex()))
                && !m_directionSelector->hasFocus()) {
            m_directionSelector->setCurrentIndex(translateDirectionToIndex(dependency->direction()));
        }
    } else {
        m_directionSelector->setCurrentIndex(-1);
    }
    if (m_directionSelector->isEnabled() != isSingleSelection) {
        m_directionSelector->setEnabled(isSingleSelection);
    }
}

void PropertiesView::MView::visitMInheritance(const MInheritance *inheritance)
{
    setTitle<MInheritance>(m_modelElements, tr("Inheritance"), tr("Inheritances"));
    MObject *derivedClass = m_propertiesView->modelController()->findObject(inheritance->derived());
    QMT_CHECK(derivedClass);
    setEndAName(tr("Derived class: %1").arg(derivedClass->name()));
    MObject *baseClass = m_propertiesView->modelController()->findObject(inheritance->base());
    setEndBName(tr("Base class: %1").arg(baseClass->name()));
    visitMRelation(inheritance);
}

void PropertiesView::MView::visitMAssociation(const MAssociation *association)
{
    setTitle<MAssociation>(m_modelElements, tr("Association"), tr("Associations"));
    visitMRelation(association);
    QList<MAssociation *> selection = filter<MAssociation>(m_modelElements);
    bool isSingleSelection = selection.size() == 1;
    if (m_endALabel == 0) {
        m_endALabel = new QLabel(QStringLiteral("<b>") + m_endAName + QStringLiteral("</b>"));
        m_topLayout->addRow(m_endALabel);
    }
    if (m_endAEndName == 0) {
        m_endAEndName = new QLineEdit(m_topWidget);
        m_topLayout->addRow(tr("Role:"), m_endAEndName);
        connect(m_endAEndName, SIGNAL(textChanged(QString)), this, SLOT(onAssociationEndANameChanged(QString)));
    }
    if (isSingleSelection) {
        if (association->endA().name() != m_endAEndName->text() && !m_endAEndName->hasFocus()) {
            m_endAEndName->setText(association->endA().name());
        }
    } else {
        m_endAEndName->clear();
    }
    if (m_endAEndName->isEnabled() != isSingleSelection) {
        m_endAEndName->setEnabled(isSingleSelection);
    }
    if (m_endACardinality == 0) {
        m_endACardinality = new QLineEdit(m_topWidget);
        m_topLayout->addRow(tr("Cardinality:"), m_endACardinality);
        connect(m_endACardinality, SIGNAL(textChanged(QString)), this, SLOT(onAssociationEndACardinalityChanged(QString)));
    }
    if (isSingleSelection) {
        if (association->endA().cardinality() != m_endACardinality->text() && !m_endACardinality->hasFocus()) {
            m_endACardinality->setText(association->endA().cardinality());
        }
    } else {
        m_endACardinality->clear();
    }
    if (m_endACardinality->isEnabled() != isSingleSelection) {
        m_endACardinality->setEnabled(isSingleSelection);
    }
    if (m_endANavigable == 0) {
        m_endANavigable = new QCheckBox(tr("Navigable"), m_topWidget);
        m_topLayout->addRow(QString(), m_endANavigable);
        connect(m_endANavigable, SIGNAL(clicked(bool)), this, SLOT(onAssociationEndANavigableChanged(bool)));
    }
    if (isSingleSelection) {
        if (association->endA().isNavigable() != m_endANavigable->isChecked()) {
            m_endANavigable->setChecked(association->endA().isNavigable());
        }
    } else {
        m_endANavigable->setChecked(false);
    }
    if (m_endANavigable->isEnabled() != isSingleSelection) {
        m_endANavigable->setEnabled(isSingleSelection);
    }
    if (m_endAKind == 0) {
        m_endAKind = new QComboBox(m_topWidget);
        m_endAKind->addItems(QStringList() << tr("Association") << tr("Aggregation") << tr("Composition"));
        m_topLayout->addRow(tr("Relationship:"), m_endAKind);
        connect(m_endAKind, SIGNAL(activated(int)), this, SLOT(onAssociationEndAKindChanged(int)));
    }
    if (isSingleSelection) {
        if ((!isValidAssociationKindIndex(m_endAKind->currentIndex()) || association->endA().kind() != translateIndexToAssociationKind(m_endAKind->currentIndex()))
                && !m_endAKind->hasFocus()) {
            m_endAKind->setCurrentIndex(translateAssociationKindToIndex(association->endA().kind()));
        }
    } else {
        m_endAKind->setCurrentIndex(-1);
    }
    if (m_endAKind->isEnabled() != isSingleSelection) {
        m_endAKind->setEnabled(isSingleSelection);
    }

    if (m_endBLabel == 0) {
        m_endBLabel = new QLabel(QStringLiteral("<b>") + m_endBName + QStringLiteral("</b>"));
        m_topLayout->addRow(m_endBLabel);
    }
    if (m_endBEndName == 0) {
        m_endBEndName = new QLineEdit(m_topWidget);
        m_topLayout->addRow(tr("Role:"), m_endBEndName);
        connect(m_endBEndName, SIGNAL(textChanged(QString)), this, SLOT(onAssociationEndBNameChanged(QString)));
    }
    if (isSingleSelection) {
        if (association->endB().name() != m_endBEndName->text() && !m_endBEndName->hasFocus()) {
            m_endBEndName->setText(association->endB().name());
        }
    } else {
        m_endBEndName->clear();
    }
    if (m_endBEndName->isEnabled() != isSingleSelection) {
        m_endBEndName->setEnabled(isSingleSelection);
    }
    if (m_endBCardinality == 0) {
        m_endBCardinality = new QLineEdit(m_topWidget);
        m_topLayout->addRow(tr("Cardinality:"), m_endBCardinality);
        connect(m_endBCardinality, SIGNAL(textChanged(QString)), this, SLOT(onAssociationEndBCardinalityChanged(QString)));
    }
    if (isSingleSelection) {
        if (association->endB().cardinality() != m_endBCardinality->text() && !m_endBCardinality->hasFocus()) {
            m_endBCardinality->setText(association->endB().cardinality());
        }
    } else {
        m_endBCardinality->clear();
    }
    if (m_endBCardinality->isEnabled() != isSingleSelection) {
        m_endBCardinality->setEnabled(isSingleSelection);
    }
    if (m_endBNavigable == 0) {
        m_endBNavigable = new QCheckBox(tr("Navigable"), m_topWidget);
        m_topLayout->addRow(QString(), m_endBNavigable);
        connect(m_endBNavigable, SIGNAL(clicked(bool)), this, SLOT(onAssociationEndBNavigableChanged(bool)));
    }
    if (isSingleSelection) {
        if (association->endB().isNavigable() != m_endBNavigable->isChecked()) {
            m_endBNavigable->setChecked(association->endB().isNavigable());
        }
    } else {
        m_endBNavigable->setChecked(false);
    }
    if (m_endBNavigable->isEnabled() != isSingleSelection) {
        m_endBNavigable->setEnabled(isSingleSelection);
    }
    if (m_endBKind == 0) {
        m_endBKind = new QComboBox(m_topWidget);
        m_endBKind->addItems(QStringList() << tr("Association") << tr("Aggregation") << tr("Composition"));
        m_topLayout->addRow(tr("Relationship:"), m_endBKind);
        connect(m_endBKind, SIGNAL(activated(int)), this, SLOT(onAssociationEndBKindChanged(int)));
    }
    if (isSingleSelection) {
        if ((!isValidAssociationKindIndex(m_endAKind->currentIndex()) || association->endB().kind() != translateIndexToAssociationKind(m_endBKind->currentIndex()))
                && !m_endBKind->hasFocus()) {
            m_endBKind->setCurrentIndex(translateAssociationKindToIndex(association->endB().kind()));
        }
    } else {
        m_endBKind->setCurrentIndex(-1);
    }
    if (m_endBKind->isEnabled() != isSingleSelection) {
        m_endBKind->setEnabled(isSingleSelection);
    }
}

void PropertiesView::MView::visitDElement(const DElement *element)
{
    Q_UNUSED(element);

    if (m_modelElements.size() > 0 && m_modelElements.at(0)) {
        m_propertiesTitle.clear();
        m_modelElements.at(0)->accept(this);
#ifdef SHOW_DEBUG_PROPERTIES
        if (m_separatorLine == 0) {
            m_separatorLine = new QFrame(m_topWidget);
            m_separatorLine->setFrameShape(QFrame::StyledPanel);
            m_separatorLine->setLineWidth(2);
            m_separatorLine->setMinimumHeight(2);
            m_topLayout->addRow(m_separatorLine);
        }
#endif
    } else {
        prepare();
    }
}

void PropertiesView::MView::visitDObject(const DObject *object)
{
    visitDElement(object);
#ifdef SHOW_DEBUG_PROPERTIES
    if (m_posRectLabel == 0) {
        m_posRectLabel = new QLabel(m_topWidget);
        m_topLayout->addRow(tr("Position and size:"), m_posRectLabel);
    }
    m_posRectLabel->setText(QString(QStringLiteral("(%1,%2):(%3,%4)-(%5,%6)"))
                             .arg(object->pos().x())
                             .arg(object->pos().y())
                             .arg(object->rect().left())
                             .arg(object->rect().top())
                             .arg(object->rect().right())
                             .arg(object->rect().bottom()));
#endif
    if (m_autoSizedCheckbox == 0) {
        m_autoSizedCheckbox = new QCheckBox(tr("Auto sized"), m_topWidget);
        m_topLayout->addRow(QString(), m_autoSizedCheckbox);
        connect(m_autoSizedCheckbox, SIGNAL(clicked(bool)), this, SLOT(onAutoSizedChanged(bool)));
    }
    if (!m_autoSizedCheckbox->hasFocus()) {
        bool autoSized;
        if (haveSameValue(m_diagramElements, &DObject::hasAutoSize, &autoSized)) {
            m_autoSizedCheckbox->setChecked(autoSized);
        } else {
            m_autoSizedCheckbox->setChecked(false);
        }
    }
    if (m_visualPrimaryRoleSelector == 0) {
        m_visualPrimaryRoleSelector = new PaletteBox(m_topWidget);
        setPrimaryRolePalette(m_styleElementType, DObject::PrimaryRoleCustom1, QColor());
        setPrimaryRolePalette(m_styleElementType, DObject::PrimaryRoleCustom2, QColor());
        setPrimaryRolePalette(m_styleElementType, DObject::PrimaryRoleCustom3, QColor());
        setPrimaryRolePalette(m_styleElementType, DObject::PrimaryRoleCustom4, QColor());
        setPrimaryRolePalette(m_styleElementType, DObject::PrimaryRoleCustom5, QColor());
        m_topLayout->addRow(tr("Color:"), m_visualPrimaryRoleSelector);
        connect(m_visualPrimaryRoleSelector, SIGNAL(activated(int)), this, SLOT(onVisualPrimaryRoleChanged(int)));
    }
    if (!m_visualPrimaryRoleSelector->hasFocus()) {
        StereotypeDisplayVisitor stereotypeDisplayVisitor;
        stereotypeDisplayVisitor.setModelController(m_propertiesView->modelController());
        stereotypeDisplayVisitor.setStereotypeController(m_propertiesView->stereotypeController());
        object->accept(&stereotypeDisplayVisitor);
        QString shapeId = stereotypeDisplayVisitor.shapeIconId();
        QColor baseColor;
        if (!shapeId.isEmpty()) {
            StereotypeIcon stereotypeIcon = m_propertiesView->stereotypeController()->findStereotypeIcon(shapeId);
            baseColor = stereotypeIcon.baseColor();
        }
        setPrimaryRolePalette(m_styleElementType, DObject::PrimaryRoleNormal, baseColor);
        DObject::VisualPrimaryRole visualPrimaryRole;
        if (haveSameValue(m_diagramElements, &DObject::visualPrimaryRole, &visualPrimaryRole)) {
            m_visualPrimaryRoleSelector->setCurrentIndex(translateVisualPrimaryRoleToIndex(visualPrimaryRole));
        } else {
            m_visualPrimaryRoleSelector->setCurrentIndex(-1);
        }
    }
    if (m_visualSecondaryRoleSelector == 0) {
        m_visualSecondaryRoleSelector = new QComboBox(m_topWidget);
        m_visualSecondaryRoleSelector->addItems(QStringList() << tr("Normal")
                                                  << tr("Lighter") << tr("Darker")
                                                  << tr("Soften") << tr("Outline"));
        m_topLayout->addRow(tr("Role:"), m_visualSecondaryRoleSelector);
        connect(m_visualSecondaryRoleSelector, SIGNAL(activated(int)), this, SLOT(onVisualSecondaryRoleChanged(int)));
    }
    if (!m_visualSecondaryRoleSelector->hasFocus()) {
        DObject::VisualSecondaryRole visualSecondaryRole;
        if (haveSameValue(m_diagramElements, &DObject::visualSecondaryRole, &visualSecondaryRole)) {
            m_visualSecondaryRoleSelector->setCurrentIndex(translateVisualSecondaryRoleToIndex(visualSecondaryRole));
        } else {
            m_visualSecondaryRoleSelector->setCurrentIndex(-1);
        }
    }
    if (m_visualEmphasizedCheckbox == 0) {
        m_visualEmphasizedCheckbox = new QCheckBox(tr("Emphasized"), m_topWidget);
        m_topLayout->addRow(QString(), m_visualEmphasizedCheckbox);
        connect(m_visualEmphasizedCheckbox, SIGNAL(clicked(bool)), this, SLOT(onVisualEmphasizedChanged(bool)));
    }
    if (!m_visualEmphasizedCheckbox->hasFocus()) {
        bool emphasized;
        if (haveSameValue(m_diagramElements, &DObject::isVisualEmphasized, &emphasized)) {
            m_visualEmphasizedCheckbox->setChecked(emphasized);
        } else {
            m_visualEmphasizedCheckbox->setChecked(false);
        }
    }
    if (m_stereotypeDisplaySelector == 0) {
        m_stereotypeDisplaySelector = new QComboBox(m_topWidget);
        m_stereotypeDisplaySelector->addItems(QStringList() << tr("Smart") << tr("None") << tr("Label")
                                               << tr("Decoration") << tr("Icon"));
        m_topLayout->addRow(tr("Stereotype display:"), m_stereotypeDisplaySelector);
        connect(m_stereotypeDisplaySelector, SIGNAL(activated(int)), this, SLOT(onStereotypeDisplayChanged(int)));
    }
    if (!m_stereotypeDisplaySelector->hasFocus()) {
        DObject::StereotypeDisplay stereotypeDisplay;
        if (haveSameValue(m_diagramElements, &DObject::stereotypeDisplay, &stereotypeDisplay)) {
            m_stereotypeDisplaySelector->setCurrentIndex(translateStereotypeDisplayToIndex(stereotypeDisplay));
        } else {
            m_stereotypeDisplaySelector->setCurrentIndex(-1);
        }
    }
#ifdef SHOW_DEBUG_PROPERTIES
    if (m_depthLabel == 0) {
        m_depthLabel = new QLabel(m_topWidget);
        m_topLayout->addRow(tr("Depth:"), m_depthLabel);
    }
    m_depthLabel->setText(QString::number(object->depth()));
#endif
}

void PropertiesView::MView::visitDPackage(const DPackage *package)
{
    setTitle<DPackage>(m_diagramElements, tr("Package"), tr("Packages"));
    setStereotypeIconElement(StereotypeIcon::ElementPackage);
    setStyleElementType(StyleEngine::TypePackage);
    visitDObject(package);
}

void PropertiesView::MView::visitDClass(const DClass *klass)
{
    setTitle<DClass>(m_diagramElements, tr("Class"), tr("Classes"));
    setStereotypeIconElement(StereotypeIcon::ElementClass);
    setStyleElementType(StyleEngine::TypeClass);
    visitDObject(klass);
    if (m_templateDisplaySelector == 0) {
        m_templateDisplaySelector = new QComboBox(m_topWidget);
        m_templateDisplaySelector->addItems(QStringList() << tr("Smart") << tr("Box") << tr("Angle Brackets"));
        m_topLayout->addRow(tr("Template display:"), m_templateDisplaySelector);
        connect(m_templateDisplaySelector, SIGNAL(activated(int)), this, SLOT(onTemplateDisplayChanged(int)));
    }
    if (!m_templateDisplaySelector->hasFocus()) {
        DClass::TemplateDisplay templateDisplay;
        if (haveSameValue(m_diagramElements, &DClass::templateDisplay, &templateDisplay)) {
            m_templateDisplaySelector->setCurrentIndex(translateTemplateDisplayToIndex(templateDisplay));
        } else {
            m_templateDisplaySelector->setCurrentIndex(-1);
        }
    }
    if (m_showAllMembersCheckbox == 0) {
        m_showAllMembersCheckbox = new QCheckBox(tr("Show members"), m_topWidget);
        m_topLayout->addRow(QString(), m_showAllMembersCheckbox);
        connect(m_showAllMembersCheckbox, SIGNAL(clicked(bool)), this, SLOT(onShowAllMembersChanged(bool)));
    }
    if (!m_showAllMembersCheckbox->hasFocus()) {
        bool showAllMembers;
        if (haveSameValue(m_diagramElements, &DClass::showAllMembers, &showAllMembers)) {
            m_showAllMembersCheckbox->setChecked(showAllMembers);
        } else {
            m_showAllMembersCheckbox->setChecked(false);
        }
    }
}

void PropertiesView::MView::visitDComponent(const DComponent *component)
{
    setTitle<DComponent>(m_diagramElements, tr("Component"), tr("Components"));
    setStereotypeIconElement(StereotypeIcon::ElementComponent);
    setStyleElementType(StyleEngine::TypeComponent);
    visitDObject(component);
    if (m_plainShapeCheckbox == 0) {
        m_plainShapeCheckbox = new QCheckBox(tr("Plain shape"), m_topWidget);
        m_topLayout->addRow(QString(), m_plainShapeCheckbox);
        connect(m_plainShapeCheckbox, SIGNAL(clicked(bool)), this, SLOT(onPlainShapeChanged(bool)));
    }
    if (!m_plainShapeCheckbox->hasFocus()) {
        bool plainShape;
        if (haveSameValue(m_diagramElements, &DComponent::plainShape, &plainShape)) {
            m_plainShapeCheckbox->setChecked(plainShape);
        } else {
            m_plainShapeCheckbox->setChecked(false);
        }
    }
}

void PropertiesView::MView::visitDDiagram(const DDiagram *diagram)
{
    setTitle<DDiagram>(m_diagramElements, tr("Diagram"), tr("Diagrams"));
    setStyleElementType(StyleEngine::TypeOther);
    visitDObject(diagram);
}

void PropertiesView::MView::visitDItem(const DItem *item)
{
    setTitle<DItem>(m_diagramElements, tr("Item"), tr("Items"));
    setStereotypeIconElement(StereotypeIcon::ElementItem);
    setStyleElementType(StyleEngine::TypeItem);
    visitDObject(item);
    QList<DItem *> selection = filter<DItem>(m_diagramElements);
    bool isSingleSelection = selection.size() == 1;
    if (item->isShapeEditable()) {
        if (m_itemShapeEdit == 0) {
            m_itemShapeEdit = new QLineEdit(m_topWidget);
            m_topLayout->addRow(tr("Shape:"), m_itemShapeEdit);
            connect(m_itemShapeEdit, SIGNAL(textChanged(QString)), this, SLOT(onItemShapeChanged(QString)));
        }
        if (isSingleSelection) {
            if (item->shape() != m_itemShapeEdit->text() && !m_itemShapeEdit->hasFocus()) {
                m_itemShapeEdit->setText(item->shape());
            }
        } else {
            m_itemShapeEdit->clear();
        }
        if (m_itemShapeEdit->isEnabled() != isSingleSelection) {
            m_itemShapeEdit->setEnabled(isSingleSelection);
        }
    }
}

void PropertiesView::MView::visitDRelation(const DRelation *relation)
{
    visitDElement(relation);
}

void PropertiesView::MView::visitDInheritance(const DInheritance *inheritance)
{
    setTitle<DInheritance>(m_diagramElements, tr("Inheritance"), tr("Inheritances"));
    visitDRelation(inheritance);
}

void PropertiesView::MView::visitDDependency(const DDependency *dependency)
{
    setTitle<DDependency>(m_diagramElements, tr("Dependency"), tr("Dependencies"));
    visitDRelation(dependency);
}

void PropertiesView::MView::visitDAssociation(const DAssociation *association)
{
    setTitle<DAssociation>(m_diagramElements, tr("Association"), tr("Associations"));
    visitDRelation(association);
}

void PropertiesView::MView::visitDAnnotation(const DAnnotation *annotation)
{
    setTitle<DAnnotation>(m_diagramElements, tr("Annotation"), tr("Annotations"));
    visitDElement(annotation);
    if (m_annotationAutoWidthCheckbox == 0) {
        m_annotationAutoWidthCheckbox = new QCheckBox(tr("Auto width"), m_topWidget);
        m_topLayout->addRow(QString(), m_annotationAutoWidthCheckbox);
        connect(m_annotationAutoWidthCheckbox, SIGNAL(clicked(bool)), this, SLOT(onAutoWidthChanged(bool)));
    }
    if (!m_annotationAutoWidthCheckbox->hasFocus()) {
        bool autoSized;
        if (haveSameValue(m_diagramElements, &DAnnotation::hasAutoSize, &autoSized)) {
            m_annotationAutoWidthCheckbox->setChecked(autoSized);
        } else {
            m_annotationAutoWidthCheckbox->setChecked(false);
        }
    }
    if (m_annotationVisualRoleSelector == 0) {
        m_annotationVisualRoleSelector = new QComboBox(m_topWidget);
        m_annotationVisualRoleSelector->addItems(QStringList() << tr("Normal") << tr("Title") << tr("Subtitle") << tr("Emphasized") << tr("Soften") << tr("Footnote"));
        m_topLayout->addRow(tr("Role:"), m_annotationVisualRoleSelector);
        connect(m_annotationVisualRoleSelector, SIGNAL(activated(int)), this, SLOT(onAnnotationVisualRoleChanged(int)));
    }
    if (!m_annotationVisualRoleSelector->hasFocus()) {
        DAnnotation::VisualRole visualRole;
        if (haveSameValue(m_diagramElements, &DAnnotation::visualRole, &visualRole)) {
            m_annotationVisualRoleSelector->setCurrentIndex(translateAnnotationVisualRoleToIndex(visualRole));
        } else {
            m_annotationVisualRoleSelector->setCurrentIndex(-1);
        }
    }
}

void PropertiesView::MView::visitDBoundary(const DBoundary *boundary)
{
    setTitle<DBoundary>(m_diagramElements, tr("Boundary"), tr("Boundaries"));
    visitDElement(boundary);
}

void PropertiesView::MView::onStereotypesChanged(const QString &stereotypes)
{
    QList<QString> set = m_stereotypesController->fromString(stereotypes);
    assignModelElement<MElement, QList<QString> >(m_modelElements, SelectionMulti, set, &MElement::stereotypes, &MElement::setStereotypes);
}

void PropertiesView::MView::onObjectNameChanged(const QString &name)
{
    assignModelElement<MObject, QString>(m_modelElements, SelectionSingle, name, &MObject::name, &MObject::setName);
}

void PropertiesView::MView::onNamespaceChanged(const QString &nameSpace)
{
    assignModelElement<MClass, QString>(m_modelElements, SelectionMulti, nameSpace, &MClass::nameSpace, &MClass::setNameSpace);
}

void PropertiesView::MView::onTemplateParametersChanged(const QString &templateParameters)
{
    QList<QString> templateParametersList = splitTemplateParameters(templateParameters);
    assignModelElement<MClass, QList<QString> >(m_modelElements, SelectionSingle, templateParametersList, &MClass::templateParameters, &MClass::setTemplateParameters);
}

void PropertiesView::MView::onClassMembersStatusChanged(bool valid)
{
    if (valid) {
        m_classMembersStatusLabel->clear();
    } else {
        m_classMembersStatusLabel->setText(tr("<font color=red>Invalid syntax!</font>"));
    }
}

void PropertiesView::MView::onParseClassMembers()
{
    m_classMembersEdit->reparse();
}

void PropertiesView::MView::onClassMembersChanged(QList<MClassMember> &classMembers)
{
    assignModelElement<MClass, QList<MClassMember> >(m_modelElements, SelectionSingle, classMembers, &MClass::members, &MClass::setMembers);
}

void PropertiesView::MView::onItemVarietyChanged(const QString &variety)
{
    assignModelElement<MItem, QString>(m_modelElements, SelectionSingle, variety, &MItem::variety, &MItem::setVariety);
}

void PropertiesView::MView::onRelationNameChanged(const QString &name)
{
    assignModelElement<MRelation, QString>(m_modelElements, SelectionSingle, name, &MRelation::name, &MRelation::setName);
}

void PropertiesView::MView::onDependencyDirectionChanged(int directionIndex)
{
    MDependency::Direction direction = translateIndexToDirection(directionIndex);
    assignModelElement<MDependency, MDependency::Direction>(m_modelElements, SelectionSingle, direction, &MDependency::direction, &MDependency::setDirection);
}

void PropertiesView::MView::onAssociationEndANameChanged(const QString &name)
{
    assignEmbeddedModelElement<MAssociation, MAssociationEnd, QString>(m_modelElements, SelectionSingle, name, &MAssociation::endA, &MAssociation::setEndA, &MAssociationEnd::name, &MAssociationEnd::setName);
}

void PropertiesView::MView::onAssociationEndACardinalityChanged(const QString &cardinality)
{
    assignEmbeddedModelElement<MAssociation, MAssociationEnd, QString>(m_modelElements, SelectionSingle, cardinality, &MAssociation::endA, &MAssociation::setEndA, &MAssociationEnd::cardinality, &MAssociationEnd::setCardinality);
}

void PropertiesView::MView::onAssociationEndANavigableChanged(bool navigable)
{
    assignEmbeddedModelElement<MAssociation, MAssociationEnd, bool>(m_modelElements, SelectionSingle, navigable, &MAssociation::endA, &MAssociation::setEndA, &MAssociationEnd::isNavigable, &MAssociationEnd::setNavigable);
}

void PropertiesView::MView::onAssociationEndAKindChanged(int kindIndex)
{
    MAssociationEnd::Kind kind = translateIndexToAssociationKind(kindIndex);
    assignEmbeddedModelElement<MAssociation, MAssociationEnd, MAssociationEnd::Kind>(m_modelElements, SelectionSingle, kind, &MAssociation::endA, &MAssociation::setEndA, &MAssociationEnd::kind, &MAssociationEnd::setKind);
}

void PropertiesView::MView::onAssociationEndBNameChanged(const QString &name)
{
    assignEmbeddedModelElement<MAssociation, MAssociationEnd, QString>(m_modelElements, SelectionSingle, name, &MAssociation::endB, &MAssociation::setEndB, &MAssociationEnd::name, &MAssociationEnd::setName);
}

void PropertiesView::MView::onAssociationEndBCardinalityChanged(const QString &cardinality)
{
    assignEmbeddedModelElement<MAssociation, MAssociationEnd, QString>(m_modelElements, SelectionSingle, cardinality, &MAssociation::endB, &MAssociation::setEndB, &MAssociationEnd::cardinality, &MAssociationEnd::setCardinality);
}

void PropertiesView::MView::onAssociationEndBNavigableChanged(bool navigable)
{
    assignEmbeddedModelElement<MAssociation, MAssociationEnd, bool>(m_modelElements, SelectionSingle, navigable, &MAssociation::endB, &MAssociation::setEndB, &MAssociationEnd::isNavigable, &MAssociationEnd::setNavigable);
}

void PropertiesView::MView::onAssociationEndBKindChanged(int kindIndex)
{
    MAssociationEnd::Kind kind = translateIndexToAssociationKind(kindIndex);
    assignEmbeddedModelElement<MAssociation, MAssociationEnd, MAssociationEnd::Kind>(m_modelElements, SelectionSingle, kind, &MAssociation::endB, &MAssociation::setEndB, &MAssociationEnd::kind, &MAssociationEnd::setKind);
}

void PropertiesView::MView::onAutoSizedChanged(bool autoSized)
{
    assignModelElement<DObject, bool>(m_diagramElements, SelectionMulti, autoSized, &DObject::hasAutoSize, &DObject::setAutoSize);
}

void PropertiesView::MView::onVisualPrimaryRoleChanged(int visualRoleIndex)
{
    DObject::VisualPrimaryRole visualRole = translateIndexToVisualPrimaryRole(visualRoleIndex);
    assignModelElement<DObject, DObject::VisualPrimaryRole>(m_diagramElements, SelectionMulti, visualRole, &DObject::visualPrimaryRole, &DObject::setVisualPrimaryRole);
}

void PropertiesView::MView::onVisualSecondaryRoleChanged(int visualRoleIndex)
{
    DObject::VisualSecondaryRole visualRole = translateIndexToVisualSecondaryRole(visualRoleIndex);
    assignModelElement<DObject, DObject::VisualSecondaryRole>(m_diagramElements, SelectionMulti, visualRole, &DObject::visualSecondaryRole, &DObject::setVisualSecondaryRole);
}

void PropertiesView::MView::onVisualEmphasizedChanged(bool visualEmphasized)
{
    assignModelElement<DObject, bool>(m_diagramElements, SelectionMulti, visualEmphasized, &DObject::isVisualEmphasized, &DObject::setVisualEmphasized);
}

void PropertiesView::MView::onStereotypeDisplayChanged(int stereotypeDisplayIndex)
{
    DObject::StereotypeDisplay stereotypeDisplay = translateIndexToStereotypeDisplay(stereotypeDisplayIndex);
    assignModelElement<DObject, DObject::StereotypeDisplay>(m_diagramElements, SelectionMulti, stereotypeDisplay, &DObject::stereotypeDisplay, &DObject::setStereotypeDisplay);
}

void PropertiesView::MView::onTemplateDisplayChanged(int templateDisplayIndex)
{
    DClass::TemplateDisplay templateDisplay = translateIndexToTemplateDisplay(templateDisplayIndex);
    assignModelElement<DClass, DClass::TemplateDisplay>(m_diagramElements, SelectionMulti, templateDisplay, &DClass::templateDisplay, &DClass::setTemplateDisplay);
}

void PropertiesView::MView::onShowAllMembersChanged(bool showAllMembers)
{
    assignModelElement<DClass, bool>(m_diagramElements, SelectionMulti, showAllMembers, &DClass::showAllMembers, &DClass::setShowAllMembers);
}

void PropertiesView::MView::onPlainShapeChanged(bool plainShape)
{
    assignModelElement<DComponent, bool>(m_diagramElements, SelectionMulti, plainShape, &DComponent::plainShape, &DComponent::setPlainShape);
}

void PropertiesView::MView::onItemShapeChanged(const QString &shape)
{
    assignModelElement<DItem, QString>(m_diagramElements, SelectionSingle, shape, &DItem::shape, &DItem::setShape);
}

void PropertiesView::MView::onAutoWidthChanged(bool autoWidthed)
{
    assignModelElement<DAnnotation, bool>(m_diagramElements, SelectionMulti, autoWidthed, &DAnnotation::hasAutoSize, &DAnnotation::setAutoSize);
}

void PropertiesView::MView::onAnnotationVisualRoleChanged(int visualRoleIndex)
{
    DAnnotation::VisualRole visualRole = translateIndexToAnnotationVisualRole((visualRoleIndex));
    assignModelElement<DAnnotation, DAnnotation::VisualRole>(m_diagramElements, SelectionMulti, visualRole, &DAnnotation::visualRole, &DAnnotation::setVisualRole);
}

void PropertiesView::MView::prepare()
{
    QMT_CHECK(!m_propertiesTitle.isEmpty());
    if (m_topWidget == 0) {
        m_topWidget = new QWidget();
        m_topLayout = new QFormLayout(m_topWidget);
        m_topWidget->setLayout(m_topLayout);
    }
    if (m_classNameLabel == 0) {
        m_classNameLabel = new QLabel();
        m_topLayout->addRow(m_classNameLabel);
    }
    QString title(QStringLiteral("<b>") + m_propertiesTitle + QStringLiteral("</b>"));
    if (m_classNameLabel->text() != title) {
        m_classNameLabel->setText(title);
    }
}

template<class T, class V>
void PropertiesView::MView::setTitle(const QList<V *> &elements, const QString &singularTitle, const QString &pluralTitle)
{
    QList<T *> filtered = filter<T>(elements);
    if (filtered.size() == elements.size()) {
        if (elements.size() == 1) {
            m_propertiesTitle = singularTitle;
        } else {
            m_propertiesTitle = pluralTitle;
        }
    } else {
        m_propertiesTitle = tr("Multi-Selection");
    }
}

template<class T, class V>
void PropertiesView::MView::setTitle(const MItem *item, const QList<V *> &elements, const QString &singularTitle, const QString &pluralTitle)
{
    if (m_propertiesTitle.isEmpty()) {
        QList<T *> filtered = filter<T>(elements);
        if (filtered.size() == elements.size()) {
            if (elements.size() == 1) {
                if (item && !item->isVarietyEditable()) {
                    QString stereotypeIconId = m_propertiesView->stereotypeController()->findStereotypeIconId(StereotypeIcon::ElementItem, QStringList() << item->variety());
                    if (!stereotypeIconId.isEmpty()) {
                        StereotypeIcon stereotypeIcon = m_propertiesView->stereotypeController()->findStereotypeIcon(stereotypeIconId);
                        m_propertiesTitle = stereotypeIcon.title();
                    }
                }
                if (m_propertiesTitle.isEmpty()) {
                    m_propertiesTitle = singularTitle;
                }
            } else {
                m_propertiesTitle = pluralTitle;
            }
        } else {
            m_propertiesTitle = tr("Multi-Selection");
        }
    }
}

void PropertiesView::MView::setStereotypeIconElement(StereotypeIcon::Element stereotypeElement)
{
    if (m_stereotypeElement == StereotypeIcon::ElementAny) {
        m_stereotypeElement = stereotypeElement;
    }
}

void PropertiesView::MView::setStyleElementType(StyleEngine::ElementType elementType)
{
    if (m_styleElementType == StyleEngine::TypeOther) {
        m_styleElementType = elementType;
    }
}

void PropertiesView::MView::setPrimaryRolePalette(StyleEngine::ElementType elementType, DObject::VisualPrimaryRole visualPrimaryRole, const QColor &baseColor)
{
    int index = translateVisualPrimaryRoleToIndex(visualPrimaryRole);
    const Style *style = m_propertiesView->styleController()->adaptObjectStyle(elementType, ObjectVisuals(visualPrimaryRole, DObject::SecondaryRoleNone, false, baseColor, 0));
    m_visualPrimaryRoleSelector->setBrush(index, style->fillBrush());
    m_visualPrimaryRoleSelector->setLinePen(index, style->linePen());
}

void PropertiesView::MView::setEndAName(const QString &endAName)
{
    if (m_endAName.isEmpty()) {
        m_endAName = endAName;
    }
}

void PropertiesView::MView::setEndBName(const QString &endBName)
{
    if (m_endBName.isEmpty()) {
        m_endBName = endBName;
    }
}

QList<QString> PropertiesView::MView::splitTemplateParameters(const QString &templateParameters)
{
    QList<QString> templateParametersList;
    foreach (const QString &parameter, templateParameters.split(QLatin1Char(','))) {
        const QString &p = parameter.trimmed();
        if (!p.isEmpty()) {
            templateParametersList.append(p);
        }
    }
    return templateParametersList;
}

QString PropertiesView::MView::formatTemplateParameters(const QList<QString> &templateParametersList)
{
    QString templateParamters;
    bool first = true;
    foreach (const QString &parameter, templateParametersList) {
        if (!first) {
            templateParamters += QStringLiteral(", ");
        }
        templateParamters += parameter;
        first = false;
    }
    return templateParamters;
}

template<class T, class V>
QList<T *> PropertiesView::MView::filter(const QList<V *> &elements)
{
    QList<T *> filtered;
    foreach (V *element, elements) {
        T *t = dynamic_cast<T *>(element);
        if (t) {
            filtered.append(t);
        }
    }
    return filtered;
}

template<class T, class V, class BASE>
bool PropertiesView::MView::haveSameValue(const QList<BASE *> &baseElements, V (T::*getter)() const, V *value)
{
    QList<T *> elements = filter<T>(baseElements);
    QMT_CHECK(!elements.isEmpty());
    V candidate = V(); // avoid warning of reading uninitialized variable
    bool haveCandidate = false;
    foreach (T *element, elements) {
        if (!haveCandidate) {
            candidate = ((*element).*getter)();
            haveCandidate = true;
        } else {
            if (candidate != ((*element).*getter)()) {
                return false;
            }
        }
    }
    QMT_CHECK(haveCandidate);
    if (!haveCandidate) {
        return false;
    }
    if (value) {
        *value = candidate;
    }
    return true;
}

template<class T, class V, class BASE>
void PropertiesView::MView::assignModelElement(const QList<BASE *> &baseElements, SelectionType selectionType, const V &value, V (T::*getter)() const, void (T::*setter)(const V &))
{
    QList<T *> elements = filter<T>(baseElements);
    if ((selectionType == SelectionSingle && elements.size() == 1) || selectionType == SelectionMulti) {
        foreach (T *element, elements) {
            if (value != ((*element).*getter)()) {
                m_propertiesView->beginUpdate(element);
                ((*element).*setter)(value);
                m_propertiesView->endUpdate(element, false);
            }
        }
    }
}

template<class T, class V, class BASE>
void PropertiesView::MView::assignModelElement(const QList<BASE *> &baseElements, SelectionType selectionType, const V &value, V (T::*getter)() const, void (T::*setter)(V))
{
    QList<T *> elements = filter<T>(baseElements);
    if ((selectionType == SelectionSingle && elements.size() == 1) || selectionType == SelectionMulti) {
        foreach (T *element, elements) {
            if (value != ((*element).*getter)()) {
                m_propertiesView->beginUpdate(element);
                ((*element).*setter)(value);
                m_propertiesView->endUpdate(element, false);
            }
        }
    }
}

template<class T, class E, class V, class BASE>
void PropertiesView::MView::assignEmbeddedModelElement(const QList<BASE *> &baseElements, SelectionType selectionType, const V &value, E (T::*getter)() const, void (T::*setter)(const E &), V (E::*vGetter)() const, void (E::*vSetter)(const V &))
{
    QList<T *> elements = filter<T>(baseElements);
    if ((selectionType == SelectionSingle && elements.size() == 1) || selectionType == SelectionMulti) {
        foreach (T *element, elements) {
            E embedded = ((*element).*getter)();
            if (value != (embedded.*vGetter)()) {
                m_propertiesView->beginUpdate(element);
                (embedded.*vSetter)(value);
                ((*element).*setter)(embedded);
                m_propertiesView->endUpdate(element, false);
            }
        }
    }
}

template<class T, class E, class V, class BASE>
void PropertiesView::MView::assignEmbeddedModelElement(const QList<BASE *> &baseElements, SelectionType selectionType, const V &value, E (T::*getter)() const, void (T::*setter)(const E &), V (E::*vGetter)() const, void (E::*vSetter)(V))
{
    QList<T *> elements = filter<T>(baseElements);
    if ((selectionType == SelectionSingle && elements.size() == 1) || selectionType == SelectionMulti) {
        foreach (T *element, elements) {
            E embedded = ((*element).*getter)();
            if (value != (embedded.*vGetter)()) {
                m_propertiesView->beginUpdate(element);
                (embedded.*vSetter)(value);
                ((*element).*setter)(embedded);
                m_propertiesView->endUpdate(element, false);
            }
        }
    }
}

}
