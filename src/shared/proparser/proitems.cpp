/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

void ProItem::disposeItems(ProItem *nitm)
{
    for (ProItem *itm; (itm = nitm); ) {
        nitm = itm->next();
        switch (itm->kind()) {
        case ProItem::ConditionKind: delete static_cast<ProCondition *>(itm); break;
        case ProItem::VariableKind: delete static_cast<ProVariable *>(itm); break;
        case ProItem::BranchKind: delete static_cast<ProBranch *>(itm); break;
        case ProItem::LoopKind: delete static_cast<ProLoop *>(itm); break;
        case ProItem::FunctionDefKind: static_cast<ProFunctionDef *>(itm)->deref(); break;
        case ProItem::OpNotKind:
        case ProItem::OpAndKind:
        case ProItem::OpOrKind:
            delete itm;
            break;
        }
    }
}

ProBranch::~ProBranch()
{
    disposeItems(m_thenItems);
    disposeItems(m_elseItems);
}

ProLoop::~ProLoop()
{
    disposeItems(m_proitems);
}

ProFunctionDef::~ProFunctionDef()
{
    disposeItems(m_proitems);
}

ProFile::ProFile(const QString &fileName)
    : m_proitems(0),
      m_refCount(1),
      m_fileName(fileName)
{
    int nameOff = fileName.lastIndexOf(QLatin1Char('/'));
    m_displayFileName = QString(fileName.constData() + nameOff + 1,
                                fileName.length() - nameOff - 1);
    m_directoryName = QString(fileName.constData(), nameOff);
}

ProFile::~ProFile()
{
    ProItem::disposeItems(m_proitems);
}

QT_END_NAMESPACE
