/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmleditorwidgets_global.h"
#include <QFrame>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QToolButton;
class QVariant;
class QGraphicsDropShadowEffect;
class QGraphicsOpacityEffect;
QT_END_NAMESPACE

namespace QmlJS { class PropertyReader; }

namespace QmlEditorWidgets {

class CustomColorDialog;
class ContextPaneTextWidget;
class EasingContextPane;
class ContextPaneWidgetRectangle;
class ContextPaneWidgetImage;

class QMLEDITORWIDGETS_EXPORT DragWidget : public QFrame
{
    Q_OBJECT

public:
    explicit DragWidget(QWidget *parent = 0);
    void setSecondaryTarget(QWidget* w)
    { m_secondaryTarget = w; }

protected:
    QPoint m_pos;
    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);
    void virtual protectedMoved();
    void leaveEvent(QEvent *);
    void enterEvent(QEvent *);

private:
    QGraphicsDropShadowEffect *m_dropShadowEffect;
    QGraphicsOpacityEffect *m_opacityEffect;
    QPoint m_startPos;
    QPointer<QWidget> m_secondaryTarget;
};

class QMLEDITORWIDGETS_EXPORT ContextPaneWidget : public DragWidget
{
    Q_OBJECT

public:
    explicit ContextPaneWidget(QWidget *parent = 0);
    ~ContextPaneWidget();
    void activate(const QPoint &pos, const QPoint &alternative, const QPoint &alternative2, bool pinned);
    void rePosition(const QPoint &pos, const QPoint &alternative , const QPoint &alternative3, bool pinned);
    void deactivate();
    void setOptions(bool enabled, bool pinned);
    CustomColorDialog *colorDialog();
    void setProperties(QmlJS::PropertyReader *propertyReader);
    void setPath(const QString &path);
    bool setType(const QStringList &types);
    bool acceptsType(const QStringList &types);
    QWidget* currentWidget() const { return m_currentWidget; }
    void setIsPropertyChanges(bool b)
    { m_isPropertyChanges = b; }
    bool isPropertyChanges() const
    { return m_isPropertyChanges; }

    void onTogglePane();
    void onShowColorDialog(bool, const QPoint &);

signals:
    void propertyChanged(const QString &, const QVariant &);
    void removeProperty(const QString &);
    void removeAndChangeProperty(const QString &, const QString &, const QVariant &, bool);
    void pinnedChanged(bool);
    void enabledChanged(bool);
    void closed();

private:
    void onDisable(bool);
    void onResetPosition(bool toggle);

protected:
    void protectedMoved();

    QToolButton *m_toolButton;
    QWidget *createFontWidget();
    QWidget *createEasingWidget();
    QWidget *createImageWidget();
    QWidget *createBorderImageWidget();
    QWidget *createRectangleWidget();

    void setPinButton();
    void setLineButton();

private:
    QWidget *m_currentWidget;
    ContextPaneTextWidget *m_textWidget;
    EasingContextPane *m_easingWidget;
    ContextPaneWidgetImage *m_imageWidget;
    ContextPaneWidgetImage *m_borderImageWidget;
    ContextPaneWidgetRectangle *m_rectangleWidget;
    QPointer<CustomColorDialog> m_bauhausColorDialog;
    QPointer<QAction> m_resetAction;
    QPointer<QAction> m_disableAction;
    QString m_colorName;
    QPoint m_originalPos;
    bool m_pinned;
    bool m_isPropertyChanges;
};

} //QmlDesigner
