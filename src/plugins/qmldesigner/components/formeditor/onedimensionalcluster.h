// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QList>

namespace QmlDesigner {

class OneDimensionalCluster
{
    friend inline OneDimensionalCluster operator+(const OneDimensionalCluster & firstCluster, const OneDimensionalCluster & secondCluster);
    friend inline bool operator < (const OneDimensionalCluster & firstCluster, const OneDimensionalCluster & secondCluster);
public:
    static QList<double> reduceLines(const QList<double> & oneDimensionalCoordinateList, double maximumDistance);

private:

    OneDimensionalCluster(const QList<double> & coordinateList );
    double mean() const;
    double constFirst() const;

    static QList<OneDimensionalCluster> createOneDimensionalClusterList(const QList<double> & oneDimensionalCoordinateList);
    static QList<OneDimensionalCluster> reduceOneDimensionalClusterList(const QList<OneDimensionalCluster> & unreducedClusterList, double maximumDistance);

    QList<double> m_coordinateList;
};

inline bool operator < (const OneDimensionalCluster & firstCluster, const OneDimensionalCluster & secondCluster)
{
    return firstCluster.mean() < secondCluster.mean();
}

inline OneDimensionalCluster operator+(const OneDimensionalCluster & firstCluster, const OneDimensionalCluster & secondCluster)
{
    return OneDimensionalCluster(firstCluster.m_coordinateList + secondCluster.m_coordinateList);
}

} // namespace QmlDesigner
