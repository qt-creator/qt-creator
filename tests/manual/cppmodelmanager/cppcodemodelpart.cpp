/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QFileInfoList>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

#include "cppcodemodelpart.h"
#include "cpppartparser.h"
#include "cppcodemodelitems.h"

CppCodeModelPart::CppCodeModelPart(const QString &path, QObject *parent)
    : QObject(parent)
{
    m_path = path;
}

CppCodeModelPart::~CppCodeModelPart()
{

}

QString CppCodeModelPart::path() const
{
    return m_path;
}

void CppCodeModelPart::update()
{

}

QByteArray *CppCodeModelPart::contents(const QString &file)
{
    QByteArray *result = new QByteArray();
    if (!m_files.contains(file)) {
        m_files.insert(file);
        QFile f(file);
        if (!f.open(QIODevice::ReadOnly))
            return 0;
        (*result) = f.readAll();
        f.close();
    }

    return result;
}

void CppCodeModelPart::store(CppFileModelItem *item)
{
    qDebug() << "Deleting: " << item->name();
    delete item;
}

bool CppCodeModelPart::hasScope(const QString &name) const
{
    // ### implement me
    return true;
}
