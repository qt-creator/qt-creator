#include "ichecklib.h"
#include "ParseManager.h"
#include <QtCore/QCoreApplication>
#include <QString>
#include <QStringList>
#include <iostream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

QStringList getQTIncludePath()
{
    QStringList ret;
    QStringList processevironment = QProcess::systemEnvironment();
    foreach(QString item, processevironment){
        if(item.indexOf("QTDIR=") == 0){
            QString qtpath = item.remove("QTDIR=");
            ret << qtpath + "/include/QtCore";
            break;
        }
    }
    return ret;
}

ICheckLib::ICheckLib()
: pParseManager(0)
{
}

void ICheckLib::ParseHeader(const QStringList& includePath, const QStringList& filelist)
{
    if(pParseManager)
        delete pParseManager;
    pParseManager = 0;
    pParseManager = new CPlusPlus::ParseManager();
    pParseManager->setIncludePath(includePath);
    pParseManager->parse(filelist);
}

bool ICheckLib::check(const ICheckLib& ichecklib /*ICheckLib from interface header*/)
{
    if(pParseManager){
        CPlusPlus::ParseManager* cpparsemanager = ichecklib.pParseManager;
        return pParseManager->checkAllMetadatas(cpparsemanager);
    }
    return false;
}

QStringList ICheckLib::getErrorMsg()
{
    QStringList ret;
    if(pParseManager){
        ret.append(pParseManager->getErrorMsg());
    }
    else
        ret << "no file was parsed.";
    return ret;
}
