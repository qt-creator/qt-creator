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

#ifndef MODELUTILITIES_H
#define MODELUTILITIES_H

#include "corelib_global.h"
#include <modelnode.h>
#include <nodeinstance.h>

namespace QmlDesigner {
namespace ModelUtilities {

CORESHARED_EXPORT bool canReparent(const ModelNode &child, const ModelNode &parent);
CORESHARED_EXPORT bool isGraphicsItem(const ModelNode &node);
CORESHARED_EXPORT bool isQWidget(const ModelNode &node);
CORESHARED_EXPORT QVariant parseProperty(const QString &className, const QString &propertyName, const QString &value);
CORESHARED_EXPORT void setAbsolutePosition(ModelNode node, QPointF position);
CORESHARED_EXPORT QList<ModelNode> descendantNodes(const ModelNode &parent);

CORESHARED_EXPORT NodeInstance instanceForNode(const ModelNode &node);
}  //namespace ModelUtilities
}  //namespace QmlDesigner

#endif //MODELUTILITIES_H
