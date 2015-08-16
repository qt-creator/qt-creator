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

#ifndef QMT_PROPERTIESVIEWMVIEW_H
#define QMT_PROPERTIESVIEWMVIEW_H

#include <QObject>

#include "propertiesview.h"

#include "qmt/model/mconstvisitor.h"
#include "qmt/diagram/dconstvisitor.h"
#include "qmt/diagram/dobject.h"
#include "qmt/stereotype/stereotypeicon.h"
#include "qmt/style/styleengine.h"

#include <QList>

QT_BEGIN_NAMESPACE
class QWidget;
class QLabel;
class QFormLayout;
class QLineEdit;
class QPushButton;
class QComboBox;
class QCheckBox;
class QFrame;
QT_END_NAMESPACE


namespace qmt {

class StereotypesController;
class MClassMember;
class ClassMembersEdit;
class PaletteBox;


class QMT_EXPORT PropertiesView::MView :
        public QObject,
        public MConstVisitor,
        public DConstVisitor
{
    Q_OBJECT

public:

    MView(PropertiesView *properties_view);

    ~MView();

public:

    QWidget *getTopLevelWidget() const { return _top_widget; }

public:

    void visitMElement(const MElement *element);

    void visitMObject(const MObject *object);

    void visitMPackage(const MPackage *package);

    void visitMClass(const MClass *klass);

    void visitMComponent(const MComponent *component);

    void visitMDiagram(const MDiagram *diagram);

    void visitMCanvasDiagram(const MCanvasDiagram *diagram);

    void visitMItem(const MItem *item);

    void visitMRelation(const MRelation *relation);

    void visitMDependency(const MDependency *dependency);

    void visitMInheritance(const MInheritance *inheritance);

    void visitMAssociation(const MAssociation *association);

public:

    void visitDElement(const DElement *element);

    void visitDObject(const DObject *object);

    void visitDPackage(const DPackage *package);

    void visitDClass(const DClass *klass);

    void visitDComponent(const DComponent *component);

    void visitDDiagram(const DDiagram *diagram);

    void visitDItem(const DItem *item);

    void visitDRelation(const DRelation *relation);

    void visitDInheritance(const DInheritance *inheritance);

    void visitDDependency(const DDependency *dependency);

    void visitDAssociation(const DAssociation *association);

    void visitDAnnotation(const DAnnotation *annotation);

    void visitDBoundary(const DBoundary *boundary);

public:

    void update(QList<MElement *> &model_elements);

    void update(QList<DElement *> &diagram_elements, MDiagram *diagram);

    void edit();

private slots:

    void onStereotypesChanged(const QString &stereotypes);

    void onObjectNameChanged(const QString &name);

    void onNamespaceChanged(const QString &name_space);

    void onTemplateParametersChanged(const QString &template_parameters);

    void onClassMembersStatusChanged(bool valid);

    void onParseClassMembers();

    void onClassMembersChanged(QList<MClassMember> &class_members);

    void onItemVarietyChanged(const QString &variety);

    void onRelationNameChanged(const QString &name);

    void onDependencyDirectionChanged(int direction_index);

    void onAssociationEndANameChanged(const QString &name);

    void onAssociationEndACardinalityChanged(const QString &cardinality);

    void onAssociationEndANavigableChanged(bool navigable);

    void onAssociationEndAKindChanged(int kind_index);

    void onAssociationEndBNameChanged(const QString &name);

    void onAssociationEndBCardinalityChanged(const QString &cardinality);

    void onAssociationEndBNavigableChanged(bool navigable);

    void onAssociationEndBKindChanged(int kind_index);

    void onAutoSizedChanged(bool auto_sized);

    void onVisualPrimaryRoleChanged(int visual_role_index);

    void onVisualSecondaryRoleChanged(int visual_role_index);

    void onVisualEmphasizedChanged(bool visual_emphasized);

    void onStereotypeDisplayChanged(int stereotype_display_index);

    void onTemplateDisplayChanged(int template_display_index);

    void onShowAllMembersChanged(bool show_all_members);

    void onPlainShapeChanged(bool plain_shape);

    void onItemShapeChanged(const QString &shape);

    void onAutoWidthChanged(bool auto_widthed);

    void onAnnotationVisualRoleChanged(int visual_role_index);

private:

    void prepare();

    template<class T, class V>
    void setTitle(const QList<V *> &elements, const QString &singular_title, const QString &plural_title);

    template<class T, class V>
    void setTitle(const MItem *item, const QList<V *> &elements, const QString &singular_title, const QString &plural_title);

    void setStereotypeIconElement(StereotypeIcon::Element stereotype_element);

    void setStyleElementType(StyleEngine::ElementType element_type);

    void setPrimaryRolePalette(StyleEngine::ElementType element_type, DObject::VisualPrimaryRole visual_primary_role, const QColor &base_color);

    void setEndAName(const QString &end_a_name);

    void setEndBName(const QString &end_b_name);

    QList<QString> splitTemplateParameters(const QString &template_parameters);

    QString formatTemplateParameters(const QList<QString> &template_parameters_list);

    enum SelectionType {
        SELECTION_SINGLE,
        SELECTION_MULTI
    };

    template<class T, class V>
    QList<T *> filter(const QList<V *> &elements);

    template<class T, class V, class BASE>
    bool haveSameValue(const QList<BASE *> &base_elements, V (T::*getter)() const, V *value);

    template<class T, class V, class BASE>
    void assignModelElement(const QList<BASE *> &base_elements, SelectionType selection_type, const V &value, V (T::*getter)() const, void (T::*setter)(const V &));

    template<class T, class V, class BASE>
    void assignModelElement(const QList<BASE *> &base_elements, SelectionType selection_type, const V &value, V (T::*getter)() const, void (T::*setter)(V));

    template<class T, class E, class V, class BASE>
    void assignEmbeddedModelElement(const QList<BASE *> &base_elements, SelectionType selection_type, const V &value, E (T::*getter)() const, void (T::*setter)(const E &), V (E::*v_getter)() const, void (E::*v_setter)(const V &));

    template<class T, class E, class V, class BASE>
    void assignEmbeddedModelElement(const QList<BASE *> &base_elements, SelectionType selection_type, const V &value, E (T::*getter)() const, void (T::*setter)(const E &), V (E::*v_getter)() const, void (E::*v_setter)(V));

private:
    PropertiesView *_properties_view;

    QList<MElement *> _model_elements;

    QList<DElement *> _diagram_elements;
    MDiagram *_diagram;

    StereotypesController *_stereotypes_controller;

    QWidget *_top_widget;
    QFormLayout *_top_layout;
    QString _properties_title;
    // MElement
    StereotypeIcon::Element _stereotype_element;
    QLabel *_class_name_label;
    QComboBox *_stereotype_combo_box;
    QLabel *_reverse_engineered_label;
    // MObject
    QLineEdit *_element_name_line_edit;
    QLabel *_children_label;
    QLabel *_relations_label;
    // MClass
    QLineEdit *_namespace_line_edit;
    QLineEdit *_template_parameters_line_edit;
    QLabel *_class_members_status_label;
    QPushButton *_class_members_parse_button;
    ClassMembersEdit *_class_members_edit;
    // MDiagram
    QLabel *_diagrams_label;
    // MItem
    QLineEdit *_item_variety_edit;
    // MRelation
    QString _end_a_name;
    QLabel *_end_a_label;
    QString _end_b_name;
    QLabel *_end_b_label;
    // MDependency
    QComboBox *_direction_selector;
    // MAssociation
    QLineEdit *_end_a_end_name;
    QLineEdit *_end_a_cardinality;
    QCheckBox *_end_a_navigable;
    QComboBox *_end_a_kind;
    QLineEdit *_end_b_end_name;
    QLineEdit *_end_b_cardinality;
    QCheckBox *_end_b_navigable;
    QComboBox *_end_b_kind;

    // DElement
    QFrame *_separator_line;
    // DObject
    StyleEngine::ElementType _style_element_type;
    QLabel *_pos_rect_label;
    QCheckBox *_auto_sized_checkbox;
    PaletteBox *_visual_primary_role_selector;
    QComboBox *_visual_secondary_role_selector;
    QCheckBox *_visual_emphasized_checkbox;
    QComboBox *_stereotype_display_selector;
    QLabel *_depth_label;
    // DClass
    QComboBox *_template_display_selector;
    QCheckBox *_show_all_members_checkbox;
    // DComponent
    QCheckBox *_plain_shape_checkbox;
    // DItem
    QLineEdit *_item_shape_edit;
    // DAnnotation
    QCheckBox *_annotation_auto_width_checkbox;
    QComboBox *_annotation_visual_role_selector;
};

}

#endif // QMT_PROPERTIESVIEWMVIEW_H
