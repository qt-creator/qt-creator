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
    Q_PROPERTY(bool isInLayout READ isInLayout NOTIFY parentChanged)

    Q_PROPERTY(QString topTarget READ topTarget WRITE setTopTarget NOTIFY topTargetChanged)
    Q_PROPERTY(QString bottomTarget READ bottomTarget WRITE setBottomTarget NOTIFY bottomTargetChanged)
    Q_PROPERTY(QString leftTarget READ leftTarget WRITE setLeftTarget NOTIFY leftTargetChanged)
    Q_PROPERTY(QString rightTarget READ rightTarget WRITE setRightTarget NOTIFY rightTargetChanged)

    Q_PROPERTY(RelativeAnchorTarget relativeAnchorTargetTop READ relativeAnchorTargetTop WRITE setRelativeAnchorTargetTop NOTIFY relativeAnchorTargetTopChanged)
    Q_PROPERTY(RelativeAnchorTarget relativeAnchorTargetBottom READ relativeAnchorTargetBottom WRITE setRelativeAnchorTargetBottom NOTIFY relativeAnchorTargetBottomChanged)
    Q_PROPERTY(RelativeAnchorTarget relativeAnchorTargetLeft READ relativeAnchorTargetLeft WRITE setRelativeAnchorTargetLeft NOTIFY relativeAnchorTargetLeftChanged)
    Q_PROPERTY(RelativeAnchorTarget relativeAnchorTargetRight READ relativeAnchorTargetRight WRITE setRelativeAnchorTargetRight NOTIFY relativeAnchorTargetRightChanged)

    Q_PROPERTY(RelativeAnchorTarget relativeAnchorTargetVertical READ relativeAnchorTargetVertical WRITE setRelativeAnchorTargetVertical NOTIFY relativeAnchorTargetVerticalChanged)
    Q_PROPERTY(RelativeAnchorTarget relativeAnchorTargetHorizontal READ relativeAnchorTargetHorizontal WRITE setRelativeAnchorTargetHorizontal NOTIFY relativeAnchorTargetHorizontalChanged)

    Q_PROPERTY(QString verticalTarget READ verticalTarget WRITE setVerticalTarget NOTIFY verticalTargetChanged)
    Q_PROPERTY(QString horizontalTarget READ horizontalTarget WRITE setHorizontalTarget NOTIFY horizontalTargetChanged)

    Q_PROPERTY(bool hasAnchors READ hasAnchors NOTIFY anchorsChanged)
    Q_PROPERTY(bool isFilled READ isFilled NOTIFY anchorsChanged)

    Q_PROPERTY(bool horizontalCentered READ horizontalCentered WRITE setHorizontalCentered NOTIFY centeredHChanged)
    Q_PROPERTY(bool verticalCentered READ verticalCentered WRITE setVerticalCentered NOTIFY centeredVChanged)
    Q_PROPERTY(QVariant itemNode READ itemNode NOTIFY itemNodeChanged)

    Q_PROPERTY(QStringList possibleTargetItems READ possibleTargetItems NOTIFY itemNodeChanged)

    Q_ENUMS(RelativeAnchorTarget)

public:
    enum RelativeAnchorTarget {
        SameEdge = 0,
        Center = 1,
        OppositeEdge = 2
    };

    //only enable if node has parent

    QmlAnchorBindingProxy(QObject *parent = 0);
    ~QmlAnchorBindingProxy();

    void setup(const QmlItemNode &itemNode);
    void invalidate(const QmlItemNode &itemNode);

    bool topAnchored() const;
    bool bottomAnchored() const;
    bool leftAnchored() const;
    bool rightAnchored() const;

    bool hasParent() const;
    bool isFilled() const;

    bool isInLayout() const;

    void removeTopAnchor();
    void removeBottomAnchor();
    void removeLeftAnchor();
    void removeRightAnchor();
    bool hasAnchors() const;

    bool horizontalCentered();
    bool verticalCentered();
    QVariant itemNode() const { return QVariant::fromValue(m_qmlItemNode.modelNode().id()); }

    QString topTarget() const;
    QString bottomTarget() const;
    QString leftTarget() const;
    QString rightTarget() const;

    RelativeAnchorTarget relativeAnchorTargetTop() const;
    RelativeAnchorTarget relativeAnchorTargetBottom() const;
    RelativeAnchorTarget relativeAnchorTargetLeft() const;
    RelativeAnchorTarget relativeAnchorTargetRight() const;

    RelativeAnchorTarget relativeAnchorTargetVertical() const;
    RelativeAnchorTarget relativeAnchorTargetHorizontal() const;

    QString verticalTarget() const;
    QString horizontalTarget() const;

    QmlItemNode getItemNode() const { return m_qmlItemNode; }

public:
    void setTopTarget(const QString &target);
    void setBottomTarget(const QString &target);
    void setLeftTarget(const QString &target);
    void setRightTarget(const QString &target);
    void setVerticalTarget(const QString &target);
    void setHorizontalTarget(const QString &target);

    void setRelativeAnchorTargetTop(RelativeAnchorTarget target);
    void setRelativeAnchorTargetBottom(RelativeAnchorTarget target);
    void setRelativeAnchorTargetLeft(RelativeAnchorTarget target);
    void setRelativeAnchorTargetRight(RelativeAnchorTarget target);

    void setRelativeAnchorTargetVertical(RelativeAnchorTarget target);
    void setRelativeAnchorTargetHorizontal(RelativeAnchorTarget target);

    QStringList possibleTargetItems() const;
    Q_INVOKABLE int indexOfPossibleTargetItem(const QString &targetName) const;

    static void registerDeclarativeType();

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

    void relativeAnchorTargetTopChanged();
    void relativeAnchorTargetBottomChanged();
    void relativeAnchorTargetLeftChanged();
    void relativeAnchorTargetRightChanged();

    void relativeAnchorTargetVerticalChanged();
    void relativeAnchorTargetHorizontalChanged();

    void invalidated();

private:
    void setDefaultAnchorTarget(const ModelNode &modelNode);
    void anchorTop();
    void anchorBottom();
    void anchorLeft();
    void anchorRight();

    void anchorVertical();
    void anchorHorizontal();

    void setupAnchorTargets();
    void emitAnchorSignals();

    void setDefaultRelativeTopTarget();
    void setDefaultRelativeBottomTarget();
    void setDefaultRelativeLeftTarget();
    void setDefaultRelativeRightTarget();

    RewriterTransaction beginRewriterTransaction(const QByteArray &identifier);

    QmlItemNode targetIdToNode(const QString &id) const;
    QString idForNode(const QmlItemNode &qmlItemNode) const;

    ModelNode modelNode() const;

    QmlItemNode m_qmlItemNode;

    QRectF parentBoundingBox();

    QRectF boundingBox(const QmlItemNode &node);

    QRectF transformedBoundingBox();

    QmlItemNode m_topTarget;
    QmlItemNode m_bottomTarget;
    QmlItemNode m_leftTarget;
    QmlItemNode m_rightTarget;

    QmlItemNode m_verticalTarget;
    QmlItemNode m_horizontalTarget;

    RelativeAnchorTarget m_relativeTopTarget;
    RelativeAnchorTarget m_relativeBottomTarget;
    RelativeAnchorTarget m_relativeLeftTarget;
    RelativeAnchorTarget m_relativeRightTarget;

    RelativeAnchorTarget m_relativeVerticalTarget;
    RelativeAnchorTarget m_relativeHorizontalTarget;

    bool m_locked;
    bool m_ignoreQml;
};

} // namespace Internal
} // namespace QmlDesigner


#endif //QmlAnchorBindingProxy

