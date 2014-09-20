/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "cppinsertvirtualmethods.h"
#include "cppquickfixassistant.h"

#include <coreplugin/icore.h>
#include <cpptools/cppcodestylesettings.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/functionutils.h>
#include <cpptools/insertionpointlocator.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <cplusplus/CppRewriter.h>
#include <cplusplus/Overview.h>
#include <utils/changeset.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QPointer>
#include <QQueue>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTextDocument>
#include <QTreeView>
#include <QVBoxLayout>

#ifdef WITH_TESTS

#include "cppeditorplugin.h"
#include "cppquickfix_test.h"

#include <QtTest>

#endif


using namespace CPlusPlus;
using namespace CppTools;
using namespace TextEditor;

namespace CppEditor {
namespace Internal {

class InsertVirtualMethodsModel;

class InsertVirtualMethodsDialog : public QDialog
{
    Q_OBJECT
public:
    enum CustomItemRoles {
        Reimplemented = Qt::UserRole
    };

    enum ImplementationMode {
        ModeOnlyDeclarations = 0x00000001,
        ModeInsideClass = 0x00000002,
        ModeOutsideClass = 0x00000004,
        ModeImplementationFile = 0x00000008
    };

    InsertVirtualMethodsDialog(QWidget *parent = 0);
    void initGui();
    void initData();
    virtual ImplementationMode implementationMode() const;
    void setImplementationsMode(ImplementationMode mode);
    virtual bool insertKeywordVirtual() const;
    void setInsertKeywordVirtual(bool insert);
    void setHasImplementationFile(bool file);
    void setHasReimplementedFunctions(bool functions);
    bool hideReimplementedFunctions() const;
    virtual bool gather();

private slots:
    void setHideReimplementedFunctions(bool hide);

private:
    QTreeView *m_view;
    QCheckBox *m_hideReimplementedFunctions;
    QComboBox *m_insertMode;
    QCheckBox *m_virtualKeyword;
    QDialogButtonBox *m_buttons;
    QList<bool> m_expansionStateNormal;
    QList<bool> m_expansionStateReimp;
    bool m_hasImplementationFile;
    bool m_hasReimplementedFunctions;

    void saveExpansionState();
    void restoreExpansionState();

protected:
    ImplementationMode m_implementationMode;
    bool m_insertKeywordVirtual;

public:
    InsertVirtualMethodsModel *classFunctionModel;
    QSortFilterProxyModel *classFunctionFilterModel;
};

} // namespace Internal
} // namespace CppEditor

Q_DECLARE_METATYPE(CppEditor::Internal::InsertVirtualMethodsDialog::ImplementationMode)

namespace {

class InsertVirtualMethodsItem
{
public:
    InsertVirtualMethodsItem(InsertVirtualMethodsItem *parent) :
        row(-1),
        m_parent(parent)
    {
    }

    virtual ~InsertVirtualMethodsItem()
    {
    }

    virtual QString description() const = 0;
    virtual Qt::ItemFlags flags() const = 0;
    virtual Qt::CheckState checkState() const = 0;

    InsertVirtualMethodsItem *parent() { return m_parent; }

    int row;

private:
    InsertVirtualMethodsItem *m_parent;
};

class FunctionItem;

class ClassItem : public InsertVirtualMethodsItem
{
public:
    ClassItem(const QString &className, const Class *clazz);
    ~ClassItem();

    QString description() const { return name; }
    Qt::ItemFlags flags() const;
    Qt::CheckState checkState() const;
    void removeFunction(int row);

    const Class *klass;
    const QString name;
    QList<FunctionItem *> functions;
};

class FunctionItem : public InsertVirtualMethodsItem
{
public:
    FunctionItem(const Function *func, const QString &functionName, ClassItem *parent);
    QString description() const;
    Qt::ItemFlags flags() const;
    Qt::CheckState checkState() const { return checked ? Qt::Checked : Qt::Unchecked; }

    const Function *function;
    InsertionPointLocator::AccessSpec accessSpec;
    bool reimplemented;
    bool alreadyFound;
    bool checked;
    FunctionItem *nextOverride;

private:
    QString name;
};

ClassItem::ClassItem(const QString &className, const Class *clazz) :
    InsertVirtualMethodsItem(0),
    klass(clazz),
    name(className)
{
}

ClassItem::~ClassItem()
{
    qDeleteAll(functions);
    functions.clear();
}

Qt::ItemFlags ClassItem::flags() const
{
    foreach (FunctionItem *func, functions) {
        if (!func->alreadyFound)
            return Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled;
    }

    return Qt::ItemIsSelectable;
}

Qt::CheckState ClassItem::checkState() const
{
    if (functions.isEmpty())
        return Qt::Unchecked;
    Qt::CheckState state = functions.first()->checkState();
    foreach (FunctionItem *function, functions) {
        Qt::CheckState functionState = function->checkState();
        if (functionState != state)
            return Qt::PartiallyChecked;
    }
    return state;
}

void ClassItem::removeFunction(int row)
{
    QTC_ASSERT(row >= 0 && row < functions.count(), return);
    functions.removeAt(row);
    // Update row number for all the following functions
    for (int r = row, total = functions.count(); r < total; ++r)
        functions[r]->row = r;
}

FunctionItem::FunctionItem(const Function *func, const QString &functionName, ClassItem *parent) :
    InsertVirtualMethodsItem(parent),
    function(func),
    reimplemented(false),
    alreadyFound(false),
    checked(false),
    nextOverride(this)
{
    name = functionName;
}

QString FunctionItem::description() const
{
    return name;
}

Qt::ItemFlags FunctionItem::flags() const
{
    Qt::ItemFlags res = Qt::NoItemFlags;
    if (!alreadyFound)
        res |= Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled;
    return res;
}

} // namespace

namespace CppEditor {
namespace Internal {

class InsertVirtualMethodsModel : public QAbstractItemModel
{
public:
    InsertVirtualMethodsModel(QObject *parent = 0) : QAbstractItemModel(parent)
    {
        const TextEditor::FontSettings &fs = TextEditor::TextEditorSettings::fontSettings();
        formatReimpFunc = fs.formatFor(C_DISABLED_CODE);
    }

    ~InsertVirtualMethodsModel()
    {
        clear();
    }

    void clear()
    {
        beginResetModel();
        qDeleteAll(classes);
        classes.clear();
        endResetModel();
    }

    QModelIndex index(int row, int column, const QModelIndex &parent) const
    {
        if (column != 0)
            return QModelIndex();
        if (parent.isValid()) {
            ClassItem *classItem = static_cast<ClassItem *>(parent.internalPointer());
            return createIndex(row, column, classItem->functions.at(row));
        }
        return createIndex(row, column, classes.at(row));
    }

    QModelIndex parent(const QModelIndex &child) const
    {
        if (!child.isValid())
            return QModelIndex();
        InsertVirtualMethodsItem *parent = itemForIndex(child)->parent();
        return parent ? createIndex(parent->row, 0, parent) : QModelIndex();
    }

    int rowCount(const QModelIndex &parent) const
    {
        if (!parent.isValid())
            return classes.count();
        InsertVirtualMethodsItem *item = itemForIndex(parent);
        if (item->parent()) // function -> no children
            return 0;
        return static_cast<ClassItem *>(item)->functions.count();
    }

    int columnCount(const QModelIndex &) const
    {
        return 1;
    }

    void addClass(ClassItem *classItem)
    {
        int row = classes.count();
        classItem->row = row;
        beginInsertRows(QModelIndex(), row, row);
        classes.append(classItem);
        endInsertRows();
    }

    void removeFunction(FunctionItem *funcItem)
    {
        ClassItem *classItem = static_cast<ClassItem *>(funcItem->parent());
        beginRemoveRows(createIndex(classItem->row, 0, classItem), funcItem->row, funcItem->row);
        classItem->removeFunction(funcItem->row);
        endRemoveRows();
    }

    QVariant data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return QVariant();

        InsertVirtualMethodsItem *item = itemForIndex(index);
        switch (role) {
        case Qt::DisplayRole:
            return item->description();
        case Qt::CheckStateRole:
            return item->checkState();
        case Qt::ForegroundRole:
            if (item->parent() && static_cast<FunctionItem *>(item)->alreadyFound)
                return formatReimpFunc.foreground();
            break;
        case Qt::BackgroundRole:
            if (item->parent() && static_cast<FunctionItem *>(item)->alreadyFound) {
                const QColor background = formatReimpFunc.background();
                if (background.isValid())
                    return background;
            }
            break;
        case InsertVirtualMethodsDialog::Reimplemented:
            if (item->parent()) {
                FunctionItem *function = static_cast<FunctionItem *>(item);
                return QVariant(function->alreadyFound);
            }

        }
        return QVariant();
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role)
    {
        if (!index.isValid())
            return false;

        InsertVirtualMethodsItem *item = itemForIndex(index);
        switch (role) {
        case Qt::CheckStateRole: {
            bool checked = value.toInt() == Qt::Checked;
            if (item->parent()) {
                FunctionItem *funcItem = static_cast<FunctionItem *>(item);
                while (funcItem->checked != checked) {
                    funcItem->checked = checked;
                    const QModelIndex funcIndex = createIndex(funcItem->row, 0, funcItem);
                    emit dataChanged(funcIndex, funcIndex);
                    const QModelIndex parentIndex =
                            createIndex(funcItem->parent()->row, 0, funcItem->parent());
                    emit dataChanged(parentIndex, parentIndex);
                    funcItem = funcItem->nextOverride;
                }
            } else {
                ClassItem *classItem = static_cast<ClassItem *>(item);
                foreach (FunctionItem *funcItem, classItem->functions) {
                    if (funcItem->alreadyFound || funcItem->checked == checked)
                        continue;
                    QModelIndex funcIndex = createIndex(funcItem->row, 0, funcItem);
                    setData(funcIndex, value, role);
                }
            }
            return true;
        }
        }
        return QAbstractItemModel::setData(index, value, role);
    }

    Qt::ItemFlags flags(const QModelIndex &index) const
    {
        if (!index.isValid())
            return Qt::NoItemFlags;
        return itemForIndex(index)->flags();
    }

    QList<ClassItem *> classes;

private:
    Format formatReimpFunc;

    InsertVirtualMethodsItem *itemForIndex(const QModelIndex &index) const
    {
        return static_cast<InsertVirtualMethodsItem *>(index.internalPointer());
    }
};

class InsertVirtualMethodsOp : public CppQuickFixOperation
{
public:
    InsertVirtualMethodsOp(const CppQuickFixInterface &interface,
                           InsertVirtualMethodsDialog *factory)
        : CppQuickFixOperation(interface, 0)
        , m_factory(factory)
        , m_classAST(0)
        , m_valid(false)
        , m_cppFileName(QString::null)
        , m_insertPosDecl(0)
        , m_insertPosOutside(0)
        , m_functionCount(0)
    {
        setDescription(QCoreApplication::translate(
                           "CppEditor::QuickFix", "Insert Virtual Functions of Base Classes"));

        const QList<AST *> &path = interface.path();
        const int pathSize = path.size();
        if (pathSize < 2)
            return;

        // Determine if cursor is on a class or a base class
        if (SimpleNameAST *nameAST = path.at(pathSize - 1)->asSimpleName()) {
            if (!interface.isCursorOn(nameAST))
                return;

            if (!(m_classAST = path.at(pathSize - 2)->asClassSpecifier())) { // normal class
                int index = pathSize - 2;
                const BaseSpecifierAST *baseAST = path.at(index)->asBaseSpecifier();// simple bclass
                if (!baseAST) {
                    if (index > 0 && path.at(index)->asQualifiedName()) // namespaced base class
                        baseAST = path.at(--index)->asBaseSpecifier();
                }
                --index;
                if (baseAST && index >= 0)
                    m_classAST = path.at(index)->asClassSpecifier();
            }
        }
        if (!m_classAST || !m_classAST->base_clause_list)
            return;

        // Determine insert positions
        const int endOfClassAST = interface.currentFile()->endOf(m_classAST);
        m_insertPosDecl = endOfClassAST - 1; // Skip last "}"
        m_insertPosOutside = endOfClassAST + 1; // Step over ";"

        // Determine base classes
        QList<const Class *> baseClasses;
        QQueue<ClassOrNamespace *> baseClassQueue;
        QSet<ClassOrNamespace *> visitedBaseClasses;
        if (ClassOrNamespace *clazz = interface.context().lookupType(m_classAST->symbol))
            baseClassQueue.enqueue(clazz);
        while (!baseClassQueue.isEmpty()) {
            ClassOrNamespace *clazz = baseClassQueue.dequeue();
            visitedBaseClasses.insert(clazz);
            const QList<ClassOrNamespace *> bases = clazz->usings();
            foreach (ClassOrNamespace *baseClass, bases) {
                foreach (Symbol *symbol, baseClass->symbols()) {
                    Class *base = symbol->asClass();
                    if (base
                            && (clazz = interface.context().lookupType(symbol))
                            && !visitedBaseClasses.contains(clazz)
                            && !baseClasses.contains(base)) {
                        baseClasses.prepend(base);
                        baseClassQueue.enqueue(clazz);
                    }
                }
            }
        }

        // Determine virtual functions
        m_factory->classFunctionModel->clear();
        Overview printer = CppCodeStyleSettings::currentProjectCodeStyleOverview();
        printer.showFunctionSignatures = true;
        QHash<const Function *, FunctionItem *> virtualFunctions;
        foreach (const Class *clazz, baseClasses) {
            ClassItem *itemBase = new ClassItem(printer.prettyName(clazz->name()), clazz);
            for (Scope::iterator it = clazz->firstMember(); it != clazz->lastMember(); ++it) {
                if (const Function *func = (*it)->type()->asFunctionType()) {
                    // Filter virtual destructors
                    if (func->name()->asDestructorNameId())
                        continue;

                    const Function *firstVirtual = 0;
                    const bool isVirtual = FunctionUtils::isVirtualFunction(
                                func, interface.context(), &firstVirtual);
                    if (!isVirtual)
                        continue;

                    if (func->isFinal()) {
                        if (FunctionItem *first = virtualFunctions[firstVirtual]) {
                            FunctionItem *next = 0;
                            for (FunctionItem *removed = first; next != first; removed = next) {
                                next = removed->nextOverride;
                                m_factory->classFunctionModel->removeFunction(removed);
                                delete removed;
                            };
                            virtualFunctions.remove(firstVirtual);
                        }
                        continue;
                    }
                    // Filter OQbject's
                    //   - virtual const QMetaObject *metaObject() const;
                    //   - virtual void *qt_metacast(const char *);
                    //   - virtual int qt_metacall(QMetaObject::Call, int, void **);
                    if (printer.prettyName(firstVirtual->enclosingClass()->name())
                            == QLatin1String("QObject")) {
                        const QString funcName = printer.prettyName(func->name());
                        if (funcName == QLatin1String("metaObject")
                                || funcName == QLatin1String("qt_metacast")
                                || funcName == QLatin1String("qt_metacall")) {
                            continue;
                        }
                    }

                    // Do not implement existing functions inside target class
                    bool funcExistsInClass = false;
                    const Name *funcName = func->name();
                    for (Symbol *symbol = m_classAST->symbol->find(funcName->identifier());
                         symbol; symbol = symbol->next()) {
                        if (!symbol->name()
                                || !funcName->identifier()->match(symbol->identifier())) {
                            continue;
                        }
                        if (symbol->type().match(func->type())) {
                            funcExistsInClass = true;
                            break;
                        }
                    }

                    // Construct function item
                    const bool isReimplemented = (func != firstVirtual);
                    const bool isPureVirtual = func->isPureVirtual();
                    QString itemName = printer.prettyType(func->type(), func->name());
                    if (isPureVirtual)
                        itemName += QLatin1String(" = 0");
                    const QString itemReturnTypeString = printer.prettyType(func->returnType());
                    itemName += QLatin1String(" : ") + itemReturnTypeString;
                    if (isReimplemented)
                        itemName += QLatin1String(" (redeclared)");
                    FunctionItem *funcItem = new FunctionItem(func, itemName, itemBase);
                    if (isReimplemented) {
                        factory->setHasReimplementedFunctions(true);
                        funcItem->reimplemented = true;
                        funcItem->alreadyFound = funcExistsInClass;
                        if (FunctionItem *first = virtualFunctions[firstVirtual]) {
                            if (!first->alreadyFound) {
                                while (first->checked != isPureVirtual) {
                                    first->checked = isPureVirtual;
                                    first = first->nextOverride;
                                }
                            }
                            funcItem->checked = first->checked;
                            funcItem->nextOverride = first->nextOverride;
                            first->nextOverride = funcItem;
                        }
                    } else {
                        if (!funcExistsInClass) {
                            funcItem->checked = isPureVirtual;
                        } else {
                            funcItem->alreadyFound = true;
                            funcItem->checked = true;
                            factory->setHasReimplementedFunctions(true);
                        }
                    }

                    funcItem->accessSpec = acessSpec(*it);
                    funcItem->row = itemBase->functions.count();
                    itemBase->functions.append(funcItem);

                    virtualFunctions[func] = funcItem;

                    // update internal counters
                    if (!funcExistsInClass)
                        ++m_functionCount;
                }
            }

            if (itemBase->functions.isEmpty())
                delete itemBase;
            else
                m_factory->classFunctionModel->addClass(itemBase);
        }
        if (m_factory->classFunctionModel->classes.isEmpty() || m_functionCount == 0)
            return;

        bool isHeaderFile = false;
        m_cppFileName = correspondingHeaderOrSource(interface.fileName(), &isHeaderFile);
        m_factory->setHasImplementationFile(isHeaderFile && !m_cppFileName.isEmpty());

        m_valid = true;
    }

    bool isValid() const
    {
        return m_valid;
    }

    InsertionPointLocator::AccessSpec acessSpec(const Symbol *symbol)
    {
        const Function *func = symbol->type()->asFunctionType();
        if (!func)
            return InsertionPointLocator::Invalid;
        if (func->isSignal())
            return InsertionPointLocator::Signals;

        InsertionPointLocator::AccessSpec spec = InsertionPointLocator::Invalid;
        if (symbol->isPrivate())
            spec = InsertionPointLocator::Private;
        else if (symbol->isProtected())
            spec = InsertionPointLocator::Protected;
        else if (symbol->isPublic())
            spec = InsertionPointLocator::Public;
        else
            return InsertionPointLocator::Invalid;

        if (func->isSlot()) {
            switch (spec) {
            case InsertionPointLocator::Private:
                return InsertionPointLocator::PrivateSlot;
            case InsertionPointLocator::Protected:
                return InsertionPointLocator::ProtectedSlot;
            case InsertionPointLocator::Public:
                return InsertionPointLocator::PublicSlot;
            default:
                return spec;
            }
        }
        return spec;
    }

    void perform()
    {
        if (!m_factory->gather())
            return;

        Core::ICore::settings()->setValue(
                    QLatin1String("QuickFix/InsertVirtualMethods/insertKeywordVirtual"),
                    m_factory->insertKeywordVirtual());
        Core::ICore::settings()->setValue(
                    QLatin1String("QuickFix/InsertVirtualMethods/implementationMode"),
                    m_factory->implementationMode());
        Core::ICore::settings()->setValue(
                    QLatin1String("QuickFix/InsertVirtualMethods/hideReimplementedFunctions"),
                    m_factory->hideReimplementedFunctions());

        // Insert declarations (and definition if Inside-/OutsideClass)
        Overview printer = CppCodeStyleSettings::currentProjectCodeStyleOverview();
        printer.showFunctionSignatures = true;
        printer.showReturnTypes = true;
        printer.showArgumentNames = true;
        Utils::ChangeSet headerChangeSet;
        const CppRefactoringChanges refactoring(snapshot());
        const QString filename = currentFile()->fileName();
        const CppRefactoringFilePtr headerFile = refactoring.file(filename);
        const LookupContext targetContext(headerFile->cppDocument(), snapshot());

        const Class *targetClass = m_classAST->symbol;
        ClassOrNamespace *targetCoN = targetContext.lookupType(targetClass->enclosingScope());
        if (!targetCoN)
            targetCoN = targetContext.globalNamespace();
        UseMinimalNames useMinimalNames(targetCoN);
        Control *control = context().bindings()->control().data();
        foreach (ClassItem *classItem, m_factory->classFunctionModel->classes) {
            if (classItem->checkState() == Qt::Unchecked)
                continue;

            // Insert Declarations (+ definitions)
            QString lastAccessSpecString;
            bool first = true;
            foreach (FunctionItem *funcItem, classItem->functions) {
                if (funcItem->reimplemented || funcItem->alreadyFound || !funcItem->checked)
                    continue;

                if (first) {
                    // Add comment
                    const QString comment = QLatin1String("\n// ") +
                            printer.prettyName(classItem->klass->name()) +
                            QLatin1String(" interface\n");
                    headerChangeSet.insert(m_insertPosDecl, comment);
                    first = false;
                }
                // Construct declaration
                // setup rewriting to get minimally qualified names
                SubstitutionEnvironment env;
                env.setContext(context());
                env.switchScope(classItem->klass->enclosingScope());
                env.enter(&useMinimalNames);

                QString declaration;
                const FullySpecifiedType tn = rewriteType(funcItem->function->type(), &env, control);
                declaration += printer.prettyType(tn, funcItem->function->unqualifiedName());

                if (m_factory->insertKeywordVirtual())
                    declaration = QLatin1String("virtual ") + declaration;
                if (m_factory->implementationMode() & InsertVirtualMethodsDialog::ModeInsideClass)
                    declaration += QLatin1String("\n{\n}\n");
                else
                    declaration += QLatin1String(";\n");

                const QString accessSpecString =
                        InsertionPointLocator::accessSpecToString(funcItem->accessSpec);
                if (accessSpecString != lastAccessSpecString) {
                    declaration = accessSpecString + declaration;
                    if (!lastAccessSpecString.isEmpty()) // separate if not direct after the comment
                        declaration = QLatin1String("\n") + declaration;
                    lastAccessSpecString = accessSpecString;
                }
                headerChangeSet.insert(m_insertPosDecl, declaration);

                // Insert definition outside class
                if (m_factory->implementationMode() & InsertVirtualMethodsDialog::ModeOutsideClass) {
                    const QString name = printer.prettyName(targetClass->name()) +
                            QLatin1String("::") + printer.prettyName(funcItem->function->name());
                    const QString defText = printer.prettyType(tn, name) + QLatin1String("\n{\n}");
                    headerChangeSet.insert(m_insertPosOutside,  QLatin1String("\n\n") + defText);
                }
            }
        }

        // Write header file
        headerFile->setChangeSet(headerChangeSet);
        headerFile->appendIndentRange(Utils::ChangeSet::Range(m_insertPosDecl, m_insertPosDecl + 1));
        headerFile->setOpenEditor(true, m_insertPosDecl);
        headerFile->apply();

        // Insert in implementation file
        if (m_factory->implementationMode() & InsertVirtualMethodsDialog::ModeImplementationFile) {
            const Symbol *symbol = headerFile->cppDocument()->lastVisibleSymbolAt(
                        targetClass->line(), targetClass->column());
            if (!symbol)
                return;
            const Class *clazz = symbol->asClass();
            if (!clazz)
                return;

            CppRefactoringFilePtr implementationFile = refactoring.file(m_cppFileName);
            Utils::ChangeSet implementationChangeSet;
            const int insertPos = qMax(0, implementationFile->document()->characterCount() - 1);

            // make target lookup context
            Document::Ptr implementationDoc = implementationFile->cppDocument();
            unsigned line, column;
            implementationDoc->translationUnit()->getPosition(insertPos, &line, &column);
            Scope *targetScope = implementationDoc->scopeAt(line, column);
            const LookupContext targetContext(implementationDoc, snapshot());
            ClassOrNamespace *targetCoN = targetContext.lookupType(targetScope);
            if (!targetCoN)
                targetCoN = targetContext.globalNamespace();

            // Loop through inserted declarations
            for (unsigned i = targetClass->memberCount(); i < clazz->memberCount(); ++i) {
                Declaration *decl = clazz->memberAt(i)->asDeclaration();
                if (!decl)
                    continue;

                // setup rewriting to get minimally qualified names
                SubstitutionEnvironment env;
                env.setContext(context());
                env.switchScope(decl->enclosingScope());
                UseMinimalNames q(targetCoN);
                env.enter(&q);
                Control *control = context().bindings()->control().data();

                // rewrite the function type and name + create definition
                const FullySpecifiedType type = rewriteType(decl->type(), &env, control);
                const QString name = printer.prettyName(
                            LookupContext::minimalName(decl, targetCoN, control));
                const QString defText = printer.prettyType(type, name) + QLatin1String("\n{\n}");

                implementationChangeSet.insert(insertPos,  QLatin1String("\n\n") + defText);
            }

            implementationFile->setChangeSet(implementationChangeSet);
            implementationFile->appendIndentRange(Utils::ChangeSet::Range(insertPos, insertPos + 1));
            implementationFile->apply();
        }
    }

private:
    InsertVirtualMethodsDialog *m_factory;
    const ClassSpecifierAST *m_classAST;
    bool m_valid;
    QString m_cppFileName;
    int m_insertPosDecl;
    int m_insertPosOutside;
    unsigned m_functionCount;
};

class InsertVirtualMethodsFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    InsertVirtualMethodsFilterModel(QObject *parent = 0)
        : QSortFilterProxyModel(parent)
        , m_hideReimplemented(false)
    {}

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
    {
        QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

        // Handle base class
        if (!sourceParent.isValid()) {
            // check if any child is valid
            if (!sourceModel()->hasChildren(index))
                return false;
            if (!m_hideReimplemented)
                return true;

            for (int i = 0; i < sourceModel()->rowCount(index); ++i) {
                const QModelIndex child = sourceModel()->index(i, 0, index);
                if (!child.data(InsertVirtualMethodsDialog::Reimplemented).toBool())
                    return true;
            }
            return false;
        }

        if (m_hideReimplemented)
            return !index.data(InsertVirtualMethodsDialog::Reimplemented).toBool();
        return true;
    }

    bool hideReimplemented() const
    {
        return m_hideReimplemented;
    }

    void setHideReimplementedFunctions(bool show)
    {
        m_hideReimplemented = show;
        invalidateFilter();
    }

private:
    bool m_hideReimplemented;
};

InsertVirtualMethodsDialog::InsertVirtualMethodsDialog(QWidget *parent)
    : QDialog(parent)
    , m_view(0)
    , m_hideReimplementedFunctions(0)
    , m_insertMode(0)
    , m_virtualKeyword(0)
    , m_buttons(0)
    , m_hasImplementationFile(false)
    , m_hasReimplementedFunctions(false)
    , m_implementationMode(ModeOnlyDeclarations)
    , m_insertKeywordVirtual(false)
    , classFunctionModel(new InsertVirtualMethodsModel(this))
    , classFunctionFilterModel(new InsertVirtualMethodsFilterModel(this))
{
    classFunctionFilterModel->setSourceModel(classFunctionModel);
}

void InsertVirtualMethodsDialog::initGui()
{
    if (m_view)
        return;

    setWindowTitle(tr("Insert Virtual Functions"));
    QVBoxLayout *globalVerticalLayout = new QVBoxLayout;

    // View
    QGroupBox *groupBoxView = new QGroupBox(tr("&Functions to insert:"), this);
    QVBoxLayout *groupBoxViewLayout = new QVBoxLayout(groupBoxView);
    m_view = new QTreeView(this);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setHeaderHidden(true);
    groupBoxViewLayout->addWidget(m_view);
    m_hideReimplementedFunctions =
            new QCheckBox(tr("&Hide reimplemented functions"), this);
    groupBoxViewLayout->addWidget(m_hideReimplementedFunctions);

    // Insertion options
    QGroupBox *groupBoxImplementation = new QGroupBox(tr("&Insertion options:"), this);
    QVBoxLayout *groupBoxImplementationLayout = new QVBoxLayout(groupBoxImplementation);
    m_insertMode = new QComboBox(this);
    m_insertMode->addItem(tr("Insert only declarations"), ModeOnlyDeclarations);
    m_insertMode->addItem(tr("Insert definitions inside class"), ModeInsideClass);
    m_insertMode->addItem(tr("Insert definitions outside class"), ModeOutsideClass);
    m_insertMode->addItem(tr("Insert definitions in implementation file"), ModeImplementationFile);
    m_virtualKeyword = new QCheckBox(tr("&Add keyword 'virtual' to function declaration"), this);
    groupBoxImplementationLayout->addWidget(m_insertMode);
    groupBoxImplementationLayout->addWidget(m_virtualKeyword);
    groupBoxImplementationLayout->addStretch(99);

    // Bottom button box
    m_buttons = new QDialogButtonBox(this);
    m_buttons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttons, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttons, SIGNAL(rejected()), this, SLOT(reject()));

    globalVerticalLayout->addWidget(groupBoxView, 9);
    globalVerticalLayout->addWidget(groupBoxImplementation, 0);
    globalVerticalLayout->addWidget(m_buttons, 0);
    setLayout(globalVerticalLayout);

    connect(m_hideReimplementedFunctions, SIGNAL(toggled(bool)),
            this, SLOT(setHideReimplementedFunctions(bool)));
}

void InsertVirtualMethodsDialog::initData()
{
    m_insertKeywordVirtual = Core::ICore::settings()->value(
                QLatin1String("QuickFix/InsertVirtualMethods/insertKeywordVirtual"),
                false).toBool();
    m_implementationMode = static_cast<InsertVirtualMethodsDialog::ImplementationMode>(
                Core::ICore::settings()->value(
                    QLatin1String("QuickFix/InsertVirtualMethods/implementationMode"), 1).toInt());
    m_hideReimplementedFunctions->setChecked(
                Core::ICore::settings()->value(
                    QLatin1String("QuickFix/InsertVirtualMethods/hideReimplementedFunctions"),
                    false).toBool());

    m_view->setModel(classFunctionFilterModel);
    m_expansionStateNormal.clear();
    m_expansionStateReimp.clear();
    m_hideReimplementedFunctions->setEnabled(m_hasReimplementedFunctions);
    m_virtualKeyword->setChecked(m_insertKeywordVirtual);
    m_insertMode->setCurrentIndex(m_insertMode->findData(m_implementationMode));

    setHideReimplementedFunctions(m_hideReimplementedFunctions->isChecked());

    if (m_hasImplementationFile) {
        if (m_insertMode->count() == 3) {
            m_insertMode->addItem(tr("Insert definitions in implementation file"),
                                  ModeImplementationFile);
        }
    } else {
        if (m_insertMode->count() == 4)
            m_insertMode->removeItem(3);
    }
}

bool InsertVirtualMethodsDialog::gather()
{
    initGui();
    initData();

    // Expand the dialog a little bit
    adjustSize();
    resize(size() * 1.5);

    QPointer<InsertVirtualMethodsDialog> that(this);
    const int ret = exec();
    if (!that)
        return false;

    m_implementationMode = implementationMode();
    m_insertKeywordVirtual = insertKeywordVirtual();
    return (ret == QDialog::Accepted);
}

InsertVirtualMethodsDialog::ImplementationMode
InsertVirtualMethodsDialog::implementationMode() const
{
    return static_cast<InsertVirtualMethodsDialog::ImplementationMode>(
                m_insertMode->itemData(m_insertMode->currentIndex()).toInt());
}

void InsertVirtualMethodsDialog::setImplementationsMode(InsertVirtualMethodsDialog::ImplementationMode mode)
{
    m_implementationMode = mode;
}

bool InsertVirtualMethodsDialog::insertKeywordVirtual() const
{
    return m_virtualKeyword->isChecked();
}

void InsertVirtualMethodsDialog::setInsertKeywordVirtual(bool insert)
{
    m_insertKeywordVirtual = insert;
}

void InsertVirtualMethodsDialog::setHasImplementationFile(bool file)
{
    m_hasImplementationFile = file;
}

void InsertVirtualMethodsDialog::setHasReimplementedFunctions(bool functions)
{
    m_hasReimplementedFunctions = functions;
}

bool InsertVirtualMethodsDialog::hideReimplementedFunctions() const
{
    // Safty check necessary because of testing class
    return (m_hideReimplementedFunctions && m_hideReimplementedFunctions->isChecked());
}

void InsertVirtualMethodsDialog::setHideReimplementedFunctions(bool hide)
{
    InsertVirtualMethodsFilterModel *model =
            qobject_cast<InsertVirtualMethodsFilterModel *>(classFunctionFilterModel);

    if (m_expansionStateNormal.isEmpty() && m_expansionStateReimp.isEmpty()) {
        model->setHideReimplementedFunctions(hide);
        m_view->expandAll();
        saveExpansionState();
        return;
    }

    if (model->hideReimplemented() == hide)
        return;

    saveExpansionState();
    model->setHideReimplementedFunctions(hide);
    restoreExpansionState();
}

void InsertVirtualMethodsDialog::saveExpansionState()
{
    InsertVirtualMethodsFilterModel *model =
            qobject_cast<InsertVirtualMethodsFilterModel *>(classFunctionFilterModel);

    QList<bool> &state = model->hideReimplemented() ? m_expansionStateReimp
                                                    : m_expansionStateNormal;
    state.clear();
    for (int i = 0; i < model->rowCount(); ++i)
        state << m_view->isExpanded(model->index(i, 0));
}

void InsertVirtualMethodsDialog::restoreExpansionState()
{
    InsertVirtualMethodsFilterModel *model =
            qobject_cast<InsertVirtualMethodsFilterModel *>(classFunctionFilterModel);

    const QList<bool> &state = model->hideReimplemented() ? m_expansionStateReimp
                                                          : m_expansionStateNormal;
    const int stateCount = state.count();
    for (int i = 0; i < model->rowCount(); ++i) {
        if (i < stateCount && !state.at(i)) {
            m_view->collapse(model->index(i, 0));
            continue;
        }
        m_view->expand(model->index(i, 0));
    }
}

InsertVirtualMethods::InsertVirtualMethods(InsertVirtualMethodsDialog *dialog)
    : m_dialog(dialog)
{
    if (!dialog)
        m_dialog = new InsertVirtualMethodsDialog;
}

InsertVirtualMethods::~InsertVirtualMethods()
{
    m_dialog->deleteLater();
}

void InsertVirtualMethods::match(const CppQuickFixInterface &interface,
                                 QuickFixOperations &result)
{
    InsertVirtualMethodsOp *op = new InsertVirtualMethodsOp(interface, m_dialog);
    if (op->isValid())
        result.append(op);
    else
        delete op;
}

#ifdef WITH_TESTS

namespace Tests {

typedef QByteArray _;

/// Fake dialog of InsertVirtualMethodsDialog that does not pop up anything.
class InsertVirtualMethodsDialogTest : public InsertVirtualMethodsDialog
{
public:
    InsertVirtualMethodsDialogTest(ImplementationMode mode, bool insertVirtualKeyword,
                                   QWidget *parent = 0)
        : InsertVirtualMethodsDialog(parent)
    {
        setImplementationsMode(mode);
        setInsertKeywordVirtual(insertVirtualKeyword);
    }

    bool gather() { return true; }
    ImplementationMode implementationMode() const { return m_implementationMode; }
    bool insertKeywordVirtual() const { return m_insertKeywordVirtual; }
};

} // namespace Tests

InsertVirtualMethods *InsertVirtualMethods::createTestFactory()
{
    return new InsertVirtualMethods(new Tests::InsertVirtualMethodsDialogTest(
                                        InsertVirtualMethodsDialog::ModeOutsideClass, true));
}

void CppEditorPlugin::test_quickfix_InsertVirtualMethods_data()
{
    QTest::addColumn<InsertVirtualMethodsDialog::ImplementationMode>("implementationMode");
    QTest::addColumn<bool>("insertVirtualKeyword");
    QTest::addColumn<QByteArray>("original");
    QTest::addColumn<QByteArray>("expected");

    // Check: Insert only declarations
    QTest::newRow("onlyDecl")
        << InsertVirtualMethodsDialog::ModeOnlyDeclarations << true << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA() = 0;\n"
        "};\n\n"
        "class Derived : public Bas@eA {\n"
        "};\n"
        ) << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA() = 0;\n"
        "};\n\n"
        "class Derived : public BaseA {\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int virtualFuncA();\n"
        "};\n"
    );

    // Check: Insert only declarations vithout virtual keyword
    QTest::newRow("onlyDeclWithoutVirtual")
        << InsertVirtualMethodsDialog::ModeOnlyDeclarations << false << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA() = 0;\n"
        "};\n\n"
        "class Derived : public Bas@eA {\n"
        "};\n"
        ) << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA() = 0;\n"
        "};\n\n"
        "class Derived : public BaseA {\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    int virtualFuncA();\n"
        "};\n"
    );

    // Check: Are access specifiers considered
    QTest::newRow("Access")
        << InsertVirtualMethodsDialog::ModeOnlyDeclarations << true << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int a() = 0;\n"
        "protected:\n"
        "    virtual int b() = 0;\n"
        "private:\n"
        "    virtual int c() = 0;\n"
        "public slots:\n"
        "    virtual int d() = 0;\n"
        "protected slots:\n"
        "    virtual int e() = 0;\n"
        "private slots:\n"
        "    virtual int f() = 0;\n"
        "signals:\n"
        "    virtual int g() = 0;\n"
        "};\n\n"
        "class Der@ived : public BaseA {\n"
        "};\n"
        ) << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int a() = 0;\n"
        "protected:\n"
        "    virtual int b() = 0;\n"
        "private:\n"
        "    virtual int c() = 0;\n"
        "public slots:\n"
        "    virtual int d() = 0;\n"
        "protected slots:\n"
        "    virtual int e() = 0;\n"
        "private slots:\n"
        "    virtual int f() = 0;\n"
        "signals:\n"
        "    virtual int g() = 0;\n"
        "};\n\n"
        "class Derived : public BaseA {\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int a();\n\n"
        "protected:\n"
        "    virtual int b();\n\n"
        "private:\n"
        "    virtual int c();\n\n"
        "public slots:\n"
        "    virtual int d();\n\n"
        "protected slots:\n"
        "    virtual int e();\n\n"
        "private slots:\n"
        "    virtual int f();\n\n"
        "signals:\n"
        "    virtual int g();\n"
        "};\n"
    );

    // Check: Is a base class of a base class considered.
    QTest::newRow("Superclass")
        << InsertVirtualMethodsDialog::ModeOnlyDeclarations << true << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int a() = 0;\n"
        "};\n\n"
        "class BaseB : public BaseA {\n"
        "public:\n"
        "    virtual int b() = 0;\n"
        "};\n\n"
        "class Der@ived : public BaseB {\n"
        "};\n"
        ) << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int a() = 0;\n"
        "};\n\n"
        "class BaseB : public BaseA {\n"
        "public:\n"
        "    virtual int b() = 0;\n"
        "};\n\n"
        "class Der@ived : public BaseB {\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int a();\n"
        "\n"
        "    // BaseB interface\n"
        "public:\n"
        "    virtual int b();\n"
        "};\n"
    );


    // Check: Do not insert reimplemented functions twice.
    QTest::newRow("SuperclassOverride")
        << InsertVirtualMethodsDialog::ModeOnlyDeclarations << true << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int a() = 0;\n"
        "};\n\n"
        "class BaseB : public BaseA {\n"
        "public:\n"
        "    virtual int a() = 0;\n"
        "};\n\n"
        "class Der@ived : public BaseB {\n"
        "};\n"
        ) << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int a() = 0;\n"
        "};\n\n"
        "class BaseB : public BaseA {\n"
        "public:\n"
        "    virtual int a() = 0;\n"
        "};\n\n"
        "class Der@ived : public BaseB {\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int a();\n"
        "};\n"
    );

    // Check: Insert only declarations for pure virtual function
    QTest::newRow("PureVirtualOnlyDecl")
        << InsertVirtualMethodsDialog::ModeOnlyDeclarations << true << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA() = 0;\n"
        "};\n\n"
        "class Derived : public Bas@eA {\n"
        "};\n"
        ) << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA() = 0;\n"
        "};\n\n"
        "class Derived : public BaseA {\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int virtualFuncA();\n"
        "};\n"
    );

    // Check: Insert pure virtual functions inside class
    QTest::newRow("PureVirtualInside")
        << InsertVirtualMethodsDialog::ModeInsideClass << true << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA() = 0;\n"
        "};\n\n"
        "class Derived : public Bas@eA {\n"
        "};\n"
        ) << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA() = 0;\n"
        "};\n\n"
        "class Derived : public BaseA {\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int virtualFuncA()\n"
        "    {\n"
        "    }\n"
        "};\n"
    );

    // Check: Insert inside class
    QTest::newRow("inside")
        << InsertVirtualMethodsDialog::ModeInsideClass << true << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA() = 0;\n"
        "};\n\n"
        "class Derived : public Bas@eA {\n"
        "};\n"
        ) << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA() = 0;\n"
        "};\n\n"
        "class Derived : public BaseA {\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int virtualFuncA()\n"
        "    {\n"
        "    }\n"
        "};\n"
    );

    // Check: Insert outside class
    QTest::newRow("outside")
        << InsertVirtualMethodsDialog::ModeOutsideClass << true << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA() = 0;\n"
        "};\n\n"
        "class Derived : public Bas@eA {\n"
        "};\n"
        ) << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA() = 0;\n"
        "};\n\n"
        "class Derived : public BaseA {\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int virtualFuncA();\n"
        "};\n\n"
        "int Derived::virtualFuncA()\n"
        "{\n"
        "}\n"
    );

    // Check: No trigger: all implemented
    QTest::newRow("notrigger_allImplemented")
        << InsertVirtualMethodsDialog::ModeOutsideClass << true << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA();\n"
        "};\n\n"
        "class Derived : public Bas@eA {\n"
        "public:\n"
        "    virtual int virtualFuncA() = 0;\n"
        "};\n"
        ) << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA();\n"
        "};\n\n"
        "class Derived : public Bas@eA {\n"
        "public:\n"
        "    virtual int virtualFuncA() = 0;\n"
        "};\n"
    );

    // Check: One pure, one not
    QTest::newRow("Some_Pure")
        << InsertVirtualMethodsDialog::ModeOnlyDeclarations << true << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA() = 0;\n"
        "    virtual int virtualFuncB();\n"
        "};\n\n"
        "class Derived : public Bas@eA {\n"
        "};\n"
        ) << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA() = 0;\n"
        "    virtual int virtualFuncB();\n"
        "};\n\n"
        "class Derived : public BaseA {\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int virtualFuncA();\n"
        "};\n"
    );

    // Check: Pure function in derived class
    QTest::newRow("Pure_in_Derived")
        << InsertVirtualMethodsDialog::ModeOnlyDeclarations << true << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int a();\n"
        "};\n\n"
        "class BaseB : public BaseA {\n"
        "public:\n"
        "    virtual int a() = 0;\n"
        "};\n\n"
        "class Der@ived : public BaseB {\n"
        "};\n"
        ) << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int a();\n"
        "};\n\n"
        "class BaseB : public BaseA {\n"
        "public:\n"
        "    virtual int a() = 0;\n"
        "};\n\n"
        "class Der@ived : public BaseB {\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int a();\n"
        "};\n"
    );

    // Check: One pure function in base class, one in derived
    QTest::newRow("Pure_in_Base_And_Derived")
        << InsertVirtualMethodsDialog::ModeOnlyDeclarations << true << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int a() = 0;\n"
        "    virtual int b();\n"
        "};\n\n"
        "class BaseB : public BaseA {\n"
        "public:\n"
        "    virtual int b() = 0;\n"
        "};\n\n"
        "class Der@ived : public BaseB {\n"
        "};\n"
        ) << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int a() = 0;\n"
        "    virtual int b();\n"
        "};\n\n"
        "class BaseB : public BaseA {\n"
        "public:\n"
        "    virtual int b() = 0;\n"
        "};\n\n"
        "class Der@ived : public BaseB {\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int a();\n"
        "    virtual int b();\n"
        "};\n"
    );

    // Check: One pure function in base class, two in derived
    QTest::newRow("Pure_in_Base_And_Derived_2")
        << InsertVirtualMethodsDialog::ModeOnlyDeclarations << true << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int a() = 0;\n"
        "    virtual int b();\n"
        "};\n\n"
        "class BaseB : public BaseA {\n"
        "public:\n"
        "    virtual int b() = 0;\n"
        "    virtual int c() = 0;\n"
        "};\n\n"
        "class Der@ived : public BaseB {\n"
        "};\n"
        ) << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int a() = 0;\n"
        "    virtual int b();\n"
        "};\n\n"
        "class BaseB : public BaseA {\n"
        "public:\n"
        "    virtual int b() = 0;\n"
        "    virtual int c() = 0;\n"
        "};\n\n"
        "class Der@ived : public BaseB {\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int a();\n"
        "    virtual int b();\n"
        "\n"
        "    // BaseB interface\n"
        "public:\n"
        "    virtual int c();\n"
        "};\n"
    );

    // Check: Remove final function
    QTest::newRow("final_function_removed")
        << InsertVirtualMethodsDialog::ModeOnlyDeclarations << true << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int a() = 0;\n"
        "    virtual int b() = 0;\n"
        "};\n\n"
        "class BaseB : public BaseA {\n"
        "public:\n"
        "    virtual int a() final = 0;\n"
        "};\n\n"
        "class Der@ived : public BaseB {\n"
        "};\n"
        ) << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int a() = 0;\n"
        "    virtual int b() = 0;\n"
        "};\n\n"
        "class BaseB : public BaseA {\n"
        "public:\n"
        "    virtual int a() final = 0;\n"
        "};\n\n"
        "class Der@ived : public BaseB {\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int b();\n"
        "};\n"
    );

    // Check: Remove multiple final functions
    QTest::newRow("multiple_final_functions_removed")
        << InsertVirtualMethodsDialog::ModeOnlyDeclarations << true << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int a() = 0;\n"
        "    virtual int b() = 0;\n"
        "};\n\n"
        "class BaseB : public BaseA {\n"
        "public:\n"
        "    virtual int a() = 0;\n"
        "    virtual int c() = 0;\n"
        "};\n\n"
        "class BaseC : public BaseB {\n"
        "public:\n"
        "    virtual int a() final = 0;\n"
        "    virtual int d() = 0;\n"
        "};\n\n"
        "class Der@ived : public BaseC {\n"
        "};\n"
        ) << _(
        "class BaseA {\n"
        "public:\n"
        "    virtual int a() = 0;\n"
        "    virtual int b() = 0;\n"
        "};\n\n"
        "class BaseB : public BaseA {\n"
        "public:\n"
        "    virtual int a() = 0;\n"
        "    virtual int c() = 0;\n"
        "};\n\n"
        "class BaseC : public BaseB {\n"
        "public:\n"
        "    virtual int a() final = 0;\n"
        "    virtual int d() = 0;\n"
        "};\n\n"
        "class Der@ived : public BaseC {\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int b();\n"
        "\n"
        "    // BaseB interface\n"
        "public:\n"
        "    virtual int c();\n"
        "\n"
        "    // BaseC interface\n"
        "public:\n"
        "    virtual int d();\n"
        "};\n"
    );
}

void CppEditorPlugin::test_quickfix_InsertVirtualMethods()
{
    QFETCH(InsertVirtualMethodsDialog::ImplementationMode, implementationMode);
    QFETCH(bool, insertVirtualKeyword);
    QFETCH(QByteArray, original);
    QFETCH(QByteArray, expected);

    InsertVirtualMethods factory(
                new Tests::InsertVirtualMethodsDialogTest(implementationMode, insertVirtualKeyword));
    Tests::QuickFixTestCase(Tests::singleDocument(original, expected), &factory);
}

/// Check: Insert in implementation file
void CppEditorPlugin::test_quickfix_InsertVirtualMethods_implementationFile()
{
    QList<Tests::QuickFixTestDocument::Ptr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "class BaseA {\n"
        "public:\n"
        "    virtual int a() = 0;\n"
        "};\n\n"
        "class Derived : public Bas@eA {\n"
        "public:\n"
        "    Derived();\n"
        "};\n";
    expected =
        "class BaseA {\n"
        "public:\n"
        "    virtual int a() = 0;\n"
        "};\n\n"
        "class Derived : public BaseA {\n"
        "public:\n"
        "    Derived();\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int a();\n"
        "};\n";
    testFiles << Tests::QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original = "#include \"file.h\"\n";
    expected =
        "#include \"file.h\"\n"
        "\n\n"
        "int Derived::a()\n"
        "{\n}";
    testFiles << Tests::QuickFixTestDocument::create("file.cpp", original, expected);

    InsertVirtualMethods factory(new Tests::InsertVirtualMethodsDialogTest(
                                     InsertVirtualMethodsDialog::ModeImplementationFile, true));
    Tests::QuickFixTestCase(testFiles, &factory);
}

/// Check: Qualified names.
void CppEditorPlugin::test_quickfix_InsertVirtualMethods_BaseClassInNamespace()
{
    QList<Tests::QuickFixTestDocument::Ptr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "namespace BaseNS {enum BaseEnum {EnumA = 1};}\n"
        "namespace BaseNS {\n"
        "class Base {\n"
        "public:\n"
        "    virtual BaseEnum a(BaseEnum e) = 0;\n"
        "};\n"
        "}\n"
        "class Deri@ved : public BaseNS::Base {\n"
        "public:\n"
        "    Derived();\n"
        "};\n";
    expected =
        "namespace BaseNS {enum BaseEnum {EnumA = 1};}\n"
        "namespace BaseNS {\n"
        "class Base {\n"
        "public:\n"
        "    virtual BaseEnum a(BaseEnum e) = 0;\n"
        "};\n"
        "}\n"
        "class Deri@ved : public BaseNS::Base {\n"
        "public:\n"
        "    Derived();\n"
        "\n"
        "    // Base interface\n"
        "public:\n"
        "    virtual BaseNS::BaseEnum a(BaseNS::BaseEnum e);\n"
        "};\n";
    testFiles << Tests::QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original = "#include \"file.h\"\n";
    expected =
        "#include \"file.h\"\n"
        "\n\n"
        "BaseNS::BaseEnum Derived::a(BaseNS::BaseEnum e)\n"
        "{\n}";
    testFiles << Tests::QuickFixTestDocument::create("file.cpp", original, expected);

    InsertVirtualMethods factory(new Tests::InsertVirtualMethodsDialogTest(
                                     InsertVirtualMethodsDialog::ModeImplementationFile, true));
    Tests::QuickFixTestCase(testFiles, &factory);
}
#endif // WITH_TESTS

} // namespace Internal
} // namespace CppEditor

#include "cppinsertvirtualmethods.moc"
