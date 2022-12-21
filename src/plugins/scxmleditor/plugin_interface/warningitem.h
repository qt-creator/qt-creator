// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "baseitem.h"
#include "warning.h"
#include "warningmodel.h"
#include <QGraphicsObject>
#include <QGraphicsSceneMouseEvent>
#include <QPixmap>
#include <QPointer>

namespace ScxmlEditor {

namespace PluginInterface {

/**
 * @brief The WarningItem class is a base class for the all scene-based Warning-items.
 *
 * When status changes, warning must inform it to scene. Scene will check the other warnings too.
 */
class WarningItem : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit WarningItem(QGraphicsItem *parent = nullptr);
    ~WarningItem() override;

    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QRectF boundingRect() const override;
    ScxmlTag *tag() const;

    void setSeverity(OutputPane::Warning::Severity type);
    void setTypeName(const QString &name);
    void setDescription(const QString &description);
    OutputPane::Warning *warning() const;

    void setReason(const QString &reason);
    void setPixmap(const QPixmap &pixmap);
    void setWarningActive(bool active);
    virtual void check();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *e) override;

private:
    void checkVisibility();

    OutputPane::Warning::Severity m_severity = OutputPane::Warning::ErrorType;
    QString m_typeName;
    QString m_description;
    QString m_reason;
    QPixmap m_pixmap;
    BaseItem *m_parentItem;
    QPointer<OutputPane::Warning> m_warning;
    QPointer<OutputPane::WarningModel> m_warningModel;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
