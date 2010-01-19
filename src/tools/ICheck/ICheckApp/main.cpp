#include "ichecklib.h"
#include <QtCore/QCoreApplication>
#include <QString>
#include <QStringList>
#include <iostream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

using namespace std;

QStringList getQTIncludePath()
{
    QStringList ret;
    QStringList processevironment = QProcess::systemEnvironment();
    foreach(QString item, processevironment){
        if(item.indexOf("QTDIR=") == 0){
            QString qtpath = item.remove("QTDIR=");
            ret << qtpath + "\\include\\QtCore";
            break;
        }
    }
    return ret;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    if(argc == 3){
        try{
            //Extract the file name and path from the arguments
            QString interfaceHeaderFile = argv[1];
            QString checkHeaderFile = argv[2];
            QString curpath = QDir::currentPath();
            //Create FileInfos for the header files
            QFile ifile(interfaceHeaderFile);
            if (!ifile.exists()){
                QString err = "File does not exist: " + interfaceHeaderFile;
                throw err;
            }
            QFile chfile(checkHeaderFile);
            if (!chfile.exists()){
                QString err = "File does not exist: " + checkHeaderFile;
                throw err;
            }
            QFileInfo iFileInfo(ifile);
            QFileInfo chFileInfo(chfile);

            //Now create a list of the include path 
            QString chIncludepath = chFileInfo.absolutePath();
            QStringList chIncludepathlist;
            chIncludepathlist << chIncludepath;
            chIncludepathlist << getQTIncludePath();

            QString iIncludepath = iFileInfo.absolutePath();
            QStringList iIncludepathlist;
            iIncludepathlist << iIncludepath;

            //Create a list of all the soucre files they need to be parsed.
            //In our case it is just the header file
            QStringList chFilelist;
            chFilelist << chFileInfo.filePath();

            QStringList iFilelist;
            iFilelist << iFileInfo.filePath();

            ICheckLib i_ichecklib;
            i_ichecklib.ParseHeader(iIncludepathlist, iFilelist);

            ICheckLib ichecklib;
            ichecklib.ParseHeader(chIncludepathlist, chFilelist);

            if(!ichecklib.check(i_ichecklib)){
                cout << "Folowing interface items are missing:" << endl;
                QStringList errorlist = ichecklib.getErrorMsg();
                foreach(QString msg, errorlist){
                    cout << (const char *)msg.toLatin1() << endl;
                }
                ret = -1;
            }
            else
                cout << "Interface is full defined.";

/*
            //Parse the interface header
            CPlusPlus::ParseManager* iParseManager = new CPlusPlus::ParseManager();
            iParseManager->setIncludePath(iIncludepathlist);
            iParseManager->parse(iFilelist);

            //Parse the header that needs to be compared against the interface header
            CPlusPlus::ParseManager* chParseManager = new CPlusPlus::ParseManager();
            chIncludepathlist << getQTIncludePath();
            chParseManager->setIncludePath(chIncludepathlist);
            chParseManager->parse(chFilelist);
            
            if(!chParseManager->checkAllMetadatas(iParseManager)){
                cout << "Folowing interface items are missing:" << endl;
                QStringList errorlist = chParseManager->getErrorMsg();
                foreach(QString msg, errorlist){
                    cout << (const char *)msg.toLatin1() << endl;
                }
                ret = -1;
            }
            else
                cout << "Interface is full defined.";

            delete iParseManager;
            delete chParseManager;
            */
        }
        catch (QString msg)
        {
            cout << endl << "Error:" << endl;
            cout << (const char *)msg.toLatin1();
            ret = -2;
        }
    }
    else{
        cout << "CompareHeaderWithHeader.exe";
        cout << " \"Interface header\"";
        cout << " \"headerfile to check\"";
    }

    qDebug() << endl << endl << "return value: " << ret;
    getchar();
    return ret;
}
