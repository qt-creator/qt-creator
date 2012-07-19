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

#ifndef ONEDIMENSIONALCLUSTER_H
#define ONEDIMENSIONALCLUSTER_H

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
    double first() const;



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

}
#endif // ONEDIMENSIONALCLUSTER_H
