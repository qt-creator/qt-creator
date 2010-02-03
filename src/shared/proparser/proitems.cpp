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

#include "proitems.h"

#include <QtCore/QFileInfo>

QT_BEGIN_NAMESPACE

ProBlock::ProBlock()
    : ProItem(BlockKind)
{
    m_proitems = 0;
    m_blockKind = 0;
    m_refCount = 1;
}

ProBlock::~ProBlock()
{
    for (ProItem *itm, *nitm = m_proitems; (itm = nitm); ) {
        nitm = itm->m_next;
        if (itm->kind() == BlockKind)
            static_cast<ProBlock *>(itm)->deref();
        else
            delete itm;
    }
}

ProFile::ProFile(const QString &fileName)
    : ProBlock()
{
    setBlockKind(ProBlock::ProFileKind);
    m_fileName = fileName;

    // If the full name does not outlive the parts, things will go boom ...
    int nameOff = fileName.lastIndexOf(QLatin1Char('/'));
    m_displayFileName = QString::fromRawData(fileName.constData() + nameOff + 1,
                                             fileName.length() - nameOff - 1);
    m_directoryName = QString::fromRawData(fileName.constData(), nameOff);
}

QT_END_NAMESPACE
