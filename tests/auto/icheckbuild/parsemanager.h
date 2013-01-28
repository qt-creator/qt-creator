/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

/*
** Description:
**
** The ParseManager parses and compares to different header files
** of its metadata. This can be used for checking if an Interface
** is implemented complete.
**
** How to use it:
**
**    //Parse the interface header
**    ParseManager* iParseManager = new ParseManager();
**    iParseManager->setIncludePath(iIncludepathlist);
**    iParseManager->parse(iFilelist);
**
**    //Parse the header that needs to be compared against the interface header
**    ParseManager* chParseManager = new ParseManager();
**    chIncludepathlist << getQTIncludePath();
*    chParseManager->setIncludePath(chIncludepathlist);
**    chParseManager->parse(chFilelist);
**
**    if(!chParseManager->checkAllMetadatas(iParseManager)){
**        cout << "Following interface items are missing:" << endl;
**        QStringList errorlist = chParseManager->getErrorMsg();
**        foreach(QString msg, errorlist){
**            cout << (const char *)msg.toLatin1() << endl;
**        }
**        return -1;
**    }
**    else
**        cout << "Interface is full defined.";
*/

#ifndef PARSEMANAGER_H
#define PARSEMANAGER_H

#include "cplusplus/CppDocument.h"
#include "ASTfwd.h"
#include "FullySpecifiedType.h"

#include <QObject>
#include <QList>
#include <QFuture>
#include <QStringList>
#include <QFile>

namespace CppTools{
    namespace Internal{
        class CppPreprocessor;
    }
}

namespace CPlusPlus {
    class CLASSLISTITEM
    {
    public:
        CPlusPlus::TranslationUnit* trlUnit;
        ClassSpecifierAST* classspec;
    };
    class CLASSTREE
    {
    public:
        CLASSLISTITEM* highestlevelclass;
        QList<CLASSLISTITEM*> classlist;
    };
    class FUNCTIONITEM
    {
    public:
        const CLASSLISTITEM* highestlevelclass;
        CPlusPlus::TranslationUnit* trlUnit;
        ClassSpecifierAST* classAst;
        QStringList classWichIsNotFound;
        CPlusPlus::Function* function;

        bool isEqualTo(FUNCTIONITEM* cpfct, bool ignoreName = true);

        FUNCTIONITEM()
        {
            highestlevelclass = 0;
            trlUnit = 0;
            classAst = 0;
            function = 0;
        }
    };
    class PROPERTYITEM
    {
    public:
        const CLASSLISTITEM* highestlevelclass;
        QStringList classWichIsNotFound;
        QtPropertyDeclarationAST *ast;
        CPlusPlus::TranslationUnit* trlUnit;
        FullySpecifiedType type;
        ExpressionAST *readAst;
        FUNCTIONITEM *readFct;
        ExpressionAST *writeAst;
        FUNCTIONITEM *writeFct;
        ExpressionAST *resetAst;
        FUNCTIONITEM *resetFct;
        ExpressionAST *notifyAst;
        FUNCTIONITEM *notifyFct;
        bool foundalldefinedfct;

        bool isEqualTo(PROPERTYITEM* cpppt);
        PROPERTYITEM()
        {
            highestlevelclass = 0;
            ast = 0;
            trlUnit = 0;
            readAst = 0;
            readFct = 0;
            writeAst = 0;
            writeFct = 0;
            resetAst = 0;
            resetFct = 0;
            notifyAst = 0;
            notifyFct = 0;
            foundalldefinedfct = false;
        }

        static PROPERTYITEM *create(QtPropertyDeclarationAST *ast, const CLASSLISTITEM *clazz);
    };

    class QENUMITEM
    {
    public:
        const CLASSLISTITEM* highestlevelclass;
        QStringList classWichIsNotFound;
        QString name;
        //an item in this list will be shown like:
        //EnumName.EnumItemName.Value
        //ConnectionState.disconnected.0
        QStringList values;
        bool foundallenums;

        bool isEqualTo(QENUMITEM *cpenum);
        QENUMITEM()
        {
            highestlevelclass = 0;
            values.clear();
            foundallenums = true;
        }
    };

    class ENUMITEM
    {
    public:
        const CLASSLISTITEM* highestlevelclass;
        CPlusPlus::TranslationUnit* trlUnit;
        QStringList classWichIsNotFound;
        EnumSpecifierAST* ast;

        ENUMITEM()
        {
            highestlevelclass = 0;
            trlUnit = 0;
            ast = 0;
        }
    };

    class QFLAGITEM
    {
    public:
        const CLASSLISTITEM* highestlevelclass;
        const Name *name;
        QStringList classWichIsNotFound;
        QStringList enumvalues;
        bool foundallenums;

        bool isEqualTo(QFLAGITEM *cpflag);
        QFLAGITEM()
        {
            highestlevelclass = 0;
            enumvalues.clear();
            foundallenums = true;
        }
    };

    class QDECLAREFLAGSITEM
    {
    public:
        const CLASSLISTITEM* highestlevelclass;
        CPlusPlus::TranslationUnit* trlUnit;
        QStringList classWichIsNotFound;
        QtFlagsDeclarationAST* ast;

        QDECLAREFLAGSITEM()
        {
            highestlevelclass = 0;
            trlUnit = 0;
            ast = 0;
        }
    };

    static QFile* m_resultFile = 0;
    class ParseManager : public QObject
    {
        Q_OBJECT
    public:
        ParseManager();
        virtual ~ParseManager();
        void setIncludePath(const QStringList &includePath);
        void parse(const QStringList &sourceFiles);
        bool checkAllMetadatas(ParseManager* pInterfaceParserManager, QString resultfile);
        CppTools::Internal::CppPreprocessor *getPreProcessor() { return pCppPreprocessor; }
        QList<CLASSTREE*> CreateClassLists(bool isInterfaceHeader);
        QStringList getErrorMsg() { return m_errormsgs; }

    private:
        void parse(CppTools::Internal::CppPreprocessor *preproc, const QStringList &files);
        void Trace(QString value);
        inline QString getTraceFuntionString(const FUNCTIONITEM* fctitem, const QString& classname);
        void getBaseClasses(const CLASSLISTITEM* pclass
                , QList<CLASSLISTITEM*> &baseclasslist
                , const QList<CLASSLISTITEM*> &allclasslist
                , int level
                , bool isInterfaceHeader);
        void getElements(QList<FUNCTIONITEM*> &functionlist
            , QList<PROPERTYITEM*> &propertylist
            , QList<QENUMITEM*> &qenumlist
            , QList<ENUMITEM*> &enumlist
            , QList<QFLAGITEM*> &qflaglist
            , QList<QDECLAREFLAGSITEM*> &qdeclareflaglist
            , const QList<CLASSLISTITEM*> classitems
            , const CLASSLISTITEM* highestlevelclass);

        //<--- for Metadata functions checks
        QList<FUNCTIONITEM*> checkMetadataFunctions(const QList<QList<FUNCTIONITEM*> > &classfctlist, const QList<QList<FUNCTIONITEM*> > &iclassfctlist);
        bool isMetaObjFunction(FUNCTIONITEM* fct);
        QList<FUNCTIONITEM*> containsAllMetadataFunction(const QList<FUNCTIONITEM*> &classfctlist, const QList<FUNCTIONITEM*> &iclassfctlist);
        QStringList getErrorMessage(FUNCTIONITEM* fct);
        //--->

        //<--- for Q_PROPERTY functions checks
        QList<PROPERTYITEM*> checkMetadataProperties(const QList<QList<PROPERTYITEM*> > &classproplist
            , const QList<QList<FUNCTIONITEM*> > &classfctlist
            , const QList<QList<PROPERTYITEM*> > &iclassproplist
            , const QList<QList<FUNCTIONITEM*> > &iclassfctlist);
        void assignPropertyFunctions(PROPERTYITEM* prop, const QList<QList<FUNCTIONITEM*> > &fctlookuplist);
        QList<PROPERTYITEM*> containsAllPropertyFunction(const QList<PROPERTYITEM*> &classproplist, const QList<PROPERTYITEM*> &iclassproplist);
        QStringList getErrorMessage(PROPERTYITEM* ppt);
        //--->

        //<--- for Q_ENUMS checks
        QList<QENUMITEM*> checkMetadataEnums(const QList<QList<QENUMITEM*> > &classqenumlist
            , const QList<QList<ENUMITEM*> > &classenumlist
            , const QList<QList<QENUMITEM*> > &iclassqenumlist
            , const QList<QList<ENUMITEM*> > &iclassenumlist);
        QStringList getEnumValueStringList(ENUMITEM *penum, QString mappedenumname = "");
        void assignEnumValues(QENUMITEM* qenum, const QList<QList<ENUMITEM*> > &enumlookuplist);
        QList<QENUMITEM*> containsAllEnums(const QList<QENUMITEM*> &classqenumlist, const QList<QENUMITEM*> &iclassqenumlist);
        QStringList getErrorMessage(QENUMITEM* qenum);
        //--->

        //<--- for QFlags checks --->
        QList<QFLAGITEM*> checkMetadataFlags(const QList<QList<QFLAGITEM*> > &classqflaglist
            , const QList<QList<QDECLAREFLAGSITEM*> > &classqdeclareflaglist
            , const QList<QList<ENUMITEM*> > &classenumlist
            , const QList<QList<QFLAGITEM*> > &iclassqflaglist
            , const QList<QList<QDECLAREFLAGSITEM*> > &iclassqdeclareflaglist
            , const QList<QList<ENUMITEM*> > &iclassenumlist);
        void assignFlagValues(QFLAGITEM* qflags, const QList<QList<QDECLAREFLAGSITEM*> > &qdeclareflagslookuplist, const QList<QList<ENUMITEM*> > &enumlookuplist);
        QList<QFLAGITEM*> containsAllFlags(const QList<QFLAGITEM*> &classqflaglist, const QList<QFLAGITEM*> &iclassqflaglist);
        QStringList getErrorMessage(QFLAGITEM* pfg);
        //--->

    private:
        // cache
        QStringList m_includePaths;
        QStringList m_frameworkPaths;
        QByteArray m_definedMacros;
        CppTools::Internal::CppPreprocessor* pCppPreprocessor;
        QString m_strHeaderFile;
        QStringList m_errormsgs;
    };
}
#endif // PARSEMANAGER_H
