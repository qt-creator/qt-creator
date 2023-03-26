// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
