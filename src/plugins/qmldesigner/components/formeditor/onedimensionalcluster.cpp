/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
        return m_coordinateList.first();
    }

    return sum(m_coordinateList) / m_coordinateList.size();
}

double OneDimensionalCluster::first() const
{
    Q_ASSERT(!m_coordinateList.isEmpty());

    return m_coordinateList.first();
}

QList<OneDimensionalCluster> OneDimensionalCluster::createOneDimensionalClusterList(const QList<double> & oneDimensionalCoordinateList)
{
    QList<OneDimensionalCluster> oneDimensionalClusterList;
    foreach (double coordinate, oneDimensionalCoordinateList)
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
        QListIterator<OneDimensionalCluster> clusterIterator(workingList);
        while (clusterIterator.hasNext())
        {
            OneDimensionalCluster currentCluster = clusterIterator.next();
            if (clusterIterator.hasNext())
            {
                OneDimensionalCluster nextCluster = clusterIterator.peekNext();
                if ((nextCluster.mean() - currentCluster.mean()) < maximumDistance)
                {
                    reducedList.append(currentCluster + nextCluster);
                    clusterIterator.next();
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
    foreach (const OneDimensionalCluster &cluster, clusterList)
        lineList.append(cluster.first());

    return lineList;
}

}
