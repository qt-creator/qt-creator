#include "cplusplus/Scope.h"
#include "cplusplus/Symbol.h"
#include "cpptools/cpprefactoringhelper.h"

QString CppRefactoringHelper::buildFromNamespaceElements(const QStringList& namespaceElements)
{
   return namespaceElements.join(QLatin1String("::"));
}

bool CppRefactoringHelper::isCharOfClassCompletionSymbolForward(const QChar& qChar)
{
   return (qChar.isLetterOrNumber() || qChar==QLatin1Char('_') || qChar==QLatin1Char('^') || qChar==QLatin1Char('*'));
}

bool CppRefactoringHelper::isCharOfClassCompletionSymbolBackward(const QChar& qChar)
{
   return (qChar.isLetterOrNumber() || qChar==QLatin1Char('_') || qChar==QLatin1Char('^') || qChar==QLatin1Char('*') || qChar==QLatin1Char(':'));
}

Utils::ChangeSet::Range CppRefactoringHelper::getSymbolRange(CppTools::CppRefactoringFilePtr& file, int initialPosition,
                                                 SymbolFilter* symbolFilterBackward, SymbolFilter* symbolFilterForward)
{
   int currentPosition = initialPosition;
   QChar qChar;
   qChar = file->charAt(currentPosition);

   // in case we are at the last character
   if(!symbolFilterBackward(qChar)){
       --currentPosition;
       if(currentPosition>0)
          qChar = file->charAt(currentPosition);
   }
   int firstValidPosition = currentPosition;

   while(currentPosition>0 && symbolFilterBackward(qChar)){
      --currentPosition;
      if(currentPosition>0)
         qChar = file->charAt(currentPosition);
   }
   ++currentPosition;
   int startPosition = currentPosition;

   currentPosition = firstValidPosition;
   if(currentPosition>0)
      qChar = file->charAt(currentPosition);

   while(symbolFilterForward(qChar))
   {
      ++currentPosition;
      qChar = file->charAt(currentPosition);
   }
   int endPosition = currentPosition;

   return Utils::ChangeSet::Range(startPosition, endPosition);
}

QString CppRefactoringHelper::getSymbolAtPosition(CppTools::CppRefactoringFilePtr &file, int initialPosition, SymbolFilter* symbolFilterBackward,
                                      SymbolFilter* symbolFilterForward, int& symbolPosition)
{
   Utils::ChangeSet::Range symbolRange = getSymbolRange(file, initialPosition, symbolFilterBackward, symbolFilterForward);
   symbolPosition = symbolRange.start;
   return static_cast<TextEditor::RefactoringFile*>(file.data())->textOf(symbolRange);
}

void CppRefactoringHelper::getNamespaceElementsFromClass(CPlusPlus::Class* clazz, QStringList& vNamespaceElements)
{
   CPlusPlus::Namespace* enclosingNamespace = clazz->enclosingNamespace();

   while(enclosingNamespace) {
      const CPlusPlus::Identifier *identifier = enclosingNamespace->identifier();

      if(identifier) {
         const char * scopeName = identifier->chars();
         if(scopeName && scopeName[0]!=0 && scopeName[0]!='<') {
            vNamespaceElements.prepend(QLatin1String(scopeName));
         }
      }

      enclosingNamespace = enclosingNamespace->enclosingNamespace();
   }
}


void CppRefactoringHelper::getNamespaceAndClassElementsFromSymbol(CPlusPlus::Symbol* symbol, QStringList& vNamespaceElements,
                                                                  QStringList& vClassElements, QString& otherElements)
{
   const CPlusPlus::Identifier *symbolIdentifier = symbol->identifier();
   if(symbolIdentifier)
      otherElements = QString::fromLatin1(symbolIdentifier->chars(), static_cast<int>(symbolIdentifier->size()));

   bool ignoreNextElement = false;
   if(symbol->isClass()||symbol->isNamespace())
      ignoreNextElement = true;

   CPlusPlus::Symbol* enclosingSymbol = symbol;

   enum Item {
      ITEM_UNDEFINED,
      ITEM_CLASS,
      ITEM_NAMESPACE
   };

   QList<QPair<Item, QString> > namespaceItems;
   // we go up on the enclosingSymbols, and we find the namespace and class
   while(enclosingSymbol)
   {
      const CPlusPlus::Identifier *identifier = NULL;
      Item item = ITEM_UNDEFINED;

      // we find the
      do
      {
         item = ITEM_UNDEFINED;
         if(CPlusPlus::Class* enclosingClass = enclosingSymbol->asClass())
         {
            identifier = enclosingClass->identifier();
            item = ITEM_CLASS;
         } else if(CPlusPlus::Namespace* enclosingNamespace = enclosingSymbol->asNamespace())
         {
            identifier = enclosingNamespace->identifier();
            item = ITEM_NAMESPACE;
         }

         enclosingSymbol = enclosingSymbol->enclosingScope();
      } while(enclosingSymbol&&!identifier);

      if(ignoreNextElement)
      {
         ignoreNextElement = false;
      } else if(identifier) {
         const char * scopeName = identifier->chars();
         if(scopeName && scopeName[0]!=0 && scopeName[0]!='<') {
            namespaceItems.prepend(qMakePair(item, QLatin1String(scopeName)));
         }
      }
   }

   bool fillNamespace = true;
   for(QList<QPair<Item, QString> >::iterator it1=namespaceItems.begin(), itEnd1=namespaceItems.end(); it1!=itEnd1; ++it1){
      Item item = it1->first;
      if(!fillNamespace || item==ITEM_CLASS){
         fillNamespace = false;
         vClassElements.append(it1->second);
      } else if (item==ITEM_NAMESPACE){
         vNamespaceElements.append(it1->second);
      }
   }
}

void CppRefactoringHelper::getNamespaceAndClassElementsFromScope(CPlusPlus::Scope *scope, QStringList& vNamespaceElements,
                                                     QStringList& vClassAsNamespaceElements)
{
   QString otherElements;
   getNamespaceAndClassElementsFromSymbol(scope, vNamespaceElements, vClassAsNamespaceElements, otherElements);

}


void CppRefactoringHelper::getNamespaceAliasesInFile(CppTools::CppRefactoringFilePtr& cppFile, QMap<QString, QString>& namespaceAliases, QMap<QString, int>& namespaceAliasesLine)
{
   CPlusPlus::Namespace* globalNamespace = cppFile->cppDocument()->globalNamespace();
   for(CPlusPlus::Scope::iterator it1=globalNamespace->memberBegin(), itEnd1=globalNamespace->memberEnd(); it1!=itEnd1; ++it1) {
      CPlusPlus::Symbol* symbol = *it1;
      if(CPlusPlus::NamespaceAlias *namespaceAlias = symbol->asNamespaceAlias()){
         // normally test not useful
         if(cppFile->fileName()==QLatin1String(symbol->fileName())){
            const char *aliasChars = 0;
            if(const CPlusPlus::Identifier *identifierAlias = namespaceAlias->identifier()){
               aliasChars = identifierAlias->chars();
            } else
               continue;

            QString aliasedNamespace;
            const CPlusPlus::QualifiedNameId* qualifiedName = namespaceAlias->namespaceName()->asQualifiedNameId();
            while(qualifiedName){

               const CPlusPlus::Identifier *identifier = qualifiedName->identifier();
               if(identifier){
                  if(!aliasedNamespace.isEmpty())
                     aliasedNamespace.prepend(QLatin1String("::"));
                  aliasedNamespace.prepend(QLatin1String(identifier->chars()));
               }

               const CPlusPlus::Name* base = qualifiedName->base();
               if(base) {
                  qualifiedName = base->asQualifiedNameId();
                  if(qualifiedName == 0) {
                     // last element
                     const CPlusPlus::Identifier *identifier = base->identifier();
                     if(identifier){
                        if(!aliasedNamespace.isEmpty())
                           aliasedNamespace.prepend(QLatin1String("::"));
                        aliasedNamespace.prepend(QLatin1String(identifier->chars()));
                     }
                  }
               } else
                  qualifiedName = 0;

            }

            if(aliasChars && aliasedNamespace.size()>0){
               namespaceAliases.insert(aliasedNamespace, QLatin1String(aliasChars));
               namespaceAliasesLine.insert(aliasedNamespace, static_cast<int>(symbol->line()));
            }
         }
      }
   }
}

bool CppRefactoringHelper::isHeaderFile(const QString& filename)
{
    return (!filename.endsWith(QLatin1String(".cpp"))&&!filename.endsWith(QLatin1String(".c")));

}

void CppRefactoringHelper::getNamespaceAndClass(const QString& fullySpecifiedClassName, QStringList& vNamespaceElements, QStringList& vClassAsNamespaceElements, QString& className)
{
    enum Status{
        NORMAL_CHR,
        COLON_FOUND
    };

    bool bClassNamespace = false;
    QString currentSymbol;
    Status status = NORMAL_CHR;
    for(QString::const_iterator it1 = fullySpecifiedClassName.cbegin(), itEnd1 = fullySpecifiedClassName.cend();it1!=itEnd1;++it1)
    {
       const QChar& chr = *it1;
       if(status==COLON_FOUND) {
          if(chr==QLatin1Char(':')){
             if(currentSymbol.size()>0) {
                if(currentSymbol[0].isUpper())
                   bClassNamespace = true;
                if(bClassNamespace)
                   vClassAsNamespaceElements.append(currentSymbol);
                else
                   vNamespaceElements.append(currentSymbol);
             }
             currentSymbol.clear();
          } else {
             currentSymbol.append(chr);
          }
          status = NORMAL_CHR;
       }else if (status==NORMAL_CHR) {
          if(chr==QLatin1Char(':'))
             status = COLON_FOUND;
          else
             currentSymbol.append(chr);
       }
    }

    className = currentSymbol;
}


void CppRefactoringHelper::reduceNamespace(const QStringList& vNamespaceUsed, const QStringList& vNamespaceUsing, QStringList& vDiffNamespace)
{

   QStringList::const_iterator itUsed = vNamespaceUsed.cbegin();
   QStringList::const_iterator itUsedEnd = vNamespaceUsed.cend();
   QStringList::const_iterator itUsing = vNamespaceUsing.cbegin();
   QStringList::const_iterator itUsingEnd = vNamespaceUsing.cend();

   while(itUsed!=itUsedEnd && itUsing!=itUsingEnd) {
       if(*itUsed != *itUsing)
          break;
       ++itUsed;
       ++itUsing;
   }

   while (itUsed!=itUsedEnd) {
      vDiffNamespace.append(*itUsed);
      ++itUsed;
   }

}
