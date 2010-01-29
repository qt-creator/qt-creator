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

#include "ParseManager.h"
#include "cplusplus/CppDocument.h"
#include "Control.h"
#include "TranslationUnit.h"
#include "AST.h"
#include "Symbols.h"
#include <QDebug>
#include "Name.h"
#include "cpptools/cppmodelmanager.h"

using namespace CppTools;
using namespace CppTools::Internal;


using namespace CPlusPlus;

//<------------------------------------------------------- Compare function for the internal structures
/**********************************
Compares function with function
with return type, function name
and their arguments and arguments
types.
**********************************/
bool FUNCTIONITEM::isEqualTo(FUNCTIONITEM *cpfct, bool ignoreName/* = true*/)
{
    if(ignoreName)
        return function->isEqualTo(cpfct->function, true);
    return function->isEqualTo(cpfct->function);
}

/*****************************************************************
Compares two property regarding
of their function definition,
type definition, function arguments
and function types.

Q_PROPERTY( ConnectionState state READ state NOTIFY stateChanged);
******************************************************************/
bool PROPERTYITEM::isEqualTo(PROPERTYITEM *cpppt)
{
    QString thistype = this->trlUnit->spell(this->ast->type_token);
    QString cppttype = cpppt->trlUnit->spell(cpppt->ast->type_token);

    if(thistype != cppttype)
        return false;

    QString thistypename = this->trlUnit->spell(this->ast->type_name_token);
    QString cppttypename = cpppt->trlUnit->spell(cpppt->ast->type_name_token);
    if(thistypename != cppttypename)
        return false;

    if(this->readdefined != cpppt->readdefined)
        return false;
    if(this->writedefined != cpppt->writedefined)
        return false;
    if(this->resetdefined != cpppt->resetdefined)
        return false;
    if(this->notifydefined != cpppt->notifydefined)
        return false;
    //check for read function
    if(this->readdefined){
        if(!this->readFct || !cpppt->readFct)
            return false;
        if(!this->readFct->isEqualTo(cpppt->readFct))
            return false;
    }
    //check for write function
    if(this->writedefined){
        if(!this->writeFct || !cpppt->writeFct)
            return false;
        if(!this->writeFct->isEqualTo(cpppt->writeFct))
            return false;
    }
    //check for reset function
    if(this->resetdefined){
        if(!this->resetFct || !cpppt->resetFct)
            return false;
        if(!this->resetFct->isEqualTo(cpppt->resetFct))
            return false;
    }
    //check for notify function
    if(this->notifydefined){
        if(!this->notifyFct || !cpppt->notifyFct)
            return false;
        if(!this->notifyFct->isEqualTo(cpppt->notifyFct))
            return false;
    }
    return true;
}

/*****************************************************************
Compares two enums regarding
of their values created by the getEnumValueStringList function.
*****************************************************************/
bool QENUMITEM::isEqualTo(QENUMITEM *cpenum)
{
    if(this->values.count() != cpenum->values.count())
        return false;
    foreach(QString str, this->values){
        if(!cpenum->values.contains(str))
            return false;
    }
    return true;
}

/*****************************************************************
Compares two flags regarding
of their enum definitions and their
values created by the getEnumValueStringList function.
*****************************************************************/
bool QFLAGITEM::isEqualTo(QFLAGITEM *cpflag)
{
    if(this->enumvalues.count() != cpflag->enumvalues.count())
        return false;
    foreach(QString str, this->enumvalues){
        if(!cpflag->enumvalues.contains(str))
            return false;
    }
    return true;
}



ParseManager::ParseManager()
: pCppPreprocessor(0)
{

}

ParseManager::~ParseManager()
{
    if(pCppPreprocessor)
        delete pCppPreprocessor;
}

/**************************************
Function for setting the include
Paths
**************************************/
void ParseManager::setIncludePath(const QStringList &includePath)
{
    m_includePaths = includePath;
}

/**************************************
public Function that starts the parsing
all of the files in the sourceFiles
string list.
**************************************/
void ParseManager::parse(const QStringList &sourceFiles)
{
    m_errormsgs.clear();
    if(pCppPreprocessor){
        delete pCppPreprocessor;
        pCppPreprocessor = 0;
    }

    if (! sourceFiles.isEmpty()) {
        m_strHeaderFile = sourceFiles[0];
        pCppPreprocessor = new CppTools::Internal::CppPreprocessor(QPointer<CPlusPlus::ParseManager>(this));
        pCppPreprocessor->setIncludePaths(m_includePaths);
        pCppPreprocessor->setFrameworkPaths(m_frameworkPaths);
        parse(pCppPreprocessor, sourceFiles);
    }
}

/*********************************************
private function that prepare the filelist
to parse and starts the parser.
*********************************************/
void ParseManager::parse(CppTools::Internal::CppPreprocessor *preproc,
                            const QStringList &files)
{
    if (files.isEmpty())
        return;

    //check if file is C++ header file
    QStringList headers;
    foreach (const QString &file, files) {
        const QFileInfo fileInfo(file);
        QString ext = fileInfo.suffix();
        if (ext.toLower() == "h")
            headers.append(file);
    }

    foreach (const QString &file, files) {
        preproc->snapshot.remove(file);
    }
    preproc->setTodo(headers);
    QString conf = QLatin1String("<configuration>");

    preproc->run(conf);
    for (int i = 0; i < headers.size(); ++i) {
        QString fileName = headers.at(i);
        preproc->run(fileName);
    }
}

//This function creates a class list for each class and its base classes in
//the header file that needs to be checked.
//e.g.
//      Cl1          Cl2
//     __|__        __|__
//    |     |      |     |
//   Cl11  Cl12   Cl21  Cl22
//
//==> list[0] = {Cl1, Cl11, Cl12}
//    list[1] = {Cl2, Cl21, Cl22}

QList<CLASSTREE*> ParseManager::CreateClassLists()
{
    QList<CLASSTREE*>ret;
    QList<CLASSLISTITEM*> classlist;
    QList<CLASSLISTITEM*> allclasslist;

    //Iteration over all parsed documents
    if(getPreProcessor()){
        for (Snapshot::const_iterator it = getPreProcessor()->snapshot.begin()
            ; it != getPreProcessor()->snapshot.end(); ++it)
        {
            Document::Ptr doc = (*it);
            if(doc){
                QFileInfo fileinf(doc->fileName());
                QFileInfo fileinf1(m_strHeaderFile);
                //Get the Translated unit
                Control* ctrl = doc->control();
                TranslationUnit* trlUnit = ctrl->translationUnit();
                AST* pAst = trlUnit->ast();
                TranslationUnitAST *ptrAst = 0;
                if(pAst && (ptrAst = pAst->asTranslationUnit())){
                    //iteration over all translated declaration in this document
                    for (DeclarationListAST *pDecllist = ptrAst->declaration_list; pDecllist; pDecllist = pDecllist->next) {
                        if(pDecllist->value){
                            SimpleDeclarationAST *pSimpleDec = pDecllist->value->asSimpleDeclaration();
                            if(pSimpleDec){
                                //Iteration over class specifier
                                for (SpecifierListAST *pSimpleDecDecllist = pSimpleDec->decl_specifier_list; pSimpleDecDecllist; pSimpleDecDecllist = pSimpleDecDecllist->next) {
                                    ClassSpecifierAST * pclassspec = pSimpleDecDecllist->value->asClassSpecifier();
                                    if(pclassspec){
                                        CLASSLISTITEM* item = new CLASSLISTITEM();
                                        item->classspec = pclassspec;
                                        item->trlUnit = trlUnit;
                                        allclasslist.push_back(item);
                                        //We found a class that is defined in the header file that needs to be checked
                                        if(fileinf.fileName().toLower() == fileinf1.fileName().toLower()){
                                            CLASSTREE* cltree = new CLASSTREE();
                                            cltree->highestlevelclass = item;
                                            cltree->classlist.push_back(item);
                                            //now add all baseclasses from this class
                                            ret.push_back(cltree);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    //after we search for the classes we need to search for the baseclasses
    foreach(CLASSTREE *cltree, ret){
        QList<CLASSLISTITEM*> baseclasslist;
        getBaseClasses(cltree->highestlevelclass, baseclasslist, allclasslist);
        cltree->classlist.append(baseclasslist);
    }
    return ret;
}

/********************************************
Gets all the baseclass from a class and
add those base classes into the baseclasslist
********************************************/
void ParseManager::getBaseClasses(const CLASSLISTITEM* pclass, QList<CLASSLISTITEM*> &baseclasslist, const QList<CLASSLISTITEM*> &allclasslist)
{
    //iteration over the base_clause_list of the current class
    QList<CLASSLISTITEM*>child;
    for(BaseSpecifierListAST *pBaseSpecList = pclass->classspec->base_clause_list; pBaseSpecList; pBaseSpecList = pBaseSpecList->next)
    {
        BaseSpecifierAST *pBaseSpec = pBaseSpecList->value;
        foreach(CLASSLISTITEM* pclspec, allclasslist)
        {
            if(pclspec->classspec->symbol->name()->isEqualTo(pBaseSpec->symbol->name()))
            {
                child.push_back(pclspec);
                baseclasslist.push_back(pclspec);
                break;
            }
        }
    }
    //call the function recursive because all the basclasses can have other base classes
    foreach(CLASSLISTITEM* pchclass, child){
        getBaseClasses(pchclass, baseclasslist, allclasslist);
    }
}

/**************************************************
This function finds and creates all Elements wich
are significant for MetaDatas.
Those element will be added in the aparameter
lists.
**************************************************/
void ParseManager::getElements(QList<FUNCTIONITEM*> &functionlist
                               , QList<PROPERTYITEM*> &propertylist
                               , QList<QENUMITEM*> &qenumlist
                               , QList<ENUMITEM*> &enumlist
                               , QList<QFLAGITEM*> &qflaglist
                               , QList<QDECLAREFLAGSITEM*> &qdeclareflaglist
                               , const QList<CLASSLISTITEM*> classitems
                               , const CLASSLISTITEM* highestlevelclass)
{
    foreach(CLASSLISTITEM* classitem, classitems){
        for (DeclarationListAST *pmemberlist = classitem->classspec->member_specifier_list; pmemberlist; pmemberlist = pmemberlist->next) {
            /**********
            Functions
            **********/
            FunctionDefinitionAST *pfctdef = pmemberlist->value->asFunctionDefinition();
            if(pfctdef){
                FUNCTIONITEM* item = new FUNCTIONITEM();
                item->trlUnit = classitem->trlUnit;
                item->function = pfctdef->symbol;
//                item->fctast = pfctdef;
                item->classAst = classitem->classspec;
                item->highestlevelclass = highestlevelclass;
                functionlist.push_back(item);
            }

            SimpleDeclarationAST *pdecl = pmemberlist->value->asSimpleDeclaration();
            if(pdecl){
                for(List<Declaration*>* decllist = pdecl->symbols; decllist; decllist = decllist->next)
                {
                    Function* pfct = decllist->value->type()->asFunctionType();
                    if(pfct){
                        FUNCTIONITEM* item = new FUNCTIONITEM();
                        item->trlUnit = classitem->trlUnit;
                        item->function = pfct;
//                        item->ast = pdecl;
                        item->classAst = classitem->classspec;
                        item->highestlevelclass = highestlevelclass;
                        functionlist.push_back(item);
                    }
                }
                /******
                enum
                ******/
                for(List<SpecifierAST*>* decllist = pdecl->decl_specifier_list; decllist; decllist = decllist->next)
                {
                    EnumSpecifierAST * penum = decllist->value->asEnumSpecifier();
                    if(penum){
                        ENUMITEM* item = new ENUMITEM();
                        item->ast = penum;
                        item->highestlevelclass = highestlevelclass;
                        item->trlUnit = classitem->trlUnit;
                        enumlist.push_back(item);
                    }
                }
            }
            else{
                /**********
                Q_PROPERTY
                **********/
                QPropertyDeclarationAST *ppdecl = pmemberlist->value->asQPropertyDeclarationAST();
                if(ppdecl){
                    PROPERTYITEM* item = new PROPERTYITEM();
                    item->ast = ppdecl;
                    item->highestlevelclass = highestlevelclass;
                    item->trlUnit = classitem->trlUnit;
                    item->readdefined = (ppdecl->read_token > 0);
                    item->writedefined = (ppdecl->write_token > 0);
                    item->resetdefined = (ppdecl->reset_token > 0);
                    item->notifydefined = (ppdecl->notify_token > 0);
                    propertylist.push_back(item);
                }
                else{
                    /**********
                    Q_ENUM
                    **********/
                    QEnumDeclarationAST *pqenum = pmemberlist->value->asQEnumDeclarationAST();
                    if(pqenum){
                        QENUMITEM* item = new QENUMITEM();
                        item->ast = pqenum;
                        item->highestlevelclass = highestlevelclass;
                        item->trlUnit = classitem->trlUnit;
                        qenumlist.push_back(item);
                    }
                    else{
                        /**********
                        Q_FLAGS
                        **********/
                        QFlagsDeclarationAST *pqflags = pmemberlist->value->asQFlagsDeclarationAST();
                        if(pqflags){
                            QFLAGITEM* item = new QFLAGITEM();
                            item->ast = pqflags;
                            item->highestlevelclass = highestlevelclass;
                            item->trlUnit = classitem->trlUnit;
                            qflaglist.push_back(item);
                        }
                        else {
                            /****************
                            Q_DECLARE_FLAGS
                            ****************/
                            QDeclareFlagsDeclarationAST *pqdeclflags = pmemberlist->value->asQDeclareFlagsDeclarationAST();
                            if(pqdeclflags){
                                QDECLAREFLAGSITEM* item = new QDECLAREFLAGSITEM();
                                item->ast = pqdeclflags;
                                item->highestlevelclass = highestlevelclass;
                                item->trlUnit = classitem->trlUnit;
                                qdeclareflaglist.push_back(item);
                            }
                        }
                    }
                }
            }
        }
    }
}

/*********************************************
Function that starts the comare between the
parser result and their metadata content.
*********************************************/
bool ParseManager::checkAllMetadatas(ParseManager* pInterfaceParserManager)
{
    bool ret = true;

    /************************************************
    Get all elements from the interface header file
    ************************************************/
    QList<CLASSTREE*> ilookuplist = pInterfaceParserManager->CreateClassLists();
    QList<QList<FUNCTIONITEM*> > ifunctionslookuplist;
    QList<QList<PROPERTYITEM*> > ipropertieslookuplist;
    QList<QList<QENUMITEM*> > iqenumlookuplist;
    QList<QList<ENUMITEM*> > ienumlookuplist;
    QList<QList<QFLAGITEM*> > iqflaglookuplist;
    QList<QList<QDECLAREFLAGSITEM*> > iqdeclareflaglookuplist;
    foreach(CLASSTREE* iclasstree, ilookuplist){
        QList<FUNCTIONITEM*>functionlist;
        QList<PROPERTYITEM*>propertylist;
        QList<QENUMITEM*>qenumlist;
        QList<ENUMITEM*>enumlist;
        QList<QFLAGITEM*> qflaglist;
        QList<QDECLAREFLAGSITEM*> qdeclareflag;
        getElements(functionlist
            , propertylist
            , qenumlist
            , enumlist
            , qflaglist
            , qdeclareflag
            , iclasstree->classlist
            , iclasstree->highestlevelclass);
        if(functionlist.size() > 0)
            ifunctionslookuplist.append(functionlist);
        if(propertylist.size() > 0)
            ipropertieslookuplist.append(propertylist);
        if(qenumlist.size() > 0)
            iqenumlookuplist.append(qenumlist);
        if(enumlist.size() > 0)
            ienumlookuplist.append(enumlist);
        if(qflaglist.size() > 0)
            iqflaglookuplist.append(qflaglist);
        if(qdeclareflag.size() > 0)
            iqdeclareflaglookuplist.append(qdeclareflag);
    }

    /************************************************
    Get all elements from the compare header file
    ************************************************/
    QList<CLASSTREE*> lookuplist = CreateClassLists();
    QList<QList<FUNCTIONITEM*> > functionslookuplist;
    QList<QList<PROPERTYITEM*> > propertieslookuplist;
    QList<QList<QENUMITEM*> > qenumlookuplist;
    QList<QList<ENUMITEM*> > enumlookuplist;
    QList<QList<QFLAGITEM*> > qflaglookuplist;
    QList<QList<QDECLAREFLAGSITEM*> > qdeclareflaglookuplist;
    foreach(CLASSTREE* classtree, lookuplist){
        QList<FUNCTIONITEM*>functionlist;
        QList<PROPERTYITEM*>propertylist;
        QList<QENUMITEM*>qenumlist;
        QList<ENUMITEM*>enumlist;
        QList<QFLAGITEM*> qflaglist;
        QList<QDECLAREFLAGSITEM*> qdeclareflag;
        getElements(functionlist
            , propertylist
            , qenumlist
            , enumlist
            , qflaglist
            , qdeclareflag
            , classtree->classlist
            , classtree->highestlevelclass);
        if(functionlist.size() > 0)
            functionslookuplist.append(functionlist);
        if(propertylist.size() > 0)
            propertieslookuplist.append(propertylist);
        if(qenumlist.size() > 0)
            qenumlookuplist.append(qenumlist);
        if(enumlist.size() > 0)
            enumlookuplist.append(enumlist);
        if(qflaglist.size() > 0)
            qflaglookuplist.append(qflaglist);
        if(qdeclareflag.size() > 0)
            qdeclareflaglookuplist.append(qdeclareflag);
    }

    /******************************
    Check for function
    ******************************/
    QList<FUNCTIONITEM*> missingifcts = checkMetadataFunctions(functionslookuplist, ifunctionslookuplist);
    if(missingifcts.size() > 0){
        foreach(FUNCTIONITEM* ifct, missingifcts){
            m_errormsgs.push_back(getErrorMessage(ifct));
        }
        ret =  false;
    }

    /******************************
    Check for properies
    ******************************/
    QList<PROPERTYITEM*> missingippts = checkMetadataProperties(propertieslookuplist, functionslookuplist, ipropertieslookuplist, ifunctionslookuplist);
    if(missingippts.size() > 0){
        foreach(PROPERTYITEM* ippt, missingippts){
            m_errormsgs.push_back(getErrorMessage(ippt));
        }
        ret =  false;
    }

    /******************************
    Check for enums
    ******************************/
    QList<QENUMITEM*> missingiqenums = checkMetadataEnums(qenumlookuplist, enumlookuplist, iqenumlookuplist, ienumlookuplist);
    if(missingiqenums.size() > 0){
        foreach(QENUMITEM* ienum, missingiqenums){
            m_errormsgs.push_back(getErrorMessage(ienum));
        }
        ret =  false;
    }

    /******************************
    Check for flags
    ******************************/
    QList<QFLAGITEM*> missingiqflags = checkMetadataFlags(qflaglookuplist, qdeclareflaglookuplist, enumlookuplist
                                                        , iqflaglookuplist, iqdeclareflaglookuplist, ienumlookuplist);
    if(missingiqflags.size() > 0){
        foreach(QFLAGITEM* iflags, missingiqflags){
            m_errormsgs.push_back(getErrorMessage(iflags));
        }
        ret =  false;
    }

    //now delet all Classitems
    foreach(CLASSTREE* l, ilookuplist){
        l->classlist.clear();
    }
    foreach(CLASSTREE* l, lookuplist){
        l->classlist.clear();
    }
    //delete all functionitems
    foreach(QList<FUNCTIONITEM*>l, ifunctionslookuplist){
        l.clear();
    }
    foreach(QList<FUNCTIONITEM*>l, functionslookuplist){
        l.clear();
    }
    //delete all properties
    foreach(QList<PROPERTYITEM*>l, ipropertieslookuplist){
        l.clear();
    }
    foreach(QList<PROPERTYITEM*>l, propertieslookuplist){
        l.clear();
    }
    //delete all qenums
    foreach(QList<QENUMITEM*>l, iqenumlookuplist){
        l.clear();
    }
    foreach(QList<QENUMITEM*>l, iqenumlookuplist){
        l.clear();
    }
    //delete all enums
    foreach(QList<ENUMITEM*>l, ienumlookuplist){
        l.clear();
    }
    foreach(QList<ENUMITEM*>l, enumlookuplist){
        l.clear();
    }
    //delete all qflags
    foreach(QList<QFLAGITEM*>l, iqflaglookuplist){
        l.clear();
    }
    foreach(QList<QFLAGITEM*>l, qflaglookuplist){
        l.clear();
    }
    //delete all qdeclareflags
    foreach(QList<QDECLAREFLAGSITEM*>l, iqdeclareflaglookuplist){
        l.clear();
    }
    foreach(QList<QDECLAREFLAGSITEM*>l, qdeclareflaglookuplist){
        l.clear();
    }

    return ret;
}

//<-------------------------------------------------------  Start of MetaData functions
/***********************************
Function that checks all functions
which will occur in the MetaData
***********************************/
QList<FUNCTIONITEM*> ParseManager::checkMetadataFunctions(const QList<QList<FUNCTIONITEM*> > &classfctlist, const QList<QList<FUNCTIONITEM*> > &iclassfctlist)
{
    QList<FUNCTIONITEM*> missingifcts;
    //Compare each function from interface with function from header (incl. baseclass functions)
    QList<FUNCTIONITEM*> ifcts;
    foreach(QList<FUNCTIONITEM*>ifunctionlist, iclassfctlist){
        ifcts.clear();
        //check if one header class contains all function from one interface header class
        if(classfctlist.count() > 0){
            foreach(QList<FUNCTIONITEM*>functionlist, classfctlist){
                QList<FUNCTIONITEM*> tmpl = containsAllMetadataFunction(functionlist, ifunctionlist);
                if(tmpl.size() == 0)
                    ifcts.clear();
                else
                    ifcts.append(tmpl);
            }
        }
        else {
            foreach(FUNCTIONITEM *pfct, ifunctionlist)
                pfct->classWichIsNotFound << "<all classes>";
            ifcts.append(ifunctionlist);
        }
        missingifcts.append(ifcts);
    }
    return missingifcts;
}

/*********************************************
Helper function to check if a function will
occure in the MetaData.
*********************************************/
bool ParseManager::isMetaObjFunction(FUNCTIONITEM* fct)
{
    if(fct->function->isInvokable()
        || fct->function->isSignal()
        || fct->function->isSlot())
        return true;
    return false;
}

/****************************************************
Check if all function from iclassfctlist are defined
in the classfctlist as well.
It will return all the function they are missing.
****************************************************/
QList<FUNCTIONITEM*> ParseManager::containsAllMetadataFunction(const QList<FUNCTIONITEM*> &classfctlist, const QList<FUNCTIONITEM*> &iclassfctlist)
{
    QList<FUNCTIONITEM*> ret;
    foreach(FUNCTIONITEM* ifct, iclassfctlist){
        if(isMetaObjFunction(ifct)){
            bool found = false;
            QStringList missingimplinclasses;
            ClassSpecifierAST* clspec = 0;
            foreach(FUNCTIONITEM* fct, classfctlist){
                if(clspec != fct->highestlevelclass->classspec){
                    clspec = fct->highestlevelclass->classspec;
                    //get the classname
                    QString classname = "";
                    unsigned int firsttoken = clspec->name->firstToken();
                    classname += fct->trlUnit->spell(firsttoken);
                    if(missingimplinclasses.indexOf(classname) < 0)
                        missingimplinclasses.push_back(classname);
                }
                //wb
                //qDebug() << "---------------------------------";
                //qDebug() << getErrorMessage(ifct);
                //qDebug() << getErrorMessage(fct);
                if(fct->isEqualTo(ifct, false)){
                    found = true;
                    missingimplinclasses.clear();
                    break;
                }
            }
            if(!found){
                ifct->classWichIsNotFound.append(missingimplinclasses);
                ret.push_back(ifct);
            }
        }
    }
    return ret;
}

/************************************
Function that gives back an error
string for a MetaData function
mismatch.
************************************/
QString ParseManager::getErrorMessage(FUNCTIONITEM* fct)
{
    QString ret = "";
    QTextStream out(&ret);
    QString fctstring = "";
    QString fcttype = "";

    foreach(QString classname, fct->classWichIsNotFound){
        if(ret.size() > 0)
            out << endl;

        fcttype = "";
        fctstring = classname;
        fctstring += "::";

        unsigned int token = fct->function->sourceLocation() - 1;
        if(token >= 0){
            //tok.isNot(T_EOF_SYMBOL)
            while(fct->trlUnit->tokenAt(token).isNot(T_EOF_SYMBOL)){
                fctstring += fct->trlUnit->tokenAt(token).spell();
                if(*fct->trlUnit->tokenAt(token).spell() == ')')
                    break;
                fctstring += " ";
                token++;
            }
        }

        Function* pfct = fct->function;
        if(pfct){
            fcttype = "type: ";
            //Check for private, protected and public
            if(pfct->isPublic())
                fcttype = "public ";
            if(pfct->isProtected())
                fcttype = "protected ";
            if(pfct->isPrivate())
                fcttype = "private ";

            if(pfct->isVirtual())
                fcttype += "virtual ";
            if(pfct->isPureVirtual())
                fcttype += "pure virtual ";

            if(pfct->isSignal())
                fcttype += "Signal ";
            if(pfct->isSlot())
                fcttype += "Slot ";
            if(pfct->isNormal())
                fcttype += "Normal ";
            if(pfct->isInvokable())
                fcttype += "Invokable ";
        }
        out << fcttype << fctstring;
    }
    return ret;
}
//--->

//<-------------------------------------------------------  Start of Q_PROPERTY checks
/***********************************
Function that checks all Property
which will occur in the MetaData
***********************************/
QList<PROPERTYITEM*> ParseManager::checkMetadataProperties(const QList<QList<PROPERTYITEM*> > &classproplist
                                                                         , const QList<QList<FUNCTIONITEM*> > &classfctlist
                                                                         , const QList<QList<PROPERTYITEM*> > &iclassproplist
                                                                         , const QList<QList<FUNCTIONITEM*> > &iclassfctlist)
{
    QList<PROPERTYITEM*> missingifcts;
    //assign the property functions
    foreach(QList<PROPERTYITEM*>proplist, classproplist){
        foreach(PROPERTYITEM* prop, proplist){
            assignPropertyFunctions(prop, classfctlist);
        }
    }

    foreach(QList<PROPERTYITEM*>proplist, iclassproplist){
        foreach(PROPERTYITEM* prop, proplist){
            assignPropertyFunctions(prop, iclassfctlist);
        }
    }

    //Compare each qproperty from interface with qproperty from header (incl. baseclass functions)
    QList<PROPERTYITEM*> ippts;
    foreach(QList<PROPERTYITEM*>ipropertylist, iclassproplist){
        ippts.clear();
        //check if one header class contains all function from one interface header class
        if(classproplist.count() > 0){
            foreach(QList<PROPERTYITEM*>propertylist, classproplist){
                QList<PROPERTYITEM*> tmpl = containsAllPropertyFunction(propertylist, ipropertylist);
                if(tmpl.size() == 0)
                    ippts.clear();
                else
                    ippts.append(tmpl);
            }
        }
        else {
            foreach(PROPERTYITEM *pprop, ipropertylist)
                pprop->classWichIsNotFound << "<all classes>";
            ippts.append(ipropertylist);
        }
        missingifcts.append(ippts);
    }
    return missingifcts;
}

/**************************************
Function that resolves the dependensies
between Q_PROPERTY
and thier READ, WRITE, NOTIFY and RESET
functions.
***************************************/
void ParseManager::assignPropertyFunctions(PROPERTYITEM* prop, const QList<QList<FUNCTIONITEM*> > &fctlookuplist)
{
    //get the name of the needed functions
    QString type;
    QString readfctname;
    QString writefctname;
    QString resetfctname;
    QString notifyfctname;

    int needtofind = 0;
    type = prop->trlUnit->spell(prop->ast->type_token);
    if(prop->readdefined){
        readfctname = prop->trlUnit->spell(prop->ast->read_function_token);
        needtofind++;
    }
    if(prop->writedefined){
        writefctname = prop->trlUnit->spell(prop->ast->write_function_token);
        needtofind++;
    }
    if(prop->resetdefined){
        resetfctname = prop->trlUnit->spell(prop->ast->reset_function_token);
        needtofind++;
    }
    if(prop->notifydefined){
        notifyfctname = prop->trlUnit->spell(prop->ast->notify_function_token);
        needtofind++;
    }
    //Now iterate over all function to find all functions wich are defined in the Q_PROPERTY macro
    if(needtofind > 0){
        prop->foundalldefinedfct = false;
        foreach(QList<FUNCTIONITEM*> fctlist, fctlookuplist){
            foreach(FUNCTIONITEM* pfct, fctlist){
/*
                long start = pfct->ast->firstToken();
                long stop = pfct->ast->lastToken();
                QString val;
                for(long i = start; i < stop; i++){
                    val += pfct->trlUnit->spell(i);
                    val += " ";
                }
                qDebug() << val;
*/
                QString fctname = pfct->trlUnit->spell(pfct->function->sourceLocation());
                //check the function type against the property type
                QString fcttype =pfct->trlUnit->spell(pfct->function->sourceLocation() - 1);

                if(fctname.length() > 0 && fcttype.length() > 0){
                    if(prop->readdefined && fctname == readfctname){
                        if(type != fcttype)
                            continue;
                        prop->readFct = pfct;
                        needtofind--;
                    }
                    if(prop->writedefined && fctname == writefctname){
                        prop->writeFct = pfct;
                        needtofind--;
                    }
                    if(prop->resetdefined && fctname == resetfctname){
                        prop->resetFct = pfct;
                        needtofind--;
                    }
                    if(prop->notifydefined && fctname == notifyfctname){
                        prop->notifyFct = pfct;
                        needtofind--;
                    }
                    if(needtofind <= 0){
                        //a flag that indicates if a function was missing
                        prop->foundalldefinedfct = true;
                        return;
                    }
                }
            }
        }
    }
}

/**************************************
Function that checks if all functions
dependencies in Q_PROPERTY have the
same arguments and retunr value.
***************************************/
QList<PROPERTYITEM*> ParseManager::containsAllPropertyFunction(const QList<PROPERTYITEM*> &classproplist, const QList<PROPERTYITEM*> &iclassproplist)
{
    QList<PROPERTYITEM*> ret;
    foreach(PROPERTYITEM* ipropt, iclassproplist){
        if(ipropt->foundalldefinedfct){
            bool found = false;
            QStringList missingimplinclasses;
            ClassSpecifierAST* clspec = 0;
            foreach(PROPERTYITEM* propt, classproplist){
                if(clspec != propt->highestlevelclass->classspec){
                    clspec = propt->highestlevelclass->classspec;
                    //get the classname
                    QString classname = "";
                    unsigned int firsttoken = clspec->name->firstToken();
                    classname += propt->trlUnit->spell(firsttoken);
                    if(missingimplinclasses.indexOf(classname) < 0)
                        missingimplinclasses.push_back(classname);
                }
                if(propt->isEqualTo(ipropt)){
                    found = true;
                    missingimplinclasses.clear();
                    break;
                }
            }
            if(!found){
                ipropt->classWichIsNotFound.append(missingimplinclasses);
                ret.push_back(ipropt);
            }
        }
        else{
            ret.push_back(ipropt);
        }
    }
    return ret;
}

/************************************
Function that gives back an error
string for a Q_PROPERTY mismatch.
************************************/
QString ParseManager::getErrorMessage(PROPERTYITEM* ppt)
{
    QString ret = "";
    QTextStream out(&ret);
    QString pptstring = "";
    QString ppttype = "";

    if(!ppt->foundalldefinedfct)
    {
        unsigned int firsttoken = ppt->highestlevelclass->classspec->name->firstToken();
        unsigned int lasttoken = ppt->highestlevelclass->classspec->name->lastToken();
        for(unsigned int i = firsttoken; i < lasttoken; i++){
            out << ppt->trlUnit->spell(i);
        }
        out << "::";
        firsttoken = ppt->ast->firstToken();
        lasttoken = ppt->ast->lastToken();
        for(unsigned int i = firsttoken; i <= lasttoken; i++){
            out << ppt->trlUnit->spell(i) << " ";
        }
        out << endl << " -";
        if(ppt->readdefined && !ppt->readFct)
            out << "READ ";
        if(ppt->writedefined && !ppt->writeFct)
            out << "WRITE ";
        if(ppt->resetdefined && !ppt->resetFct)
            out << "RESET.";
        if(ppt->notifydefined && !ppt->notifyFct)
            out << "NOTIFY ";
        out << "functions missing." << endl;
    }
    for(int i = 0; i < ppt->classWichIsNotFound.size(); i++){
        if(ret.size() > 0)
            out << endl;

        pptstring = ppt->classWichIsNotFound[i];
        pptstring += "::";

        unsigned int firsttoken = ppt->ast->firstToken();
        unsigned int lasttoken = ppt->ast->lastToken();
        for(unsigned int i = firsttoken; i <= lasttoken; i++){
            pptstring += ppt->trlUnit->spell(i);
            pptstring += " ";
        }

        out << ppttype << pptstring;
    }
    return ret;
}
//--->


//<------------------------------------------------------- Start of Q_ENUMS checks
/***********************************
Function that checks all enums
which will occur in the MetaData
***********************************/
QList<QENUMITEM*> ParseManager::checkMetadataEnums(const QList<QList<QENUMITEM*> > &classqenumlist
                                                , const QList<QList<ENUMITEM*> > &classenumlist
                                                , const QList<QList<QENUMITEM*> > &iclassqenumlist
                                                , const QList<QList<ENUMITEM*> > &iclassenumlist)
{
    QList<QENUMITEM*> missingiqenums;
    //assign the property functions
    foreach(QList<QENUMITEM*>qenumlist, classqenumlist){
        foreach(QENUMITEM* qenum, qenumlist){
            assignEnumValues(qenum, classenumlist);
        }
    }
    foreach(QList<QENUMITEM*>qenumlist, iclassqenumlist){
        foreach(QENUMITEM* qenum, qenumlist){
            assignEnumValues(qenum, iclassenumlist);
        }
    }

    //Compare each qenum from interface with qenum from header (incl. baseclass functions)
    QList<QENUMITEM*> iqenums;
    foreach(QList<QENUMITEM*>iqenumlist, iclassqenumlist){
        iqenums.clear();
        //check if one header class contains all function from one interface header class
        if(classqenumlist.count() > 0){
            foreach(QList<QENUMITEM*>qenumlist, classqenumlist){
                QList<QENUMITEM*> tmpl = containsAllEnums(qenumlist, iqenumlist);
                if(tmpl.size() == 0)
                    iqenums.clear();
                else
                    iqenums.append(tmpl);

            }
        }
        else {
            foreach(QENUMITEM *qenum, iqenumlist)
                qenum->classWichIsNotFound << "<all classes>";
            iqenums.append(iqenumlist);
        }
        missingiqenums.append(iqenums);
    }

    return missingiqenums;
}

/*********************************************
Helper function which creates a string out of
an enumerator including its values.
*********************************************/
QStringList ParseManager::getEnumValueStringList(ENUMITEM *penum, QString mappedenumname/* = ""*/)
{
    QStringList ret;
    EnumSpecifierAST *penumsec = penum->ast;
    QString enumname = penum->trlUnit->spell(penumsec->name->firstToken());
    int enumvalue = 0;
    //now iterrate over all enumitems and create a string like following:
    //EnumName.EnumItemName.Value
    //ConnectionState.disconnected.0
    for (EnumeratorListAST *plist = penum->ast->enumerator_list; plist; plist = plist->next) {
        QString value = enumname;
        if(mappedenumname.size() > 0)
            value = mappedenumname;
        value += ".";
        value += penum->trlUnit->spell(plist->value->identifier_token);
        value += ".";
        if(plist->value->equal_token > 0 && plist->value->expression){
            QString v = penum->trlUnit->spell(plist->value->expression->firstToken());
            bool ch;
            int newval = enumvalue;
            if(v.indexOf("0x") >= 0)
                newval = v.toInt(&ch, 16);
            else
                newval = v.toInt(&ch, 10);
            if(ch)
                enumvalue = newval;
        }
        value += QString::number(enumvalue);
        enumvalue++;
        // now add this enumitem string in the VALUE list
        ret << value;
    }
    return ret;
}

/**************************************
Function that resolves the dependensies
between Q_ENUMS and enums.
***************************************/
void ParseManager::assignEnumValues(QENUMITEM* qenum, const QList<QList<ENUMITEM*> > &enumlookuplist)
{
    QString enumname;
    for (EnumeratorListAST *plist = qenum->ast->enumerator_list; plist; plist = plist->next) {
        EnumeratorAST *penum = plist->value->asEnumerator();
        if(penum){
            //get the name of the enum definition
            enumname = qenum->trlUnit->spell(penum->firstToken());
            //iterate over all enums and find the one with the same name like enumname
            bool found = false;
            foreach (QList<ENUMITEM*> penumlist, enumlookuplist) {
                foreach(ENUMITEM *penum, penumlist){
                    EnumSpecifierAST *penumsec = penum->ast;
                    QString enumname1 = penum->trlUnit->spell(penumsec->name->firstToken());
                    if(enumname == enumname1){
                        qenum->values << getEnumValueStringList(penum);
                        found = true;
                        break;
                    }
                }
                if(!found)
                    qenum->foundallenums = false;
            }
        }
    }
}

/***********************************
Function that checkt if the Q_ENUMS
are completed defined and if the
Enum values are the same.
***********************************/
QList<QENUMITEM*> ParseManager::containsAllEnums(const QList<QENUMITEM*> &classqenumlist, const QList<QENUMITEM*> &iclassqenumlist)
{
    QList<QENUMITEM*> ret;
    foreach(QENUMITEM* iqenum, iclassqenumlist){
        bool found = false;
        QStringList missingimplinclasses;
        ClassSpecifierAST* clspec = 0;
        foreach(QENUMITEM* qenum, classqenumlist){
            if(clspec != qenum->highestlevelclass->classspec){
                clspec = qenum->highestlevelclass->classspec;
                //get the classname
                QString classname = "";
                unsigned int firsttoken = clspec->name->firstToken();
                classname += qenum->trlUnit->spell(firsttoken);
                if(missingimplinclasses.indexOf(classname) < 0)
                    missingimplinclasses.push_back(classname);
            }
            if(qenum->isEqualTo(iqenum)){
                found = true;
                missingimplinclasses.clear();
                break;
            }
        }
        if(!found){
            iqenum->classWichIsNotFound.append(missingimplinclasses);
            ret.push_back(iqenum);
        }
    }
    return ret;
}

/************************************
Function that gives back an error
string for a Q_ENUMS mismatch.
************************************/
QString ParseManager::getErrorMessage(QENUMITEM* qenum)
{
    QString ret = "";
    QTextStream out(&ret);

    if(!qenum->foundallenums)
    {
        unsigned int firsttoken = qenum->highestlevelclass->classspec->name->firstToken();
        unsigned int lasttoken = qenum->highestlevelclass->classspec->name->lastToken();
        for(unsigned int i = firsttoken; i < lasttoken; i++){
            out << qenum->trlUnit->spell(i);
        }
        out << "::";
        firsttoken = qenum->ast->firstToken();
        lasttoken = qenum->ast->lastToken();
        for(unsigned int i = firsttoken; i <= lasttoken; i++){
            out << qenum->trlUnit->spell(i) << " ";
        }
        out << endl << " - one or more Enums missing." << endl;
    }

    for(int i = 0; i < qenum->classWichIsNotFound.size(); i++){
        if(ret.size() > 0)
            out << endl;

        out << qenum->classWichIsNotFound[i] << "::";

        unsigned int firsttoken = qenum->ast->firstToken();
        unsigned int lasttoken = qenum->ast->lastToken();
        for(unsigned int i = firsttoken; i <= lasttoken; i++){
            out << qenum->trlUnit->spell(i) << " ";
        }
    }
    return ret;
}
//--->

//<------------------------------------------------------- Start of Q_FLAGS checks
/***********************************
Function that checks all flags
which will occur in the MetaData
***********************************/
QList<QFLAGITEM*> ParseManager::checkMetadataFlags(const QList<QList<QFLAGITEM*> > &classqflaglist
                                            , const QList<QList<QDECLAREFLAGSITEM*> > &classqdeclareflaglist
                                            , const QList<QList<ENUMITEM*> > &classenumlist
                                            , const QList<QList<QFLAGITEM*> > &iclassqflaglist
                                            , const QList<QList<QDECLAREFLAGSITEM*> > &iclassqdeclareflaglist
                                            , const QList<QList<ENUMITEM*> > &iclassenumlist)
{
    QList<QFLAGITEM*> missingqflags;
    //assign the enums to the flags
    foreach(QList<QFLAGITEM*>qflaglist, classqflaglist){
        foreach(QFLAGITEM* qflag, qflaglist){
            assignFlagValues(qflag, classqdeclareflaglist, classenumlist);
        }
    }
    foreach(QList<QFLAGITEM*>qflaglist, iclassqflaglist){
        foreach(QFLAGITEM* qflag, qflaglist){
            assignFlagValues(qflag, iclassqdeclareflaglist, iclassenumlist);
        }
    }

    //Compare each qenum from interface with qenum from header (incl. baseclass functions)
    QList<QFLAGITEM*> iqflags;
    foreach(QList<QFLAGITEM*>iqflaglist, iclassqflaglist){
        iqflags.clear();
        //check if one header class contains all function from one interface header class
        if(classqflaglist.count() >0){
            foreach(QList<QFLAGITEM*>qflaglist, classqflaglist){
                QList<QFLAGITEM*> tmpl = containsAllFlags(qflaglist, iqflaglist);
                if(tmpl.size() == 0)
                    iqflags.clear();
                else
                    iqflags.append(tmpl);

            }
        }
        else {
            foreach(QFLAGITEM *pflag, iqflaglist)
                pflag->classWichIsNotFound << "<all classes>";
            iqflags.append(iqflaglist);
        }
        missingqflags.append(iqflags);
    }
    return missingqflags;
}

/**************************************
Function that resolves the dependensies
between Q_FLAG, Q_DECLARE_FLAGS
and enums.
***************************************/
void ParseManager::assignFlagValues(QFLAGITEM* qflags, const QList<QList<QDECLAREFLAGSITEM*> > &qdeclareflagslookuplist, const QList<QList<ENUMITEM*> > &enumlookuplist)
{
    QString qflagname;
    QString enumname;
    //read the flag names
    for (EnumeratorListAST *plist = qflags->ast->enumerator_list; plist; plist = plist->next) {
        EnumeratorAST *pflags = plist->value->asEnumerator();
        if(pflags){
            qflagname = qflags->trlUnit->spell(pflags->firstToken());
            enumname = qflagname;
            //try to find if there is a deflare flag macro with the same name as in qflagname
            bool found = false;
            foreach(QList<QDECLAREFLAGSITEM*> qdeclarelist, qdeclareflagslookuplist){
                foreach(QDECLAREFLAGSITEM* qdeclare, qdeclarelist){
                    QString declarename = qdeclare->trlUnit->spell(qdeclare->ast->flag_token);
                    if(declarename == qflagname){
                        //now map the right enum name to the flag
                        enumname = qdeclare->trlUnit->spell(qdeclare->ast->enum_token);
                        found = true;
                        break;
                    }
                }
                if(found)
                    break;
            }
            //now we have the right enum name now we need to find the enum
            found = false;
            foreach(QList<ENUMITEM*> enumitemlist, enumlookuplist){
                foreach(ENUMITEM* enumitem, enumitemlist){
                    EnumSpecifierAST *penumspec = enumitem->ast;
                    QString enumspecname = enumitem->trlUnit->spell(penumspec->name->firstToken());
                    if(enumspecname == enumname){
                        qflags->enumvalues << getEnumValueStringList(enumitem, qflagname);
                        found = true;
                        break;
                    }
                }
                if(found)
                    break;
            }
            if(!found)
                qflags->foundallenums = false;
        }
    }
}

/*****************************************
Function that compares if all enums
and flags assigned by using the Q_FLAGS
are complete defined.
*****************************************/
QList<QFLAGITEM*> ParseManager::containsAllFlags(const QList<QFLAGITEM*> &classqflaglist, const QList<QFLAGITEM*> &iclassqflaglist)
{
    QList<QFLAGITEM*> ret;
    foreach(QFLAGITEM* iqflags, iclassqflaglist){
        if(iqflags->foundallenums){
            bool found = false;
            QStringList missingimplinclasses;
            ClassSpecifierAST* clspec = 0;
            foreach(QFLAGITEM* qflags, classqflaglist){
                if(clspec != qflags->highestlevelclass->classspec){
                    clspec = qflags->highestlevelclass->classspec;
                    //get the classname
                    QString classname = "";
                    unsigned int firsttoken = clspec->name->firstToken();
                    classname += qflags->trlUnit->spell(firsttoken);
                    if(missingimplinclasses.indexOf(classname) < 0)
                        missingimplinclasses.push_back(classname);
                }
                if(qflags->isEqualTo(iqflags)){
                    found = true;
                    missingimplinclasses.clear();
                    break;
                }
            }
            if(!found){
                iqflags->classWichIsNotFound.append(missingimplinclasses);
                ret.push_back(iqflags);
            }
        }
        else
            ret.push_back(iqflags);
    }
    return ret;
}

/************************************
Function that gives back an error
string for a Q_FLAGS mismatch.
************************************/
QString ParseManager::getErrorMessage(QFLAGITEM* pfg)
{
    QString ret = "";
    QTextStream out(&ret);

    if(!pfg->foundallenums)
    {
        unsigned int firsttoken = pfg->highestlevelclass->classspec->name->firstToken();
        unsigned int lasttoken = pfg->highestlevelclass->classspec->name->lastToken();
        for(unsigned int i = firsttoken; i < lasttoken; i++){
            out << pfg->trlUnit->spell(i);
        }
        out << "::";
        firsttoken = pfg->ast->firstToken();
        lasttoken = pfg->ast->lastToken();
        for(unsigned int i = firsttoken; i <= lasttoken; i++){
            out << pfg->trlUnit->spell(i) << " ";
        }
        out << endl << " - one or more Enums missing." << endl;
    }
    for(int i = 0; i < pfg->classWichIsNotFound.size(); i++){
        if(ret.size() > 0)
            out << endl;

        out << pfg->classWichIsNotFound[i] << "::";

        unsigned int firsttoken = pfg->ast->firstToken();
        unsigned int lasttoken = pfg->ast->lastToken();
        for(unsigned int i = firsttoken; i <= lasttoken; i++){
            out << pfg->trlUnit->spell(i) << " ";
        }
    }
    return ret;
}

#include <moc_ParseManager.cpp>
//--->
