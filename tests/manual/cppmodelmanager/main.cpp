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
#include <QtCore>

#include "cppcodemodel.h"
#include "preprocessor.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QFile f("C:\\depot\\research\\main\\cppparser\\rpp\\pp-qt-configuration");
    f.open(QIODevice::ReadOnly);
    CppCodeModel model(f.readAll());
    f.close();

    model.addIncludePath("C:\\depot\\qt\\4.1\\include\\");
    model.addIncludePath("C:\\depot\\qt\\4.1\\include\\QtCore");
    model.addIncludePath("C:\\depot\\qt\\4.1\\include\\QtGui");

/*    model.addIncludePath("C:\\depot\\research\\main\\qworkbench\\tests\\manual\\cppmodelmanager\\tests"); */
    model.update(QStringList() << "qwidget.h");

//    return app.exec();
    return 0;

/*    Preprocessor pp;
    pp.addIncludePaths(QStringList() << "C:/depot/qt/4.1/include/QtCore/");
    pp.processFile("C:/depot/research/main/cppparser/rpp/pp-qt-configuration");
    QString ppstuff = pp.processFile("test.h");

    
    Control control;
    Parser p(&control);
    pool __pool;

    QByteArray byteArray = ppstuff.toUtf8();
    TranslationUnitAST *ast = p.parse(byteArray, byteArray.size(), &__pool);
    qDebug() << p.problemCount();

    CodeModel model;
    Binder binder(&model, p.location());
    FileModelItem fileModel = binder.run(ast);
    
    qDebug() << "Count: " << model.files().count(); */
    
/*    DBCodeModel db;
    db.open("c:/bin/test.cdb"); */


}
