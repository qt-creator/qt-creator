/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef COMPONENTACTION_H
#define COMPONENTACTION_H

#include <QWidgetAction>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace QmlDesigner {

class Model;
class ComponentView;
class ModelNode;

class ComponentAction : public QWidgetAction
{
    Q_OBJECT
public:
    ComponentAction(ComponentView  *componentView);
    void setCurrentIndex(int index);


protected:
    QWidget  *createWidget(QWidget *parent);

signals:
    void currentComponentChanged(const ModelNode &node);
    void changedToMaster();
    void currentIndexChanged(int index);

public slots:
    void emitCurrentComponentChanged(int index);

private:
    QPointer<ComponentView> m_componentView;
    bool dontEmitCurrentComponentChanged;
};

} // namespace QmlDesigner

#endif // COMPONENTACTION_H
