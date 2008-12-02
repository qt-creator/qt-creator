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
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QDebug>

#include "cppcodemodelitems.h"
#include "cpppartparser.h"
#include "cppcodemodel.h"
#include "cppcodemodelpart.h"

CppCodeModel::CppCodeModel(const QByteArray &configuration, QObject *parent)
    : QObject(parent)
{
    m_configuration = configuration;
    m_parsefiles << QLatin1String("<configfile>");
}

CppCodeModel::~CppCodeModel()
{
    
}

void CppCodeModel::addIncludePath(const QString &path)
{
    QString newpath = QDir::cleanPath(path);
    if (m_includedirs.contains(newpath))
        return;

    m_includedirs.insert(newpath, new CppCodeModelPart(newpath, this));
}

void CppCodeModel::removeIncludePath(const QString &path)
{
    QString newpath = QDir::cleanPath(path + QLatin1Char('/'));
    if (!m_includedirs.contains(newpath))
        return;

    delete m_includedirs.take(newpath);
}

void CppCodeModel::update(const QStringList &files)
{
    m_parsefiles += m_parsefiles.fromList(files);

    CppPartParser *parser = CppPartParser::instance(parent());
    parser->parse(this);
}

QStringList CppCodeModel::needsParsing()
{
    QStringList result = m_parsefiles.toList();

    if (m_parsefiles.contains(QLatin1String("<configfile>")))
        result.prepend(QLatin1String("<configfile>"));

    m_parsefiles.clear();
    return result;
}

void CppCodeModel::resolvePart(const QString &abspath, CppCodeModelPart **part) const
{
    int length = 0;

    QMap<QString, CppCodeModelPart *>::const_iterator i = m_includedirs.constBegin();
    while (i != m_includedirs.constEnd()) {
        if (abspath.startsWith(i.key()) && i.key().count() > length) {
            length = i.key().count();
            (*part) = i.value();
        }
        ++i;
    }
}

void CppCodeModel::resolveGlobalPath(QString &file, CppCodeModelPart **part) const
{
    QString abspath;

    (*part) = 0;
    QMap<QString, CppCodeModelPart *>::const_iterator i = m_includedirs.constBegin();
    while (i != m_includedirs.constEnd()) {
        abspath = i.key() + QLatin1Char('/') + file;
        QFileInfo fi(abspath);
        if (fi.exists() && fi.isFile()) {
            (*part) = i.value();
            break;
        }
        ++i;
    }

    if (*part)
        file = QDir::cleanPath(abspath);
}

void CppCodeModel::resolveLocalPath(QString &file, const QString &local, CppCodeModelPart **part) const
{
    (*part) = m_partcache.value(local, 0);
    QFileInfo fi(local);
    file = QDir::cleanPath(fi.absolutePath() + QLatin1Char('/') + file);
}

QByteArray *CppCodeModel::contents(QString &file, const QString &local)
{
    CppCodeModelPart *part = 0;

    if (file == QLatin1String("<configfile>"))
        return new QByteArray(m_configuration);

    if (local.isEmpty()) {
        resolveGlobalPath(file, &part);
        if (!m_partcache.contains(file)) {
            resolvePart(file, &part);
            m_partcache.insert(file, part);
        } else {
            part = m_partcache.value(file, 0);
        }
    } else {
        resolveLocalPath(file, local, &part);
        m_partcache.insert(file, part);
    }

    if (!part) {
        qDebug() << "Didn't find: " << file;
        return 0;
    }

    return part->contents(file);
}

QHash<QString, pp_macro*> *CppCodeModel::macros()
{
    return &m_macros;
}

void CppCodeModel::store()
{
    QMap<QString, CppFileModelItem *>::const_iterator i = m_fileitems.constBegin();
    while (i != m_fileitems.constEnd()) {
        if (CppCodeModelPart *part = m_partcache.value(i.key()))
            part->store(i.value());
        ++i;
    }
    
    m_partcache.clear();
    m_fileitems.clear();
}

CppFileModelItem *CppCodeModel::fileItem(const QString &name)
{
    if (!m_partcache.contains(name))
        return 0;

    if (m_fileitems.contains(name))
        return m_fileitems.value(name);

    CppFileModelItem *item = new CppFileModelItem(this);
    item->setPart(m_partcache.value(name));
    item->setName(name);
    item->setFile(item);
    m_fileitems.insert(name, item);
    return item;
}

bool CppCodeModel::hasScope(const QString &name) const
{
    QMap<QString, CppCodeModelPart *>::const_iterator i = m_includedirs.constBegin();
    while (i != m_includedirs.constEnd()) {
        if (i.value()->hasScope(name))
            return true;
        ++i;
    }
    return false;
}

