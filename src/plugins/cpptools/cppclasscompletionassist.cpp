
#include "cppclasscompletionassist.h"
#include <cpptools/cppcompletionassist.h>
#include <cpptools/cpplocatorfilter.h>
#include <cpptools/cpprefactoringhelper.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/genericproposal.h>
#include <utils/qtcassert.h>

namespace CppTools {

TextEditor::IAssistProcessor*CppClassCompletionAssistProvider::createProcessor() const
{
   return new CppClassCompletionAssistProcessor;
}

TextEditor::AssistInterface*CppClassCompletionAssistProvider::createAssistInterface(const QString& filePath, const TextEditor::TextEditorWidget* textEditorWidget,
                                                                                    const CPlusPlus::LanguageFeatures& languageFeatures,
                                                                                    int position, TextEditor::AssistReason reason) const
{
   QTC_ASSERT(textEditorWidget, return 0);

   return new CppTools::Internal::CppCompletionAssistInterface(filePath,
                                                               textEditorWidget,
                                                               BuiltinEditorDocumentParser::get(filePath),
                                                               languageFeatures,
                                                               position,
                                                               reason,
                                                               CppModelManager::instance()->workingCopy());
}

TextEditor::IAssistProposal*CppClassCompletionAssistProcessor::perform(const TextEditor::AssistInterface* interface)
{
   classCompletion(interface->fileName(), m_positionForProposal);
   return createContentProposal();
}


bool CppClassCompletionAssistProcessor::classCompletion(const QString &filename, int& positionOfProposals)
{
   bool result = true;
   CPlusPlus::Snapshot snapshot = CppTools::CppModelManager::instance()->snapshot();
   CppTools::CppRefactoringChanges cppRefactoringChanges(snapshot);
   CppTools::CppRefactoringFilePtr currentFile = cppRefactoringChanges.file(filename);

   if(currentFile->cppDocument().data()==NULL || !currentFile->cppDocument())
      return result;

   int requestPosition = currentFile->cursor().position();

   QStringList foundProposals;
   QString regexpString;
   executeClassCompletion(currentFile, requestPosition, positionOfProposals, foundProposals, regexpString);
   regexpString = QLatin1String("(^|:)") + regexpString + QLatin1String("$");
   m_qRegExp = QSharedPointer<QRegExp>(new QRegExp(regexpString));

   int order = 0;
   foreach(const QString& proposals, foundProposals)
   {
      if(!proposals.isEmpty()) {
         addCompletionItemWithRegexp(proposals, QIcon(), order, m_qRegExp);
         ++order;
      }
   }

   return result;
}

TextEditor::IAssistProposal*CppClassCompletionAssistProcessor::createContentProposal()
{
   TextEditor::GenericProposalModel *model = new TextEditor::GenericProposalModel;
   model->loadContent(m_completions);
   return new TextEditor::GenericProposal(m_positionForProposal, model);

}

void CppClassCompletionAssistProcessor::createRegexpForUpperCase(const QString& expression, QString& regexp) const
{
   bool first = true;
   bool ignoreNext = false;
   for(QString::const_iterator it1 = expression.cbegin(), itEnd1 = expression.cend();it1!=itEnd1;++it1)
   {
       const QChar& chr = *it1;

       if(chr.isUpper() && !first &&!ignoreNext) {
          regexp += QLatin1String("[a-z0-9_]*");
          ignoreNext = false;
       } else if(chr == QLatin1Char('*')) {
          regexp += QLatin1String("[A-Za-z0-9_]");
          ignoreNext = true;
       } else if(ignoreNext)
          ignoreNext = false;

      regexp += chr;
       if(first)
          first = false;
   }
   regexp += QLatin1String("[A-Za-z0-9_]*");
}

void CppClassCompletionAssistProcessor::getCurrentSymbolForClassCompletion(CppTools::CppRefactoringFilePtr currentFile, int requestPosition, QString& symbol, int& symbolPosition)
{
   symbol = CppRefactoringHelper::getSymbolAtPosition(currentFile, requestPosition, CppRefactoringHelper::isCharOfClassCompletionSymbolBackward,
                                          CppRefactoringHelper::isCharOfClassCompletionSymbolForward, symbolPosition);
}


void CppClassCompletionAssistProcessor::executeClassCompletion(CppTools::CppRefactoringFilePtr currentFile,
                                                             int requestPosition, int& proposalPosition, QStringList& proposals, QString& regExp)
{
   unsigned int line, column;
   currentFile->lineAndColumn(requestPosition, &line, &column);

   CPlusPlus::Scope *currentScope = currentFile->cppDocument()->scopeAt(line, column);

   QStringList vCallerNamespace;
   QStringList vCallerClassAsNamespaceElements;
   CppRefactoringHelper::getNamespaceAndClassElementsFromScope(currentScope, vCallerNamespace, vCallerClassAsNamespaceElements);

   QMap<QString, QString> namespaceAliases;
   QMap<QString, int> namespaceAliasesLine;

   if(!CppRefactoringHelper::isHeaderFile(currentFile->fileName()))
      CppRefactoringHelper::getNamespaceAliasesInFile(currentFile, namespaceAliases, namespaceAliasesLine);

    QString cursorSymbol;
    getCurrentSymbolForClassCompletion(currentFile, requestPosition, cursorSymbol, proposalPosition);
    if(cursorSymbol.size()==0)
        return;

    CPlusPlus::Scope *initScope = currentFile->cppDocument()->scopeAt(line,column);
    if(!initScope)
        return;

    // complete the regexp
    QString regexpPattern;
    createRegexpForUpperCase(cursorSymbol, regexpPattern);
    regExp = regexpPattern;
    regexpPattern.prepend(QLatin1Char('^'));
    QRegExp qRegExp(regexpPattern);

    QMap<int, QSet<QString> > proposalsMap;
    CppTools::Internal::CppLocatorFilter* cppLocatorFilter = ExtensionSystem::PluginManager::getObject<CppTools::Internal::CppLocatorFilter>();
    QFutureInterface<Core::LocatorFilterEntry> dummyInterface;
    QList<Core::LocatorFilterEntry> matches = cppLocatorFilter->matchesRegexp(dummyInterface, qRegExp);

    foreach (const Core::LocatorFilterEntry &entry, matches)
    {
        IndexItem::Ptr info = entry.internalData.value<IndexItem::Ptr>();
        // if the symbol is in an header file or in the current file
        if(CppRefactoringHelper::isHeaderFile(info->fileName())||info->fileName()==currentFile->fileName())
        {
            QString fullClassName = info->scopedSymbolName();
            QStringList vNamespaceElements;
            QStringList vClassAsNamespaceElements;
            QString className;

            CppRefactoringHelper::getNamespaceAndClass(fullClassName, vNamespaceElements, vClassAsNamespaceElements, className);

            QString proposition;
            int callerNamespaceDistance;
            transformQualifiedClassIntoProposition(className, vNamespaceElements, vClassAsNamespaceElements, vCallerNamespace,
                                                   vCallerClassAsNamespaceElements, namespaceAliases, proposition, callerNamespaceDistance);
            proposalsMap[callerNamespaceDistance].insert(proposition);
        }
    }

    orderProposals(cursorSymbol, proposalsMap, proposals);

    if(!proposalsMap.empty())
    {
       TextEditor::TextEditorWidget* textEditorWidget = currentFile->editor();
       if(textEditorWidget)
       {
          int endOfCursorSymbolPosition = proposalPosition + cursorSymbol.size();
          QTextCursor qTextCursor = textEditorWidget->textCursor();
          qTextCursor.setPosition(endOfCursorSymbolPosition);
          textEditorWidget->setTextCursor(qTextCursor);
       }
    }
}


void CppClassCompletionAssistProcessor::transformQualifiedClassIntoProposition(const QString& className, const QStringList& vNamespaceElements, const QStringList& vClassAsNamespaceElements,
                                                                               const QStringList& vCallerNamespace, const QStringList& vCallerClassAsNamespaceElements,
                                                                               const QMap<QString, QString>& namespaceAliases,
                                                                               QString& proposition, int& callerNamespaceDistance)
{
   QStringList vDiffNamespace;
   CppRefactoringHelper::reduceNamespace(vNamespaceElements, vCallerNamespace, vDiffNamespace);
   callerNamespaceDistance = vDiffNamespace.size();

   proposition = QLatin1String("");
   if(callerNamespaceDistance!=0) {
      QString propositionNamespace = CppRefactoringHelper::buildFromNamespaceElements(vNamespaceElements);
      QMap<QString, QString>::const_iterator itAlias = namespaceAliases.find(propositionNamespace);
      if(itAlias!=namespaceAliases.end()){
         proposition = itAlias.value();
         callerNamespaceDistance = 1;
      } else
         proposition = propositionNamespace;
      proposition += QLatin1String("::");
   }


   QStringList vDiffClassAsNamespace;
   if(callerNamespaceDistance==0){
      CppRefactoringHelper::reduceNamespace(vClassAsNamespaceElements, vCallerClassAsNamespaceElements, vDiffClassAsNamespace);
   } else
      vDiffClassAsNamespace = vClassAsNamespaceElements;

   if(vDiffClassAsNamespace.size()!=0){
      callerNamespaceDistance += vDiffClassAsNamespace.size();
      proposition += CppRefactoringHelper::buildFromNamespaceElements(vDiffClassAsNamespace) + QLatin1String("::");
   }

   proposition += className;
}

void CppClassCompletionAssistProcessor::orderProposals(const QString& cursorSymbol, const QMap<int, QSet<QString> >& proposalsMap, QStringList& proposals)
{
   QStringList exactMatches;
   QStringList otherMatches;

   for(QMap<int, QSet<QString> >::const_iterator it1=proposalsMap.begin(), itEnd1=proposalsMap.end();it1!=itEnd1;++it1)
   {
      QStringList localExactMatches;
      QStringList localOtherMatches;
      const QSet<QString>& proposalsSet = it1.value();

      foreach(const QString& proposal, proposalsSet){
         if(proposal.endsWith(cursorSymbol))
            localExactMatches.append(proposal);
         else
            localOtherMatches.append(proposal);
      }

      localExactMatches.sort();
      localOtherMatches.sort();

      exactMatches.append(localExactMatches);
      otherMatches.append(localOtherMatches);
   }

   foreach(const QString& exactMatch, exactMatches){
      proposals.push_back(exactMatch);
   }

   foreach(const QString& otherMatch, otherMatches){
      proposals.push_back(otherMatch);
   }
}

void CppClassCompletionAssistProcessor::addCompletionItemWithRegexp(const QString& text, const QIcon& icon, int order, const QSharedPointer<QRegExp>& qRegExp)
{
   TextEditor::AssistProposalItem *item = new TextEditor::AssistProposalItem;
   item->setText(text);
   item->setIcon(icon);
   item->setOrder(order);
   item->setCustomizeRegexp(qRegExp);
   m_completions.append(item);
}

}
