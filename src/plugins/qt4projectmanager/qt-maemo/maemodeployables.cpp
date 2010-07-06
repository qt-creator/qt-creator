/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "maemodeployables.h"

#include "maemodeployablelistmodel.h"

namespace Qt4ProjectManager {
namespace Internal {

MaemoDeployables::MaemoDeployables(MaemoPackageCreationStep *packagingStep)
{
    // TODO: Enumerate sub projects and create one list model for each app or lib template.
    // Their constructurs will then take this object as parent and the
    // project node
    m_listModels << new MaemoDeployableListModel(packagingStep);
}

void MaemoDeployables::setUnmodified()
{
    foreach (MaemoDeployableListModel *model, m_listModels)
        model->setUnModified();
}

bool MaemoDeployables::isModified() const
{
    foreach (const MaemoDeployableListModel *model, m_listModels) {
        if (model->isModified())
            return true;
    }
    return false;
}

int MaemoDeployables::deployableCount() const
{
    int count = 0;
    foreach (const MaemoDeployableListModel *model, m_listModels)
        count += model->rowCount();
    return count;
}

MaemoDeployable MaemoDeployables::deployableAt(int i) const
{
    foreach (const MaemoDeployableListModel *model, m_listModels) {
        Q_ASSERT(i >= 0);
        if (i < model->rowCount())
            return model->deployableAt(i);
        i -= model->rowCount();
    }

    Q_ASSERT(!"Invalid deployable number");
    return MaemoDeployable(QString(), QString());
}

QString MaemoDeployables::remoteExecutableFilePath(const QString &localExecutableFilePath) const
{
    foreach (const MaemoDeployableListModel *model, m_listModels) {
        if (model->localExecutableFilePath() == localExecutableFilePath)
            return model->remoteExecutableFilePath();
    }
    Q_ASSERT(!"Invalid local executable!");
    return QString();
}

} // namespace Qt4ProjectManager
} // namespace Internal
