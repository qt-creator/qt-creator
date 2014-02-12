/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "qmlprofilerbasemodel.h"
#include "qmlprofilermodelmanager.h"

namespace QmlProfiler {

QmlProfilerBaseModel::QmlProfilerBaseModel(QmlProfilerModelManager *manager) :
    m_modelManager(manager), m_processingDone(false)
{
    Q_ASSERT(m_modelManager);
    m_modelId = m_modelManager->registerModelProxy();
}

void QmlProfilerBaseModel::clear()
{
    m_modelManager->modelProxyCountUpdated(m_modelId, 0, 1);
    m_processingDone = false;
    emit changed();
}

void QmlProfilerBaseModel::complete()
{
    emit changed();
    m_processingDone = true;
    m_modelManager->modelProxyCountUpdated(m_modelId, isEmpty() ? 0 : 1, 1);
    m_modelManager->modelProcessingDone();
}

}
