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

#include "classitem.h"

#include "qmt/diagram/dclass.h"
#include "qmt/diagram_scene/diagramsceneconstants.h"
#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/diagram_scene/parts/contextlabelitem.h"
#include "qmt/diagram_scene/parts/customiconitem.h"
#include "qmt/diagram_scene/parts/relationstarter.h"
#include "qmt/diagram_scene/parts/stereotypesitem.h"
#include "qmt/diagram_scene/parts/templateparameterbox.h"
#include "qmt/infrastructure/contextmenuaction.h"
#include "qmt/infrastructure/geometryutilities.h"
#include "qmt/infrastructure/qmtassert.h"
#include "qmt/model/mclassmember.h"
#include "qmt/stereotype/stereotypecontroller.h"
#include "qmt/stereotype/stereotypeicon.h"
#include "qmt/style/stylecontroller.h"
#include "qmt/style/style.h"
#include "qmt/tasks/diagramscenecontroller.h"
#include "qmt/tasks/ielementtasks.h"

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsLineItem>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QMenu>

#include <algorithm>


namespace qmt {

static const qreal MINIMUM_AUTO_WIDTH = 80.0;
static const qreal MINIMUM_AUTO_HEIGHT = 60.0;
static const qreal BODY_VERT_BORDER = 4.0;
static const qreal BODY_HORIZ_BORDER = 4.0;


ClassItem::ClassItem(DClass *klass, DiagramSceneModel *diagram_scene_model, QGraphicsItem *parent)
    : ObjectItem(klass, diagram_scene_model, parent),
      m_customIcon(0),
      m_shape(0),
      m_namespace(0),
      m_className(0),
      m_contextLabel(0),
      m_attributesSeparator(0),
      m_attributes(0),
      m_methodsSeparator(0),
      m_methods(0),
      m_templateParameterBox(0),
      m_relationStarter(0)
{
}

ClassItem::~ClassItem()
{
}

void ClassItem::update()
{
    prepareGeometryChange();

    updateStereotypeIconDisplay();

    DClass *diagram_class = dynamic_cast<DClass *>(getObject());
    QMT_CHECK(diagram_class);

    const Style *style = getAdaptedStyle(getStereotypeIconId());

    if (diagram_class->getShowAllMembers()) {
        updateMembers(style);
    } else {
        m_attributesText.clear();
        m_methodsText.clear();
    }

    // custom icon
    if (getStereotypeIconDisplay() == StereotypeIcon::DISPLAY_ICON) {
        if (!m_customIcon) {
            m_customIcon = new CustomIconItem(getDiagramSceneModel(), this);
        }
        m_customIcon->setStereotypeIconId(getStereotypeIconId());
        m_customIcon->setBaseSize(getStereotypeIconMinimumSize(m_customIcon->getStereotypeIcon(), CUSTOM_ICON_MINIMUM_AUTO_WIDTH, CUSTOM_ICON_MINIMUM_AUTO_HEIGHT));
        m_customIcon->setBrush(style->getFillBrush());
        m_customIcon->setPen(style->getOuterLinePen());
        m_customIcon->setZValue(SHAPE_ZVALUE);
    } else if (m_customIcon) {
        m_customIcon->scene()->removeItem(m_customIcon);
        delete m_customIcon;
        m_customIcon = 0;
    }

    // shape
    if (!m_customIcon) {
        if (!m_shape) {
            m_shape = new QGraphicsRectItem(this);
        }
        m_shape->setBrush(style->getFillBrush());
        m_shape->setPen(style->getOuterLinePen());
        m_shape->setZValue(SHAPE_ZVALUE);
    } else if (m_shape){
        m_shape->scene()->removeItem(m_shape);
        delete m_shape;
        m_shape = 0;
    }

    // stereotypes
    updateStereotypes(getStereotypeIconId(), getStereotypeIconDisplay(), style);

    // namespace
    if (!diagram_class->getNamespace().isEmpty()) {
        if (!m_namespace) {
            m_namespace = new QGraphicsSimpleTextItem(this);
        }
        m_namespace->setFont(style->getSmallFont());
        m_namespace->setBrush(style->getTextBrush());
        m_namespace->setText(diagram_class->getNamespace());
    } else if (m_namespace) {
        m_namespace->scene()->removeItem(m_namespace);
        delete m_namespace;
        m_namespace = 0;
    }

    DClass::TemplateDisplay template_display = diagram_class->getTemplateDisplay();
    if (template_display == DClass::TEMPLATE_SMART) {
        if (m_customIcon) {
            template_display = DClass::TEMPLATE_NAME;
        } else {
            template_display = DClass::TEMPLATE_BOX;
        }
    }

    // class name
    if (!m_className) {
        m_className = new QGraphicsSimpleTextItem(this);
    }
    m_className->setFont(style->getHeaderFont());
    m_className->setBrush(style->getTextBrush());
    if (template_display == DClass::TEMPLATE_NAME && !diagram_class->getTemplateParameters().isEmpty()) {
        QString name = getObject()->getName();
        name += QLatin1Char('<');
        bool first = true;
        foreach (const QString &p, diagram_class->getTemplateParameters()) {
            if (!first) {
                name += QLatin1Char(',');
            }
            name += p;
            first = false;
        }
        name += QLatin1Char('>');
        m_className->setText(name);
    } else {
        m_className->setText(getObject()->getName());
    }

    // context
    if (showContext()) {
        if (!m_contextLabel) {
            m_contextLabel = new ContextLabelItem(this);
        }
        m_contextLabel->setFont(style->getSmallFont());
        m_contextLabel->setBrush(style->getTextBrush());
        m_contextLabel->setContext(getObject()->getContext());
    } else if (m_contextLabel) {
        m_contextLabel->scene()->removeItem(m_contextLabel);
        delete m_contextLabel;
        m_contextLabel = 0;
    }

    // attributes separator
    if (m_shape || !m_attributesText.isEmpty() || !m_methodsText.isEmpty()) {
        if (!m_attributesSeparator) {
            m_attributesSeparator = new QGraphicsLineItem(this);
        }
        m_attributesSeparator->setPen(style->getInnerLinePen());
        m_attributesSeparator->setZValue(SHAPE_DETAILS_ZVALUE);
    } else if (m_attributesSeparator) {
        m_attributesSeparator->scene()->removeItem(m_attributesSeparator);
        delete m_attributesSeparator;
        m_attributesSeparator = 0;
    }

    // attributes
    if (!m_attributesText.isEmpty()) {
        if (!m_attributes) {
            m_attributes = new QGraphicsTextItem(this);
        }
        m_attributes->setFont(style->getNormalFont());
        //m_attributes->setBrush(style->getTextBrush());
        m_attributes->setDefaultTextColor(style->getTextBrush().color());
        m_attributes->setHtml(m_attributesText);
    } else if (m_attributes) {
        m_attributes->scene()->removeItem(m_attributes);
        delete m_attributes;
        m_attributes = 0;
    }

    // methods separator
    if (m_shape || !m_attributesText.isEmpty() || !m_methodsText.isEmpty()) {
        if (!m_methodsSeparator) {
            m_methodsSeparator = new QGraphicsLineItem(this);
        }
        m_methodsSeparator->setPen(style->getInnerLinePen());
        m_methodsSeparator->setZValue(SHAPE_DETAILS_ZVALUE);
    } else if (m_methodsSeparator) {
        m_methodsSeparator->scene()->removeItem(m_methodsSeparator);
        delete m_methodsSeparator;
        m_methodsSeparator = 0;
    }

    // methods
    if (!m_methodsText.isEmpty()) {
        if (!m_methods) {
            m_methods = new QGraphicsTextItem(this);
        }
        m_methods->setFont(style->getNormalFont());
        //m_methods->setBrush(style->getTextBrush());
        m_methods->setDefaultTextColor(style->getTextBrush().color());
        m_methods->setHtml(m_methodsText);
    } else if (m_methods) {
        m_methods->scene()->removeItem(m_methods);
        delete m_methods;
        m_methods = 0;
    }

    // template parameters
    if (template_display == DClass::TEMPLATE_BOX && !diagram_class->getTemplateParameters().isEmpty()) {
        if (!m_templateParameterBox) {
            m_templateParameterBox = new TemplateParameterBox(this);
        }
        QPen pen = style->getOuterLinePen();
        pen.setStyle(Qt::DashLine);
        m_templateParameterBox->setPen(pen);
        m_templateParameterBox->setBrush(QBrush(Qt::white));
        m_templateParameterBox->setFont(style->getSmallFont());
        m_templateParameterBox->setTextBrush(style->getTextBrush());
        m_templateParameterBox->setTemplateParameters(diagram_class->getTemplateParameters());
    } else if (m_templateParameterBox) {
        m_templateParameterBox->scene()->removeItem(m_templateParameterBox);
        delete m_templateParameterBox;
        m_templateParameterBox = 0;
    }

    updateSelectionMarker(m_customIcon);

    // relation starters
    if (isFocusSelected()) {
        if (!m_relationStarter) {
            m_relationStarter = new RelationStarter(this, getDiagramSceneModel(), 0);
            scene()->addItem(m_relationStarter);
            m_relationStarter->setZValue(RELATION_STARTER_ZVALUE);
            m_relationStarter->addArrow(QLatin1String("inheritance"), ArrowItem::SHAFT_SOLID, ArrowItem::HEAD_TRIANGLE);
            m_relationStarter->addArrow(QLatin1String("dependency"), ArrowItem::SHAFT_DASHED, ArrowItem::HEAD_OPEN);
            m_relationStarter->addArrow(QLatin1String("association"), ArrowItem::SHAFT_SOLID, ArrowItem::HEAD_FILLED_TRIANGLE);
        }
    } else if (m_relationStarter) {
        scene()->removeItem(m_relationStarter);
        delete m_relationStarter;
        m_relationStarter = 0;
    }

    updateAlignmentButtons();

    updateGeometry();
}

bool ClassItem::intersectShapeWithLine(const QLineF &line, QPointF *intersection_point, QLineF *intersection_line) const
{
    QPolygonF polygon;
    if (m_customIcon) {
        QRectF rect = getObject()->getRect();
//        polygon = m_customIcon->path().toFillPolygon(QTransform()
//                                                     .scale(rect.width() / m_customIcon->getShapeWidth(), rect.height() / m_customIcon->getShapeHeight())
//                                                     .translate(getObject()->getPos().x(), getObject()->getPos().y()));
        rect.translate(getObject()->getPos());
        polygon << rect.topLeft() << rect.topRight() << rect.bottomRight() << rect.bottomLeft() << rect.topLeft();
    } else {
        QRectF rect = getObject()->getRect();
        rect.translate(getObject()->getPos());
        polygon << rect.topLeft() << rect.topRight() << rect.bottomRight() << rect.bottomLeft() << rect.topLeft();
    }
    return GeometryUtilities::intersect(polygon, line, intersection_point, intersection_line);
}

QSizeF ClassItem::getMinimumSize() const
{
    return calcMinimumGeometry();
}

QPointF ClassItem::getRelationStartPos() const
{
    return pos();
}

void ClassItem::relationDrawn(const QString &id, const QPointF &to_scene_pos, const QList<QPointF> &intermediate_points)
{
    DElement *target_element = getDiagramSceneModel()->findTopmostElement(to_scene_pos);
    if (target_element) {
        if (id == QLatin1String("inheritance")) {
            DClass *base_class = dynamic_cast<DClass *>(target_element);
            if (base_class) {
                DClass *derived_class = dynamic_cast<DClass *>(getObject());
                QMT_CHECK(derived_class);
                getDiagramSceneModel()->getDiagramSceneController()->createInheritance(derived_class, base_class, intermediate_points, getDiagramSceneModel()->getDiagram());
            }
        } else if (id == QLatin1String("dependency")) {
            DObject *dependant_object = dynamic_cast<DObject *>(target_element);
            if (dependant_object) {
                getDiagramSceneModel()->getDiagramSceneController()->createDependency(getObject(), dependant_object, intermediate_points, getDiagramSceneModel()->getDiagram());
            }
        } else if (id == QLatin1String("association")) {
            DClass *assoziated_class = dynamic_cast<DClass *>(target_element);
            if (assoziated_class) {
                DClass *derived_class = dynamic_cast<DClass *>(getObject());
                QMT_CHECK(derived_class);
                getDiagramSceneModel()->getDiagramSceneController()->createAssociation(derived_class, assoziated_class, intermediate_points, getDiagramSceneModel()->getDiagram());
            }
        }
    }
}

bool ClassItem::extendContextMenu(QMenu *menu)
{
    bool extended = false;
    if (getDiagramSceneModel()->getDiagramSceneController()->getElementTasks()->hasClassDefinition(getObject(), getDiagramSceneModel()->getDiagram())) {
        menu->addAction(new ContextMenuAction(QObject::tr("Show Definition"), QStringLiteral("showDefinition"), menu));
        extended = true;
    }
    return extended;
}

bool ClassItem::handleSelectedContextMenuAction(QAction *action)
{
    ContextMenuAction *klass_action = dynamic_cast<ContextMenuAction *>(action);
    if (klass_action) {
        if (klass_action->getId() == QStringLiteral("showDefinition")) {
            getDiagramSceneModel()->getDiagramSceneController()->getElementTasks()->openClassDefinition(getObject(), getDiagramSceneModel()->getDiagram());
            return true;
        }
    }
    return false;
}

QSizeF ClassItem::calcMinimumGeometry() const
{
    double width = 0.0;
    double height = 0.0;

    if (m_customIcon) {
        return getStereotypeIconMinimumSize(m_customIcon->getStereotypeIcon(), CUSTOM_ICON_MINIMUM_AUTO_WIDTH, CUSTOM_ICON_MINIMUM_AUTO_HEIGHT);
    }

    height += BODY_VERT_BORDER;
    if (CustomIconItem *stereotype_icon_item = getStereotypeIconItem()) {
        width = std::max(width, stereotype_icon_item->boundingRect().width() + 2 * BODY_HORIZ_BORDER);
        height += stereotype_icon_item->boundingRect().height();
    }
    if (StereotypesItem *stereotypes_item = getStereotypesItem()) {
        width = std::max(width, stereotypes_item->boundingRect().width() + 2 * BODY_HORIZ_BORDER);
        height += stereotypes_item->boundingRect().height();
    }
    if (m_namespace) {
        width = std::max(width, m_namespace->boundingRect().width() + 2 * BODY_HORIZ_BORDER);
        height += m_namespace->boundingRect().height();
    }
    if (m_className) {
        width = std::max(width, m_className->boundingRect().width() + 2 * BODY_HORIZ_BORDER);
        height += m_className->boundingRect().height();
    }
    if (m_contextLabel) {
        height += m_contextLabel->getHeight();
    }
    if (m_attributesSeparator) {
        height += 8.0;
    }
    if (m_attributes) {
        width = std::max(width, m_attributes->boundingRect().width() + 2 * BODY_HORIZ_BORDER);
        height += m_attributes->boundingRect().height();
    }
    if (m_methodsSeparator) {
        height += 8.0;
    }
    if (m_methods) {
        width = std::max(width, m_methods->boundingRect().width() + 2 * BODY_HORIZ_BORDER);
        height += m_methods->boundingRect().height();
    }
    height += BODY_VERT_BORDER;

    return GeometryUtilities::ensureMinimumRasterSize(QSizeF(width, height), 2 * RASTER_WIDTH, 2 * RASTER_HEIGHT);
}

void ClassItem::updateGeometry()
{
    prepareGeometryChange();

    // calc width and height
    double width = 0.0;
    double height = 0.0;

    QSizeF geometry = calcMinimumGeometry();
    width = geometry.width();
    height = geometry.height();

    if (getObject()->hasAutoSize()) {
        if (!m_customIcon) {
            if (width < MINIMUM_AUTO_WIDTH) {
                width = MINIMUM_AUTO_WIDTH;
            }
            if (height < MINIMUM_AUTO_HEIGHT) {
                height = MINIMUM_AUTO_HEIGHT;
            }
        }
    } else {
        QRectF rect = getObject()->getRect();
        if (rect.width() > width) {
            width = rect.width();
        }
        if (rect.height() > height) {
            height = rect.height();
        }
    }

    // update sizes and positions
    double left = -width / 2.0;
    double right = width / 2.0;
    double top = -height / 2.0;
    //double bottom = height / 2.0;
    double y = top;

    setPos(getObject()->getPos());

    QRectF rect(left, top, width, height);

    // the object is updated without calling DiagramController intentionally.
    // attribute rect is not a real attribute stored on DObject but
    // a backup for the graphics item used for manual resized and persistency.
    getObject()->setRect(rect);

    if (m_customIcon) {
        m_customIcon->setPos(left, top);
        m_customIcon->setActualSize(QSizeF(width, height));
        y += height;
    }

    if (m_shape) {
        m_shape->setRect(rect);
    }

    y += BODY_VERT_BORDER;
    if (CustomIconItem *stereotype_icon_item = getStereotypeIconItem()) {
        stereotype_icon_item->setPos(right - stereotype_icon_item->boundingRect().width() - BODY_HORIZ_BORDER, y);
        y += stereotype_icon_item->boundingRect().height();
    }
    if (StereotypesItem *stereotypes_item = getStereotypesItem()) {
        stereotypes_item->setPos(-stereotypes_item->boundingRect().width() / 2.0, y);
        y += stereotypes_item->boundingRect().height();
    }
    if (m_namespace) {
        m_namespace->setPos(-m_namespace->boundingRect().width() / 2.0, y);
        y += m_namespace->boundingRect().height();
    }
    if (m_className) {
        m_className->setPos(-m_className->boundingRect().width() / 2.0, y);
        y += m_className->boundingRect().height();
    }
    if (m_contextLabel) {
        if (m_customIcon) {
            m_contextLabel->resetMaxWidth();
        } else {
            m_contextLabel->setMaxWidth(width - 2 * BODY_HORIZ_BORDER);
        }
        m_contextLabel->setPos(-m_contextLabel->boundingRect().width() / 2.0, y);
        y += m_contextLabel->boundingRect().height();
    }
    if (m_attributesSeparator) {
        m_attributesSeparator->setLine(left, 4.0, right, 4.0);
        m_attributesSeparator->setPos(0, y);
        y += 8.0;
    }
    if (m_attributes) {
        if (m_customIcon) {
            m_attributes->setPos(-m_attributes->boundingRect().width() / 2.0, y);
        } else {
            m_attributes->setPos(left + BODY_HORIZ_BORDER, y);
        }
        y += m_attributes->boundingRect().height();
    }
    if (m_methodsSeparator) {
        m_methodsSeparator->setLine(left, 4.0, right, 4.0);
        m_methodsSeparator->setPos(0, y);
        y += 8.0;
    }
    if (m_methods) {
        if (m_customIcon) {
            m_methods->setPos(-m_methods->boundingRect().width() / 2.0, y);
        } else {
            m_methods->setPos(left + BODY_HORIZ_BORDER, y);
        }
        y += m_methods->boundingRect().height();
    }

    if (m_templateParameterBox) {
        m_templateParameterBox->setBreakLines(false);
        double x = right - m_templateParameterBox->boundingRect().width() * 0.8;
        if (x < 0) {
            m_templateParameterBox->setBreakLines(true);
            x = right - m_templateParameterBox->boundingRect().width() * 0.8;
        }
        if (x < 0) {
            x = 0;
        }
        m_templateParameterBox->setPos(x, top - m_templateParameterBox->boundingRect().height() + BODY_VERT_BORDER);
    }

    updateSelectionMarkerGeometry(rect);

    if (m_relationStarter) {
        m_relationStarter->setPos(mapToScene(QPointF(right + 8.0, top)));
    }

    updateAlignmentButtonsGeometry(rect);

    updateDepth();

}

void ClassItem::updateMembers(const Style *style)
{
    Q_UNUSED(style)

    m_attributesText.clear();
    m_methodsText.clear();

    MClassMember::Visibility attributes_visibility = MClassMember::VISIBILITY_UNDEFINED;
    MClassMember::Visibility methods_visibility = MClassMember::VISIBILITY_UNDEFINED;
    QString attributes_group;
    QString methods_group;

    MClassMember::Visibility *current_visibility = 0;
    QString *current_group = 0;
    QString *text = 0;

    DClass *dclass = dynamic_cast<DClass *>(getObject());
    QMT_CHECK(dclass);

    // TODO move bool have_icon_fonts into class Style?
    bool have_icon_fonts = false; // style->getNormalFont().family() == QStringLiteral("Modelling");
    // TODO any reason to show visibility as group instead of per member?
    bool use_group_visibility = false;

    foreach (const MClassMember &member, dclass->getMembers()) {

        switch (member.getMemberType()) {
        case MClassMember::MEMBER_UNDEFINED:
            QMT_CHECK(false);
            break;
        case MClassMember::MEMBER_ATTRIBUTE:
            current_visibility = &attributes_visibility;
            current_group = &attributes_group;
            text = &m_attributesText;
            break;
        case MClassMember::MEMBER_METHOD:
            current_visibility = &methods_visibility;
            current_group = &methods_group;
            text = &m_methodsText;
            break;
        }

        if (!text->isEmpty()) {
            *text += QStringLiteral("<br/>");
        }

        bool add_newline = false;
        bool add_space = false;
        if (member.getVisibility() != *current_visibility) {
            if (use_group_visibility) {
                if (member.getVisibility() != MClassMember::VISIBILITY_UNDEFINED) {
                    QString vis;
                    switch (member.getVisibility()) {
                    case MClassMember::VISIBILITY_UNDEFINED:
                        break;
                    case MClassMember::VISIBILITY_PUBLIC:
                        vis = QStringLiteral("public:");
                        break;
                    case MClassMember::VISIBILITY_PROTECTED:
                        vis = QStringLiteral("protected:");
                        break;
                    case MClassMember::VISIBILITY_PRIVATE:
                        vis = QStringLiteral("private:");
                        break;
                    case MClassMember::VISIBILITY_SIGNALS:
                        vis = QStringLiteral("signals:");
                        break;
                    case MClassMember::VISIBILITY_PRIVATE_SLOTS:
                        vis = QStringLiteral("private slots:");
                        break;
                    case MClassMember::VISIBILITY_PROTECTED_SLOTS:
                        vis = QStringLiteral("protected slots:");
                        break;
                    case MClassMember::VISIBILITY_PUBLIC_SLOTS:
                        vis = QStringLiteral("public slots:");
                        break;
                    }
                    *text += vis;
                    add_newline = true;
                    add_space = true;
                }
            }
            *current_visibility = member.getVisibility();
        }
        if (member.getGroup() != current_group) {
            if (add_space) {
                *text += QStringLiteral(" ");
            }
            *text += QString(QStringLiteral("[%1]")).arg(member.getGroup());
            add_newline = true;
            *current_group = member.getGroup();
        }
        if (add_newline) {
            *text += QStringLiteral("<br/>");
        }

        add_space = false;
        bool have_signal = false;
        bool have_slot = false;
        if (!use_group_visibility) {
            if (member.getVisibility() != MClassMember::VISIBILITY_UNDEFINED) {
                QString vis;
                switch (member.getVisibility()) {
                case MClassMember::VISIBILITY_UNDEFINED:
                    break;
                case MClassMember::VISIBILITY_PUBLIC:
                    vis = have_icon_fonts ? QString(QChar(0xe990)) : QStringLiteral("+");
                    add_space = true;
                    break;
                case MClassMember::VISIBILITY_PROTECTED:
                    vis = have_icon_fonts ? QString(QChar(0xe98e)) : QStringLiteral("#");
                    add_space = true;
                    break;
                case MClassMember::VISIBILITY_PRIVATE:
                    vis = have_icon_fonts ? QString(QChar(0xe98f)) : QStringLiteral("-");
                    add_space = true;
                    break;
                case MClassMember::VISIBILITY_SIGNALS:
                    vis = have_icon_fonts ? QString(QChar(0xe994)) : QStringLiteral(">");
                    have_signal = true;
                    add_space = true;
                    break;
                case MClassMember::VISIBILITY_PRIVATE_SLOTS:
                    vis = have_icon_fonts ? QString(QChar(0xe98f)) + QChar(0xe9cb)
                                          : QStringLiteral("-$");
                    have_slot = true;
                    add_space = true;
                    break;
                case MClassMember::VISIBILITY_PROTECTED_SLOTS:
                    vis = have_icon_fonts ? QString(QChar(0xe98e)) + QChar(0xe9cb)
                                          : QStringLiteral("#$");
                    have_slot = true;
                    add_space = true;
                    break;
                case MClassMember::VISIBILITY_PUBLIC_SLOTS:
                    vis = have_icon_fonts ? QString(QChar(0xe990)) + QChar(0xe9cb)
                                          : QStringLiteral("+$");
                    have_slot = true;
                    add_space = true;
                    break;
                }
                *text += vis;
            }
        }

        if (member.getProperties() & MClassMember::PROPERTY_QSIGNAL && !have_signal) {
            *text += have_icon_fonts ? QString(QChar(0xe994)) : QStringLiteral(">");
            add_space = true;
        }
        if (member.getProperties() & MClassMember::PROPERTY_QSLOT && !have_slot) {
            *text += have_icon_fonts ? QString(QChar(0xe9cb)) : QStringLiteral("$");
            add_space = true;
        }
        if (add_space) {
            *text += QStringLiteral(" ");
        }
        if (member.getProperties() & MClassMember::PROPERTY_QINVOKABLE) {
            *text += QStringLiteral("invokable ");
        }
        if (!member.getStereotypes().isEmpty()) {
            *text += StereotypesItem::format(member.getStereotypes());
            *text += QStringLiteral(" ");
        }
        if (member.getProperties() & MClassMember::PROPERTY_VIRTUAL) {
            *text += QStringLiteral("virtual ");
        }
        *text += member.getDeclaration();
        if (member.getProperties() & MClassMember::PROPERTY_CONST) {
            *text += QStringLiteral(" const");
        }
        if (member.getProperties() & MClassMember::PROPERTY_OVERRIDE) {
            *text += QStringLiteral(" override");
        }
        if (member.getProperties() & MClassMember::PROPERTY_FINAL) {
            *text += QStringLiteral(" final");
        }
        if (member.getProperties() & MClassMember::PROPERTY_ABSTRACT) {
            *text += QStringLiteral(" = 0");
        }
    }
}

}
