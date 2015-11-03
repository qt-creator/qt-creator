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

    QWidget *getTopLevelWidget() const { return m_topWidget; }

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
    PropertiesView *m_propertiesView;

    QList<MElement *> m_modelElements;

    QList<DElement *> m_diagramElements;
    MDiagram *m_diagram;

    StereotypesController *m_stereotypesController;

    QWidget *m_topWidget;
    QFormLayout *m_topLayout;
    QString m_propertiesTitle;
    // MElement
    StereotypeIcon::Element m_stereotypeElement;
    QLabel *m_classNameLabel;
    QComboBox *m_stereotypeComboBox;
    QLabel *m_reverseEngineeredLabel;
    // MObject
    QLineEdit *m_elementNameLineEdit;
    QLabel *m_childrenLabel;
    QLabel *m_relationsLabel;
    // MClass
    QLineEdit *m_namespaceLineEdit;
    QLineEdit *m_templateParametersLineEdit;
    QLabel *m_classMembersStatusLabel;
    QPushButton *m_classMembersParseButton;
    ClassMembersEdit *m_classMembersEdit;
    // MDiagram
    QLabel *m_diagramsLabel;
    // MItem
    QLineEdit *m_itemVarietyEdit;
    // MRelation
    QString m_endAName;
    QLabel *m_endALabel;
    QString m_endBName;
    QLabel *m_endBLabel;
    // MDependency
    QComboBox *m_directionSelector;
    // MAssociation
    QLineEdit *m_endAEndName;
    QLineEdit *m_endACardinality;
    QCheckBox *m_endANavigable;
    QComboBox *m_endAKind;
    QLineEdit *m_endBEndName;
    QLineEdit *m_endBCardinality;
    QCheckBox *m_endBNavigable;
    QComboBox *m_endBKind;

    // DElement
    QFrame *m_separatorLine;
    // DObject
    StyleEngine::ElementType m_styleElementType;
    QLabel *m_posRectLabel;
    QCheckBox *m_autoSizedCheckbox;
    PaletteBox *m_visualPrimaryRoleSelector;
    QComboBox *m_visualSecondaryRoleSelector;
    QCheckBox *m_visualEmphasizedCheckbox;
    QComboBox *m_stereotypeDisplaySelector;
    QLabel *m_depthLabel;
    // DClass
    QComboBox *m_templateDisplaySelector;
    QCheckBox *m_showAllMembersCheckbox;
    // DComponent
    QCheckBox *m_plainShapeCheckbox;
    // DItem
    QLineEdit *m_itemShapeEdit;
    // DAnnotation
    QCheckBox *m_annotationAutoWidthCheckbox;
    QComboBox *m_annotationVisualRoleSelector;
};

}

#endif // QMT_PROPERTIESVIEWMVIEW_H
