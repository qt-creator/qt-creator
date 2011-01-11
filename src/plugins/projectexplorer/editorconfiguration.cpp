/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "editorconfiguration.h"

#include <QtCore/QTextCodec>

using namespace ProjectExplorer;

namespace {
const char * const CODEC("EditorConfiguration.Codec");
}

EditorConfiguration::EditorConfiguration()
    : m_defaultTextCodec(0)
{
}

QTextCodec *EditorConfiguration::defaultTextCodec() const
{
    return m_defaultTextCodec;
}

void EditorConfiguration::setDefaultTextCodec(QTextCodec *codec)
{
    m_defaultTextCodec = codec;
}

QVariantMap EditorConfiguration::toMap() const
{
    QVariantMap map;
    QByteArray name = "Default";
    if (m_defaultTextCodec)
        name = m_defaultTextCodec->name();
    map.insert(QLatin1String(CODEC), name);
    return map;
}

void EditorConfiguration::fromMap(const QVariantMap &map)
{
    QByteArray name = map.value(QLatin1String(CODEC)).toString().toLocal8Bit();
    QTextCodec *codec = QTextCodec::codecForName(name);
    m_defaultTextCodec = codec;
}
