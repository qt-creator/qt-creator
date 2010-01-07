/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef COLORWIDGET_H
#define COLORWIDGET_H

#include <QWeakPointer>
#include <QtGui/QWidget>
#include <QLabel>
#include <modelnode.h>
#include <qml.h>
#include <propertyeditorvalue.h>


class QtColorButton;
class QToolButton;
class QtGradientDialog;

namespace QmlDesigner {

class ColorWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QColor strColor READ strColor)
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QmlDesigner::ModelNode gradient READ gradientNode WRITE setGradientNode NOTIFY gradientNodeChanged)
    Q_PROPERTY(bool showGradientButton READ gradientButtonShown WRITE setShowGradientButton)
    Q_PROPERTY(QmlDesigner::ModelNode modelNode READ modelNode WRITE setModelNode NOTIFY modelNodeChanged)
    Q_PROPERTY(PropertyEditorNodeWrapper* complexGradientNode READ complexGradientNode WRITE setComplexGradientNode)
public:

    ColorWidget(QWidget *parent = 0);
    ~ColorWidget();

    QColor color() const;
    QString text() const;
    QString strColor() const;
    void setText(const QString &text);
    ModelNode gradientNode() const;
    bool gradientButtonShown() const;
    void setShowGradientButton(bool showButton);

    void setModelNode(const ModelNode &modelNode);
    ModelNode modelNode() const;

    PropertyEditorNodeWrapper* complexGradientNode() const;
    void setComplexGradientNode(PropertyEditorNodeWrapper* complexNode);

public slots:
    void setColor(const QColor &color);
    void setGradientNode(const QmlDesigner::ModelNode &gradient);

signals:
    void colorChanged(const QColor &color);
    void gradientNodeChanged();
    void modelStateChanged();
    void modelNodeChanged();

protected:
    QGradient gradient() const;

private slots:
    void openGradientEditor(bool show);
    void updateGradientNode();
    void resetGradientButton();
private:
    QColor m_color;
    ModelNode m_gradientNode;
    ModelNode m_modelNode;
    QWeakPointer<QtGradientDialog> m_gradientDialog;
    QLabel *m_label;
    QtColorButton *m_colorButton;
    QToolButton *m_gradientButton;
    PropertyEditorNodeWrapper* m_complexGradientNode;
};

}

QML_DECLARE_TYPE(QmlDesigner::ColorWidget);

#endif
