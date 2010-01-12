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
