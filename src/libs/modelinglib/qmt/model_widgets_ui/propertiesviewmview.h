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
class QLayout;
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

class QMT_EXPORT PropertiesView::MView : public QObject, public MConstVisitor, public DConstVisitor
{
    Q_OBJECT

public:
    explicit MView(PropertiesView *propertiesView);
    ~MView();

    QWidget *topLevelWidget() const { return m_topWidget; }

    void visitMElement(const MElement *element) override;
    void visitMObject(const MObject *object) override;
    void visitMPackage(const MPackage *package) override;
    void visitMClass(const MClass *klass) override;
    void visitMComponent(const MComponent *component) override;
    void visitMDiagram(const MDiagram *diagram) override;
    void visitMCanvasDiagram(const MCanvasDiagram *diagram) override;
    void visitMItem(const MItem *item) override;
    void visitMRelation(const MRelation *relation) override;
    void visitMDependency(const MDependency *dependency) override;
    void visitMInheritance(const MInheritance *inheritance) override;
    void visitMAssociation(const MAssociation *association) override;

    void visitDElement(const DElement *element) override;
    void visitDObject(const DObject *object) override;
    void visitDPackage(const DPackage *package) override;
    void visitDClass(const DClass *klass) override;
    void visitDComponent(const DComponent *component) override;
    void visitDDiagram(const DDiagram *diagram) override;
    void visitDItem(const DItem *item) override;
    void visitDRelation(const DRelation *relation) override;
    void visitDInheritance(const DInheritance *inheritance) override;
    void visitDDependency(const DDependency *dependency) override;
    void visitDAssociation(const DAssociation *association) override;
    void visitDAnnotation(const DAnnotation *annotation) override;
    void visitDBoundary(const DBoundary *boundary) override;

    void update(QList<MElement *> &modelElements);
    void update(QList<DElement *> &diagramElements, MDiagram *diagram);
    void edit();

protected:
    void onStereotypesChanged(const QString &stereotypes);
    void onObjectNameChanged(const QString &name);
    void onNamespaceChanged(const QString &umlNamespace);
    void onTemplateParametersChanged(const QString &templateParameters);
    void onClassMembersStatusChanged(bool valid);
    void onParseClassMembers();
    void onClassMembersChanged(QList<MClassMember> &classMembers);
    void onItemVarietyChanged(const QString &variety);
    void onRelationNameChanged(const QString &name);
    void onDependencyDirectionChanged(int directionIndex);
    void onAssociationEndANameChanged(const QString &name);
    void onAssociationEndACardinalityChanged(const QString &cardinality);
    void onAssociationEndANavigableChanged(bool navigable);
    void onAssociationEndAKindChanged(int kindIndex);
    void onAssociationEndBNameChanged(const QString &name);
    void onAssociationEndBCardinalityChanged(const QString &cardinality);
    void onAssociationEndBNavigableChanged(bool navigable);
    void onAssociationEndBKindChanged(int kindIndex);
    void onAutoSizedChanged(bool autoSized);
    void onVisualPrimaryRoleChanged(int visualRoleIndex);
    void onVisualSecondaryRoleChanged(int visualRoleIndex);
    void onVisualEmphasizedChanged(bool visualEmphasized);
    void onStereotypeDisplayChanged(int stereotypeDisplayIndex);
    void onTemplateDisplayChanged(int templateDisplayIndex);
    void onShowAllMembersChanged(bool showAllMembers);
    void onPlainShapeChanged(bool plainShape);
    void onItemShapeChanged(const QString &shape);
    void onAutoWidthChanged(bool autoWidthed);
    void onAnnotationVisualRoleChanged(int visualRoleIndex);

    void prepare();
    void addRow(const QString &label, QLayout *layout, const char *id);
    void addRow(const QString &label, QWidget *widget, const char *id);
    void addRow(QWidget *widget, const char *id);
    void insertRow(const char *before_id, const QString &label, QLayout *layout, const char *id);
    void insertRow(const char *before_id, const QString &label, QWidget *widget, const char *id);
    void insertRow(const char *before_id, QWidget *widget, const char *id);
    template<typename T, typename V>
    void setTitle(const QList<V *> &elements, const QString &singularTitle,
                  const QString &pluralTitle);
    template<typename T, typename V>
    void setTitle(const MItem *item, const QList<V *> &elements,
                  const QString &singularTitle, const QString &pluralTitle);
    void setStereotypeIconElement(StereotypeIcon::Element stereotypeElement);
    void setStyleElementType(StyleEngine::ElementType elementType);
    void setPrimaryRolePalette(StyleEngine::ElementType elementType,
                               DObject::VisualPrimaryRole visualPrimaryRole,
                               const QColor &baseColor);
    void setEndAName(const QString &endAName);
    void setEndBName(const QString &endBName);

    QList<QString> splitTemplateParameters(const QString &templateParameters);
    QString formatTemplateParameters(const QList<QString> &templateParametersList);

    enum SelectionType {
        SelectionSingle,
        SelectionMulti
    };

    template<class T, class V>
    QList<T *> filter(const QList<V *> &elements);
    template<class T, class V, class BASE>
    bool haveSameValue(const QList<BASE *> &baseElements, V (T::*getter)() const, V *value);
    template<class T, class V, class BASE>
    void assignModelElement(const QList<BASE *> &baseElements, SelectionType selectionType,
                            const V &value, V (T::*getter)() const, void (T::*setter)(const V &));
    template<class T, class V, class BASE>
    void assignModelElement(const QList<BASE *> &baseElements, SelectionType selectionType,
                            const V &value, V (T::*getter)() const, void (T::*setter)(V));
    template<class T, class E, class V, class BASE>
    void assignEmbeddedModelElement(const QList<BASE *> &baseElements, SelectionType selectionType,
                                    const V &value, E (T::*getter)() const,
                                    void (T::*setter)(const E &),
                                    V (E::*vGetter)() const, void (E::*vSetter)(const V &));
    template<class T, class E, class V, class BASE>
    void assignEmbeddedModelElement(const QList<BASE *> &baseElements, SelectionType selectionType,
                                    const V &value, E (T::*getter)() const,
                                    void (T::*setter)(const E &),
                                    V (E::*vGetter)() const, void (E::*vSetter)(V));

    PropertiesView *m_propertiesView;
    QList<MElement *> m_modelElements;
    QList<DElement *> m_diagramElements;
    MDiagram *m_diagram;
    StereotypesController *m_stereotypesController;
    QWidget *m_topWidget;
    QFormLayout *m_topLayout;
    QList<const char *> m_rowToId;
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

} // namespace qmt
