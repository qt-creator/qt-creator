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
      _custom_icon(0),
      _shape(0),
      _namespace(0),
      _class_name(0),
      _context_label(0),
      _attributes_separator(0),
      _attributes(0),
      _methods_separator(0),
      _methods(0),
      _template_parameter_box(0),
      _relation_starter(0)
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
        _attributes_text.clear();
        _methods_text.clear();
    }

    // custom icon
    if (getStereotypeIconDisplay() == StereotypeIcon::DISPLAY_ICON) {
        if (!_custom_icon) {
            _custom_icon = new CustomIconItem(getDiagramSceneModel(), this);
        }
        _custom_icon->setStereotypeIconId(getStereotypeIconId());
        _custom_icon->setBaseSize(getStereotypeIconMinimumSize(_custom_icon->getStereotypeIcon(), CUSTOM_ICON_MINIMUM_AUTO_WIDTH, CUSTOM_ICON_MINIMUM_AUTO_HEIGHT));
        _custom_icon->setBrush(style->getFillBrush());
        _custom_icon->setPen(style->getOuterLinePen());
        _custom_icon->setZValue(SHAPE_ZVALUE);
    } else if (_custom_icon) {
        _custom_icon->scene()->removeItem(_custom_icon);
        delete _custom_icon;
        _custom_icon = 0;
    }

    // shape
    if (!_custom_icon) {
        if (!_shape) {
            _shape = new QGraphicsRectItem(this);
        }
        _shape->setBrush(style->getFillBrush());
        _shape->setPen(style->getOuterLinePen());
        _shape->setZValue(SHAPE_ZVALUE);
    } else if (_shape){
        _shape->scene()->removeItem(_shape);
        delete _shape;
        _shape = 0;
    }

    // stereotypes
    updateStereotypes(getStereotypeIconId(), getStereotypeIconDisplay(), style);

    // namespace
    if (!diagram_class->getNamespace().isEmpty()) {
        if (!_namespace) {
            _namespace = new QGraphicsSimpleTextItem(this);
        }
        _namespace->setFont(style->getSmallFont());
        _namespace->setBrush(style->getTextBrush());
        _namespace->setText(diagram_class->getNamespace());
    } else if (_namespace) {
        _namespace->scene()->removeItem(_namespace);
        delete _namespace;
        _namespace = 0;
    }

    DClass::TemplateDisplay template_display = diagram_class->getTemplateDisplay();
    if (template_display == DClass::TEMPLATE_SMART) {
        if (_custom_icon) {
            template_display = DClass::TEMPLATE_NAME;
        } else {
            template_display = DClass::TEMPLATE_BOX;
        }
    }

    // class name
    if (!_class_name) {
        _class_name = new QGraphicsSimpleTextItem(this);
    }
    _class_name->setFont(style->getHeaderFont());
    _class_name->setBrush(style->getTextBrush());
    if (template_display == DClass::TEMPLATE_NAME && !diagram_class->getTemplateParameters().isEmpty()) {
        QString name = getObject()->getName();
        name += QLatin1Char('<');
        bool first = true;
        foreach (const QString p, diagram_class->getTemplateParameters()) {
            if (!first) {
                name += QLatin1Char(',');
            }
            name += p;
            first = false;
        }
        name += QLatin1Char('>');
        _class_name->setText(name);
    } else {
        _class_name->setText(getObject()->getName());
    }

    // context
    if (showContext()) {
        if (!_context_label) {
            _context_label = new ContextLabelItem(this);
        }
        _context_label->setFont(style->getSmallFont());
        _context_label->setBrush(style->getTextBrush());
        _context_label->setContext(getObject()->getContext());
    } else if (_context_label) {
        _context_label->scene()->removeItem(_context_label);
        delete _context_label;
        _context_label = 0;
    }

    // attributes separator
    if (_shape || !_attributes_text.isEmpty() || !_methods_text.isEmpty()) {
        if (!_attributes_separator) {
            _attributes_separator = new QGraphicsLineItem(this);
        }
        _attributes_separator->setPen(style->getInnerLinePen());
        _attributes_separator->setZValue(SHAPE_DETAILS_ZVALUE);
    } else if (_attributes_separator) {
        _attributes_separator->scene()->removeItem(_attributes_separator);
        delete _attributes_separator;
        _attributes_separator = 0;
    }

    // attributes
    if (!_attributes_text.isEmpty()) {
        if (!_attributes) {
            _attributes = new QGraphicsTextItem(this);
        }
        _attributes->setFont(style->getNormalFont());
        //_attributes->setBrush(style->getTextBrush());
        _attributes->setDefaultTextColor(style->getTextBrush().color());
        _attributes->setHtml(_attributes_text);
    } else if (_attributes) {
        _attributes->scene()->removeItem(_attributes);
        delete _attributes;
        _attributes = 0;
    }

    // methods separator
    if (_shape || !_attributes_text.isEmpty() || !_methods_text.isEmpty()) {
        if (!_methods_separator) {
            _methods_separator = new QGraphicsLineItem(this);
        }
        _methods_separator->setPen(style->getInnerLinePen());
        _methods_separator->setZValue(SHAPE_DETAILS_ZVALUE);
    } else if (_methods_separator) {
        _methods_separator->scene()->removeItem(_methods_separator);
        delete _methods_separator;
        _methods_separator = 0;
    }

    // methods
    if (!_methods_text.isEmpty()) {
        if (!_methods) {
            _methods = new QGraphicsTextItem(this);
        }
        _methods->setFont(style->getNormalFont());
        //_methods->setBrush(style->getTextBrush());
        _methods->setDefaultTextColor(style->getTextBrush().color());
        _methods->setHtml(_methods_text);
    } else if (_methods) {
        _methods->scene()->removeItem(_methods);
        delete _methods;
        _methods = 0;
    }

    // template parameters
    if (template_display == DClass::TEMPLATE_BOX && !diagram_class->getTemplateParameters().isEmpty()) {
        if (!_template_parameter_box) {
            _template_parameter_box = new TemplateParameterBox(this);
        }
        QPen pen = style->getOuterLinePen();
        pen.setStyle(Qt::DashLine);
        _template_parameter_box->setPen(pen);
        _template_parameter_box->setBrush(QBrush(Qt::white));
        _template_parameter_box->setFont(style->getSmallFont());
        _template_parameter_box->setTextBrush(style->getTextBrush());
        _template_parameter_box->setTemplateParameters(diagram_class->getTemplateParameters());
    } else if (_template_parameter_box) {
        _template_parameter_box->scene()->removeItem(_template_parameter_box);
        delete _template_parameter_box;
        _template_parameter_box = 0;
    }

    updateSelectionMarker(_custom_icon);

    // relation starters
    if (isFocusSelected()) {
        if (!_relation_starter) {
            _relation_starter = new RelationStarter(this, getDiagramSceneModel(), 0);
            scene()->addItem(_relation_starter);
            _relation_starter->setZValue(RELATION_STARTER_ZVALUE);
            _relation_starter->addArrow(QLatin1String("inheritance"), ArrowItem::SHAFT_SOLID, ArrowItem::HEAD_TRIANGLE);
            _relation_starter->addArrow(QLatin1String("dependency"), ArrowItem::SHAFT_DASHED, ArrowItem::HEAD_OPEN);
            _relation_starter->addArrow(QLatin1String("association"), ArrowItem::SHAFT_SOLID, ArrowItem::HEAD_FILLED_TRIANGLE);
        }
    } else if (_relation_starter) {
        scene()->removeItem(_relation_starter);
        delete _relation_starter;
        _relation_starter = 0;
    }

    updateAlignmentButtons();

    updateGeometry();
}

bool ClassItem::intersectShapeWithLine(const QLineF &line, QPointF *intersection_point, QLineF *intersection_line) const
{
    QPolygonF polygon;
    if (_custom_icon) {
        QRectF rect = getObject()->getRect();
//        polygon = _custom_icon->path().toFillPolygon(QTransform()
//                                                     .scale(rect.width() / _custom_icon->getShapeWidth(), rect.height() / _custom_icon->getShapeHeight())
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

    if (_custom_icon) {
        return getStereotypeIconMinimumSize(_custom_icon->getStereotypeIcon(), CUSTOM_ICON_MINIMUM_AUTO_WIDTH, CUSTOM_ICON_MINIMUM_AUTO_HEIGHT);
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
    if (_namespace) {
        width = std::max(width, _namespace->boundingRect().width() + 2 * BODY_HORIZ_BORDER);
        height += _namespace->boundingRect().height();
    }
    if (_class_name) {
        width = std::max(width, _class_name->boundingRect().width() + 2 * BODY_HORIZ_BORDER);
        height += _class_name->boundingRect().height();
    }
    if (_context_label) {
        height += _context_label->getHeight();
    }
    if (_attributes_separator) {
        height += 8.0;
    }
    if (_attributes) {
        width = std::max(width, _attributes->boundingRect().width() + 2 * BODY_HORIZ_BORDER);
        height += _attributes->boundingRect().height();
    }
    if (_methods_separator) {
        height += 8.0;
    }
    if (_methods) {
        width = std::max(width, _methods->boundingRect().width() + 2 * BODY_HORIZ_BORDER);
        height += _methods->boundingRect().height();
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
        if (!_custom_icon) {
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

    if (_custom_icon) {
        _custom_icon->setPos(left, top);
        _custom_icon->setActualSize(QSizeF(width, height));
        y += height;
    }

    if (_shape) {
        _shape->setRect(rect);
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
    if (_namespace) {
        _namespace->setPos(-_namespace->boundingRect().width() / 2.0, y);
        y += _namespace->boundingRect().height();
    }
    if (_class_name) {
        _class_name->setPos(-_class_name->boundingRect().width() / 2.0, y);
        y += _class_name->boundingRect().height();
    }
    if (_context_label) {
        if (_custom_icon) {
            _context_label->resetMaxWidth();
        } else {
            _context_label->setMaxWidth(width - 2 * BODY_HORIZ_BORDER);
        }
        _context_label->setPos(-_context_label->boundingRect().width() / 2.0, y);
        y += _context_label->boundingRect().height();
    }
    if (_attributes_separator) {
        _attributes_separator->setLine(left, 4.0, right, 4.0);
        _attributes_separator->setPos(0, y);
        y += 8.0;
    }
    if (_attributes) {
        if (_custom_icon) {
            _attributes->setPos(-_attributes->boundingRect().width() / 2.0, y);
        } else {
            _attributes->setPos(left + BODY_HORIZ_BORDER, y);
        }
        y += _attributes->boundingRect().height();
    }
    if (_methods_separator) {
        _methods_separator->setLine(left, 4.0, right, 4.0);
        _methods_separator->setPos(0, y);
        y += 8.0;
    }
    if (_methods) {
        if (_custom_icon) {
            _methods->setPos(-_methods->boundingRect().width() / 2.0, y);
        } else {
            _methods->setPos(left + BODY_HORIZ_BORDER, y);
        }
        y += _methods->boundingRect().height();
    }

    if (_template_parameter_box) {
        _template_parameter_box->setBreakLines(false);
        double x = right - _template_parameter_box->boundingRect().width() * 0.8;
        if (x < 0) {
            _template_parameter_box->setBreakLines(true);
            x = right - _template_parameter_box->boundingRect().width() * 0.8;
        }
        if (x < 0) {
            x = 0;
        }
        _template_parameter_box->setPos(x, top - _template_parameter_box->boundingRect().height() + BODY_VERT_BORDER);
    }

    updateSelectionMarkerGeometry(rect);

    if (_relation_starter) {
        _relation_starter->setPos(mapToScene(QPointF(right + 8.0, top)));
    }

    updateAlignmentButtonsGeometry(rect);

    updateDepth();

}

void ClassItem::updateMembers(const Style *style)
{
    Q_UNUSED(style)

    _attributes_text.clear();
    _methods_text.clear();

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
            text = &_attributes_text;
            break;
        case MClassMember::MEMBER_METHOD:
            current_visibility = &methods_visibility;
            current_group = &methods_group;
            text = &_methods_text;
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
