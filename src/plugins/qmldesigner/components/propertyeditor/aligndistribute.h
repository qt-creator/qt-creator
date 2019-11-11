/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#ifndef ALIGNDISTRIBUTE_H
#define ALIGNDISTRIBUTE_H

#include <QtQml>

#include <selectioncontext.h>
#include <qmlitemnode.h>

namespace QmlDesigner {

class AlignDistribute : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool multiSelection READ multiSelection NOTIFY modelNodeBackendChanged)
    Q_PROPERTY(bool selectionHasAnchors READ selectionHasAnchors NOTIFY modelNodeBackendChanged)
    Q_PROPERTY(
        bool selectionExclusivlyItems READ selectionExclusivlyItems NOTIFY modelNodeBackendChanged)
    Q_PROPERTY(bool selectionContainsRootItem READ selectionContainsRootItem NOTIFY
                   modelNodeBackendChanged)

    Q_PROPERTY(QVariant modelNodeBackendProperty READ modelNodeBackend WRITE setModelNodeBackend
                   NOTIFY modelNodeBackendChanged)

public:
    explicit AlignDistribute(QObject *parent = nullptr);

    enum AlignTo { Selection, Root, KeyObject };
    Q_ENUM(AlignTo)

    enum DistributeOrigin { None, TopLeft, Center, BottomRight };
    Q_ENUM(DistributeOrigin)

    enum Dimension { X, Y };
    Q_ENUM(Dimension)

    enum Target { Left, CenterH, Right, Top, CenterV, Bottom };
    Q_ENUM(Target)

    bool multiSelection() const;
    bool selectionHasAnchors() const;
    bool selectionExclusivlyItems() const;
    bool selectionContainsRootItem() const;

    void setModelNodeBackend(const QVariant &modelNodeBackend);

    static void registerDeclarativeType();

    Q_INVOKABLE void alignObjects(Target target, AlignTo alignTo, const QString &keyObject);
    Q_INVOKABLE void distributeObjects(Target target, AlignTo alignTo, const QString &keyObject);
    Q_INVOKABLE void distributeSpacing(Dimension dimension,
                                       AlignTo alignTo,
                                       const QString &keyObject,
                                       const qreal &distance,
                                       DistributeOrigin distributeOrigin);

signals:
    void modelNodeBackendChanged();

private:
    QVariant modelNodeBackend() const;

private:
    using CompareFunction = std::function<bool(const ModelNode &, const ModelNode &)>;

    CompareFunction getCompareFunction(Target target) const;
    Dimension getDimension(Target target) const;
    bool executePixelPerfectDialog() const;

    QmlObjectNode m_qmlObjectNode;
};

} // namespace QmlDesigner

QML_DECLARE_TYPE(QmlDesigner::AlignDistribute)

#endif //ALIGNDISTRIBUTE_H
