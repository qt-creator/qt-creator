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
***************************************************************************/
#include <QtCore/QFile>

#include "dbcodemodel.h"

DBCodeModel::DBCodeModel(QObject *parent) 
    : QObject(parent)
{
                
}

DBCodeModel::~DBCodeModel()
{
    
}

bool DBCodeModel::open(const QString &fileName)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE", fileName);
    
    if (!QFile::exists(fileName)) {
        m_db.setDatabaseName(fileName);
        if (!m_db.open() || !create())
            return false;
    } else {
        m_db.setDatabaseName(fileName);
        if (!m_db.open()) // || !update(fileName))
            return false;
    }
    
    return true;
}

bool DBCodeModel::create()
{
    QSqlQuery query(m_db);

    // table to store type information
    query.exec("CREATE TABLE TYPEINFO ("
               "id         INTEGER PRIMARY KEY, "
               "typename   TEXT, "
               "typeflags  INTEGER, "
               ")");

    // table to store position information
    query.exec("CREATE TABLE POSITION ("
               "id         INTEGER PRIMARY KEY, "
               "sline      INTEGER, "
               "scolumn    INTEGER, "
               "eline      INTEGER, "
               "ecolumn    INTEGER"
               ")");
    
    // table to store files (global namespace), namespaces, unions, structs and classes
    query.exec("CREATE TABLE SCOPE ("
               "id         INTEGER PRIMARY KEY, "
               "scopetype  INTEGER, "
               "name       TEXT, "
               "parent     INTEGER, "
               "posid      INTEGER, "
               "fileid     INTEGER"
               ")");
    
    // table to store scope member information
    // a scope member is a enum, function, variable or typealias
    query.exec("CREATE TABLE SCOPEMEMBER ("
               "id         INTEGER PRIMARY KEY, "
               "membertype INTEGER, "
               "name       TEXT, "
               "scopeid    INTEGER, "
               "flags      INTEGER, "
               "typeid     INTEGER, "
               "posid      INTEGER, "
               "fileid     INTEGER"
               ")");    
    
    // table to store arguments
    // used if the membertype is a function
    query.exec("CREATE TABLE ARGUMENT ("
               "name       TEXT, "
               "default    TEXT, "
               "argnr      INTEGER, "
               "typeid     INTEGER, "
               "memberid   INTEGER"
               ")");
    
    // table to store enumerators
    // used if the membertype is an enum
    query.exec("CREATE TABLE ENUMERATOR ("
               "name       TEXT, "
               "value      INTEGER, "
               "memberid   INTEGER"
               ")");    

    // table to store arguments to types
    // used if typeflags indicates that it has arguments (i.e. function pointers)
    query.exec("CREATE TABLE TYPEARGUMENT ("
               "parentid   INTEGER, "
               "argnr      INTEGER, "
               "typeid     INTEGER"
               ")");
    
    // table to store the class hierarchy
    query.exec("CREATE TABLE CLASSHIERARCHY ("
               "scopeid  INTEGER, "
               "basename   TEXT"
               ")");
    
    // table to store all the modified timestamps used
    // for updating the database
    query.exec("CREATE TABLE MODIFIED ("
               "fileid     INTEGER, "
               "modified   INTEGER)");
    
    return true;
}
