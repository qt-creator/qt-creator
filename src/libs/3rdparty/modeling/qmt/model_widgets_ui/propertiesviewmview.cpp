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
    case MDependency::A_TO_B:
        return 0;
    case MDependency::B_TO_A:
        return 1;
    case MDependency::BIDIRECTIONAL:
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
        MDependency::A_TO_B, MDependency::B_TO_A, MDependency::BIDIRECTIONAL
    };
    QMT_CHECK(isValidDirectionIndex(index));
    return map[index];
}

static int translateAssociationKindToIndex(MAssociationEnd::Kind kind)
{
    switch (kind) {
    case MAssociationEnd::ASSOCIATION:
        return 0;
    case MAssociationEnd::AGGREGATION:
        return 1;
    case MAssociationEnd::COMPOSITION:
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
        MAssociationEnd::ASSOCIATION, MAssociationEnd::AGGREGATION, MAssociationEnd::COMPOSITION
    };
    QMT_CHECK(isValidAssociationKindIndex(index));
    return map[index];
}

static int translateVisualPrimaryRoleToIndex(DObject::VisualPrimaryRole visual_role)
{
    switch (visual_role) {
    case DObject::PRIMARY_ROLE_NORMAL:
        return 0;
    case DObject::PRIMARY_ROLE_CUSTOM1:
        return 1;
    case DObject::PRIMARY_ROLE_CUSTOM2:
        return 2;
    case DObject::PRIMARY_ROLE_CUSTOM3:
        return 3;
    case DObject::PRIMARY_ROLE_CUSTOM4:
        return 4;
    case DObject::PRIMARY_ROLE_CUSTOM5:
        return 5;
    case DObject::DEPRECATED_PRIMARY_ROLE_LIGHTER:
    case DObject::DEPRECATED_PRIMARY_ROLE_DARKER:
    case DObject::DEPRECATED_PRIMARY_ROLE_SOFTEN:
    case DObject::DEPRECATED_PRIMARY_ROLE_OUTLINE:
        return 0;
    }
    return 0;
}

static DObject::VisualPrimaryRole translateIndexToVisualPrimaryRole(int index)
{
    static const DObject::VisualPrimaryRole map[] = {
        DObject::PRIMARY_ROLE_NORMAL,
        DObject::PRIMARY_ROLE_CUSTOM1, DObject::PRIMARY_ROLE_CUSTOM2, DObject::PRIMARY_ROLE_CUSTOM3,
        DObject::PRIMARY_ROLE_CUSTOM4, DObject::PRIMARY_ROLE_CUSTOM5
    };
    QMT_CHECK(index >= 0 && index <= 5);
    return map[index];
}

static int translateVisualSecondaryRoleToIndex(DObject::VisualSecondaryRole visual_role)
{
    switch (visual_role) {
    case DObject::SECONDARY_ROLE_NONE:
        return 0;
    case DObject::SECONDARY_ROLE_LIGHTER:
        return 1;
    case DObject::SECONDARY_ROLE_DARKER:
        return 2;
    case DObject::SECONDARY_ROLE_SOFTEN:
        return 3;
    case DObject::SECONDARY_ROLE_OUTLINE:
        return 4;
    }
    return 0;
}

static DObject::VisualSecondaryRole translateIndexToVisualSecondaryRole(int index)
{
    static const DObject::VisualSecondaryRole map[] = {
        DObject::SECONDARY_ROLE_NONE,
        DObject::SECONDARY_ROLE_LIGHTER, DObject::SECONDARY_ROLE_DARKER,
        DObject::SECONDARY_ROLE_SOFTEN, DObject::SECONDARY_ROLE_OUTLINE
    };
    QMT_CHECK(index >= 0 && index <= 4);
    return map[index];
}

static int translateStereotypeDisplayToIndex(DObject::StereotypeDisplay stereotype_display)
{
    switch (stereotype_display) {
    case DObject::STEREOTYPE_NONE:
        return 1;
    case DObject::STEREOTYPE_LABEL:
        return 2;
    case DObject::STEREOTYPE_DECORATION:
        return 3;
    case DObject::STEREOTYPE_ICON:
        return 4;
    case DObject::STEREOTYPE_SMART:
        return 0;
    }
    return 0;
}

static DObject::StereotypeDisplay translateIndexToStereotypeDisplay(int index)
{
    static const DObject::StereotypeDisplay map[] = {
        DObject::STEREOTYPE_SMART,
        DObject::STEREOTYPE_NONE,
        DObject::STEREOTYPE_LABEL,
        DObject::STEREOTYPE_DECORATION,
        DObject::STEREOTYPE_ICON
    };
    QMT_CHECK(index >= 0 && index <= 4);
    return map[index];
}

static int translateTemplateDisplayToIndex(DClass::TemplateDisplay template_display)
{
    switch (template_display) {
    case DClass::TEMPLATE_SMART:
        return 0;
    case DClass::TEMPLATE_BOX:
        return 1;
    case DClass::TEMPLATE_NAME:
        return 2;
    }
    return 0;
}

static DClass::TemplateDisplay translateIndexToTemplateDisplay(int index)
{
    static const DClass::TemplateDisplay map[] = {
        DClass::TEMPLATE_SMART,
        DClass::TEMPLATE_BOX,
        DClass::TEMPLATE_NAME
    };
    QMT_CHECK(index >= 0 && index <= 2);
    return map[index];
}

static int translateAnnotationVisualRoleToIndex(DAnnotation::VisualRole visual_role)
{
    switch (visual_role) {
    case DAnnotation::ROLE_NORMAL:
        return 0;
    case DAnnotation::ROLE_TITLE:
        return 1;
    case DAnnotation::ROLE_SUBTITLE:
        return 2;
    case DAnnotation::ROLE_EMPHASIZED:
        return 3;
    case DAnnotation::ROLE_SOFTEN:
        return 4;
    case DAnnotation::ROLE_FOOTNOTE:
        return 5;
    }
    return 0;
}

static DAnnotation::VisualRole translateIndexToAnnotationVisualRole(int index)
{
    static const DAnnotation::VisualRole map[] = {
        DAnnotation::ROLE_NORMAL, DAnnotation::ROLE_TITLE, DAnnotation::ROLE_SUBTITLE,
        DAnnotation::ROLE_EMPHASIZED, DAnnotation::ROLE_SOFTEN, DAnnotation::ROLE_FOOTNOTE
    };
    QMT_CHECK(index >= 0 && index <= 5);
    return map[index];
}


PropertiesView::MView::MView(PropertiesView *properties_view)
    : _properties_view(properties_view),
      _diagram(0),
      _stereotypes_controller(new StereotypesController(this)),
      _top_widget(0),
      _top_layout(0),
      _stereotype_element(StereotypeIcon::ELEMENT_ANY),
      _class_name_label(0),
      _stereotype_combo_box(0),
      _reverse_engineered_label(0),
      _element_name_line_edit(0),
      _children_label(0),
      _relations_label(0),
      _namespace_line_edit(0),
      _template_parameters_line_edit(0),
      _class_members_status_label(0),
      _class_members_parse_button(0),
      _class_members_edit(0),
      _diagrams_label(0),
      _item_variety_edit(0),
      _end_a_label(0),
      _end_b_label(0),
      _direction_selector(0),
      _end_a_end_name(0),
      _end_a_cardinality(0),
      _end_a_navigable(0),
      _end_a_kind(0),
      _end_b_end_name(0),
      _end_b_cardinality(0),
      _end_b_navigable(0),
      _end_b_kind(0),
      _separator_line(0),
      _style_element_type(StyleEngine::TYPE_OTHER),
      _pos_rect_label(0),
      _auto_sized_checkbox(0),
      _visual_primary_role_selector(0),
      _visual_secondary_role_selector(0),
      _visual_emphasized_checkbox(0),
      _stereotype_display_selector(0),
      _depth_label(0),
      _template_display_selector(0),
      _show_all_members_checkbox(0),
      _plain_shape_checkbox(0),
      _item_shape_edit(0),
      _annotation_auto_width_checkbox(0),
      _annotation_visual_role_selector(0)
{
}

PropertiesView::MView::~MView()
{
}

void PropertiesView::MView::update(QList<MElement *> &model_elements)
{
    QMT_CHECK(model_elements.size() > 0);

    _model_elements = model_elements;
    _diagram_elements.clear();
    _diagram = 0;
    model_elements.at(0)->accept(this);
}

void PropertiesView::MView::update(QList<DElement *> &diagram_elements, MDiagram *diagram)
{
    QMT_CHECK(diagram_elements.size() > 0);
    QMT_CHECK(diagram);

    _diagram_elements = diagram_elements;
    _diagram = diagram;
    _model_elements.clear();
    foreach (DElement *delement, diagram_elements) {
        bool appended_melement = false;
        if (delement->getModelUid().isValid()) {
            MElement *melement = _properties_view->getModelController()->findElement(delement->getModelUid());
            if (melement) {
                _model_elements.append(melement);
                appended_melement = true;
            }
        }
        if (!appended_melement) {
            // ensure that indices within _diagram_elements match indices into _model_elements
            _model_elements.append(0);
        }
    }
    diagram_elements.at(0)->accept(this);
}

void PropertiesView::MView::edit()
{
    if (_element_name_line_edit) {
        _element_name_line_edit->setFocus();
        _element_name_line_edit->selectAll();
    }
}

void PropertiesView::MView::visitMElement(const MElement *element)
{
    Q_UNUSED(element);

    prepare();
    if (_stereotype_combo_box == 0) {
        _stereotype_combo_box = new QComboBox(_top_widget);
        _stereotype_combo_box->setEditable(true);
        _stereotype_combo_box->setInsertPolicy(QComboBox::NoInsert);
        _top_layout->addRow(tr("Stereotypes:"), _stereotype_combo_box);
        _stereotype_combo_box->addItems(_properties_view->getStereotypeController()->getKnownStereotypes(_stereotype_element));
        connect(_stereotype_combo_box->lineEdit(), SIGNAL(textEdited(QString)), this, SLOT(onStereotypesChanged(QString)));
        connect(_stereotype_combo_box, SIGNAL(activated(QString)), this, SLOT(onStereotypesChanged(QString)));
    }
    if (!_stereotype_combo_box->hasFocus()) {
        QList<QString> stereotype_list;
        if (haveSameValue(_model_elements, &MElement::getStereotypes, &stereotype_list)) {
            QString stereotypes = _stereotypes_controller->toString(stereotype_list);
            _stereotype_combo_box->setEnabled(true);
            if (stereotypes != _stereotype_combo_box->currentText()) {
                _stereotype_combo_box->setCurrentText(stereotypes);
            }
        } else {
            _stereotype_combo_box->clear();
            _stereotype_combo_box->setEnabled(false);
        }
    }
#ifdef SHOW_DEBUG_PROPERTIES
    if (_reverse_engineered_label == 0) {
        _reverse_engineered_label = new QLabel(_top_widget);
        _top_layout->addRow(tr("Reverese engineered:"), _reverse_engineered_label);
    }
    QString text = element->getFlags().testFlag(MElement::REVERSE_ENGINEERED) ? tr("Yes") : tr("No");
    _reverse_engineered_label->setText(text);
#endif
}

void PropertiesView::MView::visitMObject(const MObject *object)
{
    visitMElement(object);
    QList<MObject *> selection = filter<MObject>(_model_elements);
    bool is_single_selection = selection.size() == 1;
    if (_element_name_line_edit == 0) {
        _element_name_line_edit = new QLineEdit(_top_widget);
        _top_layout->addRow(tr("Name:"), _element_name_line_edit);
        connect(_element_name_line_edit, SIGNAL(textChanged(QString)), this, SLOT(onObjectNameChanged(QString)));
    }
    if (is_single_selection) {
        if (object->getName() != _element_name_line_edit->text() && !_element_name_line_edit->hasFocus()) {
            _element_name_line_edit->setText(object->getName());
        }
    } else {
        _element_name_line_edit->clear();
    }
    if (_element_name_line_edit->isEnabled() != is_single_selection) {
        _element_name_line_edit->setEnabled(is_single_selection);
    }

#ifdef SHOW_DEBUG_PROPERTIES
    if (_children_label == 0) {
        _children_label = new QLabel(_top_widget);
        _top_layout->addRow(tr("Children:"), _children_label);
    }
    _children_label->setText(QString::number(object->getChildren().size()));
    if (_relations_label == 0) {
        _relations_label = new QLabel(_top_widget);
        _top_layout->addRow(tr("Relations:"), _relations_label);
    }
    _relations_label->setText(QString::number(object->getRelations().size()));
#endif
}

void PropertiesView::MView::visitMPackage(const MPackage *package)
{
    if (_model_elements.size() == 1 && !package->getOwner()) {
        setTitle<MPackage>(_model_elements, tr("Model"), tr("Models"));
    } else {
        setTitle<MPackage>(_model_elements, tr("Package"), tr("Packages"));
    }
    visitMObject(package);
}

void PropertiesView::MView::visitMClass(const MClass *klass)
{
    setTitle<MClass>(_model_elements, tr("Class"), tr("Classes"));
    visitMObject(klass);
    QList<MClass *> selection = filter<MClass>(_model_elements);
    bool is_single_selection = selection.size() == 1;
    if (_namespace_line_edit == 0) {
        _namespace_line_edit = new QLineEdit(_top_widget);
        _top_layout->addRow(tr("Namespace:"), _namespace_line_edit);
        connect(_namespace_line_edit, SIGNAL(textEdited(QString)), this, SLOT(onNamespaceChanged(QString)));
    }
    if (!_namespace_line_edit->hasFocus()) {
        QString name_space;
        if (haveSameValue(_model_elements, &MClass::getNamespace, &name_space)) {
            _namespace_line_edit->setEnabled(true);
            if (name_space != _namespace_line_edit->text()) {
                _namespace_line_edit->setText(name_space);
            }
        } else {
            _namespace_line_edit->clear();
            _namespace_line_edit->setEnabled(false);
        }
    }
    if (_template_parameters_line_edit == 0) {
        _template_parameters_line_edit = new QLineEdit(_top_widget);
        _top_layout->addRow(tr("Template:"), _template_parameters_line_edit);
        connect(_template_parameters_line_edit, SIGNAL(textChanged(QString)), this, SLOT(onTemplateParametersChanged(QString)));
    }
    if (is_single_selection) {
        if (!_template_parameters_line_edit->hasFocus()) {
            QList<QString> template_parameters = splitTemplateParameters(_template_parameters_line_edit->text());
            if (klass->getTemplateParameters() != template_parameters) {
                _template_parameters_line_edit->setText(formatTemplateParameters(klass->getTemplateParameters()));
            }
        }
    } else {
        _template_parameters_line_edit->clear();
    }
    if (_template_parameters_line_edit->isEnabled() != is_single_selection) {
        _template_parameters_line_edit->setEnabled(is_single_selection);
    }
    if (_class_members_status_label == 0) {
        QMT_CHECK(!_class_members_parse_button);
        _class_members_status_label = new QLabel(_top_widget);
        _class_members_parse_button = new QPushButton(QStringLiteral("Clean Up"), _top_widget);
        QHBoxLayout *layout = new QHBoxLayout();
        layout->addWidget(_class_members_status_label);
        layout->addWidget(_class_members_parse_button);
        layout->setStretch(0, 1);
        layout->setStretch(1, 0);
        _top_layout->addRow(QStringLiteral(""), layout);
        connect(_class_members_parse_button, SIGNAL(clicked()), this, SLOT(onParseClassMembers()));
    }
    if (_class_members_parse_button->isEnabled() != is_single_selection) {
        _class_members_parse_button->setEnabled(is_single_selection);
    }
    if (_class_members_edit == 0) {
        _class_members_edit = new ClassMembersEdit(_top_widget);
        _class_members_edit->setLineWrapMode(QPlainTextEdit::NoWrap);
        _top_layout->addRow(tr("Members:"), _class_members_edit);
        connect(_class_members_edit, SIGNAL(membersChanged(QList<MClassMember>&)), this, SLOT(onClassMembersChanged(QList<MClassMember>&)));
        connect(_class_members_edit, SIGNAL(statusChanged(bool)), this, SLOT(onClassMembersStatusChanged(bool)));
    }
    if (is_single_selection) {
        if (klass->getMembers() != _class_members_edit->getMembers() && !_class_members_edit->hasFocus()) {
            _class_members_edit->setMembers(klass->getMembers());
        }
    } else {
        _class_members_edit->clear();
    }
    if (_class_members_edit->isEnabled() != is_single_selection) {
        _class_members_edit->setEnabled(is_single_selection);
    }
}

void PropertiesView::MView::visitMComponent(const MComponent *component)
{
    setTitle<MComponent>(_model_elements, tr("Component"), tr("Components"));
    visitMObject(component);
}

void PropertiesView::MView::visitMDiagram(const MDiagram *diagram)
{
    setTitle<MDiagram>(_model_elements, tr("Diagram"), tr("Diagrams"));
    visitMObject(diagram);
#ifdef SHOW_DEBUG_PROPERTIES
    if (_diagrams_label == 0) {
        _diagrams_label = new QLabel(_top_widget);
        _top_layout->addRow(tr("Elements:"), _diagrams_label);
    }
    _diagrams_label->setText(QString::number(diagram->getDiagramElements().size()));
#endif
}

void PropertiesView::MView::visitMCanvasDiagram(const MCanvasDiagram *diagram)
{
    setTitle<MCanvasDiagram>(_model_elements, tr("Canvas Diagram"), tr("Canvas Diagrams"));
    visitMDiagram(diagram);
}

void PropertiesView::MView::visitMItem(const MItem *item)
{
    setTitle<MItem>(item, _model_elements, tr("Item"), tr("Items"));
    visitMObject(item);
    QList<MItem *> selection = filter<MItem>(_model_elements);
    bool is_single_selection = selection.size() == 1;
    if (item->isVarietyEditable()) {
        if (_item_variety_edit == 0) {
            _item_variety_edit = new QLineEdit(_top_widget);
            _top_layout->addRow(tr("Variety:"), _item_variety_edit);
            connect(_item_variety_edit, SIGNAL(textChanged(QString)), this, SLOT(onItemVarietyChanged(QString)));
        }
        if (is_single_selection) {
            if (item->getVariety() != _item_variety_edit->text() && !_item_variety_edit->hasFocus()) {
                _item_variety_edit->setText(item->getVariety());
            }
        } else {
            _item_variety_edit->clear();
        }
        if (_item_variety_edit->isEnabled() != is_single_selection) {
            _item_variety_edit->setEnabled(is_single_selection);
        }
    }
}

void PropertiesView::MView::visitMRelation(const MRelation *relation)
{
    visitMElement(relation);
    QList<MRelation *> selection = filter<MRelation>(_model_elements);
    bool is_single_selection = selection.size() == 1;
    if (_element_name_line_edit == 0) {
        _element_name_line_edit = new QLineEdit(_top_widget);
        _top_layout->addRow(tr("Name:"), _element_name_line_edit);
        connect(_element_name_line_edit, SIGNAL(textChanged(QString)), this, SLOT(onRelationNameChanged(QString)));
    }
    if (is_single_selection) {
        if (relation->getName() != _element_name_line_edit->text() && !_element_name_line_edit->hasFocus()) {
            _element_name_line_edit->setText(relation->getName());
        }
    } else {
        _element_name_line_edit->clear();
    }
    if (_element_name_line_edit->isEnabled() != is_single_selection) {
        _element_name_line_edit->setEnabled(is_single_selection);
    }
    MObject *end_a_object = _properties_view->getModelController()->findObject(relation->getEndA());
    QMT_CHECK(end_a_object);
    setEndAName(tr("End A: %1").arg(end_a_object->getName()));
    MObject *end_b_object = _properties_view->getModelController()->findObject(relation->getEndB());
    QMT_CHECK(end_b_object);
    setEndBName(tr("End B: %1").arg(end_b_object->getName()));
}

void PropertiesView::MView::visitMDependency(const MDependency *dependency)
{
    setTitle<MDependency>(_model_elements, tr("Dependency"), tr("Dependencies"));
    visitMRelation(dependency);
    QList<MDependency *> selection = filter<MDependency>(_model_elements);
    bool is_single_selection = selection.size() == 1;
    if (_direction_selector == 0) {
        _direction_selector = new QComboBox(_top_widget);
        _direction_selector->addItems(QStringList() << QStringLiteral("->") << QStringLiteral("<-") << QStringLiteral("<->"));
        _top_layout->addRow(tr("Direction:"), _direction_selector);
        connect(_direction_selector, SIGNAL(activated(int)), this, SLOT(onDependencyDirectionChanged(int)));
    }
    if (is_single_selection) {
        if ((!isValidDirectionIndex(_direction_selector->currentIndex()) || dependency->getDirection() != translateIndexToDirection(_direction_selector->currentIndex()))
                && !_direction_selector->hasFocus()) {
            _direction_selector->setCurrentIndex(translateDirectionToIndex(dependency->getDirection()));
        }
    } else {
        _direction_selector->setCurrentIndex(-1);
    }
    if (_direction_selector->isEnabled() != is_single_selection) {
        _direction_selector->setEnabled(is_single_selection);
    }
}

void PropertiesView::MView::visitMInheritance(const MInheritance *inheritance)
{
    setTitle<MInheritance>(_model_elements, tr("Inheritance"), tr("Inheritances"));
    MObject *derived_class = _properties_view->getModelController()->findObject(inheritance->getDerived());
    QMT_CHECK(derived_class);
    setEndAName(tr("Derived class: %1").arg(derived_class->getName()));
    MObject *base_class = _properties_view->getModelController()->findObject(inheritance->getBase());
    setEndBName(tr("Base class: %1").arg(base_class->getName()));
    visitMRelation(inheritance);
}

void PropertiesView::MView::visitMAssociation(const MAssociation *association)
{
    setTitle<MAssociation>(_model_elements, tr("Association"), tr("Associations"));
    visitMRelation(association);
    QList<MAssociation *> selection = filter<MAssociation>(_model_elements);
    bool is_single_selection = selection.size() == 1;
    if (_end_a_label == 0) {
        _end_a_label = new QLabel(QStringLiteral("<b>") + _end_a_name + QStringLiteral("</b>"));
        _top_layout->addRow(_end_a_label);
    }
    if (_end_a_end_name == 0) {
        _end_a_end_name = new QLineEdit(_top_widget);
        _top_layout->addRow(tr("Role:"), _end_a_end_name);
        connect(_end_a_end_name, SIGNAL(textChanged(QString)), this, SLOT(onAssociationEndANameChanged(QString)));
    }
    if (is_single_selection) {
        if (association->getA().getName() != _end_a_end_name->text() && !_end_a_end_name->hasFocus()) {
            _end_a_end_name->setText(association->getA().getName());
        }
    } else {
        _end_a_end_name->clear();
    }
    if (_end_a_end_name->isEnabled() != is_single_selection) {
        _end_a_end_name->setEnabled(is_single_selection);
    }
    if (_end_a_cardinality == 0) {
        _end_a_cardinality = new QLineEdit(_top_widget);
        _top_layout->addRow(tr("Cardinality:"), _end_a_cardinality);
        connect(_end_a_cardinality, SIGNAL(textChanged(QString)), this, SLOT(onAssociationEndACardinalityChanged(QString)));
    }
    if (is_single_selection) {
        if (association->getA().getCardinality() != _end_a_cardinality->text() && !_end_a_cardinality->hasFocus()) {
            _end_a_cardinality->setText(association->getA().getCardinality());
        }
    } else {
        _end_a_cardinality->clear();
    }
    if (_end_a_cardinality->isEnabled() != is_single_selection) {
        _end_a_cardinality->setEnabled(is_single_selection);
    }
    if (_end_a_navigable == 0) {
        _end_a_navigable = new QCheckBox(_top_widget);
        _top_layout->addRow(tr("Navigable"), _end_a_navigable);
        connect(_end_a_navigable, SIGNAL(clicked(bool)), this, SLOT(onAssociationEndANavigableChanged(bool)));
    }
    if (is_single_selection) {
        if (association->getA().isNavigable() != _end_a_navigable->isChecked()) {
            _end_a_navigable->setChecked(association->getA().isNavigable());
        }
    } else {
        _end_a_navigable->setChecked(false);
    }
    if (_end_a_navigable->isEnabled() != is_single_selection) {
        _end_a_navigable->setEnabled(is_single_selection);
    }
    if (_end_a_kind == 0) {
        _end_a_kind = new QComboBox(_top_widget);
        _end_a_kind->addItems(QStringList() << tr("Association") << tr("Aggregation") << tr("Composition"));
        _top_layout->addRow(tr("Relationship:"), _end_a_kind);
        connect(_end_a_kind, SIGNAL(activated(int)), this, SLOT(onAssociationEndAKindChanged(int)));
    }
    if (is_single_selection) {
        if ((!isValidAssociationKindIndex(_end_a_kind->currentIndex()) || association->getA().getKind() != translateIndexToAssociationKind(_end_a_kind->currentIndex()))
                && !_end_a_kind->hasFocus()) {
            _end_a_kind->setCurrentIndex(translateAssociationKindToIndex(association->getA().getKind()));
        }
    } else {
        _end_a_kind->setCurrentIndex(-1);
    }
    if (_end_a_kind->isEnabled() != is_single_selection) {
        _end_a_kind->setEnabled(is_single_selection);
    }

    if (_end_b_label == 0) {
        _end_b_label = new QLabel(QStringLiteral("<b>") + _end_b_name + QStringLiteral("</b>"));
        _top_layout->addRow(_end_b_label);
    }
    if (_end_b_end_name == 0) {
        _end_b_end_name = new QLineEdit(_top_widget);
        _top_layout->addRow(tr("Role:"), _end_b_end_name);
        connect(_end_b_end_name, SIGNAL(textChanged(QString)), this, SLOT(onAssociationEndBNameChanged(QString)));
    }
    if (is_single_selection) {
        if (association->getB().getName() != _end_b_end_name->text() && !_end_b_end_name->hasFocus()) {
            _end_b_end_name->setText(association->getB().getName());
        }
    } else {
        _end_b_end_name->clear();
    }
    if (_end_b_end_name->isEnabled() != is_single_selection) {
        _end_b_end_name->setEnabled(is_single_selection);
    }
    if (_end_b_cardinality == 0) {
        _end_b_cardinality = new QLineEdit(_top_widget);
        _top_layout->addRow(tr("Cardinality:"), _end_b_cardinality);
        connect(_end_b_cardinality, SIGNAL(textChanged(QString)), this, SLOT(onAssociationEndBCardinalityChanged(QString)));
    }
    if (is_single_selection) {
        if (association->getB().getCardinality() != _end_b_cardinality->text() && !_end_b_cardinality->hasFocus()) {
            _end_b_cardinality->setText(association->getB().getCardinality());
        }
    } else {
        _end_b_cardinality->clear();
    }
    if (_end_b_cardinality->isEnabled() != is_single_selection) {
        _end_b_cardinality->setEnabled(is_single_selection);
    }
    if (_end_b_navigable == 0) {
        _end_b_navigable = new QCheckBox(_top_widget);
        _top_layout->addRow(tr("Navigable"), _end_b_navigable);
        connect(_end_b_navigable, SIGNAL(clicked(bool)), this, SLOT(onAssociationEndBNavigableChanged(bool)));
    }
    if (is_single_selection) {
        if (association->getB().isNavigable() != _end_b_navigable->isChecked()) {
            _end_b_navigable->setChecked(association->getB().isNavigable());
        }
    } else {
        _end_b_navigable->setChecked(false);
    }
    if (_end_b_navigable->isEnabled() != is_single_selection) {
        _end_b_navigable->setEnabled(is_single_selection);
    }
    if (_end_b_kind == 0) {
        _end_b_kind = new QComboBox(_top_widget);
        _end_b_kind->addItems(QStringList() << tr("Association") << tr("Aggregation") << tr("Composition"));
        _top_layout->addRow(tr("Relationship:"), _end_b_kind);
        connect(_end_b_kind, SIGNAL(activated(int)), this, SLOT(onAssociationEndBKindChanged(int)));
    }
    if (is_single_selection) {
        if ((!isValidAssociationKindIndex(_end_a_kind->currentIndex()) || association->getB().getKind() != translateIndexToAssociationKind(_end_b_kind->currentIndex()))
                && !_end_b_kind->hasFocus()) {
            _end_b_kind->setCurrentIndex(translateAssociationKindToIndex(association->getB().getKind()));
        }
    } else {
        _end_b_kind->setCurrentIndex(-1);
    }
    if (_end_b_kind->isEnabled() != is_single_selection) {
        _end_b_kind->setEnabled(is_single_selection);
    }
}

void PropertiesView::MView::visitDElement(const DElement *element)
{
    Q_UNUSED(element);

    if (_model_elements.size() > 0 && _model_elements.at(0)) {
        _properties_title.clear();
        _model_elements.at(0)->accept(this);
#ifdef SHOW_DEBUG_PROPERTIES
        if (_separator_line == 0) {
            _separator_line = new QFrame(_top_widget);
            _separator_line->setFrameShape(QFrame::StyledPanel);
            _separator_line->setLineWidth(2);
            _separator_line->setMinimumHeight(2);
            _top_layout->addRow(_separator_line);
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
    if (_pos_rect_label == 0) {
        _pos_rect_label = new QLabel(_top_widget);
        _top_layout->addRow(tr("Position and size:"), _pos_rect_label);
    }
    _pos_rect_label->setText(QString(QStringLiteral("(%1,%2):(%3,%4)-(%5,%6)"))
                             .arg(object->getPos().x())
                             .arg(object->getPos().y())
                             .arg(object->getRect().left())
                             .arg(object->getRect().top())
                             .arg(object->getRect().right())
                             .arg(object->getRect().bottom()));
#endif
    if (_auto_sized_checkbox == 0) {
        _auto_sized_checkbox = new QCheckBox(_top_widget);
        _top_layout->addRow(tr("Auto sized"), _auto_sized_checkbox);
        connect(_auto_sized_checkbox, SIGNAL(clicked(bool)), this, SLOT(onAutoSizedChanged(bool)));
    }
    if (!_auto_sized_checkbox->hasFocus()) {
        bool auto_sized;
        if (haveSameValue(_diagram_elements, &DObject::hasAutoSize, &auto_sized)) {
            _auto_sized_checkbox->setChecked(auto_sized);
        } else {
            _auto_sized_checkbox->setChecked(false);
        }
    }
    if (_visual_primary_role_selector == 0) {
        _visual_primary_role_selector = new PaletteBox(_top_widget);
        setPrimaryRolePalette(_style_element_type, DObject::PRIMARY_ROLE_CUSTOM1, QColor());
        setPrimaryRolePalette(_style_element_type, DObject::PRIMARY_ROLE_CUSTOM2, QColor());
        setPrimaryRolePalette(_style_element_type, DObject::PRIMARY_ROLE_CUSTOM3, QColor());
        setPrimaryRolePalette(_style_element_type, DObject::PRIMARY_ROLE_CUSTOM4, QColor());
        setPrimaryRolePalette(_style_element_type, DObject::PRIMARY_ROLE_CUSTOM5, QColor());
        _top_layout->addRow(tr("Color:"), _visual_primary_role_selector);
        connect(_visual_primary_role_selector, SIGNAL(activated(int)), this, SLOT(onVisualPrimaryRoleChanged(int)));
    }
    if (!_visual_primary_role_selector->hasFocus()) {
        StereotypeDisplayVisitor stereotype_display_visitor;
        stereotype_display_visitor.setModelController(_properties_view->getModelController());
        stereotype_display_visitor.setStereotypeController(_properties_view->getStereotypeController());
        object->accept(&stereotype_display_visitor);
        QString shape_id = stereotype_display_visitor.getShapeIconId();
        QColor base_color;
        if (!shape_id.isEmpty()) {
            StereotypeIcon stereotype_icon = _properties_view->getStereotypeController()->findStereotypeIcon(shape_id);
            base_color = stereotype_icon.getBaseColor();
        }
        setPrimaryRolePalette(_style_element_type, DObject::PRIMARY_ROLE_NORMAL, base_color);
        DObject::VisualPrimaryRole visual_primary_role;
        if (haveSameValue(_diagram_elements, &DObject::getVisualPrimaryRole, &visual_primary_role)) {
            _visual_primary_role_selector->setCurrentIndex(translateVisualPrimaryRoleToIndex(visual_primary_role));
        } else {
            _visual_primary_role_selector->setCurrentIndex(-1);
        }
    }
    if (_visual_secondary_role_selector == 0) {
        _visual_secondary_role_selector = new QComboBox(_top_widget);
        _visual_secondary_role_selector->addItems(QStringList() << tr("Normal")
                                                  << tr("Lighter") << tr("Darker")
                                                  << tr("Soften") << tr("Outline"));
        _top_layout->addRow(tr("Role:"), _visual_secondary_role_selector);
        connect(_visual_secondary_role_selector, SIGNAL(activated(int)), this, SLOT(onVisualSecondaryRoleChanged(int)));
    }
    if (!_visual_secondary_role_selector->hasFocus()) {
        DObject::VisualSecondaryRole visual_secondary_role;
        if (haveSameValue(_diagram_elements, &DObject::getVisualSecondaryRole, &visual_secondary_role)) {
            _visual_secondary_role_selector->setCurrentIndex(translateVisualSecondaryRoleToIndex(visual_secondary_role));
        } else {
            _visual_secondary_role_selector->setCurrentIndex(-1);
        }
    }
    if (_visual_emphasized_checkbox == 0) {
        _visual_emphasized_checkbox = new QCheckBox(_top_widget);
        _top_layout->addRow(tr("Emphasized"), _visual_emphasized_checkbox);
        connect(_visual_emphasized_checkbox, SIGNAL(clicked(bool)), this, SLOT(onVisualEmphasizedChanged(bool)));
    }
    if (!_visual_emphasized_checkbox->hasFocus()) {
        bool emphasized;
        if (haveSameValue(_diagram_elements, &DObject::isVisualEmphasized, &emphasized)) {
            _visual_emphasized_checkbox->setChecked(emphasized);
        } else {
            _visual_emphasized_checkbox->setChecked(false);
        }
    }
    if (_stereotype_display_selector == 0) {
        _stereotype_display_selector = new QComboBox(_top_widget);
        _stereotype_display_selector->addItems(QStringList() << tr("Smart") << tr("None") << tr("Label")
                                               << tr("Decoration") << tr("Icon"));
        _top_layout->addRow(tr("Stereotype display:"), _stereotype_display_selector);
        connect(_stereotype_display_selector, SIGNAL(activated(int)), this, SLOT(onStereotypeDisplayChanged(int)));
    }
    if (!_stereotype_display_selector->hasFocus()) {
        DObject::StereotypeDisplay stereotype_display;
        if (haveSameValue(_diagram_elements, &DObject::getStereotypeDisplay, &stereotype_display)) {
            _stereotype_display_selector->setCurrentIndex(translateStereotypeDisplayToIndex(stereotype_display));
        } else {
            _stereotype_display_selector->setCurrentIndex(-1);
        }
    }
#ifdef SHOW_DEBUG_PROPERTIES
    if (_depth_label == 0) {
        _depth_label = new QLabel(_top_widget);
        _top_layout->addRow(tr("Depth:"), _depth_label);
    }
    _depth_label->setText(QString::number(object->getDepth()));
#endif
}

void PropertiesView::MView::visitDPackage(const DPackage *package)
{
    setTitle<DPackage>(_diagram_elements, tr("Package"), tr("Packages"));
    setStereotypeIconElement(StereotypeIcon::ELEMENT_PACKAGE);
    setStyleElementType(StyleEngine::TYPE_PACKAGE);
    visitDObject(package);
}

void PropertiesView::MView::visitDClass(const DClass *klass)
{
    setTitle<DClass>(_diagram_elements, tr("Class"), tr("Classes"));
    setStereotypeIconElement(StereotypeIcon::ELEMENT_CLASS);
    setStyleElementType(StyleEngine::TYPE_CLASS);
    visitDObject(klass);
    if (_template_display_selector == 0) {
        _template_display_selector = new QComboBox(_top_widget);
        _template_display_selector->addItems(QStringList() << tr("Smart") << tr("Box") << tr("Angle Brackets"));
        _top_layout->addRow(tr("Template display:"), _template_display_selector);
        connect(_template_display_selector, SIGNAL(activated(int)), this, SLOT(onTemplateDisplayChanged(int)));
    }
    if (!_template_display_selector->hasFocus()) {
        DClass::TemplateDisplay template_display;
        if (haveSameValue(_diagram_elements, &DClass::getTemplateDisplay, &template_display)) {
            _template_display_selector->setCurrentIndex(translateTemplateDisplayToIndex(template_display));
        } else {
            _template_display_selector->setCurrentIndex(-1);
        }
    }
    if (_show_all_members_checkbox == 0) {
        _show_all_members_checkbox = new QCheckBox(_top_widget);
        _top_layout->addRow(tr("Show members"), _show_all_members_checkbox);
        connect(_show_all_members_checkbox, SIGNAL(clicked(bool)), this, SLOT(onShowAllMembersChanged(bool)));
    }
    if (!_show_all_members_checkbox->hasFocus()) {
        bool show_all_members;
        if (haveSameValue(_diagram_elements, &DClass::getShowAllMembers, &show_all_members)) {
            _show_all_members_checkbox->setChecked(show_all_members);
        } else {
            _show_all_members_checkbox->setChecked(false);
        }
    }
}

void PropertiesView::MView::visitDComponent(const DComponent *component)
{
    setTitle<DComponent>(_diagram_elements, tr("Component"), tr("Components"));
    setStereotypeIconElement(StereotypeIcon::ELEMENT_COMPONENT);
    setStyleElementType(StyleEngine::TYPE_COMPONENT);
    visitDObject(component);
    if (_plain_shape_checkbox == 0) {
        _plain_shape_checkbox = new QCheckBox(_top_widget);
        _top_layout->addRow(tr("Plain shape"), _plain_shape_checkbox);
        connect(_plain_shape_checkbox, SIGNAL(clicked(bool)), this, SLOT(onPlainShapeChanged(bool)));
    }
    if (!_plain_shape_checkbox->hasFocus()) {
        bool plain_shape;
        if (haveSameValue(_diagram_elements, &DComponent::getPlainShape, &plain_shape)) {
            _plain_shape_checkbox->setChecked(plain_shape);
        } else {
            _plain_shape_checkbox->setChecked(false);
        }
    }
}

void PropertiesView::MView::visitDDiagram(const DDiagram *diagram)
{
    setTitle<DDiagram>(_diagram_elements, tr("Diagram"), tr("Diagrams"));
    setStyleElementType(StyleEngine::TYPE_OTHER);
    visitDObject(diagram);
}

void PropertiesView::MView::visitDItem(const DItem *item)
{
    setTitle<DItem>(_diagram_elements, tr("Item"), tr("Items"));
    setStereotypeIconElement(StereotypeIcon::ELEMENT_ITEM);
    setStyleElementType(StyleEngine::TYPE_ITEM);
    visitDObject(item);
    QList<DItem *> selection = filter<DItem>(_diagram_elements);
    bool is_single_selection = selection.size() == 1;
    if (item->isShapeEditable()) {
        if (_item_shape_edit == 0) {
            _item_shape_edit = new QLineEdit(_top_widget);
            _top_layout->addRow(tr("Shape:"), _item_shape_edit);
            connect(_item_shape_edit, SIGNAL(textChanged(QString)), this, SLOT(onItemShapeChanged(QString)));
        }
        if (is_single_selection) {
            if (item->getShape() != _item_shape_edit->text() && !_item_shape_edit->hasFocus()) {
                _item_shape_edit->setText(item->getShape());
            }
        } else {
            _item_shape_edit->clear();
        }
        if (_item_shape_edit->isEnabled() != is_single_selection) {
            _item_shape_edit->setEnabled(is_single_selection);
        }
    }
}

void PropertiesView::MView::visitDRelation(const DRelation *relation)
{
    visitDElement(relation);
}

void PropertiesView::MView::visitDInheritance(const DInheritance *inheritance)
{
    setTitle<DInheritance>(_diagram_elements, tr("Inheritance"), tr("Inheritances"));
    visitDRelation(inheritance);
}

void PropertiesView::MView::visitDDependency(const DDependency *dependency)
{
    setTitle<DDependency>(_diagram_elements, tr("Dependency"), tr("Dependencies"));
    visitDRelation(dependency);
}

void PropertiesView::MView::visitDAssociation(const DAssociation *association)
{
    setTitle<DAssociation>(_diagram_elements, tr("Association"), tr("Associations"));
    visitDRelation(association);
}

void PropertiesView::MView::visitDAnnotation(const DAnnotation *annotation)
{
    setTitle<DAnnotation>(_diagram_elements, tr("Annotation"), tr("Annotations"));
    visitDElement(annotation);
    if (_annotation_auto_width_checkbox == 0) {
        _annotation_auto_width_checkbox = new QCheckBox(_top_widget);
        _top_layout->addRow(tr("Auto width"), _annotation_auto_width_checkbox);
        connect(_annotation_auto_width_checkbox, SIGNAL(clicked(bool)), this, SLOT(onAutoWidthChanged(bool)));
    }
    if (!_annotation_auto_width_checkbox->hasFocus()) {
        bool auto_sized;
        if (haveSameValue(_diagram_elements, &DAnnotation::hasAutoSize, &auto_sized)) {
            _annotation_auto_width_checkbox->setChecked(auto_sized);
        } else {
            _annotation_auto_width_checkbox->setChecked(false);
        }
    }
    if (_annotation_visual_role_selector == 0) {
        _annotation_visual_role_selector = new QComboBox(_top_widget);
        _annotation_visual_role_selector->addItems(QStringList() << tr("Normal") << tr("Title") << tr("Subtitle") << tr("Emphasized") << tr("Soften") << tr("Footnote"));
        _top_layout->addRow(tr("Role:"), _annotation_visual_role_selector);
        connect(_annotation_visual_role_selector, SIGNAL(activated(int)), this, SLOT(onAnnotationVisualRoleChanged(int)));
    }
    if (!_annotation_visual_role_selector->hasFocus()) {
        DAnnotation::VisualRole visual_role;
        if (haveSameValue(_diagram_elements, &DAnnotation::getVisualRole, &visual_role)) {
            _annotation_visual_role_selector->setCurrentIndex(translateAnnotationVisualRoleToIndex(visual_role));
        } else {
            _annotation_visual_role_selector->setCurrentIndex(-1);
        }
    }
}

void PropertiesView::MView::visitDBoundary(const DBoundary *boundary)
{
    setTitle<DBoundary>(_diagram_elements, tr("Boundary"), tr("Boundaries"));
    visitDElement(boundary);
}

void PropertiesView::MView::onStereotypesChanged(const QString &stereotypes)
{
    QList<QString> set = _stereotypes_controller->fromString(stereotypes);
    assignModelElement<MElement, QList<QString> >(_model_elements, SELECTION_MULTI, set, &MElement::getStereotypes, &MElement::setStereotypes);
}

void PropertiesView::MView::onObjectNameChanged(const QString &name)
{
    assignModelElement<MObject, QString>(_model_elements, SELECTION_SINGLE, name, &MObject::getName, &MObject::setName);
}

void PropertiesView::MView::onNamespaceChanged(const QString &name_space)
{
    assignModelElement<MClass, QString>(_model_elements, SELECTION_MULTI, name_space, &MClass::getNamespace, &MClass::setNamespace);
}

void PropertiesView::MView::onTemplateParametersChanged(const QString &template_parameters)
{
    QList<QString> template_parameters_list = splitTemplateParameters(template_parameters);
    assignModelElement<MClass, QList<QString> >(_model_elements, SELECTION_SINGLE, template_parameters_list, &MClass::getTemplateParameters, &MClass::setTemplateParameters);
}

void PropertiesView::MView::onClassMembersStatusChanged(bool valid)
{
    if (valid) {
        _class_members_status_label->clear();
    } else {
        _class_members_status_label->setText(tr("<font color=red>Invalid syntax!</font>"));
    }
}

void PropertiesView::MView::onParseClassMembers()
{
    _class_members_edit->reparse();
}

void PropertiesView::MView::onClassMembersChanged(QList<MClassMember> &class_members)
{
    assignModelElement<MClass, QList<MClassMember> >(_model_elements, SELECTION_SINGLE, class_members, &MClass::getMembers, &MClass::setMembers);
}

void PropertiesView::MView::onItemVarietyChanged(const QString &variety)
{
    assignModelElement<MItem, QString>(_model_elements, SELECTION_SINGLE, variety, &MItem::getVariety, &MItem::setVariety);
}

void PropertiesView::MView::onRelationNameChanged(const QString &name)
{
    assignModelElement<MRelation, QString>(_model_elements, SELECTION_SINGLE, name, &MRelation::getName, &MRelation::setName);
}

void PropertiesView::MView::onDependencyDirectionChanged(int direction_index)
{
    MDependency::Direction direction = translateIndexToDirection(direction_index);
    assignModelElement<MDependency, MDependency::Direction>(_model_elements, SELECTION_SINGLE, direction, &MDependency::getDirection, &MDependency::setDirection);
}

void PropertiesView::MView::onAssociationEndANameChanged(const QString &name)
{
    assignEmbeddedModelElement<MAssociation, MAssociationEnd, QString>(_model_elements, SELECTION_SINGLE, name, &MAssociation::getA, &MAssociation::setA, &MAssociationEnd::getName, &MAssociationEnd::setName);
}

void PropertiesView::MView::onAssociationEndACardinalityChanged(const QString &cardinality)
{
    assignEmbeddedModelElement<MAssociation, MAssociationEnd, QString>(_model_elements, SELECTION_SINGLE, cardinality, &MAssociation::getA, &MAssociation::setA, &MAssociationEnd::getCardinality, &MAssociationEnd::setCardinality);
}

void PropertiesView::MView::onAssociationEndANavigableChanged(bool navigable)
{
    assignEmbeddedModelElement<MAssociation, MAssociationEnd, bool>(_model_elements, SELECTION_SINGLE, navigable, &MAssociation::getA, &MAssociation::setA, &MAssociationEnd::isNavigable, &MAssociationEnd::setNavigable);
}

void PropertiesView::MView::onAssociationEndAKindChanged(int kind_index)
{
    MAssociationEnd::Kind kind = translateIndexToAssociationKind(kind_index);
    assignEmbeddedModelElement<MAssociation, MAssociationEnd, MAssociationEnd::Kind>(_model_elements, SELECTION_SINGLE, kind, &MAssociation::getA, &MAssociation::setA, &MAssociationEnd::getKind, &MAssociationEnd::setKind);
}

void PropertiesView::MView::onAssociationEndBNameChanged(const QString &name)
{
    assignEmbeddedModelElement<MAssociation, MAssociationEnd, QString>(_model_elements, SELECTION_SINGLE, name, &MAssociation::getB, &MAssociation::setB, &MAssociationEnd::getName, &MAssociationEnd::setName);
}

void PropertiesView::MView::onAssociationEndBCardinalityChanged(const QString &cardinality)
{
    assignEmbeddedModelElement<MAssociation, MAssociationEnd, QString>(_model_elements, SELECTION_SINGLE, cardinality, &MAssociation::getB, &MAssociation::setB, &MAssociationEnd::getCardinality, &MAssociationEnd::setCardinality);
}

void PropertiesView::MView::onAssociationEndBNavigableChanged(bool navigable)
{
    assignEmbeddedModelElement<MAssociation, MAssociationEnd, bool>(_model_elements, SELECTION_SINGLE, navigable, &MAssociation::getB, &MAssociation::setB, &MAssociationEnd::isNavigable, &MAssociationEnd::setNavigable);
}

void PropertiesView::MView::onAssociationEndBKindChanged(int kind_index)
{
    MAssociationEnd::Kind kind = translateIndexToAssociationKind(kind_index);
    assignEmbeddedModelElement<MAssociation, MAssociationEnd, MAssociationEnd::Kind>(_model_elements, SELECTION_SINGLE, kind, &MAssociation::getB, &MAssociation::setB, &MAssociationEnd::getKind, &MAssociationEnd::setKind);
}

void PropertiesView::MView::onAutoSizedChanged(bool auto_sized)
{
    assignModelElement<DObject, bool>(_diagram_elements, SELECTION_MULTI, auto_sized, &DObject::hasAutoSize, &DObject::setAutoSize);
}

void PropertiesView::MView::onVisualPrimaryRoleChanged(int visual_role_index)
{
    DObject::VisualPrimaryRole visual_role = translateIndexToVisualPrimaryRole(visual_role_index);
    assignModelElement<DObject, DObject::VisualPrimaryRole>(_diagram_elements, SELECTION_MULTI, visual_role, &DObject::getVisualPrimaryRole, &DObject::setVisualPrimaryRole);
}

void PropertiesView::MView::onVisualSecondaryRoleChanged(int visual_role_index)
{
    DObject::VisualSecondaryRole visual_role = translateIndexToVisualSecondaryRole(visual_role_index);
    assignModelElement<DObject, DObject::VisualSecondaryRole>(_diagram_elements, SELECTION_MULTI, visual_role, &DObject::getVisualSecondaryRole, &DObject::setVisualSecondaryRole);
}

void PropertiesView::MView::onVisualEmphasizedChanged(bool visual_emphasized)
{
    assignModelElement<DObject, bool>(_diagram_elements, SELECTION_MULTI, visual_emphasized, &DObject::isVisualEmphasized, &DObject::setVisualEmphasized);
}

void PropertiesView::MView::onStereotypeDisplayChanged(int stereotype_display_index)
{
    DObject::StereotypeDisplay stereotype_display = translateIndexToStereotypeDisplay(stereotype_display_index);
    assignModelElement<DObject, DObject::StereotypeDisplay>(_diagram_elements, SELECTION_MULTI, stereotype_display, &DObject::getStereotypeDisplay, &DObject::setStereotypeDisplay);
}

void PropertiesView::MView::onTemplateDisplayChanged(int template_display_index)
{
    DClass::TemplateDisplay template_display = translateIndexToTemplateDisplay(template_display_index);
    assignModelElement<DClass, DClass::TemplateDisplay>(_diagram_elements, SELECTION_MULTI, template_display, &DClass::getTemplateDisplay, &DClass::setTemplateDisplay);
}

void PropertiesView::MView::onShowAllMembersChanged(bool show_all_members)
{
    assignModelElement<DClass, bool>(_diagram_elements, SELECTION_MULTI, show_all_members, &DClass::getShowAllMembers, &DClass::setShowAllMembers);
}

void PropertiesView::MView::onPlainShapeChanged(bool plain_shape)
{
    assignModelElement<DComponent, bool>(_diagram_elements, SELECTION_MULTI, plain_shape, &DComponent::getPlainShape, &DComponent::setPlainShape);
}

void PropertiesView::MView::onItemShapeChanged(const QString &shape)
{
    assignModelElement<DItem, QString>(_diagram_elements, SELECTION_SINGLE, shape, &DItem::getShape, &DItem::setShape);
}

void PropertiesView::MView::onAutoWidthChanged(bool auto_widthed)
{
    assignModelElement<DAnnotation, bool>(_diagram_elements, SELECTION_MULTI, auto_widthed, &DAnnotation::hasAutoSize, &DAnnotation::setAutoSize);
}

void PropertiesView::MView::onAnnotationVisualRoleChanged(int visual_role_index)
{
    DAnnotation::VisualRole visual_role = translateIndexToAnnotationVisualRole((visual_role_index));
    assignModelElement<DAnnotation, DAnnotation::VisualRole>(_diagram_elements, SELECTION_MULTI, visual_role, &DAnnotation::getVisualRole, &DAnnotation::setVisualRole);
}

void PropertiesView::MView::prepare()
{
    QMT_CHECK(!_properties_title.isEmpty());
    if (_top_widget == 0) {
        _top_widget = new QWidget();
        _top_layout = new QFormLayout(_top_widget);
        _top_widget->setLayout(_top_layout);
    }
    if (_class_name_label == 0) {
        _class_name_label = new QLabel();
        _top_layout->addRow(_class_name_label);
    }
    QString title(QStringLiteral("<b>") + _properties_title + QStringLiteral("</b>"));
    if (_class_name_label->text() != title) {
        _class_name_label->setText(title);
    }
}

template<class T, class V>
void PropertiesView::MView::setTitle(const QList<V *> &elements, const QString &singular_title, const QString &plural_title)
{
    QList<T *> filtered = filter<T>(elements);
    if (filtered.size() == elements.size()) {
        if (elements.size() == 1) {
            _properties_title = singular_title;
        } else {
            _properties_title = plural_title;
        }
    } else {
        _properties_title = tr("Multi-Selection");
    }
}

template<class T, class V>
void PropertiesView::MView::setTitle(const MItem *item, const QList<V *> &elements, const QString &singular_title, const QString &plural_title)
{
    if (_properties_title.isEmpty()) {
        QList<T *> filtered = filter<T>(elements);
        if (filtered.size() == elements.size()) {
            if (elements.size() == 1) {
                if (item && !item->isVarietyEditable()) {
                    QString stereotype_icon_id = _properties_view->getStereotypeController()->findStereotypeIconId(StereotypeIcon::ELEMENT_ITEM, QStringList() << item->getVariety());
                    if (!stereotype_icon_id.isEmpty()) {
                        StereotypeIcon stereotype_icon = _properties_view->getStereotypeController()->findStereotypeIcon(stereotype_icon_id);
                        _properties_title = stereotype_icon.getTitle();
                    }
                }
                if (_properties_title.isEmpty()) {
                    _properties_title = singular_title;
                }
            } else {
                _properties_title = plural_title;
            }
        } else {
            _properties_title = tr("Multi-Selection");
        }
    }
}

void PropertiesView::MView::setStereotypeIconElement(StereotypeIcon::Element stereotype_element)
{
    if (_stereotype_element == StereotypeIcon::ELEMENT_ANY) {
        _stereotype_element = stereotype_element;
    }
}

void PropertiesView::MView::setStyleElementType(StyleEngine::ElementType element_type)
{
    if (_style_element_type == StyleEngine::TYPE_OTHER) {
        _style_element_type = element_type;
    }
}

void PropertiesView::MView::setPrimaryRolePalette(StyleEngine::ElementType element_type, DObject::VisualPrimaryRole visual_primary_role, const QColor &base_color)
{
    int index = translateVisualPrimaryRoleToIndex(visual_primary_role);
    const Style *style = _properties_view->getStyleController()->adaptObjectStyle(element_type, ObjectVisuals(visual_primary_role, DObject::SECONDARY_ROLE_NONE, false, base_color, 0));
    _visual_primary_role_selector->setBrush(index, style->getFillBrush());
    _visual_primary_role_selector->setLinePen(index, style->getLinePen());
}

void PropertiesView::MView::setEndAName(const QString &end_a_name)
{
    if (_end_a_name.isEmpty()) {
        _end_a_name = end_a_name;
    }
}

void PropertiesView::MView::setEndBName(const QString &end_b_name)
{
    if (_end_b_name.isEmpty()) {
        _end_b_name = end_b_name;
    }
}

QList<QString> PropertiesView::MView::splitTemplateParameters(const QString &template_parameters)
{
    QList<QString> template_parameters_list;
    foreach (const QString &parameter, template_parameters.split(QLatin1Char(','))) {
        const QString &p = parameter.trimmed();
        if (!p.isEmpty()) {
            template_parameters_list.append(p);
        }
    }
    return template_parameters_list;
}

QString PropertiesView::MView::formatTemplateParameters(const QList<QString> &template_parameters_list)
{
    QString template_paramters;
    bool first = true;
    foreach (const QString &parameter, template_parameters_list) {
        if (!first) {
            template_paramters += QStringLiteral(", ");
        }
        template_paramters += parameter;
        first = false;
    }
    return template_paramters;
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
bool PropertiesView::MView::haveSameValue(const QList<BASE *> &base_elements, V (T::*getter)() const, V *value)
{
    QList<T *> elements = filter<T>(base_elements);
    QMT_CHECK(!elements.isEmpty());
    V candidate = V(); // avoid warning of reading uninitialized variable
    bool have_candidate = false;
    foreach (T *element, elements) {
        if (!have_candidate) {
            candidate = ((*element).*getter)();
            have_candidate = true;
        } else {
            if (candidate != ((*element).*getter)()) {
                return false;
            }
        }
    }
    QMT_CHECK(have_candidate);
    if (!have_candidate) {
        return false;
    }
    if (value) {
        *value = candidate;
    }
    return true;
}

template<class T, class V, class BASE>
void PropertiesView::MView::assignModelElement(const QList<BASE *> &base_elements, SelectionType selection_type, const V &value, V (T::*getter)() const, void (T::*setter)(const V &))
{
    QList<T *> elements = filter<T>(base_elements);
    if ((selection_type == SELECTION_SINGLE && elements.size() == 1) || selection_type == SELECTION_MULTI) {
        foreach (T *element, elements) {
            if (value != ((*element).*getter)()) {
                _properties_view->beginUpdate(element);
                ((*element).*setter)(value);
                _properties_view->endUpdate(element, false);
            }
        }
    }
}

template<class T, class V, class BASE>
void PropertiesView::MView::assignModelElement(const QList<BASE *> &base_elements, SelectionType selection_type, const V &value, V (T::*getter)() const, void (T::*setter)(V))
{
    QList<T *> elements = filter<T>(base_elements);
    if ((selection_type == SELECTION_SINGLE && elements.size() == 1) || selection_type == SELECTION_MULTI) {
        foreach (T *element, elements) {
            if (value != ((*element).*getter)()) {
                _properties_view->beginUpdate(element);
                ((*element).*setter)(value);
                _properties_view->endUpdate(element, false);
            }
        }
    }
}

template<class T, class E, class V, class BASE>
void PropertiesView::MView::assignEmbeddedModelElement(const QList<BASE *> &base_elements, SelectionType selection_type, const V &value, E (T::*getter)() const, void (T::*setter)(const E &), V (E::*v_getter)() const, void (E::*v_setter)(const V &))
{
    QList<T *> elements = filter<T>(base_elements);
    if ((selection_type == SELECTION_SINGLE && elements.size() == 1) || selection_type == SELECTION_MULTI) {
        foreach (T *element, elements) {
            E embedded = ((*element).*getter)();
            if (value != (embedded.*v_getter)()) {
                _properties_view->beginUpdate(element);
                (embedded.*v_setter)(value);
                ((*element).*setter)(embedded);
                _properties_view->endUpdate(element, false);
            }
        }
    }
}

template<class T, class E, class V, class BASE>
void PropertiesView::MView::assignEmbeddedModelElement(const QList<BASE *> &base_elements, SelectionType selection_type, const V &value, E (T::*getter)() const, void (T::*setter)(const E &), V (E::*v_getter)() const, void (E::*v_setter)(V))
{
    QList<T *> elements = filter<T>(base_elements);
    if ((selection_type == SELECTION_SINGLE && elements.size() == 1) || selection_type == SELECTION_MULTI) {
        foreach (T *element, elements) {
            E embedded = ((*element).*getter)();
            if (value != (embedded.*v_getter)()) {
                _properties_view->beginUpdate(element);
                (embedded.*v_setter)(value);
                ((*element).*setter)(embedded);
                _properties_view->endUpdate(element, false);
            }
        }
    }
}

}
