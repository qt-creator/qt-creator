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

using namespace CPlusPlus;
using namespace CppEditor;
using namespace CppEditor::Internal;
using namespace CppTools;
using namespace TextEditor;

namespace CppEditor {
namespace Internal {

class InsertVirtualMethodsDialog : public QDialog
{
    Q_OBJECT
public:
    enum CustomItemRoles {
        ClassOrFunction = Qt::UserRole + 1,
        Reimplemented = Qt::UserRole + 2,
        PureVirtual = Qt::UserRole + 3,
        AccessSpec = Qt::UserRole + 4
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

public slots:
    void updateCheckBoxes(QStandardItem *item);

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
    QStandardItemModel *classFunctionModel;
    QSortFilterProxyModel *classFunctionFilterModel;
};

} // namespace Internal
} // namespace CppEditor

namespace {

class InsertVirtualMethodsOp : public CppQuickFixOperation
{
public:
    InsertVirtualMethodsOp(const QSharedPointer<const CppQuickFixAssistInterface> &interface,
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

        const QList<AST *> &path = interface->path();
        const int pathSize = path.size();
        if (pathSize < 2)
            return;

        // Determine if cursor is on a class or a base class
        if (SimpleNameAST *nameAST = path.at(pathSize - 1)->asSimpleName()) {
            if (!interface->isCursorOn(nameAST))
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
        const int endOfClassAST = interface->currentFile()->endOf(m_classAST);
        m_insertPosDecl = endOfClassAST - 1; // Skip last "}"
        m_insertPosOutside = endOfClassAST + 1; // Step over ";"

        // Determine base classes
        QList<const Class *> baseClasses;
        QQueue<ClassOrNamespace *> baseClassQueue;
        QSet<ClassOrNamespace *> visitedBaseClasses;
        if (ClassOrNamespace *clazz = interface->context().lookupType(m_classAST->symbol))
            baseClassQueue.enqueue(clazz);
        while (!baseClassQueue.isEmpty()) {
            ClassOrNamespace *clazz = baseClassQueue.dequeue();
            visitedBaseClasses.insert(clazz);
            const QList<ClassOrNamespace *> bases = clazz->usings();
            foreach (ClassOrNamespace *baseClass, bases) {
                foreach (Symbol *symbol, baseClass->symbols()) {
                    Class *base = symbol->asClass();
                    if (base
                            && (clazz = interface->context().lookupType(symbol))
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
        const FontSettings &fs = TextEditorSettings::fontSettings();
        const Format formatReimpFunc = fs.formatFor(C_DISABLED_CODE);
        QHash<const Function *, QStandardItem *> virtualFunctions;
        foreach (const Class *clazz, baseClasses) {
            QStandardItem *itemBase = new QStandardItem(printer.prettyName(clazz->name()));
            itemBase->setData(false, InsertVirtualMethodsDialog::Reimplemented);
            itemBase->setData(qVariantFromValue((void *) clazz),
                              InsertVirtualMethodsDialog::ClassOrFunction);
            for (Scope::iterator it = clazz->firstMember(); it != clazz->lastMember(); ++it) {
                if (const Function *func = (*it)->type()->asFunctionType()) {
                    // Filter virtual destructors
                    if (func->name()->asDestructorNameId())
                        continue;

                    const Function *firstVirtual = 0;
                    const bool isVirtual = FunctionUtils::isVirtualFunction(
                                func, interface->context(), &firstVirtual);
                    if (!isVirtual)
                        continue;

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
                                || !funcName->identifier()->isEqualTo(symbol->identifier())) {
                            continue;
                        }
                        if (symbol->type().isEqualTo(func->type())) {
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
                    QStandardItem *funcItem = new QStandardItem(itemName);
                    funcItem->setCheckable(true);
                    if (isReimplemented) {
                        factory->setHasReimplementedFunctions(true);
                        funcItem->setEnabled(false);
                        funcItem->setCheckState(Qt::Unchecked);
                        if (QStandardItem *first = virtualFunctions[firstVirtual]) {
                            if (!first->data(InsertVirtualMethodsDialog::Reimplemented).toBool()) {
                                first->setCheckState(isPureVirtual ? Qt::Checked : Qt::Unchecked);
                                factory->updateCheckBoxes(first);
                            }
                        }
                    } else {
                        if (!funcExistsInClass) {
                            funcItem->setCheckState(isPureVirtual ? Qt::Checked : Qt::Unchecked);
                        } else {
                            funcItem->setEnabled(false);
                            funcItem->setCheckState(Qt::Checked);
                            funcItem->setData(formatReimpFunc.foreground(), Qt::ForegroundRole);
                            factory->setHasReimplementedFunctions(true);
                            if (formatReimpFunc.background().isValid())
                                funcItem->setData(formatReimpFunc.background(), Qt::BackgroundRole);
                        }
                    }

                    funcItem->setData(qVariantFromValue((void *) func),
                                      InsertVirtualMethodsDialog::ClassOrFunction);
                    funcItem->setData(isPureVirtual, InsertVirtualMethodsDialog::PureVirtual);
                    funcItem->setData(acessSpec(*it), InsertVirtualMethodsDialog::AccessSpec);
                    funcItem->setData(funcExistsInClass || isReimplemented,
                                      InsertVirtualMethodsDialog::Reimplemented);

                    itemBase->appendRow(funcItem);
                    virtualFunctions[func] = funcItem;

                    // update internal counters
                    if (!funcExistsInClass)
                        ++m_functionCount;
                }
            }
            if (itemBase->hasChildren()) {
                itemBase->setData(false, InsertVirtualMethodsDialog::Reimplemented);
                bool enabledFound = false;
                Qt::CheckState state = Qt::Unchecked;
                for (int i = 0; i < itemBase->rowCount(); ++i) {
                    QStandardItem *childItem = itemBase->child(i, 0);
                    if (!childItem->isEnabled())
                        continue;
                    if (!enabledFound) {
                        state = childItem->checkState();
                        enabledFound = true;
                    }
                    if (childItem->isCheckable()) {
                        if (!itemBase->isCheckable()) {
                            itemBase->setCheckable(true);
                            itemBase->setTristate(true);
                            itemBase->setCheckState(state);
                        }
                        if (state != childItem->checkState()) {
                            itemBase->setCheckState(Qt::PartiallyChecked);
                            break;
                        }
                    }
                }
                if (!enabledFound) {
                    itemBase->setCheckable(true);
                    itemBase->setEnabled(false);
                }
                m_factory->classFunctionModel->invisibleRootItem()->appendRow(itemBase);
            }
        }
        if (!m_factory->classFunctionModel->invisibleRootItem()->hasChildren()
                || m_functionCount == 0) {
            return;
        }

        bool isHeaderFile = false;
        m_cppFileName = correspondingHeaderOrSource(interface->fileName(), &isHeaderFile);
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
        const CppRefactoringChanges refactoring(assistInterface()->snapshot());
        const QString filename = assistInterface()->currentFile()->fileName();
        const CppRefactoringFilePtr headerFile = refactoring.file(filename);
        const LookupContext targetContext(headerFile->cppDocument(), assistInterface()->snapshot());

        const Class *targetClass = m_classAST->symbol;
        ClassOrNamespace *targetCoN = targetContext.lookupType(targetClass->enclosingScope());
        if (!targetCoN)
            targetCoN = targetContext.globalNamespace();
        UseMinimalNames useMinimalNames(targetCoN);
        Control *control = assistInterface()->context().bindings()->control().data();
        for (int i = 0; i < m_factory->classFunctionModel->rowCount(); ++i) {
            const QStandardItem *parent =
                    m_factory->classFunctionModel->invisibleRootItem()->child(i, 0);
            if (!parent->isCheckable() || parent->checkState() == Qt::Unchecked)
                continue;
            const Class *clazz = (const Class *)
                    parent->data(InsertVirtualMethodsDialog::ClassOrFunction).value<void *>();

            // Add comment
            const QString comment = QLatin1String("\n// ") + printer.prettyName(clazz->name()) +
                    QLatin1String(" interface\n");
            headerChangeSet.insert(m_insertPosDecl, comment);

            // Insert Declarations (+ definitions)
            QString lastAccessSpecString;
            for (int j = 0; j < parent->rowCount(); ++j) {
                const QStandardItem *item = parent->child(j, 0);
                if (!item->isEnabled() || !item->isCheckable() || item->checkState() == Qt::Unchecked)
                    continue;
                const Function *func = (const Function *)
                        item->data(InsertVirtualMethodsDialog::ClassOrFunction).value<void *>();

                // Construct declaration
                // setup rewriting to get minimally qualified names
                SubstitutionEnvironment env;
                env.setContext(assistInterface()->context());
                env.switchScope(clazz->enclosingScope());
                env.enter(&useMinimalNames);

                QString declaration;
                const FullySpecifiedType tn = rewriteType(func->type(), &env, control);
                declaration += printer.prettyType(tn, func->unqualifiedName());

                if (m_factory->insertKeywordVirtual())
                    declaration = QLatin1String("virtual ") + declaration;
                if (m_factory->implementationMode() & InsertVirtualMethodsDialog::ModeInsideClass)
                    declaration += QLatin1String("\n{\n}\n");
                else
                    declaration += QLatin1String(";\n");

                const InsertionPointLocator::AccessSpec spec =
                        static_cast<InsertionPointLocator::AccessSpec>(
                            item->data(InsertVirtualMethodsDialog::AccessSpec).toInt());
                const QString accessSpecString = InsertionPointLocator::accessSpecToString(spec);
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
                            QLatin1String("::") + printer.prettyName(func->name());
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
            const LookupContext targetContext(implementationDoc, assistInterface()->snapshot());
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
                env.setContext(assistInterface()->context());
                env.switchScope(decl->enclosingScope());
                UseMinimalNames q(targetCoN);
                env.enter(&q);
                Control *control = assistInterface()->context().bindings()->control().data();

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

} // anonymous namespace

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
    , classFunctionModel(new QStandardItemModel(this))
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

    connect(classFunctionModel, SIGNAL(itemChanged(QStandardItem*)),
            this, SLOT(updateCheckBoxes(QStandardItem*)));
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

void InsertVirtualMethodsDialog::updateCheckBoxes(QStandardItem *item)
{
    if (item->hasChildren()) {
        const Qt::CheckState state = item->checkState();
        if (!item->isCheckable() || state == Qt::PartiallyChecked)
            return;
        for (int i = 0; i < item->rowCount(); ++i) {
            QStandardItem *childItem = item->child(i, 0);
            if (childItem->isCheckable() && childItem->isEnabled())
                childItem->setCheckState(state);
        }
    } else {
        QStandardItem *parent = item->parent();
        if (!parent->isCheckable())
            return;
        const Qt::CheckState state = item->checkState();
        for (int i = 0; i < parent->rowCount(); ++i) {
            QStandardItem *childItem = parent->child(i, 0);
            if (childItem->isEnabled() && state != childItem->checkState()) {
                parent->setCheckState(Qt::PartiallyChecked);
                return;
            }
        }
        parent->setCheckState(state);
    }
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

void InsertVirtualMethods::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    InsertVirtualMethodsOp *op = new InsertVirtualMethodsOp(interface, m_dialog);
    if (op->isValid())
        result.append(QuickFixOperation::Ptr(op));
    else
        delete op;
}

#ifdef WITH_TESTS

#include "cppeditorplugin.h"
#include "cppquickfix_test.h"

using namespace CppEditor::Internal::Tests;

#include <QtTest>

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

InsertVirtualMethods *InsertVirtualMethods::createTestFactory()
{
    return new InsertVirtualMethods(new InsertVirtualMethodsDialogTest(
                                        InsertVirtualMethodsDialog::ModeOutsideClass, true));
}

Q_DECLARE_METATYPE(InsertVirtualMethodsDialog::ImplementationMode)

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
        "};"
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
        "};"
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
        "};"
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
        "};"
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
        "};"
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
        "};"
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
        "};"
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
        "};"
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
        "};"
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
        "};"
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
        "};"
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
        "};"
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
        "};"
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
        "};"
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
}

void CppEditorPlugin::test_quickfix_InsertVirtualMethods()
{
    QFETCH(InsertVirtualMethodsDialog::ImplementationMode, implementationMode);
    QFETCH(bool, insertVirtualKeyword);
    QFETCH(QByteArray, original);
    QFETCH(QByteArray, expected);

    InsertVirtualMethods factory(
                new InsertVirtualMethodsDialogTest(implementationMode, insertVirtualKeyword));
    QuickFixTestCase(singleDocument(original, expected), &factory);
}

/// Check: Insert in implementation file
void CppEditorPlugin::test_quickfix_InsertVirtualMethods_implementationFile()
{
    QList<QuickFixTestDocument::Ptr> testFiles;
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
        "};";
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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original = "#include \"file.h\"\n";
    expected =
        "#include \"file.h\"\n"
        "\n\n"
        "int Derived::a()\n"
        "{\n}\n";
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertVirtualMethods factory(new InsertVirtualMethodsDialogTest(
                                     InsertVirtualMethodsDialog::ModeImplementationFile, true));
    QuickFixTestCase(testFiles, &factory);
}

/// Check: Qualified names.
void CppEditorPlugin::test_quickfix_InsertVirtualMethods_BaseClassInNamespace()
{
    QList<QuickFixTestDocument::Ptr> testFiles;
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
        "};";
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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original = "#include \"file.h\"\n";
    expected =
        "#include \"file.h\"\n"
        "\n\n"
        "BaseNS::BaseEnum Derived::a(BaseNS::BaseEnum e)\n"
        "{\n}\n";
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertVirtualMethods factory(new InsertVirtualMethodsDialogTest(
                                     InsertVirtualMethodsDialog::ModeImplementationFile, true));
    QuickFixTestCase(testFiles, &factory);
}
#endif

#include "cppinsertvirtualmethods.moc"
