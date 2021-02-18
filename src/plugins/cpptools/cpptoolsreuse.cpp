/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "cpptoolsreuse.h"

#include "cppcodemodelsettings.h"
#include "cpprefactoringchanges.h"
#include "cpptoolsconstants.h"
#include "cpptoolsplugin.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/idocument.h>
#include <coreplugin/messagemanager.h>

#include <cplusplus/Overview.h>
#include <cplusplus/LookupContext.h>
#include <utils/algorithm.h>
#include <utils/textutils.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QRegularExpression>
#include <QSet>
#include <QTextCursor>
#include <QTextDocument>

using namespace CPlusPlus;

namespace CppTools {

static int skipChars(QTextCursor *tc,
                      QTextCursor::MoveOperation op,
                      int offset,
                      std::function<bool(const QChar &)> skip)
{
    const QTextDocument *doc = tc->document();
    if (!doc)
        return 0;
    QChar ch = doc->characterAt(tc->position() + offset);
    if (ch.isNull())
        return 0;
    int count = 0;
    while (skip(ch)) {
        if (tc->movePosition(op))
            ++count;
        else
            break;
        ch = doc->characterAt(tc->position() + offset);
    }
    return count;
}

static int skipCharsForward(QTextCursor *tc, std::function<bool(const QChar &)> skip)
{
    return skipChars(tc, QTextCursor::NextCharacter, 0, skip);
}

static int skipCharsBackward(QTextCursor *tc, std::function<bool(const QChar &)> skip)
{
    return skipChars(tc, QTextCursor::PreviousCharacter, -1, skip);
}

QStringList identifierWordsUnderCursor(const QTextCursor &tc)
{
    const QTextDocument *document = tc.document();
    if (!document)
        return {};
    const auto isSpace = [](const QChar &c) { return c.isSpace(); };
    const auto isColon = [](const QChar &c) { return c == ':'; };
    const auto isValidIdentifierCharAt = [document](const QTextCursor &tc) {
        return isValidIdentifierChar(document->characterAt(tc.position()));
    };
    // move to the end
    QTextCursor endCursor(tc);
    do {
        moveCursorToEndOfIdentifier(&endCursor);
        // possibly skip ::
        QTextCursor temp(endCursor);
        skipCharsForward(&temp, isSpace);
        const int colons = skipCharsForward(&temp, isColon);
        skipCharsForward(&temp, isSpace);
        if (colons == 2 && isValidIdentifierCharAt(temp))
            endCursor = temp;
    } while (isValidIdentifierCharAt(endCursor));

    QStringList results;
    QTextCursor startCursor(endCursor);
    do {
        moveCursorToStartOfIdentifier(&startCursor);
        if (startCursor.position() == endCursor.position())
            break;
        QTextCursor temp(endCursor);
        temp.setPosition(startCursor.position(), QTextCursor::KeepAnchor);
        results.append(temp.selectedText().remove(QRegularExpression("\\s")));
        // possibly skip ::
        temp = startCursor;
        skipCharsBackward(&temp, isSpace);
        const int colons = skipCharsBackward(&temp, isColon);
        skipCharsBackward(&temp, isSpace);
        if (colons == 2
                && isValidIdentifierChar(document->characterAt(temp.position() - 1))) {
            startCursor = temp;
        }
    } while (!isValidIdentifierCharAt(startCursor));
    return results;
}

void moveCursorToEndOfIdentifier(QTextCursor *tc)
{
    skipCharsForward(tc, isValidIdentifierChar);
}

void moveCursorToStartOfIdentifier(QTextCursor *tc)
{
    skipCharsBackward(tc, isValidIdentifierChar);
}

static bool isOwnershipRAIIName(const QString &name)
{
    static QSet<QString> knownNames;
    if (knownNames.isEmpty()) {
        // Qt
        knownNames.insert(QLatin1String("QScopedPointer"));
        knownNames.insert(QLatin1String("QScopedArrayPointer"));
        knownNames.insert(QLatin1String("QMutexLocker"));
        knownNames.insert(QLatin1String("QReadLocker"));
        knownNames.insert(QLatin1String("QWriteLocker"));
        // Standard C++
        knownNames.insert(QLatin1String("auto_ptr"));
        knownNames.insert(QLatin1String("unique_ptr"));
        // Boost
        knownNames.insert(QLatin1String("scoped_ptr"));
        knownNames.insert(QLatin1String("scoped_array"));
    }

    return knownNames.contains(name);
}

bool isOwnershipRAIIType(Symbol *symbol, const LookupContext &context)
{
    if (!symbol)
        return false;

    // This is not a "real" comparison of types. What we do is to resolve the symbol
    // in question and then try to match its name with already known ones.
    if (symbol->isDeclaration()) {
        Declaration *declaration = symbol->asDeclaration();
        const NamedType *namedType = declaration->type()->asNamedType();
        if (namedType) {
            ClassOrNamespace *clazz = context.lookupType(namedType->name(),
                                                         declaration->enclosingScope());
            if (clazz && !clazz->symbols().isEmpty()) {
                Overview overview;
                Symbol *symbol = clazz->symbols().at(0);
                return isOwnershipRAIIName(overview.prettyName(symbol->name()));
            }
        }
    }

    return false;
}

bool isValidAsciiIdentifierChar(const QChar &ch)
{
    return ch.isLetterOrNumber() || ch == QLatin1Char('_');
}

bool isValidFirstIdentifierChar(const QChar &ch)
{
    return ch.isLetter() || ch == QLatin1Char('_') || ch.isHighSurrogate() || ch.isLowSurrogate();
}

bool isValidIdentifierChar(const QChar &ch)
{
    return isValidFirstIdentifierChar(ch) || ch.isNumber();
}

bool isValidIdentifier(const QString &s)
{
    const int length = s.length();
    for (int i = 0; i < length; ++i) {
        const QChar &c = s.at(i);
        if (i == 0) {
            if (!isValidFirstIdentifierChar(c))
                return false;
        } else {
            if (!isValidIdentifierChar(c))
                return false;
        }
    }
    return true;
}

bool isQtKeyword(QStringView text)
{
    switch (text.length()) {
    case 4:
        switch (text.at(0).toLatin1()) {
        case 'e':
            if (text == QLatin1String("emit"))
                return true;
            break;
        case 'S':
            if (text == QLatin1String("SLOT"))
                return true;
            break;
        }
        break;

    case 5:
        if (text.at(0) == QLatin1Char('s') && text == QLatin1String("slots"))
            return true;
        break;

    case 6:
        if (text.at(0) == QLatin1Char('S') && text == QLatin1String("SIGNAL"))
            return true;
        break;

    case 7:
        switch (text.at(0).toLatin1()) {
        case 's':
            if (text == QLatin1String("signals"))
                return true;
            break;
        case 'f':
            if (text == QLatin1String("foreach") || text ==  QLatin1String("forever"))
                return true;
            break;
        }
        break;

    default:
        break;
    }
    return false;
}

void switchHeaderSource()
{
    const Core::IDocument *currentDocument = Core::EditorManager::currentDocument();
    QTC_ASSERT(currentDocument, return);
    const QString otherFile = correspondingHeaderOrSource(currentDocument->filePath().toString());
    if (!otherFile.isEmpty())
        Core::EditorManager::openEditor(otherFile);
}

QString identifierUnderCursor(QTextCursor *cursor)
{
    cursor->movePosition(QTextCursor::StartOfWord);
    cursor->movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    return cursor->selectedText();
}

const Macro *findCanonicalMacro(const QTextCursor &cursor, Document::Ptr document)
{
    QTC_ASSERT(document, return nullptr);

    int line, column;
    Utils::Text::convertPosition(cursor.document(), cursor.position(), &line, &column);

    if (const Macro *macro = document->findMacroDefinitionAt(line)) {
        QTextCursor macroCursor = cursor;
        const QByteArray name = CppTools::identifierUnderCursor(&macroCursor).toUtf8();
        if (macro->name() == name)
            return macro;
    } else if (const Document::MacroUse *use = document->findMacroUseAt(cursor.position())) {
        return &use->macro();
    }

    return nullptr;
}

CppCodeModelSettings *codeModelSettings()
{
    return CppTools::Internal::CppToolsPlugin::instance()->codeModelSettings();
}

int indexerFileSizeLimitInMb()
{
    const CppCodeModelSettings *settings = codeModelSettings();
    QTC_ASSERT(settings, return -1);

    if (settings->skipIndexingBigFiles())
        return settings->indexerFileSizeLimitInMb();

    return -1;
}

bool fileSizeExceedsLimit(const QFileInfo &fileInfo, int sizeLimitInMb)
{
    if (sizeLimitInMb <= 0)
        return false;

    const qint64 fileSizeInMB = fileInfo.size() / (1000 * 1000);
    if (fileSizeInMB > sizeLimitInMb) {
        const QString absoluteFilePath = fileInfo.absoluteFilePath();
        const QString msg = QCoreApplication::translate(
                    "CppIndexer",
                    "C++ Indexer: Skipping file \"%1\" because it is too big.")
                        .arg(absoluteFilePath);

        QMetaObject::invokeMethod(Core::MessageManager::instance(),
                                  [msg]() { Core::MessageManager::writeSilently(msg); });

        return true;
    }

    return false;
}

UsePrecompiledHeaders getPchUsage()
{
    const CppCodeModelSettings *cms = codeModelSettings();
    if (cms->pchUsage() == CppCodeModelSettings::PchUse_None)
        return UsePrecompiledHeaders::No;
    return UsePrecompiledHeaders::Yes;
}

static void addBuiltinConfigs(ClangDiagnosticConfigsModel &model)
{
    ClangDiagnosticConfig config;

    // Questionable constructs
    config = ClangDiagnosticConfig();
    config.setId(Constants::CPP_CLANG_DIAG_CONFIG_QUESTIONABLE);
    config.setDisplayName(QCoreApplication::translate(
                              "ClangDiagnosticConfigsModel",
                              "Checks for questionable constructs"));
    config.setIsReadOnly(true);
    config.setClangOptions({
        "-Wall",
        "-Wextra",
    });
    config.setClazyMode(ClangDiagnosticConfig::ClazyMode::UseCustomChecks);
    config.setClangTidyMode(ClangDiagnosticConfig::TidyMode::UseCustomChecks);
    model.appendOrUpdate(config);

    // Warning flags from build system
    config = ClangDiagnosticConfig();
    config.setId(Constants::CPP_CLANG_DIAG_CONFIG_BUILDSYSTEM);
    config.setDisplayName(QCoreApplication::translate("ClangDiagnosticConfigsModel",
                                                      "Build-system warnings"));
    config.setIsReadOnly(true);
    config.setClazyMode(ClangDiagnosticConfig::ClazyMode::UseCustomChecks);
    config.setClangTidyMode(ClangDiagnosticConfig::TidyMode::UseCustomChecks);
    config.setUseBuildSystemWarnings(true);
    model.appendOrUpdate(config);
}

ClangDiagnosticConfigsModel diagnosticConfigsModel(const ClangDiagnosticConfigs &customConfigs)
{
    ClangDiagnosticConfigsModel model;
    addBuiltinConfigs(model);
    for (const ClangDiagnosticConfig &config : customConfigs)
        model.appendOrUpdate(config);
    return model;
}

ClangDiagnosticConfigsModel diagnosticConfigsModel()
{
    return diagnosticConfigsModel(CppTools::codeModelSettings()->clangCustomDiagnosticConfigs());
}

NSVisitor::NSVisitor(const CppRefactoringFile *file, const QStringList &namespaces, int symbolPos)
    : ASTVisitor(file->cppDocument()->translationUnit()),
      m_file(file),
      m_remainingNamespaces(namespaces),
      m_symbolPos(symbolPos)
{}

bool CppTools::NSVisitor::preVisit(AST *ast)
{
    if (!m_firstToken)
        m_firstToken = ast;
    if (m_file->startOf(ast) >= m_symbolPos)
        m_done = true;
    return !m_done;
}

bool NSVisitor::visit(NamespaceAST *ns)
{
    if (!m_firstNamespace)
        m_firstNamespace = ns;
    if (m_remainingNamespaces.isEmpty()) {
        m_done = true;
        return false;
    }

    QString name;
    const Identifier * const id = translationUnit()->identifier(ns->identifier_token);
    if (id)
        name = QString::fromUtf8(id->chars(), id->size());
    if (name != m_remainingNamespaces.first())
        return false;

    if (!ns->linkage_body) {
        m_done = true;
        return false;
    }

    m_enclosingNamespace = ns;
    m_remainingNamespaces.removeFirst();
    return !m_remainingNamespaces.isEmpty();
}

void NSVisitor::postVisit(AST *ast)
{
    if (ast == m_enclosingNamespace)
        m_done = true;
}

/**
 * @brief The NSCheckerVisitor class checks which namespaces are missing for a given list
 * of enclosing namespaces at a given position
 */
NSCheckerVisitor::NSCheckerVisitor(const CppRefactoringFile *file, const QStringList &namespaces,
                                   int symbolPos)
    : ASTVisitor(file->cppDocument()->translationUnit())
    , m_file(file)
    , m_remainingNamespaces(namespaces)
    , m_symbolPos(symbolPos)
{}

bool NSCheckerVisitor::preVisit(AST *ast)
{
    if (m_file->startOf(ast) >= m_symbolPos)
        m_done = true;
    return !m_done;
}

void NSCheckerVisitor::postVisit(AST *ast)
{
    if (!m_done && m_file->endOf(ast) > m_symbolPos)
        m_done = true;
}

bool NSCheckerVisitor::visit(NamespaceAST *ns)
{
    if (m_remainingNamespaces.isEmpty())
        return false;

    QString name = getName(ns);
    if (name != m_remainingNamespaces.first())
        return false;

    m_enteredNamespaces.push_back(ns);
    m_remainingNamespaces.removeFirst();
    // if we reached the searched namespace we don't have to search deeper
    return !m_remainingNamespaces.isEmpty();
}

bool NSCheckerVisitor::visit(UsingDirectiveAST *usingNS)
{
    // example: we search foo::bar and get 'using namespace foo;using namespace foo::bar;'
    const QString fullName = Overview{}.prettyName(usingNS->name->name);
    const QStringList namespaces = fullName.split("::");
    if (namespaces.length() > m_remainingNamespaces.length())
        return false;

    // from other using namespace statements
    const auto curList = m_usingsPerNamespace.find(currentNamespace());
    const bool isCurListValid = curList != m_usingsPerNamespace.end();

    const bool startEqual = std::equal(namespaces.cbegin(),
                                       namespaces.cend(),
                                       m_remainingNamespaces.cbegin());
    if (startEqual) {
        if (isCurListValid) {
            if (namespaces.length() > curList->second.length()) {
                // eg. we already have 'using namespace foo;' and
                // now get 'using namespace foo::bar;'
                curList->second = namespaces;
            }
            // the other case: first 'using namespace foo::bar;' and now 'using namespace foo;'
        } else
            m_usingsPerNamespace.emplace(currentNamespace(), namespaces);
    } else if (isCurListValid) {
        // ex: we have already 'using namespace foo;' and get 'using namespace bar;' now
        QStringList newlist = curList->second;
        newlist.append(namespaces);
        if (newlist.length() <= m_remainingNamespaces.length()) {
            const bool startEqual = std::equal(newlist.cbegin(),
                                               newlist.cend(),
                                               m_remainingNamespaces.cbegin());
            if (startEqual)
                curList->second.append(namespaces);
        }
    }
    return false;
}

void NSCheckerVisitor::endVisit(NamespaceAST *ns)
{
    // if the symbolPos was in the namespace and the
    // namespace has no children, m_done should be true
    postVisit(ns);
    if (!m_done && currentNamespace() == ns) {
        // we were not succesfull in this namespace, so undo all changes
        m_remainingNamespaces.push_front(getName(currentNamespace()));
        m_usingsPerNamespace.erase(currentNamespace());
        m_enteredNamespaces.pop_back();
    }
}

void NSCheckerVisitor::endVisit(TranslationUnitAST *)
{
    // the last node, create the final result
    // we must handle like the following: We search for foo::bar and have:
    // using namespace foo::bar;
    // namespace foo {
    //    // cursor/symbolPos here
    // }
    if (m_remainingNamespaces.empty()) {
        // we are already finished
        return;
    }
    // find the longest combination of normal namespaces + using statements
    int longestNamespaceList = 0;
    int enteredNamespaceCount = 0;
    // check 'using namespace ...;' statements in the global scope
    const auto namespaces = m_usingsPerNamespace.find(nullptr);
    if (namespaces != m_usingsPerNamespace.end())
        longestNamespaceList = namespaces->second.length();

    for (auto ns : m_enteredNamespaces) {
        ++enteredNamespaceCount;
        const auto namespaces = m_usingsPerNamespace.find(ns);
        int newListLength = enteredNamespaceCount;
        if (namespaces != m_usingsPerNamespace.end())
            newListLength += namespaces->second.length();
        longestNamespaceList = std::max(newListLength, longestNamespaceList);
    }
    m_remainingNamespaces.erase(m_remainingNamespaces.begin(),
                                m_remainingNamespaces.begin() + longestNamespaceList
                                - m_enteredNamespaces.size());
}

QString NSCheckerVisitor::getName(NamespaceAST *ns)
{
    const Identifier *const id = translationUnit()->identifier(ns->identifier_token);
    if (id)
        return QString::fromUtf8(id->chars(), id->size());
    return {};
}

NamespaceAST *NSCheckerVisitor::currentNamespace()
{
    return m_enteredNamespaces.empty() ? nullptr : m_enteredNamespaces.back();
}

} // CppTools
