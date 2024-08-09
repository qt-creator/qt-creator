// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcodegenerationquickfixes.h"

#include "../cppcodestylesettings.h"
#include "../cppeditortr.h"
#include "../cpprefactoringchanges.h"
#include "../insertionpointlocator.h"
#include "cppquickfix.h"
#include "cppquickfixhelpers.h"
#include "cppquickfixprojectsettings.h"
#include "cppquickfixsettings.h"

#include <cplusplus/Overview.h>
#include <cplusplus/CppRewriter.h>
#include <projectexplorer/projecttree.h>
#include <utils/basetreeview.h>
#include <utils/treemodel.h>

#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMimeData>
#include <QPushButton>
#include <QProxyStyle>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QTreeView>

#ifdef WITH_TESTS
#include "cppquickfix_test.h"
#include <QAbstractItemModelTester>
#include <QtTest>
#endif

using namespace CPlusPlus;
using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

static QStringList toStringList(const QList<Symbol *> names)
{
    QStringList list;
    list.reserve(names.size());
    for (const auto symbol : names) {
        const Identifier *const id = symbol->identifier();
        list << QString::fromUtf8(id->chars(), id->size());
    }
    return list;
}

static QList<Symbol *> getMemberFunctions(const Class *clazz)
{
    QList<Symbol *> memberFunctions;
    for (auto it = clazz->memberBegin(); it != clazz->memberEnd(); ++it) {
        Symbol *const s = *it;
        if (!s->identifier() || !s->type())
            continue;
        if ((s->asDeclaration() && s->type()->asFunctionType()) || s->asFunction())
            memberFunctions << s;
    }
    return memberFunctions;
}

static QString symbolAtDifferentLocation(
    const CppQuickFixInterface &interface,
    Symbol *symbol,
    const CppRefactoringFilePtr &targetFile,
    InsertionLocation targetLocation)
{
    QTC_ASSERT(symbol, return QString());
    Scope *scopeAtInsertPos = targetFile->cppDocument()->scopeAt(targetLocation.line(),
                                                                 targetLocation.column());

    LookupContext cppContext(targetFile->cppDocument(), interface.snapshot());
    ClassOrNamespace *cppCoN = cppContext.lookupType(scopeAtInsertPos);
    if (!cppCoN)
        cppCoN = cppContext.globalNamespace();
    SubstitutionEnvironment env;
    env.setContext(interface.context());
    env.switchScope(symbol->enclosingScope());
    UseMinimalNames q(cppCoN);
    env.enter(&q);
    Control *control = interface.context().bindings()->control().get();
    Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
    return oo.prettyName(LookupContext::minimalName(symbol, cppCoN, control));
}

static FullySpecifiedType typeAtDifferentLocation(
    const CppQuickFixInterface &interface,
    FullySpecifiedType type,
    Scope *originalScope,
    const CppRefactoringFilePtr &targetFile,
    InsertionLocation targetLocation,
    const QStringList &newNamespaceNamesAtLoc = {})
{
    Scope *scopeAtInsertPos = targetFile->cppDocument()->scopeAt(targetLocation.line(),
                                                                 targetLocation.column());
    for (const QString &nsName : newNamespaceNamesAtLoc) {
        const QByteArray utf8Name = nsName.toUtf8();
        Control *control = targetFile->cppDocument()->control();
        const Name *name = control->identifier(utf8Name.data(), utf8Name.size());
        Namespace *ns = control->newNamespace(0, name);
        ns->setEnclosingScope(scopeAtInsertPos);
        scopeAtInsertPos = ns;
    }
    LookupContext cppContext(targetFile->cppDocument(), interface.snapshot());
    ClassOrNamespace *cppCoN = cppContext.lookupType(scopeAtInsertPos);
    if (!cppCoN)
        cppCoN = cppContext.globalNamespace();
    SubstitutionEnvironment env;
    env.setContext(interface.context());
    env.switchScope(originalScope);
    UseMinimalNames q(cppCoN);
    env.enter(&q);
    Control *control = interface.context().bindings()->control().get();
    return rewriteType(type, &env, control);
}

static QString memberBaseName(const QString &name)
{
    const auto validName = [](const QString &name) {
        return !name.isEmpty() && !name.at(0).isDigit();
    };
    QString baseName = name;

    CppQuickFixSettings *settings = CppQuickFixProjectsSettings::getQuickFixSettings(
        ProjectExplorer::ProjectTree::currentProject());
    const QString &nameTemplate = settings->memberVariableNameTemplate;
    const QString prefix = nameTemplate.left(nameTemplate.indexOf('<'));
    const QString postfix = nameTemplate.mid(nameTemplate.lastIndexOf('>') + 1);
    if (name.startsWith(prefix) && name.endsWith(postfix)) {
        const QString base = name.mid(prefix.length(), name.length() - postfix.length());
        if (validName(base))
            return base;
    }

    // Remove leading and trailing "_"
    while (baseName.startsWith(QLatin1Char('_')))
        baseName.remove(0, 1);
    while (baseName.endsWith(QLatin1Char('_')))
        baseName.chop(1);
    if (baseName != name && validName(baseName))
        return baseName;

    // If no leading/trailing "_": remove "m_" and "m" prefix
    if (baseName.startsWith(QLatin1String("m_"))) {
        baseName.remove(0, 2);
    } else if (baseName.startsWith(QLatin1Char('m')) && baseName.length() > 1
               && baseName.at(1).isUpper()) {
        baseName.remove(0, 1);
        baseName[0] = baseName.at(0).toLower();
    }

    return validName(baseName) ? baseName : name;
}

static std::optional<FullySpecifiedType> getFirstTemplateParameter(const Name *name)
{
    if (const QualifiedNameId *qualifiedName = name->asQualifiedNameId())
        return getFirstTemplateParameter(qualifiedName->name());

    if (const TemplateNameId *templateName = name->asTemplateNameId()) {
        if (templateName->templateArgumentCount() > 0)
            return templateName->templateArgumentAt(0).type();
    }
    return {};
}

static std::optional<FullySpecifiedType> getFirstTemplateParameter(Type *type)
{
    if (NamedType *namedType = type->asNamedType())
        return getFirstTemplateParameter(namedType->name());

    return {};
}

static std::optional<FullySpecifiedType> getFirstTemplateParameter(FullySpecifiedType type)
{
    return getFirstTemplateParameter(type.type());
}

struct ExistingGetterSetterData
{
    Class *clazz = nullptr;
    Declaration *declarationSymbol = nullptr;
    QString getterName;
    QString setterName;
    QString resetName;
    QString signalName;
    QString qPropertyName;
    QString memberVariableName;
    Document::Ptr doc;

    int computePossibleFlags() const;
};

// FIXME: Should be a member?
static void findExistingFunctions(ExistingGetterSetterData &existing, QStringList memberFunctionNames)
{
    const CppQuickFixSettings *settings = CppQuickFixProjectsSettings::getQuickFixSettings(
        ProjectExplorer::ProjectTree::currentProject());
    const QString lowerBaseName = memberBaseName(existing.memberVariableName).toLower();
    const QStringList getterNames{lowerBaseName,
                                  "get_" + lowerBaseName,
                                  "get" + lowerBaseName,
                                  "is_" + lowerBaseName,
                                  "is" + lowerBaseName,
                                  settings->getGetterName(lowerBaseName)};
    const QStringList setterNames{"set_" + lowerBaseName,
                                  "set" + lowerBaseName,
                                  settings->getSetterName(lowerBaseName)};
    const QStringList resetNames{"reset_" + lowerBaseName,
                                 "reset" + lowerBaseName,
                                 settings->getResetName(lowerBaseName)};
    const QStringList signalNames{lowerBaseName + "_changed",
                                  lowerBaseName + "changed",
                                  settings->getSignalName(lowerBaseName)};
    for (const auto &memberFunctionName : memberFunctionNames) {
        const QString lowerName = memberFunctionName.toLower();
        if (getterNames.contains(lowerName))
            existing.getterName = memberFunctionName;
        else if (setterNames.contains(lowerName))
            existing.setterName = memberFunctionName;
        else if (resetNames.contains(lowerName))
            existing.resetName = memberFunctionName;
        else if (signalNames.contains(lowerName))
            existing.signalName = memberFunctionName;
    }
}

static void extractNames(const CppRefactoringFilePtr &file,
                  QtPropertyDeclarationAST *qtPropertyDeclaration,
                  ExistingGetterSetterData &data)
{
    QtPropertyDeclarationItemListAST *it = qtPropertyDeclaration->property_declaration_item_list;
    for (; it; it = it->next) {
        const char *tokenString = file->tokenAt(it->value->item_name_token).spell();
        if (!qstrcmp(tokenString, "READ")) {
            data.getterName = file->textOf(it->value->expression);
        } else if (!qstrcmp(tokenString, "WRITE")) {
            data.setterName = file->textOf(it->value->expression);
        } else if (!qstrcmp(tokenString, "RESET")) {
            data.resetName = file->textOf(it->value->expression);
        } else if (!qstrcmp(tokenString, "NOTIFY")) {
            data.signalName = file->textOf(it->value->expression);
        } else if (!qstrcmp(tokenString, "MEMBER")) {
            data.memberVariableName = file->textOf(it->value->expression);
        }
    }
}

class GetterSetterRefactoringHelper
{
public:
    GetterSetterRefactoringHelper(CppQuickFixOperation *operation, Class *clazz)
        : m_operation(operation)
        , m_changes(m_operation->snapshot())
        , m_locator(m_changes)
        , m_headerFile(operation->currentFile())
        , m_sourceFile([&] {
            FilePath cppFilePath = correspondingHeaderOrSource(m_headerFile->filePath(),
                                                               &m_isHeaderHeaderFile);
            if (!m_isHeaderHeaderFile || !cppFilePath.exists()) {
                // there is no "source" file
                return m_headerFile;
            } else {
                return m_changes.cppFile(cppFilePath);
            }
        }())
        , m_class(clazz)
    {}

    void performGeneration(ExistingGetterSetterData data, int generationFlags);

    void applyChanges()
    {
        const auto classLayout = {
            InsertionPointLocator::Public,
            InsertionPointLocator::PublicSlot,
            InsertionPointLocator::Signals,
            InsertionPointLocator::Protected,
            InsertionPointLocator::ProtectedSlot,
            InsertionPointLocator::PrivateSlot,
            InsertionPointLocator::Private,
        };
        for (auto spec : classLayout) {
            const auto iter = m_headerFileCode.find(spec);
            if (iter != m_headerFileCode.end()) {
                const InsertionLocation loc = headerLocationFor(spec);
                m_headerFile->setOpenEditor(true, m_headerFile->position(loc.line(), loc.column()));
                insertAndIndent(m_headerFile, loc, *iter);
            }
        }
        if (!m_sourceFileCode.isEmpty() && m_sourceFileInsertionPoint.isValid()) {
            m_sourceFile->setOpenEditor(true, m_sourceFile->position(
                                                  m_sourceFileInsertionPoint.line(),
                                                  m_sourceFileInsertionPoint.column()));
            insertAndIndent(m_sourceFile, m_sourceFileInsertionPoint, m_sourceFileCode);
        }

        m_headerFile->apply(m_headerFileChangeSet);
        m_sourceFile->apply(m_sourceFileChangeSet);
    }

    bool hasSourceFile() const { return m_headerFile != m_sourceFile; }

protected:
    void insertAndIndent(const RefactoringFilePtr &file,
                         const InsertionLocation &loc,
                         const QString &text)
    {
        int targetPosition = file->position(loc.line(), loc.column());
        ChangeSet &changeSet = file == m_headerFile ? m_headerFileChangeSet : m_sourceFileChangeSet;
        changeSet.insert(targetPosition, loc.prefix() + text + loc.suffix());
    }

    FullySpecifiedType makeConstRef(FullySpecifiedType type)
    {
        type.setConst(true);
        return m_operation->currentFile()->cppDocument()->control()->referenceType(type, false);
    }

    FullySpecifiedType addConstToReference(FullySpecifiedType type)
    {
        if (ReferenceType *ref = type.type()->asReferenceType()) {
            FullySpecifiedType elemType = ref->elementType();
            if (elemType.isConst())
                return type;
            elemType.setConst(true);
            return m_operation->currentFile()->cppDocument()->control()->referenceType(elemType,
                                                                                       false);
        }
        return type;
    }

    QString symbolAt(Symbol *symbol,
                     const CppRefactoringFilePtr &targetFile,
                     InsertionLocation targetLocation)
    {
        return symbolAtDifferentLocation(*m_operation, symbol, targetFile, targetLocation);
    }

    FullySpecifiedType typeAt(FullySpecifiedType type,
                              Scope *originalScope,
                              const CppRefactoringFilePtr &targetFile,
                              InsertionLocation targetLocation,
                              const QStringList &newNamespaceNamesAtLoc = {})
    {
        return typeAtDifferentLocation(*m_operation,
                                       type,
                                       originalScope,
                                       targetFile,
                                       targetLocation,
                                       newNamespaceNamesAtLoc);
    }

    /**
     * @brief checks if the type in the enclosing scope in the header is a value type
     * @param type a type in the m_headerFile
     * @param enclosingScope the enclosing scope
     * @param customValueType if not nullptr set to true when value type comes
     * from CppQuickFixSettings::isValueType
     * @return true if it is a pointer, enum, integer, floating point, reference, custom value type
     */
    bool isValueType(FullySpecifiedType type, Scope *enclosingScope, bool *customValueType = nullptr)
    {
        if (customValueType)
            *customValueType = false;
        // a type is a value type if it is one of the following
        const auto isTypeValueType = [](const FullySpecifiedType &t) {
            return t->asPointerType() || t->asEnumType() || t->asIntegerType() || t->asFloatType()
                   || t->asReferenceType();
        };
        if (type->asNamedType()) {
            // we need a recursive search and a lookup context
            LookupContext context(m_headerFile->cppDocument(), m_changes.snapshot());
            auto isValueType = [settings = m_settings,
                                &customValueType,
                                &context,
                                &isTypeValueType](const Name *name,
                                                  Scope *scope,
                                                  auto &isValueType) {
                // maybe the type is a custom value type by name
                if (const Identifier *id = name->identifier()) {
                    if (settings->isValueType(QString::fromUtf8(id->chars(), id->size()))) {
                        if (customValueType)
                            *customValueType = true;
                        return true;
                    }
                }
                // search for the type declaration
                QList<LookupItem> localLookup = context.lookup(name, scope);
                for (auto &&i : localLookup) {
                    if (isTypeValueType(i.type()))
                        return true;
                    if (i.type()->asNamedType()) { // check if we have to search recursively
                        const Name *newName = i.type()->asNamedType()->name();
                        Scope *newScope = i.declaration()->enclosingScope();
                        if (Matcher::match(newName, name)
                            && Matcher::match(newScope->name(), scope->name())) {
                            continue; // we have found the start location of the search
                        }
                        return isValueType(newName, newScope, isValueType);
                    }
                    return false;
                }
                return false;
            };
            // start recursion
            return isValueType(type->asNamedType()->name(), enclosingScope, isValueType);
        }
        return isTypeValueType(type);
    }

    bool isValueType(Symbol *symbol, bool *customValueType = nullptr)
    {
        return isValueType(symbol->type(), symbol->enclosingScope(), customValueType);
    }

    void addHeaderCode(InsertionPointLocator::AccessSpec spec, QString code)
    {
        QString &existing = m_headerFileCode[spec];
        existing += code;
        if (!existing.endsWith('\n'))
            existing += '\n';
    }

    InsertionLocation headerLocationFor(InsertionPointLocator::AccessSpec spec)
    {
        const auto insertionPoint = m_headerInsertionPoints.find(spec);
        if (insertionPoint != m_headerInsertionPoints.end())
            return *insertionPoint;
        const InsertionLocation loc = m_locator.methodDeclarationInClass(
            m_headerFile->filePath(), m_class, spec,
            InsertionPointLocator::ForceAccessSpec::Yes);
        m_headerInsertionPoints.insert(spec, loc);
        return loc;
    }

    InsertionLocation sourceLocationFor(Symbol *symbol, QStringList *insertedNamespaces = nullptr)
    {
        if (m_sourceFileInsertionPoint.isValid())
            return m_sourceFileInsertionPoint;
        m_sourceFileInsertionPoint
            = insertLocationForMethodDefinition(symbol,
                                                false,
                                                m_settings->createMissingNamespacesinCppFile()
                                                    ? NamespaceHandling::CreateMissing
                                                    : NamespaceHandling::Ignore,
                                                m_changes,
                                                m_sourceFile->filePath(),
                                                insertedNamespaces);
        if (m_settings->addUsingNamespaceinCppFile()) {
            // check if we have to insert a using namespace ...
            auto requiredNamespaces = getNamespaceNames(
                symbol->asClass() ? symbol : symbol->enclosingClass());
            NSCheckerVisitor visitor(m_sourceFile.get(),
                                     requiredNamespaces,
                                     m_sourceFile->position(m_sourceFileInsertionPoint.line(),
                                                            m_sourceFileInsertionPoint.column()));
            visitor.accept(m_sourceFile->cppDocument()->translationUnit()->ast());
            if (insertedNamespaces)
                insertedNamespaces->clear();
            if (auto rns = visitor.remainingNamespaces(); !rns.empty()) {
                QString ns = "using namespace ";
                for (auto &n : rns) {
                    if (!n.isEmpty()) { // we have to ignore unnamed namespaces
                        ns += n;
                        ns += "::";
                        if (insertedNamespaces)
                            insertedNamespaces->append(n);
                    }
                }
                ns.resize(ns.size() - 2); // remove last '::'
                ns += ";\n";
                const auto &loc = m_sourceFileInsertionPoint;
                m_sourceFileInsertionPoint = InsertionLocation(loc.filePath(),
                                                               loc.prefix() + ns,
                                                               loc.suffix(),
                                                               loc.line(),
                                                               loc.column());
            }
        }
        return m_sourceFileInsertionPoint;
    }

    void addSourceFileCode(QString code)
    {
        while (!m_sourceFileCode.isEmpty() && !m_sourceFileCode.endsWith("\n\n"))
            m_sourceFileCode += '\n';
        m_sourceFileCode += code;
    }

protected:
    CppQuickFixOperation *const m_operation;
    const CppRefactoringChanges m_changes;
    const InsertionPointLocator m_locator;
    const CppRefactoringFilePtr m_headerFile;
    bool m_isHeaderHeaderFile = false; // the "header" (where the class is defined) can be a source file
    const CppRefactoringFilePtr m_sourceFile;
    CppQuickFixSettings *const m_settings = CppQuickFixProjectsSettings::getQuickFixSettings(
        ProjectTree::currentProject());
    Class *const m_class;

private:
    ChangeSet m_headerFileChangeSet;
    ChangeSet m_sourceFileChangeSet;
    QMap<InsertionPointLocator::AccessSpec, InsertionLocation> m_headerInsertionPoints;
    InsertionLocation m_sourceFileInsertionPoint;
    QString m_sourceFileCode;
    QMap<InsertionPointLocator::AccessSpec, QString> m_headerFileCode;
};

struct ParentClassConstructorInfo;

class ConstructorMemberInfo
{
public:
    ConstructorMemberInfo(const QString &name, Symbol *symbol, int numberOfMember)
        : memberVariableName(name)
        , parameterName(memberBaseName(name))
        , symbol(symbol)
        , type(symbol->type())
        , numberOfMember(numberOfMember)
    {}
    ConstructorMemberInfo(const QString &memberName,
                          const QString &paramName,
                          const QString &defaultValue,
                          Symbol *symbol,
                          const ParentClassConstructorInfo *parentClassConstructor)
        : parentClassConstructor(parentClassConstructor)
        , memberVariableName(memberName)
        , parameterName(paramName)
        , defaultValue(defaultValue)
        , init(defaultValue.isEmpty())
        , symbol(symbol)
        , type(symbol->type())
    {}
    const ParentClassConstructorInfo *parentClassConstructor = nullptr;
    QString memberVariableName;
    QString parameterName;
    QString defaultValue;
    bool init = true;
    bool customValueType; // for the generation later
    Symbol *symbol; // for the right type later
    FullySpecifiedType type;
    int numberOfMember; // first member, second member, ...
};

class ConstructorParams : public QAbstractTableModel
{
    Q_OBJECT
    std::list<ConstructorMemberInfo> candidates;
    std::vector<ConstructorMemberInfo *> infos;

    void validateOrder()
    {
        // parameters with default values must be at the end
        bool foundWithDefault = false;
        for (auto info : infos) {
            if (info->init) {
                if (foundWithDefault && info->defaultValue.isEmpty()) {
                    emit validOrder(false);
                    return;
                }
                foundWithDefault |= !info->defaultValue.isEmpty();
            }
        }
        emit validOrder(true);
    }

public:
    enum Column { ShouldInitColumn, MemberNameColumn, ParameterNameColumn, DefaultValueColumn };
    template<typename... _Args>
    void emplaceBackParameter(_Args &&...__args)
    {
        candidates.emplace_back(std::forward<_Args>(__args)...);
        infos.push_back(&candidates.back());
    }
    const std::vector<ConstructorMemberInfo *> &getInfos() const { return infos; }
    void addRow(ConstructorMemberInfo *info)
    {
        beginInsertRows({}, rowCount(), rowCount());
        infos.push_back(info);
        endInsertRows();
        validateOrder();
    }
    void removeRow(ConstructorMemberInfo *info)
    {
        for (auto iter = infos.begin(); iter != infos.end(); ++iter) {
            if (*iter == info) {
                const auto index = iter - infos.begin();
                beginRemoveRows({}, index, index);
                infos.erase(iter);
                endRemoveRows();
                validateOrder();
                return;
            }
        }
    }

    int selectedCount() const
    {
        return Utils::count(infos, [](const ConstructorMemberInfo *mi) {
            return mi->init && !mi->parentClassConstructor;
        });
    }
    int memberCount() const
    {
        return Utils::count(infos, [](const ConstructorMemberInfo *mi) {
            return !mi->parentClassConstructor;
        });
    }

    int rowCount(const QModelIndex & /*parent*/ = {}) const override { return int(infos.size()); }
    int columnCount(const QModelIndex & /*parent*/ = {}) const override { return 4; }
    QVariant data(const QModelIndex &index, int role) const override
    {
        if (index.row() < 0 || index.row() >= rowCount())
            return {};
        if (role == Qt::CheckStateRole && index.column() == ShouldInitColumn
            && !infos[index.row()]->parentClassConstructor)
            return infos[index.row()]->init ? Qt::Checked : Qt::Unchecked;
        if (role == Qt::DisplayRole && index.column() == MemberNameColumn)
            return infos[index.row()]->memberVariableName;
        if ((role == Qt::DisplayRole || role == Qt::EditRole)
            && index.column() == ParameterNameColumn)
            return infos[index.row()]->parameterName;
        if ((role == Qt::DisplayRole || role == Qt::EditRole)
            && index.column() == DefaultValueColumn)
            return infos[index.row()]->defaultValue;
        if ((role == Qt::ToolTipRole) && index.column() > 0)
            return Overview{}.prettyType(infos[index.row()]->symbol->type());
        return {};
    }
    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {
        if (index.column() == ShouldInitColumn && role == Qt::CheckStateRole) {
            if (infos[index.row()]->parentClassConstructor)
                return false;
            infos[index.row()]->init = value.toInt() == Qt::Checked;
            emit dataChanged(this->index(index.row(), 0), this->index(index.row(), columnCount()));
            validateOrder();
            return true;
        }
        if (index.column() == ParameterNameColumn && role == Qt::EditRole) {
            infos[index.row()]->parameterName = value.toString();
            return true;
        }
        if (index.column() == DefaultValueColumn && role == Qt::EditRole) {
            infos[index.row()]->defaultValue = value.toString();
            validateOrder();
            return true;
        }
        return false;
    }
    Qt::DropActions supportedDropActions() const override { return Qt::MoveAction; }
    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        if (!index.isValid())
            return Qt::ItemIsSelectable | Qt::ItemIsDropEnabled;

        Qt::ItemFlags f{};
        if (infos[index.row()]->init) {
            f |= Qt::ItemIsDragEnabled;
            f |= Qt::ItemIsSelectable;
        }

        if (index.column() == ShouldInitColumn && !infos[index.row()]->parentClassConstructor)
            return f | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
        if (!infos[index.row()]->init)
            return f;
        if (index.column() == MemberNameColumn)
            return f | Qt::ItemIsEnabled;
        if (index.column() == ParameterNameColumn || index.column() == DefaultValueColumn)
            return f | Qt::ItemIsEnabled | Qt::ItemIsEditable;
        return {};
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            switch (section) {
            case ShouldInitColumn:
                return Tr::tr("Initialize in Constructor");
            case MemberNameColumn:
                return Tr::tr("Member Name");
            case ParameterNameColumn:
                return Tr::tr("Parameter Name");
            case DefaultValueColumn:
                return Tr::tr("Default Value");
            }
        }
        return {};
    }
    bool dropMimeData(const QMimeData *data,
                      Qt::DropAction /*action*/,
                      int row,
                      int /*column*/,
                      const QModelIndex & /*parent*/) override
    {
        if (row == -1)
            row = rowCount();
        bool ok;
        int sourceRow = data->data("application/x-qabstractitemmodeldatalist").toInt(&ok);
        if (ok) {
            if (sourceRow == row || row == sourceRow + 1)
                return false;
            beginMoveRows({}, sourceRow, sourceRow, {}, row);
            infos.insert(infos.begin() + row, infos.at(sourceRow));
            if (row < sourceRow)
                ++sourceRow;
            infos.erase(infos.begin() + sourceRow);
            validateOrder();
            return true;
        }
        return false;
    }

    QMimeData *mimeData(const QModelIndexList &indexes) const override
    {
        for (const auto &i : indexes) {
            if (!i.isValid())
                continue;
            auto data = new QMimeData();
            data->setData("application/x-qabstractitemmodeldatalist",
                          QString::number(i.row()).toLatin1());
            return data;
        }
        return nullptr;
    }

    class TableViewStyle : public QProxyStyle
    {
    public:
        TableViewStyle(QStyle *style)
            : QProxyStyle(style)
        {}

        void drawPrimitive(PrimitiveElement element,
                           const QStyleOption *option,
                           QPainter *painter,
                           const QWidget *widget) const override
        {
            if (element == QStyle::PE_IndicatorItemViewItemDrop && !option->rect.isNull()) {
                QStyleOption opt(*option);
                opt.rect.setLeft(0);
                if (widget)
                    opt.rect.setRight(widget->width());
                QProxyStyle::drawPrimitive(element, &opt, painter, widget);
                return;
            }
            QProxyStyle::drawPrimitive(element, option, painter, widget);
        }
    };
signals:
    void validOrder(bool valid);
};

class TopMarginDelegate : public QStyledItemDelegate
{
public:
    TopMarginDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {}

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        Q_ASSERT(index.isValid());
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        const QWidget *widget = option.widget;
        QStyle *style = widget ? widget->style() : QApplication::style();
        if (opt.rect.height() > 20)
            opt.rect.adjust(0, 5, 0, 0);
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
    }
};

struct ParentClassConstructorParameter : public ConstructorMemberInfo
{
    QString originalDefaultValue;
    QString declaration; // displayed in the treeView
    ParentClassConstructorParameter(const QString &name,
                                    const QString &defaultValue,
                                    Symbol *symbol,
                                    const ParentClassConstructorInfo *parentClassConstructor);

    ParentClassConstructorParameter(const ParentClassConstructorParameter &) = delete;
    ParentClassConstructorParameter(ParentClassConstructorParameter &&) = default;
};

struct ParentClassConstructorInfo
{
    ParentClassConstructorInfo(const QString &name, ConstructorParams &model)
        : className(name)
        , model(model)
    {}
    bool useInConstructor = false;
    const QString className;
    QString declaration;
    std::vector<ParentClassConstructorParameter> parameters;
    ConstructorParams &model;

    ParentClassConstructorInfo(const ParentClassConstructorInfo &) = delete;
    ParentClassConstructorInfo(ParentClassConstructorInfo &&) = default;

    void addParameter(ParentClassConstructorParameter &param) { model.addRow(&param); }
    void removeParameter(ParentClassConstructorParameter &param) { model.removeRow(&param); }
    void removeAllParameters()
    {
        for (auto &param : parameters)
            model.removeRow(&param);
    }
};

ParentClassConstructorParameter::ParentClassConstructorParameter(
    const QString &name,
    const QString &defaultValue,
    Symbol *symbol,
    const ParentClassConstructorInfo *parentClassConstructor)
    : ConstructorMemberInfo(parentClassConstructor->className + "::" + name,
                            name,
                            defaultValue,
                            symbol,
                            parentClassConstructor)
    , originalDefaultValue(defaultValue)
    , declaration(Overview{}.prettyType(symbol->type(), name)
                  + (defaultValue.isEmpty() ? QString{} : " = " + defaultValue))
{}

using ParentClassConstructors = std::vector<ParentClassConstructorInfo>;

class ParentClassesModel : public QAbstractItemModel
{
    ParentClassConstructors &constructors;

public:
    ParentClassesModel(QObject *parent, ParentClassConstructors &constructors)
        : QAbstractItemModel(parent)
        , constructors(constructors)
    {}
    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override
    {
        if (!parent.isValid())
            return createIndex(row, column, nullptr);
        if (parent.internalPointer())
            return {};
        auto index = createIndex(row, column, &constructors.at(parent.row()));
        return index;
    }
    QModelIndex parent(const QModelIndex &index) const override
    {
        if (!index.isValid())
            return {};
        auto *parent = static_cast<ParentClassConstructorInfo *>(index.internalPointer());
        if (!parent)
            return {};
        int i = 0;
        for (const auto &info : constructors) {
            if (&info == parent)
                return createIndex(i, 0, nullptr);
            ++i;
        }
        return {};
    }
    int rowCount(const QModelIndex &parent = {}) const override
    {
        if (!parent.isValid())
            return static_cast<int>(constructors.size());
        auto info = static_cast<ParentClassConstructorInfo *>(parent.internalPointer());
        if (!info)
            return static_cast<int>(constructors.at(parent.row()).parameters.size());
        return 0;
    }
    int columnCount(const QModelIndex & /*parent*/ = {}) const override { return 1; }
    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid())
            return {};
        auto info = static_cast<ParentClassConstructorInfo *>(index.internalPointer());

        if (info) {
            const auto &parameter = info->parameters.at(index.row());
            if (role == Qt::CheckStateRole)
                return parameter.init ? Qt::Checked : Qt::Unchecked;
            if (role == Qt::DisplayRole)
                return parameter.declaration;
            return {};
        }
        const auto &constructor = constructors.at(index.row());
        if (role == Qt::CheckStateRole)
            return constructor.useInConstructor ? Qt::PartiallyChecked : Qt::Unchecked;
        if (role == Qt::DisplayRole)
            return constructor.declaration;

        // Highlight the selected items
        if (role == Qt::FontRole && constructor.useInConstructor) {
            QFont font = QApplication::font();
            font.setBold(true);
            return font;
        }
        // Create a margin between sets of constructors for base classes
        if (role == Qt::SizeHintRole && index.row() > 0
            && constructor.className != constructors.at(index.row() - 1).className) {
            return QSize(-1, 25);
        }
        return {};
    }
    bool setData(const QModelIndex &index, const QVariant &value, int /*role*/) override
    {
        if (index.isValid() && index.column() == 0) {
            auto info = static_cast<ParentClassConstructorInfo *>(index.internalPointer());
            if (info) {
                const bool nowUse = value.toBool();
                auto &param = info->parameters.at(index.row());
                param.init = nowUse;
                if (nowUse)
                    info->addParameter(param);
                else
                    info->removeParameter(param);
                return true;
            }
            auto &newConstructor = constructors.at(index.row());
            // You have to select a base class constructor
            if (newConstructor.useInConstructor)
                return false;
            auto c = std::find_if(constructors.begin(), constructors.end(), [&](const auto &c) {
                return c.className == newConstructor.className && c.useInConstructor;
            });
            QTC_ASSERT(c == constructors.end(), return false;);
            c->useInConstructor = false;
            newConstructor.useInConstructor = true;
            emit dataChanged(this->index(index.row(), 0), this->index(index.row(), columnCount()));
            auto parentIndex = this->index(index.row(), 0);
            emit dataChanged(this->index(0, 0, parentIndex),
                             this->index(rowCount(parentIndex), columnCount()));
            const int oldIndex = c - constructors.begin();
            emit dataChanged(this->index(oldIndex, 0), this->index(oldIndex, columnCount()));
            parentIndex = this->index(oldIndex, 0);
            emit dataChanged(this->index(0, 0, parentIndex),
                             this->index(rowCount(parentIndex), columnCount()));
            // update other table
            c->removeAllParameters();
            for (auto &p : newConstructor.parameters)
                if (p.init)
                    newConstructor.addParameter(p);
            return true;
        }
        return false;
    }
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            switch (section) {
            case 0:
                return Tr::tr("Base Class Constructors");
            }
        }
        return {};
    }
    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        if (index.isValid()) {
            Qt::ItemFlags f;
            auto info = static_cast<ParentClassConstructorInfo *>(index.internalPointer());
            if (!info || info->useInConstructor) {
                f |= Qt::ItemIsEnabled;
            }
            f |= Qt::ItemIsUserCheckable;

            return f;
        }
        return {};
    }
};

class GenerateConstructorDialog : public QDialog
{
public:
    GenerateConstructorDialog(ConstructorParams *constructorParamsModel,
                              ParentClassConstructors &constructors)
    {
        setWindowTitle(Tr::tr("Constructor"));

        const auto treeModel = new ParentClassesModel(this, constructors);
        const auto treeView = new QTreeView(this);
        treeView->setModel(treeModel);
        treeView->setItemDelegate(new TopMarginDelegate(this));
        treeView->expandAll();

        const auto view = new QTableView(this);
        view->setModel(constructorParamsModel);
        int optimalWidth = 0;
        for (int i = 0; i < constructorParamsModel->columnCount(QModelIndex{}); ++i) {
            view->resizeColumnToContents(i);
            optimalWidth += view->columnWidth(i);
        }
        view->resizeRowsToContents();
        view->verticalHeader()->setDefaultSectionSize(view->rowHeight(0));
        view->setSelectionBehavior(QAbstractItemView::SelectRows);
        view->setSelectionMode(QAbstractItemView::SingleSelection);
        view->setDragEnabled(true);
        view->setDropIndicatorShown(true);
        view->setDefaultDropAction(Qt::MoveAction);
        view->setDragDropMode(QAbstractItemView::InternalMove);
        view->setDragDropOverwriteMode(false);
        view->horizontalHeader()->setStretchLastSection(true);
        view->setStyle(new ConstructorParams::TableViewStyle(view->style()));

        const auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        const auto errorLabel = new QLabel(
            Tr::tr("Parameters without default value must come before parameters with default value."));
        errorLabel->setStyleSheet("color: #ff0000");
        errorLabel->setVisible(false);
        QSizePolicy labelSizePolicy = errorLabel->sizePolicy();
        labelSizePolicy.setRetainSizeWhenHidden(true);
        errorLabel->setSizePolicy(labelSizePolicy);
        connect(constructorParamsModel, &ConstructorParams::validOrder, this,
                [errorLabel, button = buttonBox->button(QDialogButtonBox::Ok)](bool valid) {
                    button->setEnabled(valid);
                    errorLabel->setVisible(!valid);
                });

        // setup select all/none checkbox
        QCheckBox *const checkBox = new QCheckBox(Tr::tr("Initialize all members"));
        checkBox->setChecked(true);
        connect(checkBox, &QCheckBox::stateChanged, this,
                [model = constructorParamsModel](int state) {
                    if (state != Qt::PartiallyChecked) {
                        for (int i = 0; i < model->rowCount(); ++i)
                            model->setData(model->index(i, ConstructorParams::ShouldInitColumn),
                                           state,
                                           Qt::CheckStateRole);
                    }
                });
        connect(checkBox, &QCheckBox::clicked, this, [checkBox] {
            if (checkBox->checkState() == Qt::PartiallyChecked)
                checkBox->setCheckState(Qt::Checked);
        });
        connect(constructorParamsModel,
                &QAbstractItemModel::dataChanged,
                this,
                [model = constructorParamsModel, checkBox] {
                    const auto state = [model, selectedCount = model->selectedCount()]() {
                        if (selectedCount == 0)
                            return Qt::Unchecked;
                        if (static_cast<int>(model->memberCount() == selectedCount))
                            return Qt::Checked;
                        return Qt::PartiallyChecked;
                    }();
                    checkBox->setCheckState(state);
                });

        using A = InsertionPointLocator::AccessSpec;
        auto accessCombo = new QComboBox;
        connect(accessCombo, &QComboBox::currentIndexChanged, this, [this, accessCombo] {
            const auto data = accessCombo->currentData();
            m_accessSpec = static_cast<A>(data.toInt());
        });
        for (auto a : {A::Public, A::Protected, A::Private})
            accessCombo->addItem(InsertionPointLocator::accessSpecToString(a), a);
        const auto row = new QHBoxLayout();
        row->addWidget(new QLabel(Tr::tr("Access") + ":"));
        row->addWidget(accessCombo);
        row->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum));

        const auto mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(
            new QLabel(Tr::tr("Select the members to be initialized in the constructor.\n"
                              "Use drag and drop to change the order of the parameters.")));
        mainLayout->addLayout(row);
        mainLayout->addWidget(checkBox);
        mainLayout->addWidget(view);
        mainLayout->addWidget(treeView);
        mainLayout->addWidget(errorLabel);
        mainLayout->addWidget(buttonBox);
        int left, right;
        mainLayout->getContentsMargins(&left, nullptr, &right, nullptr);
        optimalWidth += left + right;
        resize(optimalWidth, mainLayout->sizeHint().height());
    }

    InsertionPointLocator::AccessSpec accessSpec() const { return m_accessSpec; }

private:
    InsertionPointLocator::AccessSpec m_accessSpec;
};

class MemberInfo
{
public:
    MemberInfo(ExistingGetterSetterData data, int possibleFlags)
        : data(data)
        , possibleFlags(possibleFlags)
    {}

    ExistingGetterSetterData data;
    int possibleFlags;
    int requestedFlags = 0;
};
using GetterSetterCandidates = std::vector<MemberInfo>;

class GenerateConstructorOperation : public CppQuickFixOperation
{
public:
    GenerateConstructorOperation(const CppQuickFixInterface &interface)
        : CppQuickFixOperation(interface)
    {
        setDescription(Tr::tr("Generate Constructor"));

        m_classAST = astForClassOperations(interface);
        if (!m_classAST)
            return;
        Class *const theClass = m_classAST->symbol;
        if (!theClass)
            return;

        // Go through all members and find member variable declarations
        int memberCounter = 0;
        for (auto it = theClass->memberBegin(); it != theClass->memberEnd(); ++it) {
            Symbol *const s = *it;
            if (!s->identifier() || !s->type() || s->type().isTypedef())
                continue;
            if ((s->asDeclaration() && s->type()->asFunctionType()) || s->asFunction())
                continue;
            if (s->asDeclaration() && (s->isPrivate() || s->isProtected()) && !s->isStatic()) {
                const auto name = QString::fromUtf8(s->identifier()->chars(),
                                                    s->identifier()->size());
                parameterModel.emplaceBackParameter(name, s, memberCounter++);
            }
        }
        Overview o = CppCodeStyleSettings::currentProjectCodeStyleOverview();
        o.showArgumentNames = true;
        o.showReturnTypes = true;
        o.showDefaultArguments = true;
        o.showTemplateParameters = true;
        o.showFunctionSignatures = true;
        LookupContext context(currentFile()->cppDocument(), interface.snapshot());
        for (BaseClass *bc : theClass->baseClasses()) {
            const QString className = o.prettyName(bc->name());

            ClassOrNamespace *localLookupType = context.lookupType(bc);
            QList<LookupItem> localLookup = localLookupType->lookup(bc->name());
            for (auto &li : localLookup) {
                Symbol *d = li.declaration();
                if (!d->asClass())
                    continue;
                for (auto i = d->asClass()->memberBegin(); i != d->asClass()->memberEnd(); ++i) {
                    Symbol *s = *i;
                    if (s->isProtected() || s->isPublic()) {
                        if (s->name()->match(d->name())) {
                            // we have found a constructor
                            Function *func = s->type().type()->asFunctionType();
                            if (!func)
                                continue;
                            const bool isFirst = parentClassConstructors.empty()
                                                 || parentClassConstructors.back().className
                                                        != className;
                            parentClassConstructors.emplace_back(className, parameterModel);
                            ParentClassConstructorInfo &constructor = parentClassConstructors.back();
                            constructor.declaration = className + o.prettyType(func->type());
                            constructor.declaration.replace("std::__1::__get_nullptr_t()",
                                                            "nullptr");
                            constructor.useInConstructor = isFirst;
                            for (auto arg = func->memberBegin(); arg != func->memberEnd(); ++arg) {
                                Symbol *param = *arg;
                                Argument *argument = param->asArgument();
                                if (!argument) // can also be a block
                                    continue;
                                const QString name = o.prettyName(param->name());
                                const StringLiteral *ini = argument->initializer();
                                QString defaultValue;
                                if (ini)
                                    defaultValue = QString::fromUtf8(ini->chars(), ini->size())
                                                       .replace("std::__1::__get_nullptr_t()",
                                                                "nullptr");
                                constructor.parameters.emplace_back(name,
                                                                    defaultValue,
                                                                    param,
                                                                    &constructor);
                                // do not show constructors like QObject(QObjectPrivate & dd, ...)
                                ReferenceType *ref = param->type()->asReferenceType();
                                if (ref && name == "dd") {
                                    auto type = o.prettyType(ref->elementType());
                                    if (type.startsWith("Q") && type.endsWith("Private")) {
                                        parentClassConstructors.pop_back();
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // add params to parameter lists
        for (auto &c : parentClassConstructors)
            if (c.useInConstructor)
                for (auto &p : c.parameters)
                    if (p.init)
                        c.addParameter(p);
    }

    bool isApplicable() const
    {
        return parameterModel.rowCount() > 0
               || Utils::anyOf(parentClassConstructors,
                               [](const auto &parent) { return !parent.parameters.empty(); });
    }

    void setTest(bool isTest = true) { m_test = isTest; }

private:
    void perform() override
    {
        auto infos = parameterModel.getInfos();

        InsertionPointLocator::AccessSpec accessSpec = InsertionPointLocator::Public;
        if (!m_test) {
            GenerateConstructorDialog dlg(&parameterModel, parentClassConstructors);
            if (dlg.exec() == QDialog::Rejected)
                return;
            accessSpec = dlg.accessSpec();
            infos = parameterModel.getInfos();
        } else {
#ifdef WITH_TESTS
            ParentClassesModel model(nullptr, parentClassConstructors);
            QAbstractItemModelTester tester(&model);
#endif
            if (infos.size() >= 3) {
                // if we are testing and have 3 or more members => change the order
                // move first element to the back
                infos.push_back(infos[0]);
                infos.erase(infos.begin());
            }
            for (auto info : infos) {
                if (info->memberVariableName.startsWith("di_"))
                    info->defaultValue = "42";
            }
            for (auto &c : parentClassConstructors) {
                if (c.useInConstructor) {
                    for (auto &p : c.parameters) {
                        if (!p.init && p.parameterName.startsWith("use_")) {
                            infos.push_back(&p);
                            p.init = true;
                        }
                    }
                }
            }
        }
        if (infos.empty())
            return;
        struct GenerateConstructorRefactoringHelper : public GetterSetterRefactoringHelper
        {
            const ClassSpecifierAST *m_classAST;
            InsertionPointLocator::AccessSpec m_accessSpec;
            GenerateConstructorRefactoringHelper(CppQuickFixOperation *operation,
                                                 Class *clazz,
                                                 const ClassSpecifierAST *classAST,
                                                 InsertionPointLocator::AccessSpec accessSpec)
                : GetterSetterRefactoringHelper(operation, clazz)
                , m_classAST(classAST)
                , m_accessSpec(accessSpec)
            {}
            void generateConstructor(std::vector<ConstructorMemberInfo *> members,
                                     const ParentClassConstructors &parentClassConstructors)
            {
                auto constructorLocation = m_settings->determineSetterLocation(int(members.size()));
                if (constructorLocation == CppQuickFixSettings::FunctionLocation::CppFile
                    && !hasSourceFile())
                    constructorLocation = CppQuickFixSettings::FunctionLocation::OutsideClass;

                Overview overview = CppCodeStyleSettings::currentProjectCodeStyleOverview();
                overview.showTemplateParameters = true;

                InsertionLocation implLoc;
                QString implCode;
                CppRefactoringFilePtr implFile;
                QString className = overview.prettyName(m_class->name());
                QStringList insertedNamespaces;
                if (constructorLocation == CppQuickFixSettings::FunctionLocation::CppFile) {
                    implLoc = sourceLocationFor(m_class, &insertedNamespaces);
                    implFile = m_sourceFile;
                    if (m_settings->rewriteTypesinCppFile())
                        implCode = symbolAt(m_class, m_sourceFile, implLoc);
                    else
                        implCode = className;
                    implCode += "::" + className + "(";
                } else if (constructorLocation
                           == CppQuickFixSettings::FunctionLocation::OutsideClass) {
                    implLoc = insertLocationForMethodDefinition(m_class,
                                                                false,
                                                                NamespaceHandling::Ignore,
                                                                m_changes,
                                                                m_headerFile->filePath(),
                                                                &insertedNamespaces);
                    implFile = m_headerFile;
                    implCode = symbolAt(m_class, m_headerFile, implLoc);
                    implCode += "::" + className + "(";
                }

                QString inClassDeclaration = overview.prettyName(m_class->name()) + "(";
                QString constructorBody = members.empty() ? QString(") {}") : QString(") : ");
                for (auto &member : members) {
                    if (isValueType(member->symbol, &member->customValueType))
                        member->type.setConst(false);
                    else
                        member->type = makeConstRef(member->type);

                    inClassDeclaration += overview.prettyType(member->type, member->parameterName);
                    if (!member->defaultValue.isEmpty())
                        inClassDeclaration += " = " + member->defaultValue;
                    inClassDeclaration += ", ";
                    if (implFile) {
                        FullySpecifiedType type = typeAt(member->type,
                                                         m_class,
                                                         implFile,
                                                         implLoc,
                                                         insertedNamespaces);
                        implCode += overview.prettyType(type, member->parameterName) + ", ";
                    }
                }
                Utils::sort(members, &ConstructorMemberInfo::numberOfMember);
                // first, do the base classes
                for (const auto &parent : parentClassConstructors) {
                    if (!parent.useInConstructor)
                        continue;
                    // Check if we really need a constructor
                    if (Utils::anyOf(parent.parameters, [](const auto &param) {
                            return param.init || param.originalDefaultValue.isEmpty();
                        })) {
                        int defaultAtEndCount = 0;
                        for (auto i = parent.parameters.crbegin(); i != parent.parameters.crend();
                             ++i) {
                            if (i->init || i->originalDefaultValue.isEmpty())
                                break;
                            ++defaultAtEndCount;
                        }
                        const int numberOfParameters = static_cast<int>(parent.parameters.size())
                                                       - defaultAtEndCount;
                        constructorBody += parent.className + "(";
                        int counter = 0;
                        for (const auto &param : parent.parameters) {
                            if (++counter > numberOfParameters)
                                break;
                            if (param.init) {
                                if (param.customValueType)
                                    constructorBody += "std::move(" + param.parameterName + ')';
                                else
                                    constructorBody += param.parameterName;
                            } else if (!param.originalDefaultValue.isEmpty())
                                constructorBody += param.originalDefaultValue;
                            else
                                constructorBody += "/* insert value */";
                            constructorBody += ", ";
                        }
                        constructorBody.resize(constructorBody.length() - 2);
                        constructorBody += "),\n";
                    }
                }
                for (auto &member : members) {
                    if (member->parentClassConstructor)
                        continue;
                    QString param = member->parameterName;
                    if (member->customValueType)
                        param = "std::move(" + member->parameterName + ')';
                    constructorBody += member->memberVariableName + '(' + param + "),\n";
                }
                if (!members.empty()) {
                    inClassDeclaration.resize(inClassDeclaration.length() - 2);
                    constructorBody.remove(constructorBody.length() - 2, 1); // ..),\n => ..)\n
                    constructorBody += "{}";
                    if (!implCode.isEmpty())
                        implCode.resize(implCode.length() - 2);
                }
                implCode += constructorBody;

                if (constructorLocation == CppQuickFixSettings::FunctionLocation::InsideClass)
                    inClassDeclaration += constructorBody;
                else
                    inClassDeclaration += QLatin1String(");");

                TranslationUnit *tu = m_headerFile->cppDocument()->translationUnit();
                insertAndIndent(m_headerFile,
                                m_locator.constructorDeclarationInClass(tu,
                                                                        m_classAST,
                                                                        m_accessSpec,
                                                                        int(members.size())),
                                inClassDeclaration);

                if (constructorLocation == CppQuickFixSettings::FunctionLocation::CppFile) {
                    addSourceFileCode(implCode);
                } else if (constructorLocation
                           == CppQuickFixSettings::FunctionLocation::OutsideClass) {
                    if (m_isHeaderHeaderFile)
                        implCode.prepend("inline ");
                    insertAndIndent(m_headerFile, implLoc, implCode);
                }
            }
        };
        GenerateConstructorRefactoringHelper helper(this,
                                                    m_classAST->symbol,
                                                    m_classAST,
                                                    accessSpec);

        auto members = Utils::filtered(infos, [](const auto mi) {
            return mi->init || mi->parentClassConstructor;
        });
        helper.generateConstructor(std::move(members), parentClassConstructors);
        helper.applyChanges();
    }

    ConstructorParams parameterModel;
    ParentClassConstructors parentClassConstructors;
    const ClassSpecifierAST *m_classAST = nullptr;
    bool m_test = false;
};

class GenerateGetterSetterOp : public CppQuickFixOperation
{
public:
    enum GenerateFlag {
        GenerateGetter = 1 << 0,
        GenerateSetter = 1 << 1,
        GenerateSignal = 1 << 2,
        GenerateMemberVariable = 1 << 3,
        GenerateReset = 1 << 4,
        GenerateProperty = 1 << 5,
        GenerateConstantProperty = 1 << 6,
        HaveExistingQProperty = 1 << 7,
        Invalid = -1,
    };

    GenerateGetterSetterOp(const CppQuickFixInterface &interface,
                           ExistingGetterSetterData data,
                           int generateFlags,
                           int priority,
                           const QString &description)
        : CppQuickFixOperation(interface)
        , m_generateFlags(generateFlags)
        , m_data(data)
    {
        setDescription(description);
        setPriority(priority);
    }

    static void generateQuickFixes(QuickFixOperations &results,
                                   const CppQuickFixInterface &interface,
                                   const ExistingGetterSetterData &data,
                                   const int possibleFlags)
    {
        // flags can have the value HaveExistingQProperty or a combination of all other values
        // of the enum 'GenerateFlag'
        int p = 0;
        if (possibleFlags & HaveExistingQProperty) {
            const QString desc = Tr::tr("Generate Missing Q_PROPERTY Members");
            results << new GenerateGetterSetterOp(interface, data, possibleFlags, ++p, desc);
        } else {
            if (possibleFlags & GenerateSetter) {
                const QString desc = Tr::tr("Generate Setter");
                results << new GenerateGetterSetterOp(interface, data, GenerateSetter, ++p, desc);
            }
            if (possibleFlags & GenerateGetter) {
                const QString desc = Tr::tr("Generate Getter");
                results << new GenerateGetterSetterOp(interface, data, GenerateGetter, ++p, desc);
            }
            if (possibleFlags & GenerateGetter && possibleFlags & GenerateSetter) {
                const QString desc = Tr::tr("Generate Getter and Setter");
                const int flags = GenerateGetter | GenerateSetter;
                results << new GenerateGetterSetterOp(interface, data, flags, ++p, desc);
            }

            if (possibleFlags & GenerateConstantProperty) {
                const QString desc = Tr::tr("Generate Constant Q_PROPERTY and Missing Members");
                const int flags = possibleFlags & ~(GenerateSetter | GenerateSignal | GenerateReset);
                results << new GenerateGetterSetterOp(interface, data, flags, ++p, desc);
            }
            if (possibleFlags & GenerateProperty) {
                if (possibleFlags & GenerateReset) {
                    const QString desc = Tr::tr(
                        "Generate Q_PROPERTY and Missing Members with Reset Function");
                    const int flags = possibleFlags & ~GenerateConstantProperty;
                    results << new GenerateGetterSetterOp(interface, data, flags, ++p, desc);
                }
                const QString desc = Tr::tr("Generate Q_PROPERTY and Missing Members");
                const int flags = possibleFlags & ~GenerateConstantProperty & ~GenerateReset;
                results << new GenerateGetterSetterOp(interface, data, flags, ++p, desc);
            }
        }
    }

    void perform() override
    {
        GetterSetterRefactoringHelper helper(this, m_data.clazz);
        helper.performGeneration(m_data, m_generateFlags);
        helper.applyChanges();
    }

private:
    int m_generateFlags;
    ExistingGetterSetterData m_data;
};

class CandidateTreeItem : public TreeItem
{
public:
    enum Column {
        NameColumn,
        GetterColumn,
        SetterColumn,
        SignalColumn,
        ResetColumn,
        QPropertyColumn,
        ConstantQPropertyColumn
    };
    using Flag = GenerateGetterSetterOp::GenerateFlag;
    constexpr static Flag ColumnFlag[] = {
        Flag::Invalid,
        Flag::GenerateGetter,
        Flag::GenerateSetter,
        Flag::GenerateSignal,
        Flag::GenerateReset,
        Flag::GenerateProperty,
        Flag::GenerateConstantProperty,
    };

    CandidateTreeItem(MemberInfo *memberInfo)
        : m_memberInfo(memberInfo)
    {}

private:
    QVariant data(int column, int role) const override
    {
        if (role == Qt::DisplayRole && column == NameColumn)
            return m_memberInfo->data.memberVariableName;
        if (role == Qt::CheckStateRole && column > 0
            && column <= static_cast<int>(std::size(ColumnFlag))) {
            return m_memberInfo->requestedFlags & ColumnFlag[column] ? Qt::Checked : Qt::Unchecked;
        }
        return {};
    }

    bool setData(int column, const QVariant &data, int role) override
    {
        if (column < 1 || column > static_cast<int>(std::size(ColumnFlag)))
            return false;
        if (role != Qt::CheckStateRole)
            return false;
        if (!(m_memberInfo->possibleFlags & ColumnFlag[column]))
            return false;
        const bool nowChecked = data.toInt() == Qt::Checked;
        if (nowChecked)
            m_memberInfo->requestedFlags |= ColumnFlag[column];
        else
            m_memberInfo->requestedFlags &= ~ColumnFlag[column];

        if (nowChecked) {
            if (column == QPropertyColumn) {
                m_memberInfo->requestedFlags |= Flag::GenerateGetter;
                m_memberInfo->requestedFlags |= Flag::GenerateSetter;
                m_memberInfo->requestedFlags |= Flag::GenerateSignal;
                m_memberInfo->requestedFlags &= ~Flag::GenerateConstantProperty;
            } else if (column == ConstantQPropertyColumn) {
                m_memberInfo->requestedFlags |= Flag::GenerateGetter;
                m_memberInfo->requestedFlags &= ~Flag::GenerateSetter;
                m_memberInfo->requestedFlags &= ~Flag::GenerateSignal;
                m_memberInfo->requestedFlags &= ~Flag::GenerateReset;
                m_memberInfo->requestedFlags &= ~Flag::GenerateProperty;
            } else if (column == SetterColumn || column == SignalColumn || column == ResetColumn) {
                m_memberInfo->requestedFlags &= ~Flag::GenerateConstantProperty;
            }
        } else {
            if (column == SignalColumn)
                m_memberInfo->requestedFlags &= ~Flag::GenerateProperty;
        }
        for (int i = 0; i < 16; ++i) {
            const bool allowed = m_memberInfo->possibleFlags & (1 << i);
            if (!allowed)
                m_memberInfo->requestedFlags &= ~(1 << i); // clear bit
        }
        update();
        return true;
    }

    Qt::ItemFlags flags(int column) const override
    {
        if (column == NameColumn)
            return Qt::ItemIsEnabled;
        if (column < 1 || column > static_cast<int>(std::size(ColumnFlag)))
            return {};
        if (m_memberInfo->possibleFlags & ColumnFlag[column])
            return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
        return {};
    }

    MemberInfo *const m_memberInfo;
};

class GenerateGettersSettersDialog : public QDialog
{
    static constexpr CandidateTreeItem::Column CheckBoxColumn[4]
        = {CandidateTreeItem::Column::GetterColumn,
           CandidateTreeItem::Column::SetterColumn,
           CandidateTreeItem::Column::SignalColumn,
           CandidateTreeItem::Column::QPropertyColumn};

public:
    GenerateGettersSettersDialog(const GetterSetterCandidates &candidates)
        : QDialog()
        , m_candidates(candidates)
    {
        using Flags = GenerateGetterSetterOp::GenerateFlag;
        setWindowTitle(Tr::tr("Getters and Setters"));
        const auto model = new TreeModel<TreeItem, CandidateTreeItem>(this);
        model->setHeader(QStringList({
            Tr::tr("Member"),
            Tr::tr("Getter"),
            Tr::tr("Setter"),
            Tr::tr("Signal"),
            Tr::tr("Reset"),
            Tr::tr("QProperty"),
            Tr::tr("Constant QProperty"),
        }));
        for (MemberInfo &candidate : m_candidates)
            model->rootItem()->appendChild(new CandidateTreeItem(&candidate));
        const auto view = new BaseTreeView(this);
        view->setModel(model);
        int optimalWidth = 0;
        for (int i = 0; i < model->columnCount(QModelIndex{}); ++i) {
            view->resizeColumnToContents(i);
            optimalWidth += view->columnWidth(i);
        }

        const auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        const auto setCheckStateForAll = [model](int column, int checkState) {
            for (int i = 0; i < model->rowCount(); ++i)
                model->setData(model->index(i, column), checkState, Qt::CheckStateRole);
        };
        const auto preventPartiallyChecked = [](QCheckBox *checkbox) {
            if (checkbox->checkState() == Qt::PartiallyChecked)
                checkbox->setCheckState(Qt::Checked);
        };
        using Column = CandidateTreeItem::Column;
        const auto createConnections = [this, setCheckStateForAll, preventPartiallyChecked](
                                           QCheckBox *checkbox, Column column) {
            connect(checkbox, &QCheckBox::stateChanged, this, [setCheckStateForAll, column](int state) {
                if (state != Qt::PartiallyChecked)
                    setCheckStateForAll(column, state);
            });
            connect(checkbox, &QCheckBox::clicked, this, [checkbox, preventPartiallyChecked] {
                preventPartiallyChecked(checkbox);
            });
        };
        std::array<QCheckBox *, 4> checkBoxes = {};

        static_assert(std::size(CheckBoxColumn) == checkBoxes.size(),
                      "Must contain the same number of elements");
        for (std::size_t i = 0; i < checkBoxes.size(); ++i) {
            if (Utils::anyOf(candidates, [i](const MemberInfo &mi) {
                    return mi.possibleFlags & CandidateTreeItem::ColumnFlag[CheckBoxColumn[i]];
                })) {
                const Column column = CheckBoxColumn[i];
                if (column == Column::GetterColumn)
                    checkBoxes[i] = new QCheckBox(Tr::tr("Create getters for all members"));
                else if (column == Column::SetterColumn)
                    checkBoxes[i] = new QCheckBox(Tr::tr("Create setters for all members"));
                else if (column == Column::SignalColumn)
                    checkBoxes[i] = new QCheckBox(Tr::tr("Create signals for all members"));
                else if (column == Column::QPropertyColumn)
                    checkBoxes[i] = new QCheckBox(Tr::tr("Create Q_PROPERTY for all members"));

                createConnections(checkBoxes[i], column);
            }
        }
        connect(model, &QAbstractItemModel::dataChanged, this, [this, checkBoxes] {
            const auto countExisting = [this](Flags flag) {
                return Utils::count(m_candidates, [flag](const MemberInfo &mi) {
                    return !(mi.possibleFlags & flag);
                });
            };
            const auto countRequested = [this](Flags flag) {
                return Utils::count(m_candidates, [flag](const MemberInfo &mi) {
                    return mi.requestedFlags & flag;
                });
            };
            const auto countToState = [this](int requestedCount, int alreadyExistsCount) {
                if (requestedCount == 0)
                    return Qt::Unchecked;
                if (int(m_candidates.size()) - requestedCount == alreadyExistsCount)
                    return Qt::Checked;
                return Qt::PartiallyChecked;
            };
            for (std::size_t i = 0; i < checkBoxes.size(); ++i) {
                if (checkBoxes[i]) {
                    const Flags flag = CandidateTreeItem::ColumnFlag[CheckBoxColumn[i]];
                    checkBoxes[i]->setCheckState(
                        countToState(countRequested(flag), countExisting(flag)));
                }
            }
        });

        const auto mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(new QLabel(Tr::tr("Select the getters and setters "
                                                "to be created.")));
        for (auto checkBox : checkBoxes) {
            if (checkBox)
                mainLayout->addWidget(checkBox);
        }
        mainLayout->addWidget(view);
        mainLayout->addWidget(buttonBox);
        int left, right;
        mainLayout->getContentsMargins(&left, nullptr, &right, nullptr);
        optimalWidth += left + right;
        resize(optimalWidth, mainLayout->sizeHint().height());
    }

    GetterSetterCandidates candidates() const { return m_candidates; }

private:
    GetterSetterCandidates m_candidates;
};

class GenerateGettersSettersOperation : public CppQuickFixOperation
{
public:
    GenerateGettersSettersOperation(const CppQuickFixInterface &interface)
        : CppQuickFixOperation(interface)
    {
        setDescription(Tr::tr("Create Getter and Setter Member Functions"));

        m_classAST = astForClassOperations(interface);
        if (!m_classAST)
            return;
        Class * const theClass = m_classAST->symbol;
        if (!theClass)
            return;

        // Go through all data members and try to find out whether they have getters and/or setters.
        QList<Symbol *> dataMembers;
        QList<Symbol *> memberFunctions;
        for (auto it = theClass->memberBegin(); it != theClass->memberEnd(); ++it) {
            Symbol *const s = *it;
            if (!s->identifier() || !s->type() || s->type().isTypedef())
                continue;
            if ((s->asDeclaration() && s->type()->asFunctionType()) || s->asFunction())
                memberFunctions << s;
            else if (s->asDeclaration() && (s->isPrivate() || s->isProtected()))
                dataMembers << s;
        }

        auto file = interface.currentFile();
        QStringList qPropertyNames; // name after MEMBER or name of the property
        for (auto it = m_classAST->member_specifier_list; it; it = it->next) {
            if (it->value->asQtPropertyDeclaration()) {
                auto propDecl = it->value->asQtPropertyDeclaration();
                // iterator over 'READ ...', ... and check if we have a MEMBER
                for (auto p = propDecl->property_declaration_item_list; p; p = p->next) {
                    const char *tokenString = file->tokenAt(p->value->item_name_token).spell();
                    if (!qstrcmp(tokenString, "MEMBER"))
                        qPropertyNames << file->textOf(p->value->expression);
                }
                // no MEMBER, but maybe the property name is the same
                qPropertyNames << file->textOf(propDecl->property_name);
            }
        }
        const QStringList memberFunctionsAsStrings = toStringList(memberFunctions);

        for (Symbol *const member : std::as_const(dataMembers)) {
            ExistingGetterSetterData existing;
            existing.memberVariableName = QString::fromUtf8(member->identifier()->chars(),
                                                            member->identifier()->size());
            existing.declarationSymbol = member->asDeclaration();
            existing.clazz = theClass;

            // check if a Q_PROPERTY exist
            const QString baseName = memberBaseName(existing.memberVariableName);
            if (qPropertyNames.contains(baseName)
                || qPropertyNames.contains(existing.memberVariableName))
                continue;

            findExistingFunctions(existing, memberFunctionsAsStrings);
            existing.qPropertyName = baseName;

            int possibleFlags = existing.computePossibleFlags();
            if (possibleFlags == 0)
                continue;
            m_candidates.emplace_back(existing, possibleFlags);
        }
    }

    GetterSetterCandidates candidates() const { return m_candidates; }
    bool isApplicable() const { return !m_candidates.empty(); }

    void setGetterSetterData(const GetterSetterCandidates &data)
    {
        m_candidates = data;
        m_hasData = true;
    }

private:
    void perform() override
    {
        if (!m_hasData) {
            GenerateGettersSettersDialog dlg(m_candidates);
            if (dlg.exec() == QDialog::Rejected)
                return;
            m_candidates = dlg.candidates();
        }
        if (m_candidates.empty())
            return;
        GetterSetterRefactoringHelper helper(this, m_candidates.front().data.clazz);
        for (MemberInfo &mi : m_candidates) {
            if (mi.requestedFlags != 0) {
                helper.performGeneration(mi.data, mi.requestedFlags);
            }
        }
        helper.applyChanges();
    }

    GetterSetterCandidates m_candidates;
    const ClassSpecifierAST *m_classAST = nullptr;
    bool m_hasData = false;
};

int ExistingGetterSetterData::computePossibleFlags() const
{
    const bool isConst = declarationSymbol->type().isConst();
    const bool isStatic = declarationSymbol->type().isStatic();
    using Flag = GenerateGetterSetterOp::GenerateFlag;
    int generateFlags = 0;
    if (getterName.isEmpty())
        generateFlags |= Flag::GenerateGetter;
    if (!isConst) {
        if (resetName.isEmpty())
            generateFlags |= Flag::GenerateReset;
        if (!isStatic && signalName.isEmpty() && setterName.isEmpty())
            generateFlags |= Flag::GenerateSignal;
        if (setterName.isEmpty())
            generateFlags |= Flag::GenerateSetter;
    }
    if (!isStatic) {
        const bool hasSignal = !signalName.isEmpty() || generateFlags & Flag::GenerateSignal;
        if (!isConst && hasSignal)
            generateFlags |= Flag::GenerateProperty;
    }
    if (setterName.isEmpty() && signalName.isEmpty())
        generateFlags |= Flag::GenerateConstantProperty;
    return generateFlags;
}

void GetterSetterRefactoringHelper::performGeneration(ExistingGetterSetterData data, int generateFlags)
{
    using Flag = GenerateGetterSetterOp::GenerateFlag;

    if (generateFlags & Flag::GenerateGetter && data.getterName.isEmpty()) {
        data.getterName = m_settings->getGetterName(data.qPropertyName);
        if (data.getterName == data.memberVariableName) {
            data.getterName = "get" + data.memberVariableName.left(1).toUpper()
                              + data.memberVariableName.mid(1);
        }
    }
    if (generateFlags & Flag::GenerateSetter && data.setterName.isEmpty())
        data.setterName = m_settings->getSetterName(data.qPropertyName);
    if (generateFlags & Flag::GenerateSignal && data.signalName.isEmpty())
        data.signalName = m_settings->getSignalName(data.qPropertyName);
    if (generateFlags & Flag::GenerateReset && data.resetName.isEmpty())
        data.resetName = m_settings->getResetName(data.qPropertyName);

    FullySpecifiedType memberVariableType = data.declarationSymbol->type();
    memberVariableType.setConst(false);
    const bool isMemberVariableStatic =  data.declarationSymbol->isStatic();
    memberVariableType.setStatic(false);
    Overview overview = CppCodeStyleSettings::currentProjectCodeStyleOverview();
    overview.showTemplateParameters = false;
    // TODO does not work with using. e.g. 'using foo = std::unique_ptr<int>'
    // TODO must be fully qualified
    auto getSetTemplate = m_settings->findGetterSetterTemplate(overview.prettyType(memberVariableType));
    overview.showTemplateParameters = true;

    // Ok... - If a type is a Named type we have to search recusive for the real type
    const bool isValueType = this->isValueType(memberVariableType,
                                               data.declarationSymbol->enclosingScope());
    const FullySpecifiedType parameterType = isValueType ? memberVariableType
                                                         : makeConstRef(memberVariableType);

    QString baseName = memberBaseName(data.memberVariableName);
    if (baseName.isEmpty())
        baseName = data.memberVariableName;

    const QString parameterName = m_settings->getSetterParameterName(baseName);
    if (parameterName == data.memberVariableName)
        data.memberVariableName = "this->" + data.memberVariableName;

    getSetTemplate.replacePlaceholders(data.memberVariableName, parameterName);

    using Pattern = CppQuickFixSettings::GetterSetterTemplate;
    std::optional<FullySpecifiedType> returnTypeTemplateParameter;
    if (getSetTemplate.returnTypeTemplate.has_value()) {
        QString returnTypeTemplate = getSetTemplate.returnTypeTemplate.value();
        if (returnTypeTemplate.contains(Pattern::TEMPLATE_PARAMETER_PATTERN)) {
            returnTypeTemplateParameter = getFirstTemplateParameter(data.declarationSymbol->type());
            if (!returnTypeTemplateParameter.has_value())
                return; // Maybe report error to the user
        }
    }
    const FullySpecifiedType returnTypeHeader = [&] {
        if (!getSetTemplate.returnTypeTemplate.has_value())
            return m_settings->returnByConstRef ? parameterType : memberVariableType;
        QString typeTemplate = getSetTemplate.returnTypeTemplate.value();
        if (returnTypeTemplateParameter.has_value())
            typeTemplate.replace(Pattern::TEMPLATE_PARAMETER_PATTERN,
                                 overview.prettyType(returnTypeTemplateParameter.value()));
        if (typeTemplate.contains(Pattern::TYPE_PATTERN))
            typeTemplate.replace(Pattern::TYPE_PATTERN,
                                 overview.prettyType(data.declarationSymbol->type()));
        Control *control = m_operation->currentFile()->cppDocument()->control();
        std::string utf8TypeName = typeTemplate.toUtf8().toStdString();
        return FullySpecifiedType(control->namedType(control->identifier(utf8TypeName.c_str())));
    }();

    // getter declaration
    if (generateFlags & Flag::GenerateGetter) {
        // maybe we added 'this->' to memberVariableName because of a collision with parameterName
        // but here the 'this->' is not needed
        const QString returnExpression = QString{getSetTemplate.returnExpression}.replace("this->",
                                                                                          "");
        QString getterInClassDeclaration = overview.prettyType(returnTypeHeader, data.getterName)
                                           + QLatin1String("()");
        if (isMemberVariableStatic)
            getterInClassDeclaration.prepend(QLatin1String("static "));
        else
            getterInClassDeclaration += QLatin1String(" const");
        getterInClassDeclaration.prepend(m_settings->getterAttributes + QLatin1Char(' '));

        auto getterLocation = m_settings->determineGetterLocation(1);
        // if we have an anonymous class we must add code inside the class
        if (data.clazz->name()->asAnonymousNameId())
            getterLocation = CppQuickFixSettings::FunctionLocation::InsideClass;

        if (getterLocation == CppQuickFixSettings::FunctionLocation::InsideClass) {
            getterInClassDeclaration += QLatin1String("\n{\nreturn ") + returnExpression
                                        + QLatin1String(";\n}\n");
        } else {
            getterInClassDeclaration += QLatin1String(";\n");
        }
        addHeaderCode(InsertionPointLocator::Public, getterInClassDeclaration);
        if (getterLocation == CppQuickFixSettings::FunctionLocation::CppFile && !hasSourceFile())
            getterLocation = CppQuickFixSettings::FunctionLocation::OutsideClass;

        if (getterLocation != CppQuickFixSettings::FunctionLocation::InsideClass) {
            const auto getReturnTypeAt = [&](CppRefactoringFilePtr targetFile,
                                             InsertionLocation targetLoc) {
                if (getSetTemplate.returnTypeTemplate.has_value()) {
                    QString returnType = getSetTemplate.returnTypeTemplate.value();
                    if (returnTypeTemplateParameter.has_value()) {
                        const QString templateTypeName = overview.prettyType(typeAt(
                            returnTypeTemplateParameter.value(), data.clazz, targetFile, targetLoc));
                        returnType.replace(Pattern::TEMPLATE_PARAMETER_PATTERN, templateTypeName);
                    }
                    if (returnType.contains(Pattern::TYPE_PATTERN)) {
                        const QString declarationType = overview.prettyType(
                            typeAt(memberVariableType, data.clazz, targetFile, targetLoc));
                        returnType.replace(Pattern::TYPE_PATTERN, declarationType);
                    }
                    Control *control = m_operation->currentFile()->cppDocument()->control();
                    std::string utf8String = returnType.toUtf8().toStdString();
                    return FullySpecifiedType(
                        control->namedType(control->identifier(utf8String.c_str())));
                } else {
                    FullySpecifiedType returnType = typeAt(memberVariableType,
                                                           data.clazz,
                                                           targetFile,
                                                           targetLoc);
                    if (m_settings->returnByConstRef && !isValueType)
                        return makeConstRef(returnType);
                    return returnType;
                }
            };
            const QString constSpec = isMemberVariableStatic ? QLatin1String("")
                                                             : QLatin1String(" const");
            if (getterLocation == CppQuickFixSettings::FunctionLocation::CppFile) {
                InsertionLocation loc = sourceLocationFor(data.declarationSymbol);
                FullySpecifiedType returnType;
                QString clazz;
                if (m_settings->rewriteTypesinCppFile()) {
                    returnType = getReturnTypeAt(m_sourceFile, loc);
                    clazz = symbolAt(data.clazz, m_sourceFile, loc);
                } else {
                    returnType = returnTypeHeader;
                    const Identifier *identifier = data.clazz->name()->identifier();
                    clazz = QString::fromUtf8(identifier->chars(), identifier->size());
                }
                const QString code = overview.prettyType(returnType, clazz + "::" + data.getterName)
                                     + "()" + constSpec + "\n{\nreturn " + returnExpression + ";\n}";
                addSourceFileCode(code);
            } else if (getterLocation == CppQuickFixSettings::FunctionLocation::OutsideClass) {
                InsertionLocation loc
                    = insertLocationForMethodDefinition(data.declarationSymbol,
                                                        false,
                                                        NamespaceHandling::Ignore,
                                                        m_changes,
                                                        m_headerFile->filePath());
                const FullySpecifiedType returnType = getReturnTypeAt(m_headerFile, loc);
                const QString clazz = symbolAt(data.clazz, m_headerFile, loc);
                QString code = overview.prettyType(returnType, clazz + "::" + data.getterName)
                               + "()" + constSpec + "\n{\nreturn " + returnExpression + ";\n}";
                if (m_isHeaderHeaderFile)
                    code.prepend("inline ");
                insertAndIndent(m_headerFile, loc, code);
            }
        }
    }

    // setter declaration
    InsertionPointLocator::AccessSpec setterAccessSpec = InsertionPointLocator::Public;
    if (m_settings->setterAsSlot) {
        const QByteArray connectName = "connect";
        const Identifier connectId(connectName.data(), connectName.size());
        const QList<LookupItem> items = m_operation->context().lookup(&connectId, data.clazz);
        for (const LookupItem &item : items) {
            if (item.declaration() && item.declaration()->enclosingClass()
                && overview.prettyName(item.declaration()->enclosingClass()->name())
                       == "QObject") {
                setterAccessSpec = InsertionPointLocator::PublicSlot;
                break;
            }
        }
    }
    const auto createSetterBodyWithSignal = [this, &getSetTemplate, &data] {
        QString body;
        QTextStream setter(&body);
        setter << "if (" << getSetTemplate.equalComparison << ")\nreturn;\n";

        setter << getSetTemplate.assignment << ";\n";
        if (m_settings->signalWithNewValue)
            setter << "emit " << data.signalName << "(" << getSetTemplate.returnExpression << ");\n";
        else
            setter << "emit " << data.signalName << "();\n";

        return body;
    };
    if (generateFlags & Flag::GenerateSetter) {
        QString headerDeclaration = "void " + data.setterName + '('
                                    + overview.prettyType(addConstToReference(parameterType),
                                                          parameterName)
                                    + ")";
        if (isMemberVariableStatic)
            headerDeclaration.prepend("static ");
        QString body = "\n{\n";
        if (data.signalName.isEmpty())
            body += getSetTemplate.assignment + ";\n";
        else
            body += createSetterBodyWithSignal();

        body += "}";

        auto setterLocation = m_settings->determineSetterLocation(body.count('\n') - 2);
        // if we have an anonymous class we must add code inside the class
        if (data.clazz->name()->asAnonymousNameId())
            setterLocation = CppQuickFixSettings::FunctionLocation::InsideClass;

        if (setterLocation == CppQuickFixSettings::FunctionLocation::CppFile && !hasSourceFile())
            setterLocation = CppQuickFixSettings::FunctionLocation::OutsideClass;

        if (setterLocation == CppQuickFixSettings::FunctionLocation::InsideClass) {
            headerDeclaration += body;
        } else {
            headerDeclaration += ";\n";
            if (setterLocation == CppQuickFixSettings::FunctionLocation::CppFile) {
                InsertionLocation loc = sourceLocationFor(data.declarationSymbol);
                QString clazz;
                FullySpecifiedType newParameterType = parameterType;
                if (m_settings->rewriteTypesinCppFile()) {
                    newParameterType = typeAt(memberVariableType, data.clazz, m_sourceFile, loc);
                    if (!isValueType)
                        newParameterType = makeConstRef(newParameterType);
                    clazz = symbolAt(data.clazz, m_sourceFile, loc);
                } else {
                    const Identifier *identifier = data.clazz->name()->identifier();
                    clazz = QString::fromUtf8(identifier->chars(), identifier->size());
                }
                newParameterType = addConstToReference(newParameterType);
                const QString code = "void " + clazz + "::" + data.setterName + '('
                                     + overview.prettyType(newParameterType, parameterName) + ')'
                                     + body;
                addSourceFileCode(code);
            } else if (setterLocation == CppQuickFixSettings::FunctionLocation::OutsideClass) {
                InsertionLocation loc
                    = insertLocationForMethodDefinition(data.declarationSymbol,
                                                        false,
                                                        NamespaceHandling::Ignore,
                                                        m_changes,
                                                        m_headerFile->filePath());

                FullySpecifiedType newParameterType = typeAt(data.declarationSymbol->type(),
                                                             data.clazz,
                                                             m_headerFile,
                                                             loc);
                if (!isValueType)
                    newParameterType = makeConstRef(newParameterType);
                newParameterType = addConstToReference(newParameterType);
                QString clazz = symbolAt(data.clazz, m_headerFile, loc);

                QString code = "void " + clazz + "::" + data.setterName + '('
                               + overview.prettyType(newParameterType, parameterName) + ')' + body;
                if (m_isHeaderHeaderFile)
                    code.prepend("inline ");
                insertAndIndent(m_headerFile, loc, code);
            }
        }
        addHeaderCode(setterAccessSpec, headerDeclaration);
    }

    // reset declaration
    if (generateFlags & Flag::GenerateReset) {
        QString headerDeclaration = "void " + data.resetName + "()";
        if (isMemberVariableStatic)
            headerDeclaration.prepend("static ");
        QString body = "\n{\n";
        if (!data.setterName.isEmpty()) {
            body += data.setterName + "({}); // TODO: Adapt to use your actual default value\n";
        } else {
            body += "static $TYPE defaultValue{}; "
                    "// TODO: Adapt to use your actual default value\n";
            if (data.signalName.isEmpty())
                body += getSetTemplate.assignment + ";\n";
            else
                body += createSetterBodyWithSignal();
        }
        body += "}";

        // the template use <parameterName> as new value name, but we want to use 'defaultValue'
        body.replace(QRegularExpression("\\b" + parameterName + "\\b"), "defaultValue");
        // body.count('\n') - 2 : do not count the 2 at start
        auto resetLocation = m_settings->determineSetterLocation(body.count('\n') - 2);
        // if we have an anonymous class we must add code inside the class
        if (data.clazz->name()->asAnonymousNameId())
            resetLocation = CppQuickFixSettings::FunctionLocation::InsideClass;

        if (resetLocation == CppQuickFixSettings::FunctionLocation::CppFile && !hasSourceFile())
            resetLocation = CppQuickFixSettings::FunctionLocation::OutsideClass;

        if (resetLocation == CppQuickFixSettings::FunctionLocation::InsideClass) {
            headerDeclaration += body.replace("$TYPE", overview.prettyType(memberVariableType));
        } else {
            headerDeclaration += ";\n";
            if (resetLocation == CppQuickFixSettings::FunctionLocation::CppFile) {
                const InsertionLocation loc = sourceLocationFor(data.declarationSymbol);
                QString clazz;
                FullySpecifiedType type = memberVariableType;
                if (m_settings->rewriteTypesinCppFile()) {
                    type = typeAt(memberVariableType, data.clazz, m_sourceFile, loc);
                    clazz = symbolAt(data.clazz, m_sourceFile, loc);
                } else {
                    const Identifier *identifier = data.clazz->name()->identifier();
                    clazz = QString::fromUtf8(identifier->chars(), identifier->size());
                }
                const QString code = "void " + clazz + "::" + data.resetName + "()"
                                     + body.replace("$TYPE", overview.prettyType(type));
                addSourceFileCode(code);
            } else if (resetLocation == CppQuickFixSettings::FunctionLocation::OutsideClass) {
                const InsertionLocation loc = insertLocationForMethodDefinition(
                    data.declarationSymbol,
                    false,
                    NamespaceHandling::Ignore,
                    m_changes,
                    m_headerFile->filePath());
                const FullySpecifiedType type = typeAt(data.declarationSymbol->type(),
                                                       data.clazz,
                                                       m_headerFile,
                                                       loc);
                const QString clazz = symbolAt(data.clazz, m_headerFile, loc);
                QString code = "void " + clazz + "::" + data.resetName + "()"
                               + body.replace("$TYPE", overview.prettyType(type));
                if (m_isHeaderHeaderFile)
                    code.prepend("inline ");
                insertAndIndent(m_headerFile, loc, code);
            }
        }
        addHeaderCode(setterAccessSpec, headerDeclaration);
    }

    // signal declaration
    if (generateFlags & Flag::GenerateSignal) {
        const auto &parameter = overview.prettyType(returnTypeHeader, data.qPropertyName);
        const QString newValue = m_settings->signalWithNewValue ? parameter : QString();
        const QString declaration = QString("void %1(%2);\n").arg(data.signalName, newValue);
        addHeaderCode(InsertionPointLocator::Signals, declaration);
    }

    // member variable
    if (generateFlags & Flag::GenerateMemberVariable) {
        QString storageDeclaration = overview.prettyType(memberVariableType, data.memberVariableName);
        if (memberVariableType->asPointerType()
            && m_operation->semanticInfo().doc->translationUnit()->languageFeatures().cxx11Enabled) {
            storageDeclaration.append(" = nullptr");
        }
        storageDeclaration.append(";\n");
        addHeaderCode(InsertionPointLocator::Private, storageDeclaration);
    }

    // Q_PROPERTY
    if (generateFlags & Flag::GenerateProperty || generateFlags & Flag::GenerateConstantProperty) {
        // Use the returnTypeHeader as base because of custom types in getSetTemplates.
        // Remove const reference from type.
        FullySpecifiedType type = returnTypeHeader;
        if (ReferenceType *ref = type.type()->asReferenceType())
            type = ref->elementType();
        type.setConst(false);

        QString propertyDeclaration = QLatin1String("Q_PROPERTY(")
                                      + overview.prettyType(type,
                                                            memberBaseName(data.memberVariableName));
        bool needMember = false;
        if (data.getterName.isEmpty())
            needMember = true;
        else
            propertyDeclaration += QLatin1String(" READ ") + data.getterName;
        if (generateFlags & Flag::GenerateConstantProperty) {
            if (needMember)
                propertyDeclaration += QLatin1String(" MEMBER ") + data.memberVariableName;
            propertyDeclaration.append(QLatin1String(" CONSTANT"));
        } else {
            if (data.setterName.isEmpty()) {
                needMember = true;
            } else if (!getSetTemplate.returnTypeTemplate.has_value()) {
                // if the return type of the getter and then Q_PROPERTY is different than
                // the setter type, we should not add WRITE to the Q_PROPERTY
                propertyDeclaration.append(QLatin1String(" WRITE ")).append(data.setterName);
            }
            if (needMember)
                propertyDeclaration += QLatin1String(" MEMBER ") + data.memberVariableName;
            if (!data.resetName.isEmpty())
                propertyDeclaration += QLatin1String(" RESET ") + data.resetName;
            propertyDeclaration.append(QLatin1String(" NOTIFY ")).append(data.signalName);
        }

        propertyDeclaration.append(QLatin1String(" FINAL)\n"));
        addHeaderCode(InsertionPointLocator::Private, propertyDeclaration);
    }
}

//! Generate constructor
class GenerateConstructor : public CppQuickFixFactory
{
public:
#ifdef WITH_TESTS
    static QObject* createTest();
#endif

protected:
    void setTest() { m_test = true; }

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const auto op = QSharedPointer<GenerateConstructorOperation>::create(interface);
        if (!op->isApplicable())
            return;
        op->setTest(m_test);
        result << op;
    }

    bool m_test = false;
};

//! Adds getter and setter functions for a member variable
class GenerateGetterSetter : public CppQuickFixFactory
{
public:
#ifdef WITH_TESTS
    static QObject* createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        ExistingGetterSetterData existing;

        const QList<AST *> &path = interface.path();
        // We expect something like
        // [0] TranslationUnitAST
        // [1] NamespaceAST
        // [2] LinkageBodyAST
        // [3] SimpleDeclarationAST
        // [4] ClassSpecifierAST
        // [5] SimpleDeclarationAST
        // [6] DeclaratorAST
        // [7] DeclaratorIdAST
        // [8] SimpleNameAST

        const int n = path.size();
        if (n < 6)
            return;

        int i = 1;
        const auto variableNameAST = path.at(n - i++)->asSimpleName();
        const auto declaratorId = path.at(n - i++)->asDeclaratorId();
        // DeclaratorAST might be preceded by PointerAST, e.g. for the case
        // "class C { char *@s; };", where '@' denotes the text cursor position.
        auto declarator = path.at(n - i++)->asDeclarator();
        if (!declarator) {
            --i;
            if (path.at(n - i++)->asPointer()) {
                if (n < 7)
                    return;
                declarator = path.at(n - i++)->asDeclarator();
            }
            if (!declarator)
                return;
        }
        const auto variableDecl = path.at(n - i++)->asSimpleDeclaration();
        const auto classSpecifier = path.at(n - i++)->asClassSpecifier();
        const auto classDecl = path.at(n - i++)->asSimpleDeclaration();

        if (!(variableNameAST && declaratorId && variableDecl && classSpecifier && classDecl))
            return;

        // Do not get triggered on member functconstions and arrays
        if (declarator->postfix_declarator_list) {
            return;
        }

        // Construct getter and setter names
        const Name *variableName = variableNameAST->name;
        if (!variableName) {
            return;
        }
        const Identifier *variableId = variableName->identifier();
        if (!variableId) {
            return;
        }
        existing.memberVariableName = QString::fromUtf8(variableId->chars(), variableId->size());

        // Find the right symbol (for typeName) in the simple declaration
        Symbol *symbol = nullptr;
        const List<Symbol *> *symbols = variableDecl->symbols;
        QTC_ASSERT(symbols, return );
        for (; symbols; symbols = symbols->next) {
            Symbol *s = symbols->value;
            if (const Name *name = s->name()) {
                if (const Identifier *id = name->identifier()) {
                    const QString symbolName = QString::fromUtf8(id->chars(), id->size());
                    if (symbolName == existing.memberVariableName) {
                        symbol = s;
                        break;
                    }
                }
            }
        }
        if (!symbol) {
            // no type can be determined
            return;
        }
        if (!symbol->asDeclaration()) {
            return;
        }
        existing.declarationSymbol = symbol->asDeclaration();

        existing.clazz = classSpecifier->symbol;
        if (!existing.clazz)
            return;

        auto file = interface.currentFile();
        // check if a Q_PROPERTY exist
        const QString baseName = memberBaseName(existing.memberVariableName);
        // eg: we have 'int m_test' and now 'Q_PROPERTY(int foo WRITE setTest MEMBER m_test NOTIFY tChanged)'
        for (auto it = classSpecifier->member_specifier_list; it; it = it->next) {
            if (it->value->asQtPropertyDeclaration()) {
                auto propDecl = it->value->asQtPropertyDeclaration();
                // iterator over 'READ ...', ...
                auto p = propDecl->property_declaration_item_list;
                // first check, if we have a MEMBER and the member is equal to the baseName
                for (; p; p = p->next) {
                    const char *tokenString = file->tokenAt(p->value->item_name_token).spell();
                    if (!qstrcmp(tokenString, "MEMBER")) {
                        if (baseName == file->textOf(p->value->expression))
                            return;
                    }
                }
                // no MEMBER, but maybe the property name is the same
                const QString propertyName = file->textOf(propDecl->property_name);
                // we compare the baseName. e.g. 'test' instead of 'm_test'
                if (propertyName == baseName)
                    return; // TODO Maybe offer quick fix "Add missing Q_PROPERTY Members"
            }
        }

        findExistingFunctions(existing, toStringList(getMemberFunctions(existing.clazz)));
        existing.qPropertyName = memberBaseName(existing.memberVariableName);

        const int possibleFlags = existing.computePossibleFlags();
        GenerateGetterSetterOp::generateQuickFixes(result, interface, existing, possibleFlags);
    }
};

//! Adds getter and setter functions for several member variables
class GenerateGettersSettersForClass : public CppQuickFixFactory
{
public:
#ifdef WITH_TESTS
    static QObject* createTest();
#endif

protected:
    void setTest() { m_test = true; }

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const auto op = QSharedPointer<GenerateGettersSettersOperation>::create(interface);
        if (!op->isApplicable())
            return;
        if (m_test) {
            GetterSetterCandidates candidates = op->candidates();
            for (MemberInfo &mi : candidates) {
                mi.requestedFlags = mi.possibleFlags;
                using Flag = GenerateGetterSetterOp::GenerateFlag;
                mi.requestedFlags &= ~Flag::GenerateConstantProperty;
            }
            op->setGetterSetterData(candidates);
        }
        result << op;
    }

    bool m_test = false;
};

//! Adds missing members for a Q_PROPERTY
class InsertQtPropertyMembers : public CppQuickFixFactory
{
public:
#ifdef WITH_TESTS
    static QObject* createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        using Flag = GenerateGetterSetterOp::GenerateFlag;
        ExistingGetterSetterData existing;
        // check for Q_PROPERTY

        const QList<AST *> &path = interface.path();
        if (path.isEmpty())
            return;

        AST *const ast = path.last();
        QtPropertyDeclarationAST *qtPropertyDeclaration = ast->asQtPropertyDeclaration();
        if (!qtPropertyDeclaration || !qtPropertyDeclaration->type_id)
            return;

        ClassSpecifierAST *klass = nullptr;
        for (int i = path.size() - 2; i >= 0; --i) {
            klass = path.at(i)->asClassSpecifier();
            if (klass)
                break;
        }
        if (!klass)
            return;
        existing.clazz = klass->symbol;

        CppRefactoringFilePtr file = interface.currentFile();
        const QString propertyName = file->textOf(qtPropertyDeclaration->property_name);
        existing.qPropertyName = propertyName;
        extractNames(file, qtPropertyDeclaration, existing);

        Control *control = interface.currentFile()->cppDocument()->control();

        existing.declarationSymbol = control->newDeclaration(ast->firstToken(),
                                                             qtPropertyDeclaration->property_name->name);
        existing.declarationSymbol->setVisibility(Symbol::Private);
        existing.declarationSymbol->setEnclosingScope(existing.clazz);

        {
            // create a 'right' Type Object
            // if we have Q_PROPERTY(int test ...) then we only get a NamedType for 'int', but we want
            // a IntegerType. So create a new dummy file with a dummy declaration to get the right
            // object
            QByteArray type = file->textOf(qtPropertyDeclaration->type_id).toUtf8();
            QByteArray newSource = file->document()
                                       ->toPlainText()
                                       .insert(file->startOf(qtPropertyDeclaration),
                                               QString::fromUtf8(type + " __dummy;\n"))
                                       .toUtf8();

            Document::Ptr doc = interface.snapshot().preprocessedDocument(newSource, "___quickfix.h");
            if (!doc->parse(Document::ParseTranlationUnit))
                return;
            doc->check();
            class TypeFinder : public ASTVisitor
            {
            public:
                FullySpecifiedType type;
                TypeFinder(TranslationUnit *u)
                    : ASTVisitor(u)
                {}
                bool visit(SimpleDeclarationAST *ast) override
                {
                    if (ast->symbols && !ast->symbols->next) {
                        const Name *name = ast->symbols->value->name();
                        if (name && name->asNameId() && name->asNameId()->identifier()) {
                            const Identifier *id = name->asNameId()->identifier();
                            if (QString::fromUtf8(id->chars(), id->size()) == "__dummy")
                                type = ast->symbols->value->type();
                        }
                    }
                    return true;
                }
            };
            TypeFinder finder(doc->translationUnit());
            finder.accept(doc->translationUnit()->ast());
            if (finder.type.type()->isUndefinedType())
                return;
            existing.declarationSymbol->setType(finder.type);
            existing.doc = doc; // to hold type
        }
        // check which methods are already there
        const bool haveFixMemberVariableName = !existing.memberVariableName.isEmpty();
        int generateFlags = Flag::GenerateMemberVariable;
        if (!existing.resetName.isEmpty())
            generateFlags |= Flag::GenerateReset;
        if (!existing.setterName.isEmpty())
            generateFlags |= Flag::GenerateSetter;
        if (!existing.getterName.isEmpty())
            generateFlags |= Flag::GenerateGetter;
        if (!existing.signalName.isEmpty())
            generateFlags |= Flag::GenerateSignal;
        Overview overview;
        for (int i = 0; i < existing.clazz->memberCount(); ++i) {
            Symbol *member = existing.clazz->memberAt(i);
            FullySpecifiedType type = member->type();
            if (member->asFunction() || (type.isValid() && type->asFunctionType())) {
                const QString name = overview.prettyName(member->name());
                if (name == existing.getterName)
                    generateFlags &= ~Flag::GenerateGetter;
                else if (name == existing.setterName)
                    generateFlags &= ~Flag::GenerateSetter;
                else if (name == existing.resetName)
                    generateFlags &= ~Flag::GenerateReset;
                else if (name == existing.signalName)
                    generateFlags &= ~Flag::GenerateSignal;
            } else if (member->asDeclaration()) {
                const QString name = overview.prettyName(member->name());
                if (haveFixMemberVariableName) {
                    if (name == existing.memberVariableName) {
                        generateFlags &= ~Flag::GenerateMemberVariable;
                    }
                } else {
                    const QString baseName = memberBaseName(name);
                    if (existing.qPropertyName == baseName) {
                        existing.memberVariableName = name;
                        generateFlags &= ~Flag::GenerateMemberVariable;
                    }
                }
            }
        }
        if (generateFlags & Flag::GenerateMemberVariable) {
            CppQuickFixSettings *settings = CppQuickFixProjectsSettings::getQuickFixSettings(
                ProjectExplorer::ProjectTree::currentProject());
            existing.memberVariableName = settings->getMemberVariableName(existing.qPropertyName);
        }
        if (generateFlags == 0) {
            // everything is already there
            return;
        }
        generateFlags |= Flag::HaveExistingQProperty;
        GenerateGetterSetterOp::generateQuickFixes(result, interface, existing, generateFlags);
    }
};


#ifdef WITH_TESTS
using namespace Tests;

class GenerateGetterSetterTest : public QObject
{
    Q_OBJECT

private slots:
    void testNamespaceHandlingCreate_data()
    {
        QTest::addColumn<QByteArrayList>("headers");
        QTest::addColumn<QByteArrayList>("sources");

        QByteArray originalSource;
        QByteArray expectedSource;

        const QByteArray originalHeader =
                "namespace N1 {\n"
                "namespace N2 {\n"
                "class Something\n"
                "{\n"
                "    int @it;\n"
                "};\n"
                "}\n"
                "}\n";
        const QByteArray expectedHeader =
                "namespace N1 {\n"
                "namespace N2 {\n"
                "class Something\n"
                "{\n"
                "    int it;\n"
                "\n"
                "public:\n"
                "    int getIt() const;\n"
                "    void setIt(int value);\n"
                "};\n"
                "}\n"
                "}\n";

        originalSource = "#include \"file.h\"\n";
        expectedSource =
                "#include \"file.h\"\n\n\n"
                "namespace N1 {\n"
                "namespace N2 {\n"
                "int Something::getIt() const\n"
                "{\n"
                "    return it;\n"
                "}\n"
                "\n"
                "void Something::setIt(int value)\n"
                "{\n"
                "    it = value;\n"
                "}\n\n"
                "}\n"
                "}\n";
        QTest::addRow("insert new namespaces")
                << QByteArrayList{originalHeader, expectedHeader}
                << QByteArrayList{originalSource, expectedSource};

        originalSource =
                "#include \"file.h\"\n"
                "namespace N2 {} // decoy\n";
        expectedSource =
                "#include \"file.h\"\n"
                "namespace N2 {} // decoy\n\n\n"
                "namespace N1 {\n"
                "namespace N2 {\n"
                "int Something::getIt() const\n"
                "{\n"
                "    return it;\n"
                "}\n"
                "\n"
                "void Something::setIt(int value)\n"
                "{\n"
                "    it = value;\n"
                "}\n\n"
                "}\n"
                "}\n";
        QTest::addRow("insert new namespaces (with decoy)")
                << QByteArrayList{originalHeader, expectedHeader}
                << QByteArrayList{originalSource, expectedSource};

        originalSource = "#include \"file.h\"\n"
                         "namespace N2 {} // decoy\n"
                         "namespace {\n"
                         "namespace N1 {\n"
                         "namespace {\n"
                         "}\n"
                         "}\n"
                         "}\n";
        expectedSource = "#include \"file.h\"\n"
                         "namespace N2 {} // decoy\n"
                         "namespace {\n"
                         "namespace N1 {\n"
                         "namespace {\n"
                         "}\n"
                         "}\n"
                         "}\n"
                         "\n"
                         "\n"
                         "namespace N1 {\n"
                         "namespace N2 {\n"
                         "int Something::getIt() const\n"
                         "{\n"
                         "    return it;\n"
                         "}\n"
                         "\n"
                         "void Something::setIt(int value)\n"
                         "{\n"
                         "    it = value;\n"
                         "}\n"
                         "\n"
                         "}\n"
                         "}\n";
        QTest::addRow("insert inner namespace (with decoy and unnamed)")
            << QByteArrayList{originalHeader, expectedHeader}
            << QByteArrayList{originalSource, expectedSource};

        const QByteArray unnamedOriginalHeader = "namespace {\n" + originalHeader + "}\n";
        const QByteArray unnamedExpectedHeader = "namespace {\n" + expectedHeader + "}\n";

        originalSource = "#include \"file.h\"\n"
                         "namespace N2 {} // decoy\n"
                         "namespace {\n"
                         "namespace N1 {\n"
                         "namespace {\n"
                         "}\n"
                         "}\n"
                         "}\n";
        expectedSource = "#include \"file.h\"\n"
                         "namespace N2 {} // decoy\n"
                         "namespace {\n"
                         "namespace N1 {\n"
                         "\n"
                         "namespace N2 {\n"
                         "int Something::getIt() const\n"
                         "{\n"
                         "    return it;\n"
                         "}\n"
                         "\n"
                         "void Something::setIt(int value)\n"
                         "{\n"
                         "    it = value;\n"
                         "}\n"
                         "\n"
                         "}\n"
                         "\n"
                         "namespace {\n"
                         "}\n"
                         "}\n"
                         "}\n";
        QTest::addRow("insert inner namespace in unnamed (with decoy)")
            << QByteArrayList{unnamedOriginalHeader, unnamedExpectedHeader}
            << QByteArrayList{originalSource, expectedSource};

        originalSource =
                "#include \"file.h\"\n"
                "namespace N1 {\n"
                "namespace N2 {\n"
                "namespace N3 {\n"
                "}\n"
                "}\n"
                "}\n";
        expectedSource =
                "#include \"file.h\"\n"
                "namespace N1 {\n"
                "namespace N2 {\n"
                "namespace N3 {\n"
                "}\n\n"
                "int Something::getIt() const\n"
                "{\n"
                "    return it;\n"
                "}\n"
                "\n"
                "void Something::setIt(int value)\n"
                "{\n"
                "    it = value;\n"
                "}\n\n"
                "}\n"
                "}\n";
        QTest::addRow("all namespaces already present")
                << QByteArrayList{originalHeader, expectedHeader}
                << QByteArrayList{originalSource, expectedSource};

        originalSource = "#include \"file.h\"\n"
                         "namespace N1 {\n"
                         "using namespace N2::N3;\n"
                         "using namespace N2;\n"
                         "using namespace N2;\n"
                         "using namespace N3;\n"
                         "}\n";
        expectedSource = "#include \"file.h\"\n"
                         "namespace N1 {\n"
                         "using namespace N2::N3;\n"
                         "using namespace N2;\n"
                         "using namespace N2;\n"
                         "using namespace N3;\n"
                         "\n"
                         "int Something::getIt() const\n"
                         "{\n"
                         "    return it;\n"
                         "}\n"
                         "\n"
                         "void Something::setIt(int value)\n"
                         "{\n"
                         "    it = value;\n"
                         "}\n\n"
                         "}\n";
        QTest::addRow("namespaces present and using namespace")
            << QByteArrayList{originalHeader, expectedHeader}
            << QByteArrayList{originalSource, expectedSource};

        originalSource = "#include \"file.h\"\n"
                         "using namespace N1::N2::N3;\n"
                         "using namespace N1::N2;\n"
                         "namespace N1 {\n"
                         "using namespace N3;\n"
                         "}\n";
        expectedSource = "#include \"file.h\"\n"
                         "using namespace N1::N2::N3;\n"
                         "using namespace N1::N2;\n"
                         "namespace N1 {\n"
                         "using namespace N3;\n"
                         "\n"
                         "int Something::getIt() const\n"
                         "{\n"
                         "    return it;\n"
                         "}\n"
                         "\n"
                         "void Something::setIt(int value)\n"
                         "{\n"
                         "    it = value;\n"
                         "}\n"
                         "\n"
                         "}\n";
        QTest::addRow("namespaces present and outer using namespace")
            << QByteArrayList{originalHeader, expectedHeader}
            << QByteArrayList{originalSource, expectedSource};

        originalSource = "#include \"file.h\"\n"
                         "using namespace N1;\n"
                         "using namespace N2;\n"
                         "namespace N3 {\n"
                         "}\n";
        expectedSource = "#include \"file.h\"\n"
                         "using namespace N1;\n"
                         "using namespace N2;\n"
                         "namespace N3 {\n"
                         "}\n"
                         "\n"
                         "int Something::getIt() const\n"
                         "{\n"
                         "    return it;\n"
                         "}\n"
                         "\n"
                         "void Something::setIt(int value)\n"
                         "{\n"
                         "    it = value;\n"
                         "}\n";
        QTest::addRow("namespaces present and outer using namespace")
            << QByteArrayList{originalHeader, expectedHeader}
            << QByteArrayList{originalSource, expectedSource};
    }

    void testNamespaceHandlingCreate()
    {
        QFETCH(QByteArrayList, headers);
        QFETCH(QByteArrayList, sources);

        QList<TestDocumentPtr> testDocuments(
            {CppTestDocument::create("file.h", headers.at(0), headers.at(1)),
             CppTestDocument::create("file.cpp", sources.at(0), sources.at(1))});

        QuickFixSettings s;
        s->cppFileNamespaceHandling = CppQuickFixSettings::MissingNamespaceHandling::CreateMissing;
        s->setterParameterNameTemplate = "value";
        s->getterNameTemplate = "get<Name>";
        s->setterInCppFileFrom = 1;
        s->getterInCppFileFrom = 1;
        GenerateGetterSetter factory;
        QuickFixOperationTest(testDocuments, &factory, ProjectExplorer::HeaderPaths(), 2);
    }

    void testNamespaceHandlingAddUsing_data()
    {
        QTest::addColumn<QByteArrayList>("headers");
        QTest::addColumn<QByteArrayList>("sources");

        QByteArray originalSource;
        QByteArray expectedSource;

        const QByteArray originalHeader = "namespace N1 {\n"
                                          "namespace N2 {\n"
                                          "class Something\n"
                                          "{\n"
                                          "    int @it;\n"
                                          "};\n"
                                          "}\n"
                                          "}\n";
        const QByteArray expectedHeader = "namespace N1 {\n"
                                          "namespace N2 {\n"
                                          "class Something\n"
                                          "{\n"
                                          "    int it;\n"
                                          "\n"
                                          "public:\n"
                                          "    void setIt(int value);\n"
                                          "};\n"
                                          "}\n"
                                          "}\n";

        originalSource = "#include \"file.h\"\n";
        expectedSource = "#include \"file.h\"\n\n"
                         "using namespace N1::N2;\n"
                         "void Something::setIt(int value)\n"
                         "{\n"
                         "    it = value;\n"
                         "}\n";
        QTest::addRow("add using namespaces") << QByteArrayList{originalHeader, expectedHeader}
                                              << QByteArrayList{originalSource, expectedSource};

        const QByteArray unnamedOriginalHeader = "namespace {\n" + originalHeader + "}\n";
        const QByteArray unnamedExpectedHeader = "namespace {\n" + expectedHeader + "}\n";

        originalSource = "#include \"file.h\"\n"
                         "namespace N2 {} // decoy\n"
                         "namespace {\n"
                         "namespace N1 {\n"
                         "namespace {\n"
                         "}\n"
                         "}\n"
                         "}\n";
        expectedSource = "#include \"file.h\"\n"
                         "namespace N2 {} // decoy\n"
                         "namespace {\n"
                         "namespace N1 {\n"
                         "using namespace N2;\n"
                         "void Something::setIt(int value)\n"
                         "{\n"
                         "    it = value;\n"
                         "}\n"
                         "namespace {\n"
                         "}\n"
                         "}\n"
                         "}\n";
        QTest::addRow("insert using namespace into unnamed nested (with decoy)")
            << QByteArrayList{unnamedOriginalHeader, unnamedExpectedHeader}
            << QByteArrayList{originalSource, expectedSource};

        originalSource = "#include \"file.h\"\n";
        expectedSource = "#include \"file.h\"\n\n"
                         "using namespace N1::N2;\n"
                         "void Something::setIt(int value)\n"
                         "{\n"
                         "    it = value;\n"
                         "}\n";
        QTest::addRow("insert using namespace into unnamed")
            << QByteArrayList{unnamedOriginalHeader, unnamedExpectedHeader}
            << QByteArrayList{originalSource, expectedSource};

        originalSource = "#include \"file.h\"\n"
                         "namespace N2 {} // decoy\n"
                         "namespace {\n"
                         "namespace N1 {\n"
                         "namespace {\n"
                         "}\n"
                         "}\n"
                         "}\n";
        expectedSource = "#include \"file.h\"\n"
                         "namespace N2 {} // decoy\n"
                         "namespace {\n"
                         "namespace N1 {\n"
                         "namespace {\n"
                         "}\n"
                         "}\n"
                         "}\n"
                         "\n"
                         "using namespace N1::N2;\n"
                         "void Something::setIt(int value)\n"
                         "{\n"
                         "    it = value;\n"
                         "}\n";
        QTest::addRow("insert using namespace (with decoy)")
            << QByteArrayList{originalHeader, expectedHeader}
            << QByteArrayList{originalSource, expectedSource};
    }

    void testNamespaceHandlingAddUsing()
    {
        QFETCH(QByteArrayList, headers);
        QFETCH(QByteArrayList, sources);

        QList<TestDocumentPtr> testDocuments(
            {CppTestDocument::create("file.h", headers.at(0), headers.at(1)),
             CppTestDocument::create("file.cpp", sources.at(0), sources.at(1))});

        QuickFixSettings s;
        s->cppFileNamespaceHandling = CppQuickFixSettings::MissingNamespaceHandling::AddUsingDirective;
        s->setterParameterNameTemplate = "value";
        s->setterInCppFileFrom = 1;

        if (std::strstr(QTest::currentDataTag(), "unnamed nested") != nullptr)
            QSKIP("TODO"); // FIXME
        GenerateGetterSetter factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    void testNamespaceHandlingFullyQualify_data()
    {
        QTest::addColumn<QByteArrayList>("headers");
        QTest::addColumn<QByteArrayList>("sources");

        QByteArray originalSource;
        QByteArray expectedSource;

        const QByteArray originalHeader = "namespace N1 {\n"
                                          "namespace N2 {\n"
                                          "class Something\n"
                                          "{\n"
                                          "    int @it;\n"
                                          "};\n"
                                          "}\n"
                                          "}\n";
        const QByteArray expectedHeader = "namespace N1 {\n"
                                          "namespace N2 {\n"
                                          "class Something\n"
                                          "{\n"
                                          "    int it;\n"
                                          "\n"
                                          "public:\n"
                                          "    void setIt(int value);\n"
                                          "};\n"
                                          "}\n"
                                          "}\n";

        originalSource = "#include \"file.h\"\n";
        expectedSource = "#include \"file.h\"\n\n"
                         "void N1::N2::Something::setIt(int value)\n"
                         "{\n"
                         "    it = value;\n"
                         "}\n";
        QTest::addRow("fully qualify") << QByteArrayList{originalHeader, expectedHeader}
                                       << QByteArrayList{originalSource, expectedSource};

        originalSource = "#include \"file.h\"\n"
                         "namespace N2 {} // decoy\n";
        expectedSource = "#include \"file.h\"\n"
                         "namespace N2 {} // decoy\n"
                         "\n"
                         "void N1::N2::Something::setIt(int value)\n"
                         "{\n"
                         "    it = value;\n"
                         "}\n";
        QTest::addRow("fully qualify (with decoy)") << QByteArrayList{originalHeader, expectedHeader}
                                                    << QByteArrayList{originalSource, expectedSource};

        originalSource = "#include \"file.h\"\n"
                         "namespace N2 {} // decoy\n"
                         "namespace {\n"
                         "namespace N1 {\n"
                         "namespace {\n"
                         "}\n"
                         "}\n"
                         "}\n";
        expectedSource = "#include \"file.h\"\n"
                         "namespace N2 {} // decoy\n"
                         "namespace {\n"
                         "namespace N1 {\n"
                         "namespace {\n"
                         "}\n"
                         "}\n"
                         "}\n"
                         "\n"
                         "void N1::N2::Something::setIt(int value)\n"
                         "{\n"
                         "    it = value;\n"
                         "}\n";
        QTest::addRow("qualify in inner namespace (with decoy)")
            << QByteArrayList{originalHeader, expectedHeader}
            << QByteArrayList{originalSource, expectedSource};

        const QByteArray unnamedOriginalHeader = "namespace {\n" + originalHeader + "}\n";
        const QByteArray unnamedExpectedHeader = "namespace {\n" + expectedHeader + "}\n";

        originalSource = "#include \"file.h\"\n"
                         "namespace N2 {} // decoy\n"
                         "namespace {\n"
                         "namespace N1 {\n"
                         "namespace {\n"
                         "}\n"
                         "}\n"
                         "}\n";
        expectedSource = "#include \"file.h\"\n"
                         "namespace N2 {} // decoy\n"
                         "namespace {\n"
                         "namespace N1 {\n"
                         "void N2::Something::setIt(int value)\n"
                         "{\n"
                         "    it = value;\n"
                         "}\n"
                         "namespace {\n"
                         "}\n"
                         "}\n"
                         "}\n";
        QTest::addRow("qualify in inner namespace unnamed nested (with decoy)")
            << QByteArrayList{unnamedOriginalHeader, unnamedExpectedHeader}
            << QByteArrayList{originalSource, expectedSource};

        originalSource = "#include \"file.h\"\n";
        expectedSource = "#include \"file.h\"\n\n"
                         "void N1::N2::Something::setIt(int value)\n"
                         "{\n"
                         "    it = value;\n"
                         "}\n";
        QTest::addRow("qualify in unnamed namespace")
            << QByteArrayList{unnamedOriginalHeader, unnamedExpectedHeader}
            << QByteArrayList{originalSource, expectedSource};
    }

    void testNamespaceHandlingFullyQualify()
    {
        QFETCH(QByteArrayList, headers);
        QFETCH(QByteArrayList, sources);

        QList<TestDocumentPtr> testDocuments(
            {CppTestDocument::create("file.h", headers.at(0), headers.at(1)),
             CppTestDocument::create("file.cpp", sources.at(0), sources.at(1))});

        QuickFixSettings s;
        s->cppFileNamespaceHandling = CppQuickFixSettings::MissingNamespaceHandling::RewriteType;
        s->setterParameterNameTemplate = "value";
        s->setterInCppFileFrom = 1;

        if (std::strstr(QTest::currentDataTag(), "unnamed nested") != nullptr)
            QSKIP("TODO"); // FIXME
        GenerateGetterSetter factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    void testCustomNames_data()
    {
        QTest::addColumn<QByteArrayList>("headers");
        QTest::addColumn<int>("operation");

        QByteArray originalSource;
        QByteArray expectedSource;

        // Check if right names are created
        originalSource = R"-(
    class Test {
        int m_fooBar_test@;
    };
)-";
        expectedSource = R"-(
    class Test {
        int m_fooBar_test;

    public:
        int give_me_foo_bar_test() const
        {
            return m_fooBar_test;
        }
        void Seet_FooBar_test(int New_Foo_Bar_Test)
        {
            if (m_fooBar_test == New_Foo_Bar_Test)
                return;
            m_fooBar_test = New_Foo_Bar_Test;
            emit newFooBarTestValue();
        }
        void set_fooBarTest_toDefault()
        {
            Seet_FooBar_test({}); // TODO: Adapt to use your actual default value
        }

    signals:
        void newFooBarTestValue();

    private:
        Q_PROPERTY(int fooBar_test READ give_me_foo_bar_test WRITE Seet_FooBar_test RESET set_fooBarTest_toDefault NOTIFY newFooBarTestValue FINAL)
    };
)-";
        QTest::addRow("create right names") << QByteArrayList{originalSource, expectedSource} << 4;

        // Check if not triggered with custom names
        originalSource = R"-(
    class Test {
        int m_fooBar_test@;

    public:
        int give_me_foo_bar_test() const
        {
            return m_fooBar_test;
        }
        void Seet_FooBar_test(int New_Foo_Bar_Test)
        {
            if (m_fooBar_test == New_Foo_Bar_Test)
                return;
            m_fooBar_test = New_Foo_Bar_Test;
            emit newFooBarTestValue();
        }
        void set_fooBarTest_toDefault()
        {
            Seet_FooBar_test({}); // TODO: Adapt to use your actual default value
        }

    signals:
        void newFooBarTestValue();

    private:
        Q_PROPERTY(int fooBar_test READ give_me_foo_bar_test WRITE Seet_FooBar_test RESET set_fooBarTest_toDefault NOTIFY newFooBarTestValue FINAL)
    };
)-";
        expectedSource = "";
        QTest::addRow("everything already exists") << QByteArrayList{originalSource, expectedSource} << 4;

        // create from Q_PROPERTY with custom names
        originalSource = R"-(
    class Test {
        Q_PROPER@TY(int fooBar_test READ give_me_foo_bar_test WRITE Seet_FooBar_test RESET set_fooBarTest_toDefault NOTIFY newFooBarTestValue FINAL)

    public:
        int give_me_foo_bar_test() const
        {
            return mem_fooBar_test;
        }
        void Seet_FooBar_test(int New_Foo_Bar_Test)
        {
            if (mem_fooBar_test == New_Foo_Bar_Test)
                return;
            mem_fooBar_test = New_Foo_Bar_Test;
            emit newFooBarTestValue();
        }
        void set_fooBarTest_toDefault()
        {
            Seet_FooBar_test({}); // TODO: Adapt to use your actual default value
        }

    signals:
        void newFooBarTestValue();
    };
)-";
        expectedSource = R"-(
    class Test {
        Q_PROPERTY(int fooBar_test READ give_me_foo_bar_test WRITE Seet_FooBar_test RESET set_fooBarTest_toDefault NOTIFY newFooBarTestValue FINAL)

    public:
        int give_me_foo_bar_test() const
        {
            return mem_fooBar_test;
        }
        void Seet_FooBar_test(int New_Foo_Bar_Test)
        {
            if (mem_fooBar_test == New_Foo_Bar_Test)
                return;
            mem_fooBar_test = New_Foo_Bar_Test;
            emit newFooBarTestValue();
        }
        void set_fooBarTest_toDefault()
        {
            Seet_FooBar_test({}); // TODO: Adapt to use your actual default value
        }

    signals:
        void newFooBarTestValue();
    private:
        int mem_fooBar_test;
    };
)-";
        QTest::addRow("create only member variable")
            << QByteArrayList{originalSource, expectedSource} << 0;

        // create from Q_PROPERTY with custom names
        originalSource = R"-(
    class Test {
        Q_PROPE@RTY(int fooBar_test READ give_me_foo_bar_test WRITE Seet_FooBar_test RESET set_fooBarTest_toDefault NOTIFY newFooBarTestValue FINAL)
        int mem_fooBar_test;
    public:
    };
)-";
        expectedSource = R"-(
    class Test {
        Q_PROPERTY(int fooBar_test READ give_me_foo_bar_test WRITE Seet_FooBar_test RESET set_fooBarTest_toDefault NOTIFY newFooBarTestValue FINAL)
        int mem_fooBar_test;
    public:
        int give_me_foo_bar_test() const
        {
            return mem_fooBar_test;
        }
        void Seet_FooBar_test(int New_Foo_Bar_Test)
        {
            if (mem_fooBar_test == New_Foo_Bar_Test)
                return;
            mem_fooBar_test = New_Foo_Bar_Test;
            emit newFooBarTestValue();
        }
        void set_fooBarTest_toDefault()
        {
            Seet_FooBar_test({}); // TODO: Adapt to use your actual default value
        }
    signals:
        void newFooBarTestValue();
    };
)-";
        QTest::addRow("create methods with given member variable")
            << QByteArrayList{originalSource, expectedSource} << 0;
    }

    void testCustomNames()
    {
        QFETCH(QByteArrayList, headers);
        QFETCH(int, operation);

        QList<TestDocumentPtr> testDocuments(
            {CppTestDocument::create("file.h", headers.at(0), headers.at(1))});

        QuickFixSettings s;
        s->setterInCppFileFrom = 0;
        s->getterInCppFileFrom = 0;
        s->setterNameTemplate = "Seet_<Name>";
        s->getterNameTemplate = "give_me_<snake>";
        s->signalNameTemplate = "new<Camel>Value";
        s->setterParameterNameTemplate = "New_<Snake>";
        s->resetNameTemplate = "set_<camel>_toDefault";
        s->memberVariableNameTemplate = "mem_<name>";
        if (operation == 0) {
            InsertQtPropertyMembers factory;
            QuickFixOperationTest(testDocuments, &factory, ProjectExplorer::HeaderPaths(), operation);
        } else {
            GenerateGetterSetter factory;
            QuickFixOperationTest(testDocuments, &factory, ProjectExplorer::HeaderPaths(), operation);
        }
    }

    void testValueTypes_data()
    {
        QTest::addColumn<QByteArrayList>("headers");
        QTest::addColumn<int>("operation");

        QByteArray originalSource;
        QByteArray expectedSource;

        // int should be a value type
        originalSource = R"-(
    class Test {
        int i@;
    };
)-";
        expectedSource = R"-(
    class Test {
        int i;

    public:
        int getI() const
        {
            return i;
        }
    };
)-";
        QTest::addRow("int") << QByteArrayList{originalSource, expectedSource} << 1;

        // return type should be only int without const
        originalSource = R"-(
    class Test {
        const int i@;
    };
)-";
        expectedSource = R"-(
    class Test {
        const int i;

    public:
        int getI() const
        {
            return i;
        }
    };
)-";
        QTest::addRow("const int") << QByteArrayList{originalSource, expectedSource} << 0;

        // float should be a value type
        originalSource = R"-(
    class Test {
        float f@;
    };
)-";
        expectedSource = R"-(
    class Test {
        float f;

    public:
        float getF() const
        {
            return f;
        }
    };
)-";
        QTest::addRow("float") << QByteArrayList{originalSource, expectedSource} << 1;

        // pointer should be a value type
        originalSource = R"-(
    class Test {
        void* v@;
    };
)-";
        expectedSource = R"-(
    class Test {
        void* v;

    public:
        void *getV() const
        {
            return v;
        }
    };
)-";
        QTest::addRow("pointer") << QByteArrayList{originalSource, expectedSource} << 1;

        // reference should be a value type (setter is const ref)
        originalSource = R"-(
    class Test {
        int& r@;
    };
)-";
        expectedSource = R"-(
    class Test {
        int& r;

    public:
        int &getR() const
        {
            return r;
        }
        void setR(const int &newR)
        {
            r = newR;
        }
    };
)-";
        QTest::addRow("reference to value type") << QByteArrayList{originalSource, expectedSource} << 2;

        // reference should be a value type
        originalSource = R"-(
    using bar = int;
    class Test {
        bar i@;
    };
)-";
        expectedSource = R"-(
    using bar = int;
    class Test {
        bar i;

    public:
        bar getI() const
        {
            return i;
        }
    };
)-";
        QTest::addRow("value type through using") << QByteArrayList{originalSource, expectedSource} << 1;

        // enum should be a value type
        originalSource = R"-(
    enum Foo{V1, V2};
    class Test {
        Foo e@;
    };
)-";
        expectedSource = R"-(
    enum Foo{V1, V2};
    class Test {
        Foo e;

    public:
        Foo getE() const
        {
            return e;
        }
    };
)-";
        QTest::addRow("enum") << QByteArrayList{originalSource, expectedSource} << 1;

        // class should not be a value type
        originalSource = R"-(
    class NoVal{};
    class Test {
        NoVal n@;
    };
)-";
        expectedSource = R"-(
    class NoVal{};
    class Test {
        NoVal n;

    public:
        const NoVal &getN() const
        {
            return n;
        }
    };
)-";
        QTest::addRow("class") << QByteArrayList{originalSource, expectedSource} << 1;

        // custom classes can be a value type
        originalSource = R"-(
    class Value{};
    class Test {
        Value v@;
    };
)-";
        expectedSource = R"-(
    class Value{};
    class Test {
        Value v;

    public:
        Value getV() const
        {
            return v;
        }
    };
)-";
        QTest::addRow("value class") << QByteArrayList{originalSource, expectedSource} << 1;

        // custom classes (in namespace) can be a value type
        originalSource = R"-(
    namespace N1{
    class Value{};
    }
    class Test {
        N1::Value v@;
    };
)-";
        expectedSource = R"-(
    namespace N1{
    class Value{};
    }
    class Test {
        N1::Value v;

    public:
        N1::Value getV() const
        {
            return v;
        }
    };
)-";
        QTest::addRow("value class in namespace") << QByteArrayList{originalSource, expectedSource} << 1;

        // custom template class can be a value type
        originalSource = R"-(
    template<typename T>
    class Value{};
    class Test {
        Value<int> v@;
    };
)-";
        expectedSource = R"-(
    template<typename T>
    class Value{};
    class Test {
        Value<int> v;

    public:
        Value<int> getV() const
        {
            return v;
        }
    };
)-";
        QTest::addRow("value template class") << QByteArrayList{originalSource, expectedSource} << 1;
    }

    void testValueTypes()
    {
        QFETCH(QByteArrayList, headers);
        QFETCH(int, operation);

        QList<TestDocumentPtr> testDocuments(
            {CppTestDocument::create("file.h", headers.at(0), headers.at(1))});

        QuickFixSettings s;
        s->setterInCppFileFrom = 0;
        s->getterInCppFileFrom = 0;
        s->getterNameTemplate = "get<Name>";
        s->valueTypes << "Value";
        s->returnByConstRef = true;

        GenerateGetterSetter factory;
        QuickFixOperationTest(testDocuments, &factory, ProjectExplorer::HeaderPaths(), operation);
    }

    /// Checks: Use template for a custom type
    void testCustomTemplate()
    {
        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        QByteArray expected;

        const QByteArray customTypeDecl = R"--(
namespace N1 {
namespace N2 {
struct test{};
}
template<typename T>
struct custom {
    void assign(const custom<T>&);
    bool equals(const custom<T>&);
    T* get();
};
)--";
        // Header File
        original = customTypeDecl + R"--(
class Foo
{
public:
    custom<N2::test> bar@;
};
})--";
        expected = customTypeDecl + R"--(
class Foo
{
public:
    custom<N2::test> bar@;
    N2::test *getBar() const;
    void setBar(const custom<N2::test> &newBar);
signals:
    void barChanged(N2::test *bar);
private:
    Q_PROPERTY(N2::test *bar READ getBar NOTIFY barChanged FINAL)
};
})--";
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original = "";
        expected = R"-(
using namespace N1;
N2::test *Foo::getBar() const
{
    return bar.get();
}

void Foo::setBar(const custom<N2::test> &newBar)
{
    if (bar.equals(newBar))
        return;
    bar.assign(newBar);
    emit barChanged(bar.get());
}
)-";

        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        QuickFixSettings s;
        s->cppFileNamespaceHandling = CppQuickFixSettings::MissingNamespaceHandling::AddUsingDirective;
        s->getterNameTemplate = "get<Name>";
        s->getterInCppFileFrom = 1;
        s->signalWithNewValue = true;
        CppQuickFixSettings::CustomTemplate t;
        t.types.append("custom");
        t.equalComparison = "<cur>.equals(<new>)";
        t.returnExpression = "<cur>.get()";
        t.returnType = "<T> *";
        t.assignment = "<cur>.assign(<new>)";
        s->customTemplates.push_back(t);

        GenerateGetterSetter factory;
        QuickFixOperationTest(testDocuments, &factory, ProjectExplorer::HeaderPaths(), 5);
    }

    /// Checks: if the setter parameter name is the same as the member variable name, this-> is needed
    void testNeedThis()
    {
        QList<TestDocumentPtr> testDocuments;

        // Header File
        const QByteArray original = R"-(
    class Foo {
        int bar@;
    public:
    };
)-";
        const QByteArray expected = R"-(
    class Foo {
        int bar@;
    public:
        void setBar(int bar)
        {
            this->bar = bar;
        }
    };
)-";
        testDocuments << CppTestDocument::create("file.h", original, expected);

        QuickFixSettings s;
        s->setterParameterNameTemplate = "<name>";
        s->setterInCppFileFrom = 0;

        GenerateGetterSetter factory;
        QuickFixOperationTest(testDocuments, &factory, ProjectExplorer::HeaderPaths(), 0);
    }

    void testOfferedFixes_data()
    {
        QTest::addColumn<QByteArray>("header");
        QTest::addColumn<QStringList>("offered");

        QByteArray header;
        QStringList offered;
        const QString setter = QStringLiteral("Generate Setter");
        const QString getter = QStringLiteral("Generate Getter");
        const QString getset = QStringLiteral("Generate Getter and Setter");
        const QString constQandMissing = QStringLiteral(
            "Generate Constant Q_PROPERTY and Missing Members");
        const QString qAndResetAndMissing = QStringLiteral(
            "Generate Q_PROPERTY and Missing Members with Reset Function");
        const QString qAndMissing = QStringLiteral("Generate Q_PROPERTY and Missing Members");
        const QStringList all{setter, getter, getset, constQandMissing, qAndResetAndMissing, qAndMissing};

        header = R"-(
    class Foo {
        static int bar@;
    };
)-";
        offered = QStringList{setter, getter, getset, constQandMissing};
        QTest::addRow("static") << header << offered;

        header = R"-(
    class Foo {
        static const int bar@;
    };
)-";
        offered = QStringList{getter, constQandMissing};
        QTest::addRow("const static") << header << offered;

        header = R"-(
    class Foo {
        const int bar@;
    };
)-";
        offered = QStringList{getter, constQandMissing};
        QTest::addRow("const") << header << offered;

        header = R"-(
    class Foo {
        const int bar@;
        int getBar() const;
    };
)-";
        offered = QStringList{constQandMissing};
        QTest::addRow("const + getter") << header << offered;

        header = R"-(
    class Foo {
        const int bar@;
        int getBar() const;
        void setBar(int value);
    };
)-";
        offered = QStringList{};
        QTest::addRow("const + getter + setter") << header << offered;

        header = R"-(
    class Foo {
        const int* bar@;
    };
)-";
        offered = all;
        QTest::addRow("pointer to const") << header << offered;

        header = R"-(
    class Foo {
        int bar@;
    public:
        int bar();
    };
)-";
        offered = QStringList{setter, constQandMissing, qAndResetAndMissing, qAndMissing};
        QTest::addRow("existing getter") << header << offered;

        header = R"-(
    class Foo {
        int bar@;
    public:
        set setBar(int);
    };
)-";
        offered = QStringList{getter};
        QTest::addRow("existing setter") << header << offered;

        header = R"-(
    class Foo {
        int bar@;
    signals:
        void barChanged(int);
    };
)-";
        offered = QStringList{setter, getter, getset, qAndResetAndMissing, qAndMissing};
        QTest::addRow("existing signal (no const Q_PROPERTY)") << header << offered;

        header = R"-(
    class Foo {
        int m_bar@;
        Q_PROPERTY(int bar)
    };
)-";
        offered = QStringList{}; // user should use "InsertQPropertyMembers", no duplicated code
        QTest::addRow("existing Q_PROPERTY") << header << offered;
    }

    void testOfferedFixes()
    {
        QFETCH(QByteArray, header);
        QFETCH(QStringList, offered);

        QList<TestDocumentPtr> testDocuments(
            {CppTestDocument::create("file.h", header, header)});

        GenerateGetterSetter factory;
        QuickFixOfferedOperationsTest(testDocuments, &factory, ProjectExplorer::HeaderPaths(), offered);
    }

    void testGeneral_data()
    {
        QTest::addColumn<int>("operation");
        QTest::addColumn<QByteArray>("original");
        QTest::addColumn<QByteArray>("expected");

        QTest::newRow("GenerateGetterSetter_referenceToNonConst")
            << 2
            << QByteArray("\n"
                 "class Something\n"
                 "{\n"
                 "    int &it@;\n"
                 "};\n")
            << QByteArray("\n"
                 "class Something\n"
                 "{\n"
                 "    int &it;\n"
                 "\n"
                 "public:\n"
                 "    int &getIt() const;\n"
                 "    void setIt(const int &it);\n"
                 "};\n"
                 "\n"
                 "int &Something::getIt() const\n"
                 "{\n"
                 "    return it;\n"
                 "}\n"
                 "\n"
                 "void Something::setIt(const int &it)\n"
                 "{\n"
                 "    this->it = it;\n"
                 "}\n");

        // Checks: No special treatment for reference to const.
        QTest::newRow("GenerateGetterSetter_referenceToConst")
            << 2
            << QByteArray("\n"
                 "class Something\n"
                 "{\n"
                 "    const int &it@;\n"
                 "};\n")
            << QByteArray("\n"
                 "class Something\n"
                 "{\n"
                 "    const int &it;\n"
                 "\n"
                 "public:\n"
                 "    const int &getIt() const;\n"
                 "    void setIt(const int &it);\n"
                 "};\n"
                 "\n"
                 "const int &Something::getIt() const\n"
                 "{\n"
                 "    return it;\n"
                 "}\n"
                 "\n"
                 "void Something::setIt(const int &it)\n"
                 "{\n"
                 "    this->it = it;\n"
                 "}\n");

        // Checks:
        // 1. Setter: Setter is a static function.
        // 2. Getter: Getter is a static, non const function.
        QTest::newRow("GenerateGetterSetter_staticMember")
            << 2
            << QByteArray("\n"
                 "class Something\n"
                 "{\n"
                 "    static int @m_member;\n"
                 "};\n")
            << QByteArray("\n"
                 "class Something\n"
                 "{\n"
                 "    static int m_member;\n"
                 "\n"
                 "public:\n"
                 "    static int member();\n"
                 "    static void setMember(int member);\n"
                 "};\n"
                 "\n"
                 "int Something::member()\n"
                 "{\n"
                 "    return m_member;\n"
                 "}\n"
                 "\n"
                 "void Something::setMember(int member)\n"
                 "{\n"
                 "    m_member = member;\n"
                 "}\n");

        // Check: Check if it works on the second declarator
        // clang-format off
        QTest::newRow("GenerateGetterSetter_secondDeclarator") << 2
            << QByteArray("\n"
                "class Something\n"
                "{\n"
                "    int *foo, @it;\n"
                "};\n")
            << QByteArray("\n"
                "class Something\n"
                "{\n"
                "    int *foo, it;\n"
                "\n"
                "public:\n"
                "    int getIt() const;\n"
                "    void setIt(int it);\n"
                "};\n"
                "\n"
                "int Something::getIt() const\n"
                "{\n"
                "    return it;\n"
                "}\n"
                "\n"
                "void Something::setIt(int it)\n"
                "{\n"
                "    this->it = it;\n"
                "}\n");
        // clang-format on

        // Check: Quick fix is offered for "int *@it;" ('@' denotes the text cursor position)
        QTest::newRow("GenerateGetterSetter_triggeringRightAfterPointerSign")
            << 2
            << QByteArray("\n"
                 "class Something\n"
                 "{\n"
                 "    int *@it;\n"
                 "};\n")
            << QByteArray("\n"
                 "class Something\n"
                 "{\n"
                 "    int *it;\n"
                 "\n"
                 "public:\n"
                 "    int *getIt() const;\n"
                 "    void setIt(int *it);\n"
                 "};\n"
                 "\n"
                 "int *Something::getIt() const\n"
                 "{\n"
                 "    return it;\n"
                 "}\n"
                 "\n"
                 "void Something::setIt(int *it)\n"
                 "{\n"
                 "    this->it = it;\n"
                 "}\n");

        // Checks if "m_" is recognized as "m" with the postfix "_" and not simply as "m_" prefix.
        QTest::newRow("GenerateGetterSetter_recognizeMasVariableName")
            << 2
            << QByteArray("\n"
                 "class Something\n"
                 "{\n"
                 "    int @m_;\n"
                 "};\n")
            << QByteArray("\n"
                 "class Something\n"
                 "{\n"
                 "    int m_;\n"
                 "\n"
                 "public:\n"
                 "    int m() const;\n"
                 "    void setM(int m);\n"
                 "};\n"
                 "\n"
                 "int Something::m() const\n"
                 "{\n"
                 "    return m_;\n"
                 "}\n"
                 "\n"
                 "void Something::setM(int m)\n"
                 "{\n"
                 "    m_ = m;\n"
                 "}\n");

        // Checks if "m" followed by an upper character is recognized as a prefix
        QTest::newRow("GenerateGetterSetter_recognizeMFollowedByCapital")
            << 2
            << QByteArray("\n"
                 "class Something\n"
                 "{\n"
                 "    int @mFoo;\n"
                 "};\n")
            << QByteArray("\n"
                 "class Something\n"
                 "{\n"
                 "    int mFoo;\n"
                 "\n"
                 "public:\n"
                 "    int foo() const;\n"
                 "    void setFoo(int foo);\n"
                 "};\n"
                 "\n"
                 "int Something::foo() const\n"
                 "{\n"
                 "    return mFoo;\n"
                 "}\n"
                 "\n"
                 "void Something::setFoo(int foo)\n"
                 "{\n"
                 "    mFoo = foo;\n"
                 "}\n");
    }

    void testGeneral()
    {
        QFETCH(int, operation);
        QFETCH(QByteArray, original);
        QFETCH(QByteArray, expected);

        QuickFixSettings s;
        s->setterParameterNameTemplate = "<name>";
        s->getterInCppFileFrom = 1;
        s->setterInCppFileFrom = 1;

        GenerateGetterSetter factory;
        QuickFixOperationTest(singleDocument(original, expected),
                              &factory,
                              ProjectExplorer::HeaderPaths(),
                              operation);
    }

    /// Checks: Only generate getter
    void testOnlyGetter()
    {
        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        QByteArray expected;

        // Header File
        original =
            "class Foo\n"
            "{\n"
            "public:\n"
            "    int bar@;\n"
            "};\n";
        expected =
            "class Foo\n"
            "{\n"
            "public:\n"
            "    int bar@;\n"
            "    int getBar() const;\n"
            "};\n";
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original.resize(0);
        expected =
            "\n"
            "int Foo::getBar() const\n"
            "{\n"
            "    return bar;\n"
            "}\n";
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        QuickFixSettings s;
        s->getterInCppFileFrom = 1;
        s->getterNameTemplate = "get<Name>";
        GenerateGetterSetter factory;
        QuickFixOperationTest(testDocuments, &factory, ProjectExplorer::HeaderPaths(), 1);
    }

    /// Checks: Only generate setter
    void testOnlySetter()
    {
        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        QByteArray expected;
        QuickFixSettings s;
        s->setterAsSlot = true; // To be ignored, as we don't have QObjects here.

        // Header File
        original =
            "class Foo\n"
            "{\n"
            "public:\n"
            "    int bar@;\n"
            "};\n";
        expected =
            "class Foo\n"
            "{\n"
            "public:\n"
            "    int bar@;\n"
            "    void setBar(int value);\n"
            "};\n";
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original.resize(0);
        expected =
            "\n"
            "void Foo::setBar(int value)\n"
            "{\n"
            "    bar = value;\n"
            "}\n";
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        s->setterInCppFileFrom = 1;
        s->setterParameterNameTemplate = "value";

        GenerateGetterSetter factory;
        QuickFixOperationTest(testDocuments, &factory, ProjectExplorer::HeaderPaths(), 0);
    }

    void testAnonymousClass()
    {
        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        QByteArray expected;
        QuickFixSettings s;
        s->setterInCppFileFrom = 1;
        s->setterParameterNameTemplate = "value";

        // Header File
        original = R"(
        class {
            int @m_foo;
        } bar;
)";
        expected = R"(
        class {
            int m_foo;

        public:
            int foo() const
            {
                return m_foo;
            }
            void setFoo(int value)
            {
                if (m_foo == value)
                    return;
                m_foo = value;
                emit fooChanged();
            }
            void resetFoo()
            {
                setFoo({}); // TODO: Adapt to use your actual default defaultValue
            }

        signals:
            void fooChanged();

        private:
            Q_PROPERTY(int foo READ foo WRITE setFoo RESET resetFoo NOTIFY fooChanged FINAL)
        } bar;
)";
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        testDocuments << CppTestDocument::create("file.cpp", {}, {});

        GenerateGetterSetter factory;
        QuickFixOperationTest(testDocuments, &factory, ProjectExplorer::HeaderPaths(), 4);
    }

    void testInlineInHeaderFile()
    {
        QList<TestDocumentPtr> testDocuments;
        const QByteArray original = R"-(
    class Foo {
    public:
        int bar@;
    };
)-";
        const QByteArray expected = R"-(
    class Foo {
    public:
        int bar;
        int getBar() const;
        void setBar(int value);
        void resetBar();
    signals:
        void barChanged();
    private:
        Q_PROPERTY(int bar READ getBar WRITE setBar RESET resetBar NOTIFY barChanged FINAL)
    };

    inline int Foo::getBar() const
    {
        return bar;
    }

    inline void Foo::setBar(int value)
    {
        if (bar == value)
            return;
        bar = value;
        emit barChanged();
    }

    inline void Foo::resetBar()
    {
        setBar({}); // TODO: Adapt to use your actual default defaultValue
    }
)-";
        testDocuments << CppTestDocument::create("file.h", original, expected);

        QuickFixSettings s;
        s->setterOutsideClassFrom = 1;
        s->getterOutsideClassFrom = 1;
        s->setterParameterNameTemplate = "value";
        s->getterNameTemplate = "get<Name>";

        GenerateGetterSetter factory;
        QuickFixOperationTest(testDocuments, &factory, ProjectExplorer::HeaderPaths(), 4);
    }

    void testOnlySetterHeaderFileWithIncludeGuard()
    {
        QList<TestDocumentPtr> testDocuments;
        const QByteArray     original =
                "#ifndef FILE__H__DECLARED\n"
                "#define FILE__H__DECLARED\n"
                "class Foo\n"
                "{\n"
                "public:\n"
                "    int bar@;\n"
                "};\n"
                "#endif\n";
        const QByteArray expected =
                "#ifndef FILE__H__DECLARED\n"
                "#define FILE__H__DECLARED\n"
                "class Foo\n"
                "{\n"
                "public:\n"
                "    int bar@;\n"
                "    void setBar(int value);\n"
                "};\n\n"
                "inline void Foo::setBar(int value)\n"
                "{\n"
                "    bar = value;\n"
                "}\n"
                "#endif\n";

        testDocuments << CppTestDocument::create("file.h", original, expected);

        QuickFixSettings s;
        s->setterOutsideClassFrom = 1;
        s->setterParameterNameTemplate = "value";

        GenerateGetterSetter factory;
        QuickFixOperationTest(testDocuments, &factory, ProjectExplorer::HeaderPaths(), 0);
    }

    void testFunctionAsTemplateArg()
    {
        QList<TestDocumentPtr> testDocuments;
        const QByteArray original = R"(
    template<typename T> class TS {};
    template<typename T, typename U> class TS<T(U)> {};

    class S2 {
        TS<int(int)> @member;
    };
)";
        const QByteArray expected = R"(
    template<typename T> class TS {};
    template<typename T, typename U> class TS<T(U)> {};

    class S2 {
        TS<int(int)> member;

    public:
        const TS<int (int)> &getMember() const
        {
            return member;
        }
    };
)";

        testDocuments << CppTestDocument::create("file.h", original, expected);

        QuickFixSettings s;
        s->getterOutsideClassFrom = 0;
        s->getterInCppFileFrom = 0;
        s->getterNameTemplate = "get<Name>";
        s->returnByConstRef = true;

        GenerateGetterSetter factory;
        QuickFixOperationTest(testDocuments, &factory, ProjectExplorer::HeaderPaths(), 1);
    }

    void testNotTriggeringOnMemberFunction()
    {
        const QByteArray input = "class Something { void @f(); };\n";
        GenerateGetterSetter factory;
        QuickFixOperationTest({CppTestDocument::create("file.h", input, {})}, &factory);
    }

    void testNotTriggeringOnMemberArray()
    {
        const QByteArray input = "class Something { void @a[10]; };\n";
        GenerateGetterSetter factory;
        QuickFixOperationTest({CppTestDocument::create("file.h", input, {})}, &factory);
    }
};

class GenerateGettersSettersTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data()
    {
        QTest::addColumn<QByteArray>("original");
        QTest::addColumn<QByteArray>("expected");

        const QByteArray onlyReset = R"(
    class Foo {
    public:
        int bar() const;
        void setBar(int bar);
    private:
        int m_bar;
    @};)";

        const QByteArray onlyResetAfter = R"(
    class @Foo {
    public:
        int bar() const;
        void setBar(int bar);
        void resetBar();

    private:
        int m_bar;
    };
    inline void Foo::resetBar()
    {
        setBar({}); // TODO: Adapt to use your actual default defaultValue
    }
)";
        QTest::addRow("only reset") << onlyReset << onlyResetAfter;

        const QByteArray withCandidates = R"(
    class @Foo {
    public:
        int bar() const;
        void setBar(int bar) { m_bar = bar; }

        int getBar2() const;

        int m_alreadyPublic;

    private:
        friend void distraction();
        class AnotherDistraction {};
        enum EvenMoreDistraction { val1, val2 };

        int m_bar;
        int bar2_;
        QString bar3;
    };)";
        const QByteArray after = R"(
    class Foo {
    public:
        int bar() const;
        void setBar(int bar) { m_bar = bar; }

        int getBar2() const;

        int m_alreadyPublic;

        void resetBar();
        void setBar2(int value);
        void resetBar2();
        const QString &getBar3() const;
        void setBar3(const QString &value);
        void resetBar3();

    signals:
        void bar2Changed();
        void bar3Changed();

    private:
        friend void distraction();
        class AnotherDistraction {};
        enum EvenMoreDistraction { val1, val2 };

        int m_bar;
        int bar2_;
        QString bar3;
        Q_PROPERTY(int bar2 READ getBar2 WRITE setBar2 RESET resetBar2 NOTIFY bar2Changed FINAL)
        Q_PROPERTY(QString bar3 READ getBar3 WRITE setBar3 RESET resetBar3 NOTIFY bar3Changed FINAL)
    };
    inline void Foo::resetBar()
    {
        setBar({}); // TODO: Adapt to use your actual default defaultValue
    }

    inline void Foo::setBar2(int value)
    {
        if (bar2_ == value)
            return;
        bar2_ = value;
        emit bar2Changed();
    }

    inline void Foo::resetBar2()
    {
        setBar2({}); // TODO: Adapt to use your actual default defaultValue
    }

    inline const QString &Foo::getBar3() const
    {
        return bar3;
    }

    inline void Foo::setBar3(const QString &value)
    {
        if (bar3 == value)
            return;
        bar3 = value;
        emit bar3Changed();
    }

    inline void Foo::resetBar3()
    {
        setBar3({}); // TODO: Adapt to use your actual default defaultValue
    }
)";
        QTest::addRow("with candidates") << withCandidates << after;
    }

    void test()
    {
        class TestFactory : public GenerateGettersSettersForClass
        {
        public:
            TestFactory() { setTest(); }
        };

        QFETCH(QByteArray, original);
        QFETCH(QByteArray, expected);

        QuickFixSettings s;
        s->getterNameTemplate = "get<Name>";
        s->setterParameterNameTemplate = "value";
        s->setterOutsideClassFrom = 1;
        s->getterOutsideClassFrom = 1;
        s->returnByConstRef = true;

        TestFactory factory;
        QuickFixOperationTest({CppTestDocument::create("file.h", original, expected)}, &factory);
    }
};

class InsertQtPropertyMembersTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data()
    {
        QTest::addColumn<QByteArray>("original");
        QTest::addColumn<QByteArray>("expected");

        QTest::newRow("InsertQtPropertyMembers")
            << QByteArray("struct QObject { void connect(); }\n"
                 "struct XmarksTheSpot : public QObject {\n"
                 "    @Q_PROPERTY(int it READ getIt WRITE setIt RESET resetIt NOTIFY itChanged)\n"
                 "};\n")
            << QByteArray("struct QObject { void connect(); }\n"
                 "struct XmarksTheSpot : public QObject {\n"
                 "    Q_PROPERTY(int it READ getIt WRITE setIt RESET resetIt NOTIFY itChanged)\n"
                 "\n"
                 "public:\n"
                 "    int getIt() const;\n"
                 "\n"
                 "public slots:\n"
                 "    void setIt(int it)\n"
                 "    {\n"
                 "        if (m_it == it)\n"
                 "            return;\n"
                 "        m_it = it;\n"
                 "        emit itChanged(m_it);\n"
                 "    }\n"
                 "    void resetIt()\n"
                 "    {\n"
                 "        setIt({}); // TODO: Adapt to use your actual default value\n"
                 "    }\n"
                 "\n"
                 "signals:\n"
                 "    void itChanged(int it);\n"
                 "\n"
                 "private:\n"
                 "    int m_it;\n"
                 "};\n"
                 "\n"
                 "int XmarksTheSpot::getIt() const\n"
                 "{\n"
                 "    return m_it;\n"
                 "}\n");

        QTest::newRow("InsertQtPropertyMembersResetWithoutSet")
            << QByteArray("struct QObject { void connect(); }\n"
                 "struct XmarksTheSpot : public QObject {\n"
                 "    @Q_PROPERTY(int it READ getIt RESET resetIt NOTIFY itChanged)\n"
                 "};\n")
            << QByteArray("struct QObject { void connect(); }\n"
                 "struct XmarksTheSpot : public QObject {\n"
                 "    Q_PROPERTY(int it READ getIt RESET resetIt NOTIFY itChanged)\n"
                 "\n"
                 "public:\n"
                 "    int getIt() const;\n"
                 "\n"
                 "public slots:\n"
                 "    void resetIt()\n"
                 "    {\n"
                 "        static int defaultValue{}; // TODO: Adapt to use your actual default "
                 "value\n"
                 "        if (m_it == defaultValue)\n"
                 "            return;\n"
                 "        m_it = defaultValue;\n"
                 "        emit itChanged(m_it);\n"
                 "    }\n"
                 "\n"
                 "signals:\n"
                 "    void itChanged(int it);\n"
                 "\n"
                 "private:\n"
                 "    int m_it;\n"
                 "};\n"
                 "\n"
                 "int XmarksTheSpot::getIt() const\n"
                 "{\n"
                 "    return m_it;\n"
                 "}\n");

        QTest::newRow("InsertQtPropertyMembersResetWithoutSetAndNotify")
            << QByteArray("struct QObject { void connect(); }\n"
                 "struct XmarksTheSpot : public QObject {\n"
                 "    @Q_PROPERTY(int it READ getIt RESET resetIt)\n"
                 "};\n")
            << QByteArray("struct QObject { void connect(); }\n"
                 "struct XmarksTheSpot : public QObject {\n"
                 "    Q_PROPERTY(int it READ getIt RESET resetIt)\n"
                 "\n"
                 "public:\n"
                 "    int getIt() const;\n"
                 "\n"
                 "public slots:\n"
                 "    void resetIt()\n"
                 "    {\n"
                 "        static int defaultValue{}; // TODO: Adapt to use your actual default "
                 "value\n"
                 "        m_it = defaultValue;\n"
                 "    }\n"
                 "\n"
                 "private:\n"
                 "    int m_it;\n"
                 "};\n"
                 "\n"
                 "int XmarksTheSpot::getIt() const\n"
                 "{\n"
                 "    return m_it;\n"
                 "}\n");

        QTest::newRow("InsertQtPropertyMembersPrivateBeforePublic")
            << QByteArray("struct QObject { void connect(); }\n"
                 "class XmarksTheSpot : public QObject {\n"
                 "private:\n"
                 "    @Q_PROPERTY(int it READ getIt WRITE setIt NOTIFY itChanged)\n"
                 "public:\n"
                 "    void find();\n"
                 "};\n")
            << QByteArray("struct QObject { void connect(); }\n"
                 "class XmarksTheSpot : public QObject {\n"
                 "private:\n"
                 "    Q_PROPERTY(int it READ getIt WRITE setIt NOTIFY itChanged)\n"
                 "    int m_it;\n"
                 "\n"
                 "public:\n"
                 "    void find();\n"
                 "    int getIt() const;\n"
                 "public slots:\n"
                 "    void setIt(int it)\n"
                 "    {\n"
                 "        if (m_it == it)\n"
                 "            return;\n"
                 "        m_it = it;\n"
                 "        emit itChanged(m_it);\n"
                 "    }\n"
                 "signals:\n"
                 "    void itChanged(int it);\n"
                 "};\n"
                 "\n"
                 "int XmarksTheSpot::getIt() const\n"
                 "{\n"
                 "    return m_it;\n"
                 "}\n");
    }

    void test()
    {
        QFETCH(QByteArray, original);
        QFETCH(QByteArray, expected);

        QuickFixSettings s;
        s->setterAsSlot = true;
        s->setterInCppFileFrom = 0;
        s->setterParameterNameTemplate = "<name>";
        s->signalWithNewValue = true;

        InsertQtPropertyMembers factory;
        QuickFixOperationTest({CppTestDocument::create("file.cpp", original, expected)}, &factory);
    }

    void testNotTriggeringOnInvalidCode()
    {
        const QByteArray input = "class C { @Q_PROPERTY(typeid foo READ foo) };\n";
        InsertQtPropertyMembers factory;
        QuickFixOperationTest({CppTestDocument::create("file.h", input, {})}, &factory);
    }
};

class GenerateConstructorTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data()
    {
        QTest::addColumn<QByteArray>("original_header");
        QTest::addColumn<QByteArray>("expected_header");
        QTest::addColumn<QByteArray>("original_source");
        QTest::addColumn<QByteArray>("expected_source");
        QTest::addColumn<int>("location");
        const int Inside = ConstructorLocation::Inside;
        const int Outside = ConstructorLocation::Outside;
        const int CppGenNamespace = ConstructorLocation::CppGenNamespace;
        const int CppGenUsingDirective = ConstructorLocation::CppGenUsingDirective;
        const int CppRewriteType = ConstructorLocation::CppRewriteType;

        QByteArray header = R"--(
class@ Foo{
    int test;
    static int s;
};
)--";
        QByteArray expected = R"--(
class Foo{
    int test;
    static int s;
public:
    Foo(int test) : test(test)
    {}
};
)--";
        QTest::newRow("ignore static") << header << expected << QByteArray() << QByteArray() << Inside;

        header = R"--(
class@ Foo{
    CustomType test;
};
)--";
        expected = R"--(
class Foo{
    CustomType test;
public:
    Foo(CustomType test) : test(std::move(test))
    {}
};
)--";
        QTest::newRow("Move custom value types")
            << header << expected << QByteArray() << QByteArray() << Inside;

        header = R"--(
class@ Foo{
    int test;
protected:
    Foo() = default;
};
)--";
        expected = R"--(
class Foo{
    int test;
public:
    Foo(int test) : test(test)
    {}

protected:
    Foo() = default;
};
)--";

        QTest::newRow("new section before existing")
            << header << expected << QByteArray() << QByteArray() << Inside;

        header = R"--(
class@ Foo{
    int test;
};
)--";
        expected = R"--(
class Foo{
    int test;
public:
    Foo(int test) : test(test)
    {}
};
)--";
        QTest::newRow("new section at end")
            << header << expected << QByteArray() << QByteArray() << Inside;

        header = R"--(
class@ Foo{
    int test;
public:
    /**
     * Random comment
     */
    Foo(int i, int i2);
};
)--";
        expected = R"--(
class Foo{
    int test;
public:
    Foo(int test) : test(test)
    {}
    /**
     * Random comment
     */
    Foo(int i, int i2);
};
)--";
        QTest::newRow("in section before")
            << header << expected << QByteArray() << QByteArray() << Inside;

        header = R"--(
class@ Foo{
    int test;
public:
    Foo() = default;
};
)--";
        expected = R"--(
class Foo{
    int test;
public:
    Foo() = default;
    Foo(int test) : test(test)
    {}
};
)--";
        QTest::newRow("in section after")
            << header << expected << QByteArray() << QByteArray() << Inside;

        header = R"--(
class@ Foo{
    int test1;
    int test2;
    int test3;
public:
};
)--";
        expected = R"--(
class Foo{
    int test1;
    int test2;
    int test3;
public:
    Foo(int test2, int test3, int test1) : test1(test1),
        test2(test2),
        test3(test3)
    {}
};
)--";
        // No worry, that is not the default behavior.
        // Move first member to the back when testing with 3 or more members
        QTest::newRow("changed parameter order")
            << header << expected << QByteArray() << QByteArray() << Inside;

        header = R"--(
class@ Foo{
    int test;
    int di_test;
public:
};
)--";
        expected = R"--(
class Foo{
    int test;
    int di_test;
public:
    Foo(int test, int di_test = 42) : test(test),
        di_test(di_test)
    {}
};
)--";
        QTest::newRow("default parameters")
            << header << expected << QByteArray() << QByteArray() << Inside;

        header = R"--(
struct Bar{
    Bar(int i);
};
class@ Foo : public Bar{
    int test;
public:
};
)--";
        expected = R"--(
struct Bar{
    Bar(int i);
};
class Foo : public Bar{
    int test;
public:
    Foo(int test, int i) : Bar(i),
        test(test)
    {}
};
)--";
        QTest::newRow("parent constructor")
            << header << expected << QByteArray() << QByteArray() << Inside;

        header = R"--(
struct Bar{
    Bar(int use_i = 6);
};
class@ Foo : public Bar{
    int test;
public:
};
)--";
        expected = R"--(
struct Bar{
    Bar(int use_i = 6);
};
class Foo : public Bar{
    int test;
public:
    Foo(int test, int use_i = 6) : Bar(use_i),
        test(test)
    {}
};
)--";
        QTest::newRow("parent constructor with default")
            << header << expected << QByteArray() << QByteArray() << Inside;

        header = R"--(
struct Bar{
    Bar(int use_i = L'A', int use_i2 = u8"B");
};
class@ Foo : public Bar{
public:
};
)--";
        expected = R"--(
struct Bar{
    Bar(int use_i = L'A', int use_i2 = u8"B");
};
class Foo : public Bar{
public:
    Foo(int use_i = L'A', int use_i2 = u8"B") : Bar(use_i, use_i2)
    {}
};
)--";
        QTest::newRow("parent constructor with char/string default value")
            << header << expected << QByteArray() << QByteArray() << Inside;

        const QByteArray common = R"--(
namespace N{
    template<typename T>
    struct vector{
    };
}
)--";
        header = common + R"--(
namespace M{
enum G{g};
class@ Foo{
    N::vector<G> g;
    enum E{e}e;
public:
};
}
)--";

        expected = common + R"--(
namespace M{
enum G{g};
class@ Foo{
    N::vector<G> g;
    enum E{e}e;
public:
    Foo(const N::vector<G> &g, E e);
};

Foo::Foo(const N::vector<G> &g, Foo::E e) : g(g),
    e(e)
{}

}
)--";
        QTest::newRow("source: right type outside class ")
            << QByteArray() << QByteArray() << header << expected << Outside;
        expected = common + R"--(
namespace M{
enum G{g};
class@ Foo{
    N::vector<G> g;
    enum E{e}e;
public:
    Foo(const N::vector<G> &g, E e);
};
}


inline M::Foo::Foo(const N::vector<M::G> &g, M::Foo::E e) : g(g),
    e(e)
{}

)--";
        QTest::newRow("header: right type outside class ")
            << header << expected << QByteArray() << QByteArray() << Outside;

        expected = common + R"--(
namespace M{
enum G{g};
class@ Foo{
    N::vector<G> g;
    enum E{e}e;
public:
    Foo(const N::vector<G> &g, E e);
};
}
)--";
        const QByteArray source = R"--(
#include "file.h"
)--";
        QByteArray expected_source = R"--(
#include "file.h"


namespace M {
Foo::Foo(const N::vector<G> &g, Foo::E e) : g(g),
    e(e)
{}

}
)--";
        QTest::newRow("source: right type inside namespace")
            << header << expected << source << expected_source << CppGenNamespace;

        expected_source = R"--(
#include "file.h"

using namespace M;
Foo::Foo(const N::vector<G> &g, Foo::E e) : g(g),
    e(e)
{}
)--";
        QTest::newRow("source: right type with using directive")
            << header << expected << source << expected_source << CppGenUsingDirective;

        expected_source = R"--(
#include "file.h"

M::Foo::Foo(const N::vector<M::G> &g, M::Foo::E e) : g(g),
    e(e)
{}
)--";
        QTest::newRow("source: right type while rewritung types")
            << header << expected << source << expected_source << CppRewriteType;

    }

    void test()
    {
        class TestFactory : public GenerateConstructor
        {
        public:
            TestFactory() { setTest(); }
        };

        QFETCH(QByteArray, original_header);
        QFETCH(QByteArray, expected_header);
        QFETCH(QByteArray, original_source);
        QFETCH(QByteArray, expected_source);
        QFETCH(int, location);

        QuickFixSettings s;
        s->valueTypes << "CustomType";
        using L = ConstructorLocation;
        if (location == L::Inside) {
            s->setterInCppFileFrom = -1;
            s->setterOutsideClassFrom = -1;
        } else if (location == L::Outside) {
            s->setterInCppFileFrom = -1;
            s->setterOutsideClassFrom = 1;
        } else if (location >= L::CppGenNamespace && location <= L::CppRewriteType) {
            s->setterInCppFileFrom = 1;
            s->setterOutsideClassFrom = -1;
            using Handling = CppQuickFixSettings::MissingNamespaceHandling;
            if (location == L::CppGenNamespace)
                s->cppFileNamespaceHandling = Handling::CreateMissing;
            else if (location == L::CppGenUsingDirective)
                s->cppFileNamespaceHandling = Handling::AddUsingDirective;
            else if (location == L::CppRewriteType)
                s->cppFileNamespaceHandling = Handling::RewriteType;
        } else {
            QFAIL("location is none of the values of the ConstructorLocation enum");
        }

        QList<TestDocumentPtr> testDocuments;
        testDocuments << CppTestDocument::create("file.h", original_header, expected_header);
        testDocuments << CppTestDocument::create("file.cpp", original_source, expected_source);
        TestFactory factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

private:
    enum ConstructorLocation { Inside, Outside, CppGenNamespace, CppGenUsingDirective, CppRewriteType };
};

QObject *GenerateGetterSetter::createTest()
{
    return new GenerateGetterSetterTest;
}

QObject *GenerateGettersSettersForClass::createTest()
{
    return new GenerateGettersSettersTest;
}

QObject *InsertQtPropertyMembers::createTest()
{
    return new InsertQtPropertyMembersTest;
}

QObject *GenerateConstructor::createTest()
{
    return new GenerateConstructorTest;
}

#endif // WITH_TESTS

} // namespace

void registerCodeGenerationQuickfixes()
{
    CppQuickFixFactory::registerFactory<GenerateGetterSetter>();
    CppQuickFixFactory::registerFactory<GenerateGettersSettersForClass>();
    CppQuickFixFactory::registerFactory<GenerateConstructor>();
    CppQuickFixFactory::registerFactory<InsertQtPropertyMembers>();
}

} // namespace CppEditor::Internal

#include <cppcodegenerationquickfixes.moc>
