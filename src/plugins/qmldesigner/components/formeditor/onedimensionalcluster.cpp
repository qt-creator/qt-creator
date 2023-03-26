// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "onedimensionalcluster.h"

#include <utils/algorithm.h>

#include <QDebug>

namespace QmlDesigner {

double sum(const QList<double> & list)
{
    double sum = 0.0;
    for (auto iterator = list.constBegin(); iterator != list.constEnd(); ++iterator)
    {
        sum += *iterator;
    }

    return sum;
}

OneDimensionalCluster::OneDimensionalCluster(const QList<double> & coordinateList )
        : m_coordinateList(coordinateList)
{
}

double OneDimensionalCluster::mean() const
{
    Q_ASSERT(!m_coordinateList.isEmpty());

    if (m_coordinateList.size() == 1)
    {
        return m_coordinateList.constFirst();
    }

    return sum(m_coordinateList) / m_coordinateList.size();
}

double OneDimensionalCluster::constFirst() const
{
    Q_ASSERT(!m_coordinateList.isEmpty());

    return m_coordinateList.constFirst();
}

QList<OneDimensionalCluster> OneDimensionalCluster::createOneDimensionalClusterList(const QList<double> & oneDimensionalCoordinateList)
{
    QList<OneDimensionalCluster> oneDimensionalClusterList;
    for (double coordinate : oneDimensionalCoordinateList)
    {
        QList<double> initialList;
        initialList.append(coordinate);
        OneDimensionalCluster cluster(initialList);
        oneDimensionalClusterList.append(initialList);
    }

    return oneDimensionalClusterList;
}

QList<OneDimensionalCluster> OneDimensionalCluster::reduceOneDimensionalClusterList(const QList<OneDimensionalCluster> & unreducedClusterList, double maximumDistance)
{
    if (unreducedClusterList.size() < 2)
        return unreducedClusterList;


    QList<OneDimensionalCluster> workingList(unreducedClusterList);
    QList<OneDimensionalCluster> reducedList;
    while (true)
    {
        Utils::sort(workingList);
        reducedList.clear();
        bool clusterMerged = false;

        for (int i = 0, n = workingList.size(); i != n; ) {

            OneDimensionalCluster currentCluster = workingList.at(i);
            if (i + 1 < n) {
                OneDimensionalCluster nextCluster = workingList.at(i + 1);
                if ((nextCluster.mean() - currentCluster.mean()) < maximumDistance)
                {
                    reducedList.append(currentCluster + nextCluster);
                    ++i;
                    clusterMerged = true;
                }
                else
                {
                    reducedList.append(currentCluster);
                }
            }
            else
            {
                reducedList.append(currentCluster);
                break;
            }

        }

        workingList = reducedList;

        if (clusterMerged == false)
            break;
    }


    return reducedList;
}

QList<double> OneDimensionalCluster::reduceLines(const QList<double> & oneDimensionalCoordinateList, double maximumDistance)
{
    QList<OneDimensionalCluster>  clusterList(createOneDimensionalClusterList(oneDimensionalCoordinateList));
    clusterList = reduceOneDimensionalClusterList(clusterList, maximumDistance);

    QList<double> lineList;
    for (const OneDimensionalCluster &cluster : std::as_const(clusterList))
        lineList.append(cluster.constFirst());

    return lineList;
}

}
