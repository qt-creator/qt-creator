/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef COMPONENTACTION_H
#define COMPONENTACTION_H

#include <QWidgetAction>
#include <QWeakPointer>

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
    void setCurrentIndex(int);

protected:
    QWidget  *createWidget(QWidget *parent);

signals:
    void currentComponentChanged(const ModelNode &node);
    void currentIndexChanged(int);

private slots:
    void emitCurrentComponentChanged(int index);

private:
    QWeakPointer<ComponentView> m_componentView;
};

} // namespace QmlDesigner

#endif // COMPONENTACTION_H
