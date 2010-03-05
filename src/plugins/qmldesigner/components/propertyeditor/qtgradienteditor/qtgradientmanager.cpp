/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "qtgradientmanager.h"
#include <QtGui/QPixmap>
#include <QtCore/QMetaEnum>

QT_BEGIN_NAMESPACE

QtGradientManager::QtGradientManager(QObject *parent)
    : QObject(parent)
{
}

QMap<QString, QGradient> QtGradientManager::gradients() const
{
    return m_idToGradient;
}

QString QtGradientManager::uniqueId(const QString &id) const
{
    if (!m_idToGradient.contains(id))
        return id;

    QString base = id;
    while (base.count() > 0 && base.at(base.count() - 1).isDigit())
        base = base.left(base.count() - 1);
    QString newId = base;
    int counter = 0;
    while (m_idToGradient.contains(newId)) {
        ++counter;
        newId = base + QString::number(counter);
    }
    return newId;
}

QString QtGradientManager::addGradient(const QString &id, const QGradient &gradient)
{
    QString newId = uniqueId(id);

    m_idToGradient[newId] = gradient;

    emit gradientAdded(newId, gradient);

    return newId;
}

void QtGradientManager::removeGradient(const QString &id)
{
    if (!m_idToGradient.contains(id))
        return;

    emit gradientRemoved(id);

    m_idToGradient.remove(id);
}

void QtGradientManager::renameGradient(const QString &id, const QString &newId)
{
    if (!m_idToGradient.contains(id))
        return;

    if (newId == id)
        return;

    QString changedId = uniqueId(newId);
    QGradient gradient = m_idToGradient.value(id);

    emit gradientRenamed(id, changedId);

    m_idToGradient.remove(id);
    m_idToGradient[changedId] = gradient;
}

void QtGradientManager::changeGradient(const QString &id, const QGradient &newGradient)
{
    if (!m_idToGradient.contains(id))
        return;

    if (m_idToGradient.value(id) == newGradient)
        return;

    emit gradientChanged(id, newGradient);

    m_idToGradient[id] = newGradient;
}

void QtGradientManager::clear()
{
    QMap<QString, QGradient> grads = gradients();
    QMapIterator<QString, QGradient> itGrad(grads);
    while (itGrad.hasNext()) {
        removeGradient(itGrad.next().key());
    }
}

QT_END_NAMESPACE
