/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QmlAnchorBindingProxy_h
#define QmlAnchorBindingProxy_h

#include <QObject>
#include <modelnode.h>
#include <nodeinstanceview.h>
#include <QRectF>
#include <qmlitemnode.h>

namespace QmlDesigner {

class NodeInstanceView;

namespace Internal {

class QmlAnchorBindingProxy : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool topAnchored READ topAnchored WRITE setTopAnchor NOTIFY topAnchorChanged)
    Q_PROPERTY(bool bottomAnchored READ bottomAnchored WRITE setBottomAnchor NOTIFY bottomAnchorChanged)
    Q_PROPERTY(bool leftAnchored READ leftAnchored WRITE setLeftAnchor NOTIFY leftAnchorChanged)
    Q_PROPERTY(bool rightAnchored READ rightAnchored WRITE setRightAnchor NOTIFY rightAnchorChanged)
    Q_PROPERTY(bool hasParent READ hasParent NOTIFY parentChanged)

    Q_PROPERTY(QVariant topTarget READ topTarget WRITE setTopTarget NOTIFY topTargetChanged)
    Q_PROPERTY(QVariant bottomTarget READ bottomTarget WRITE setBottomTarget NOTIFY bottomTargetChanged)
    Q_PROPERTY(QVariant leftTarget READ leftTarget WRITE setLeftTarget NOTIFY leftTargetChanged)
    Q_PROPERTY(QVariant rightTarget READ rightTarget WRITE setRightTarget NOTIFY rightTargetChanged)

    Q_PROPERTY(QVariant verticalTarget READ verticalTarget WRITE setVerticalTarget NOTIFY verticalTargetChanged)
    Q_PROPERTY(QVariant horizontalTarget READ horizontalTarget WRITE setHorizontalTarget NOTIFY horizontalTargetChanged)

    Q_PROPERTY(bool hasAnchors READ hasAnchors NOTIFY anchorsChanged)
    Q_PROPERTY(bool isFilled READ isFilled NOTIFY anchorsChanged)

    Q_PROPERTY(bool horizontalCentered READ horizontalCentered WRITE setHorizontalCentered NOTIFY centeredHChanged)
    Q_PROPERTY(bool verticalCentered READ verticalCentered WRITE setVerticalCentered NOTIFY centeredVChanged)
    Q_PROPERTY(QVariant itemNode READ itemNode NOTIFY itemNodeChanged)

public:
    //only enable if node has parent

    QmlAnchorBindingProxy(QObject *parent = 0);
    ~QmlAnchorBindingProxy();

    void setup(const QmlItemNode &itemNode);
    void invalidate(const QmlItemNode &itemNode);

    bool topAnchored();
    bool bottomAnchored();
    bool leftAnchored();
    bool rightAnchored();

    bool hasParent();
    bool isFilled();

    void removeTopAnchor();
    void removeBottomAnchor();
    void removeLeftAnchor();
    void removeRightAnchor();
    bool hasAnchors();

    bool horizontalCentered();
    bool verticalCentered();
    QVariant itemNode() const { return QVariant::fromValue(m_fxItemNode.modelNode()); }

    QVariant topTarget() const { return QVariant::fromValue(m_topTarget.modelNode()); }
    QVariant bottomTarget() const { return QVariant::fromValue(m_bottomTarget.modelNode()); }
    QVariant leftTarget() const { return QVariant::fromValue(m_leftTarget.modelNode()); }
    QVariant rightTarget() const { return QVariant::fromValue(m_rightTarget.modelNode()); }

    QVariant verticalTarget() const { return QVariant::fromValue(m_verticalTarget.modelNode()); }
    QVariant horizontalTarget() const { return QVariant::fromValue(m_horizontalTarget.modelNode()); }

public:
    void setTopTarget(const QVariant &target);
    void setBottomTarget(const QVariant &target);
    void setLeftTarget(const QVariant &target);
    void setRightTarget(const QVariant &target);
    void setVerticalTarget(const QVariant &target);
    void setHorizontalTarget(const QVariant &target);


public slots:
    void resetLayout();
    void fill();
    void setTopAnchor(bool anchor =true);
    void setBottomAnchor(bool anchor = true);
    void setLeftAnchor(bool anchor = true);
    void setRightAnchor(bool anchor = true);

    void setVerticalCentered(bool centered = true);
    void setHorizontalCentered(bool centered = true);

signals:
    void parentChanged();

    void topAnchorChanged();
    void bottomAnchorChanged();
    void leftAnchorChanged();
    void rightAnchorChanged();
    void centeredVChanged();
    void centeredHChanged();
    void anchorsChanged();
    void itemNodeChanged();

    void topTargetChanged();
    void bottomTargetChanged();
    void leftTargetChanged();
    void rightTargetChanged();

    void verticalTargetChanged();
    void horizontalTargetChanged();

private:

    void calcTopMargin();
    void calcBottomMargin();
    void calcLeftMargin();
    void calcRightMargin();

    ModelNode modelNode() const;

    QmlItemNode m_fxItemNode;

    QRectF parentBoundingBox();

    QRectF boundingBox(QmlItemNode node);

    QRectF transformedBoundingBox();

    QmlItemNode m_topTarget;
    QmlItemNode m_bottomTarget;
    QmlItemNode m_leftTarget;
    QmlItemNode m_rightTarget;

    QmlItemNode m_verticalTarget;
    QmlItemNode m_horizontalTarget;

    bool m_locked;
};

} // namespace Internal
} // namespace QmlDesigner


#endif //QmlAnchorBindingProxy

