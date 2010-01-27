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
**    chParseManager->setIncludePath(chIncludepathlist);
**    chParseManager->parse(chFilelist);
**    
**    if(!chParseManager->checkAllMetadatas(iParseManager)){
**        cout << "Folowing interface items are missing:" << endl;
**        QStringList errorlist = chParseManager->getErrorMsg();
**        foreach(QString msg, errorlist){
**            cout << (const char *)msg.toLatin1() << endl;
**        }
**        return -1;
**    }
**    else
**        cout << "Interface is full defined.";
**
**************************************************************************/


#ifndef PARSEMANAGER_H
#define PARSEMANAGER_H

#include <QObject>
#include <QList>
#include <QFuture>
#include <QStringList>
#include "cplusplus/CppDocument.h"

namespace CppTools{
    namespace Internal{
        class CppPreprocessor;
    }
}

namespace CPlusPlus {
    class TranslationUnit;
    class AST;
    class ClassSpecifierAST;
    class SimpleDeclarationAST;
    class QPropertyDeclarationAST;
    class QEnumDeclarationAST;
    class QFlagsDeclarationAST;
    class QDeclareFlagsDeclarationAST;
    class EnumSpecifierAST;
    class Function;

    class ParseManager : public QObject
    {
        Q_OBJECT
        struct CLASSLISTITEM
        {
            CPlusPlus::TranslationUnit* trlUnit;
            ClassSpecifierAST* classspec;
        };
        struct CLASSTREE
        {
            CLASSLISTITEM* highestlevelclass;
            QList<CLASSLISTITEM*> classlist;
        };
        struct FUNCTIONITEM
        {
            const CLASSLISTITEM* highestlevelclass;
            CPlusPlus::TranslationUnit* trlUnit;
            ClassSpecifierAST* classAst;
            QStringList classWichIsNotFound;
            CPlusPlus::Function* function;
            SimpleDeclarationAST* ast;

            bool isEqualTo(FUNCTIONITEM* cpfct, bool ignoreName = true);
            void init()
            {
                highestlevelclass = 0;
                trlUnit = 0;
                classAst = 0;
                function = 0;
                ast = 0;
            }
        };
        struct PROPERTYITEM
        {
            const CLASSLISTITEM* highestlevelclass;
            QStringList classWichIsNotFound;
            QPropertyDeclarationAST *ast;
            CPlusPlus::TranslationUnit* trlUnit;
            bool readdefined;
            FUNCTIONITEM *readFct;
            bool writedefined;
            FUNCTIONITEM *writeFct;
            bool resetdefined;
            FUNCTIONITEM *resetFct;
            bool notifydefined;
            FUNCTIONITEM *notifyFct;
            bool foundalldefinedfct;

            bool isEqualTo(PROPERTYITEM* cpppt);
            void init()
            {
                highestlevelclass = 0;
                ast = 0;
                trlUnit = 0;
                readdefined = false;
                readFct = 0;
                writedefined = false;
                writeFct = 0;
                resetdefined = false;
                resetFct = 0;
                notifydefined = false;
                notifyFct = 0;
                foundalldefinedfct = false;
            }
        };

        struct QENUMITEM
        {
            const CLASSLISTITEM* highestlevelclass;
            CPlusPlus::TranslationUnit* trlUnit;
            QStringList classWichIsNotFound;
            QEnumDeclarationAST* ast;
            //an item in this list will be shown like:
            //EnumName.EnumItemName.Value
            //ConnectionState.disconnected.0
            QStringList values;
            bool foundallenums;

            bool isEqualTo(QENUMITEM *cpenum);
            void init()
            {
                highestlevelclass = 0;
                trlUnit = 0;
                ast = 0;
                values.clear();
                foundallenums = true;
            }
        };

        struct ENUMITEM
        {
            const CLASSLISTITEM* highestlevelclass;
            CPlusPlus::TranslationUnit* trlUnit;
            QStringList classWichIsNotFound;
            EnumSpecifierAST* ast;

            void init()
            {
                highestlevelclass = 0;
                trlUnit = 0;
                ast = 0;
            }
        };

        struct QFLAGITEM
        {
            const CLASSLISTITEM* highestlevelclass;
            CPlusPlus::TranslationUnit* trlUnit;
            QStringList classWichIsNotFound;
            QFlagsDeclarationAST* ast;
            QStringList enumvalues;
            bool foundallenums;

            bool isEqualTo(QFLAGITEM *cpflag);
            void init()
            {
                highestlevelclass = 0;
                trlUnit = 0;
                ast = 0;
                enumvalues.clear();
                foundallenums = true;
            }
        };

        struct QDECLAREFLAGSITEM
        {
            const CLASSLISTITEM* highestlevelclass;
            CPlusPlus::TranslationUnit* trlUnit;
            QStringList classWichIsNotFound;
            QDeclareFlagsDeclarationAST* ast;

            void init()
            {
                highestlevelclass = 0;
                trlUnit = 0;
                ast = 0;
            }
        };


    public:
        ParseManager();
        virtual ~ParseManager();
        void setIncludePath(const QStringList &includePath);
        void parse(const QStringList &sourceFiles);
        bool checkAllMetadatas(ParseManager* pInterfaceParserManager);
        CppTools::Internal::CppPreprocessor *getPreProcessor() { return pCppPreprocessor; }
        QList<CLASSTREE*> CreateClassLists();
        QStringList getErrorMsg() { return m_errormsgs; }

    private:
        void parse(CppTools::Internal::CppPreprocessor *preproc, const QStringList &files);
        void getBaseClasses(const CLASSLISTITEM* pclass, QList<CLASSLISTITEM*> &baseclasslist, const QList<CLASSLISTITEM*> &allclasslist);
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
        QString getErrorMessage(FUNCTIONITEM* fct);
        //--->

        //<--- for Q_PROPERTY functions checks
        QList<PROPERTYITEM*> checkMetadataProperties(const QList<QList<PROPERTYITEM*> > &classproplist
            , const QList<QList<FUNCTIONITEM*> > &classfctlist
            , const QList<QList<PROPERTYITEM*> > &iclassproplist
            , const QList<QList<FUNCTIONITEM*> > &iclassfctlist);
        void assignPropertyFunctions(PROPERTYITEM* prop, const QList<QList<FUNCTIONITEM*> > &fctlookuplist);
        QList<PROPERTYITEM*> containsAllPropertyFunction(const QList<PROPERTYITEM*> &classproplist, const QList<PROPERTYITEM*> &iclassproplist);
        QString getErrorMessage(PROPERTYITEM* ppt);
        //--->

        //<--- for Q_ENUMS checks
        QList<QENUMITEM*> checkMetadataEnums(const QList<QList<QENUMITEM*> > &classqenumlist
            , const QList<QList<ENUMITEM*> > &classenumlist
            , const QList<QList<QENUMITEM*> > &iclassqenumlist
            , const QList<QList<ENUMITEM*> > &iclassenumlist);
        QStringList getEnumValueStringList(ENUMITEM *penum, QString mappedenumname = "");
        void assignEnumValues(QENUMITEM* qenum, const QList<QList<ENUMITEM*> > &enumlookuplist);
        QList<QENUMITEM*> containsAllEnums(const QList<QENUMITEM*> &classqenumlist, const QList<QENUMITEM*> &iclassqenumlist);
        QString getErrorMessage(QENUMITEM* qenum);
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
        QString getErrorMessage(QFLAGITEM* pfg);
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
