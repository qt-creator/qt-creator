/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "fakevimplugin.h"

#include "fakevimhandler.h"
#include "ui_fakevimoptions.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/commandmappings.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/openeditorsmodel.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/ifile.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/statusbarwidget.h>
#include <coreplugin/statusbarmanager.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <texteditor/basetextdocumentlayout.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/basetextmark.h>
#include <texteditor/completionsupport.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/indenter.h>
#include <texteditor/icompletioncollector.h>

#include <find/findplugin.h>
#include <find/textfindconstants.h>

#include <utils/qtcassert.h>
#include <utils/savedaction.h>
#include <utils/treewidgetcolumnstretcher.h>

#include <cppeditor/cppeditorconstants.h>

#include <cpptools/cpptoolsconstants.h>

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QtPlugin>
#include <QtCore/QObject>
#include <QtCore/QSettings>
#include <QtCore/QTextStream>

#include <QtGui/QDesktopServices>
#include <QtGui/QMessageBox>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QShortcut>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>
#include <QtGui/QTextEdit>
#include <QtGui/QTreeWidgetItem>

using namespace FakeVim::Internal;
using namespace TextEditor;
using namespace Core;

namespace FakeVim {
namespace Constants {

const char * const INSTALL_HANDLER                = "TextEditor.FakeVimHandler";
const char * const MINI_BUFFER                    = "TextEditor.FakeVimMiniBuffer";
const char * const INSTALL_KEY                    = "Alt+V,Alt+V";
const char * const SETTINGS_CATEGORY              = "D.FakeVim";
const char * const SETTINGS_CATEGORY_FAKEVIM_ICON = ":/core/images/category_fakevim.png";
const char * const SETTINGS_ID                    = "A.General";
const char * const SETTINGS_EX_CMDS_ID            = "B.ExCommands";

} // namespace Constants
} // namespace FakeVim


namespace FakeVim {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// FakeVimOptionPage
//
///////////////////////////////////////////////////////////////////////

typedef QMap<QString, QRegExp> CommandMap;
typedef QLatin1String _;

class FakeVimOptionPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    FakeVimOptionPage() {}

    // IOptionsPage
    QString id() const { return _(Constants::SETTINGS_ID); }
    QString displayName() const { return tr("General"); }
    QString category() const { return _(Constants::SETTINGS_CATEGORY); }
    QString displayCategory() const { return tr("FakeVim"); }
    QIcon categoryIcon() const
        { return QIcon(_(Constants::SETTINGS_CATEGORY_FAKEVIM_ICON)); }

    QWidget *createPage(QWidget *parent);
    void apply() { m_group.apply(ICore::instance()->settings()); }
    void finish() { m_group.finish(); }
    virtual bool matches(const QString &) const;

private slots:
    void copyTextEditorSettings();
    void setQtStyle();
    void setPlainStyle();

private:
    friend class DebuggerPlugin;
    Ui::FakeVimOptionPage m_ui;
    QString m_searchKeywords;
    Utils::SavedActionSet m_group;
};

QWidget *FakeVimOptionPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_ui.setupUi(w);

    m_group.clear();
    m_group.insert(theFakeVimSetting(ConfigUseFakeVim),
        m_ui.checkBoxUseFakeVim);
    m_group.insert(theFakeVimSetting(ConfigReadVimRc),
        m_ui.checkBoxReadVimRc);

    m_group.insert(theFakeVimSetting(ConfigExpandTab),
        m_ui.checkBoxExpandTab);
    m_group.insert(theFakeVimSetting(ConfigHlSearch),
        m_ui.checkBoxHlSearch);
    m_group.insert(theFakeVimSetting(ConfigShiftWidth),
        m_ui.spinBoxShiftWidth);
    m_group.insert(theFakeVimSetting(ConfigShowMarks),
        m_ui.checkBoxShowMarks);

    m_group.insert(theFakeVimSetting(ConfigSmartTab),
        m_ui.checkBoxSmartTab);
    m_group.insert(theFakeVimSetting(ConfigStartOfLine),
        m_ui.checkBoxStartOfLine);
    m_group.insert(theFakeVimSetting(ConfigTabStop),
        m_ui.spinBoxTabStop);
    m_group.insert(theFakeVimSetting(ConfigBackspace),
        m_ui.lineEditBackspace);
    m_group.insert(theFakeVimSetting(ConfigIsKeyword),
        m_ui.lineEditIsKeyword);

    m_group.insert(theFakeVimSetting(ConfigPassControlKey),
        m_ui.checkBoxPassControlKey);
    m_group.insert(theFakeVimSetting(ConfigAutoIndent),
        m_ui.checkBoxAutoIndent);
    m_group.insert(theFakeVimSetting(ConfigSmartIndent),
        m_ui.checkBoxSmartIndent);
    m_group.insert(theFakeVimSetting(ConfigIncSearch),
        m_ui.checkBoxIncSearch);
    m_group.insert(theFakeVimSetting(ConfigUseCoreSearch),
        m_ui.checkBoxUseCoreSearch);

    connect(m_ui.pushButtonCopyTextEditorSettings, SIGNAL(clicked()),
        this, SLOT(copyTextEditorSettings()));
    connect(m_ui.pushButtonSetQtStyle, SIGNAL(clicked()),
        this, SLOT(setQtStyle()));
    connect(m_ui.pushButtonSetPlainStyle, SIGNAL(clicked()),
        this, SLOT(setPlainStyle()));
    if (m_searchKeywords.isEmpty()) {
        QLatin1Char sep(' ');
        QTextStream(&m_searchKeywords)
                << sep << m_ui.checkBoxUseFakeVim->text()
                << sep << m_ui.checkBoxReadVimRc->text()
                << sep << m_ui.checkBoxAutoIndent->text()
                << sep << m_ui.checkBoxSmartIndent->text()
                << sep << m_ui.checkBoxExpandTab->text()
                << sep << m_ui.checkBoxSmartTab->text()
                << sep << m_ui.checkBoxHlSearch->text()
                << sep << m_ui.checkBoxIncSearch->text()
                << sep << m_ui.checkBoxStartOfLine->text()
                << sep << m_ui.checkBoxUseCoreSearch->text()
                << sep << m_ui.checkBoxShowMarks->text()
                << sep << m_ui.checkBoxPassControlKey->text()
                << sep << m_ui.labelShiftWidth->text()
                << sep << m_ui.labelTabulator->text()
                << sep << m_ui.labelBackspace->text()
                << sep << m_ui.labelIsKeyword->text();
        m_searchKeywords.remove(QLatin1Char('&'));
    }
    return w;
}

void FakeVimOptionPage::copyTextEditorSettings()
{
    TabSettings ts = TextEditorSettings::instance()->tabSettings();
    m_ui.checkBoxExpandTab->setChecked(ts.m_spacesForTabs);
    m_ui.spinBoxTabStop->setValue(ts.m_tabSize);
    m_ui.spinBoxShiftWidth->setValue(ts.m_indentSize);
    m_ui.checkBoxSmartTab->setChecked(ts.m_smartBackspace);
    m_ui.checkBoxAutoIndent->setChecked(true);
    m_ui.checkBoxSmartIndent->setChecked(ts.m_autoIndent);
    m_ui.checkBoxIncSearch->setChecked(true);
}

void FakeVimOptionPage::setQtStyle()
{
    m_ui.checkBoxExpandTab->setChecked(true);
    m_ui.spinBoxTabStop->setValue(4);
    m_ui.spinBoxShiftWidth->setValue(4);
    m_ui.checkBoxSmartTab->setChecked(true);
    m_ui.checkBoxAutoIndent->setChecked(true);
    m_ui.checkBoxSmartIndent->setChecked(true);
    m_ui.checkBoxIncSearch->setChecked(true);
    m_ui.lineEditBackspace->setText(_("indent,eol,start"));
}

void FakeVimOptionPage::setPlainStyle()
{
    m_ui.checkBoxExpandTab->setChecked(false);
    m_ui.spinBoxTabStop->setValue(8);
    m_ui.spinBoxShiftWidth->setValue(8);
    m_ui.checkBoxSmartTab->setChecked(false);
    m_ui.checkBoxAutoIndent->setChecked(false);
    m_ui.checkBoxSmartIndent->setChecked(false);
    m_ui.checkBoxIncSearch->setChecked(false);
    m_ui.lineEditBackspace->setText(QString());
}

bool FakeVimOptionPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

//const char *FAKEVIM_CONTEXT = "FakeVim";

///////////////////////////////////////////////////////////////////////
//
// FakeVimExCommandsPage
//
///////////////////////////////////////////////////////////////////////

struct CommandItem
{
    Command *m_cmd;
    QString m_regex;
    QTreeWidgetItem *m_item;
};

} // namespace Internal
} // namespace FakeVim


Q_DECLARE_METATYPE(FakeVim::Internal::CommandItem *)

namespace FakeVim {
namespace Internal {

class FakeVimExCommandsPage : public Core::CommandMappings
{
    Q_OBJECT

public:
    FakeVimExCommandsPage(FakeVimPluginPrivate *q) : m_q(q) {}
    ~FakeVimExCommandsPage() { qDeleteAll(m_citems); }

    // IOptionsPage
    QString id() const { return _(Constants::SETTINGS_EX_CMDS_ID); }
    QString displayName() const { return tr("Ex Command Mapping"); }
    QString category() const { return _(Constants::SETTINGS_CATEGORY); }
    QString displayCategory() const { return tr("FakeVim"); }
    QIcon categoryIcon() const { return QIcon(); } // TODO: Icon for FakeVim

    QWidget *createPage(QWidget *parent);
    void initialize();
    CommandMap &exCommandMap();
    CommandMap &defaultExCommandMap();

public slots:
    void commandChanged(QTreeWidgetItem *current);
    void targetIdentifierChanged();
    void resetTargetIdentifier();
    void removeTargetIdentifier();
    void defaultAction();

private:
    void setRegex(const QString &regex);
    QList<CommandItem *> m_citems;
    FakeVimPluginPrivate *m_q;
};

QWidget *FakeVimExCommandsPage::createPage(QWidget *parent)
{
    QWidget *w = CommandMappings::createPage(parent);
    setPageTitle(tr("Ex Command Mapping"));
    setTargetHeader(tr("Ex Trigger Expression"));
    setTargetLabelText(tr("Regular expression:"));
    setTargetEditTitle(tr("Ex Command"));
    setImportExportEnabled(false);
    return w;
}

void FakeVimExCommandsPage::initialize()
{
    ActionManager *am = ICore::instance()->actionManager();
    QTC_ASSERT(am, return);
    UniqueIDManager *uidm = UniqueIDManager::instance();
    QTC_ASSERT(uidm, return);

    QMap<QString, QTreeWidgetItem *> sections;

    foreach (Command *c, am->commands()) {
        if (c->action() && c->action()->isSeparator())
            continue;

        CommandItem *ci = new CommandItem;
        QTreeWidgetItem *item = new QTreeWidgetItem;
        ci->m_cmd = c;
        ci->m_item = item;
        m_citems.append(ci);

        const QString name = uidm->stringForUniqueIdentifier(c->id());
        const int pos = name.indexOf(QLatin1Char('.'));
        const QString section = name.left(pos);
        const QString subId = name.mid(pos + 1);

        if (!sections.contains(section)) {
            QTreeWidgetItem *categoryItem =
                new QTreeWidgetItem(commandList(), QStringList() << section);
            QFont f = categoryItem->font(0);
            f.setBold(true);
            categoryItem->setFont(0, f);
            sections.insert(section, categoryItem);
            commandList()->expandItem(categoryItem);
        }
        sections[section]->addChild(item);

        item->setText(0, subId);

        if (c->action()) {
            QString text = c->hasAttribute(Command::CA_UpdateText)
                    && !c->defaultText().isNull()
                ? c->defaultText() : c->action()->text();
            text.remove(QRegExp("&(?!&)"));
            item->setText(1, text);
        } else {
            item->setText(1, c->shortcut()->whatsThis());
        }
        if (exCommandMap().contains(name)) {
            ci->m_regex = exCommandMap()[name].pattern();
        } else {
            ci->m_regex.clear();
        }

        item->setText(2, ci->m_regex);
        item->setData(0, Qt::UserRole, qVariantFromValue(ci));

        if (ci->m_regex != defaultExCommandMap()[name].pattern())
            setModified(item, true);
    }

    commandChanged(0);
}

void FakeVimExCommandsPage::commandChanged(QTreeWidgetItem *current)
{
    CommandMappings::commandChanged(current);

    if (!current || !current->data(0, Qt::UserRole).isValid())
        return;

    CommandItem *citem = qVariantValue<CommandItem *>(current->data(0, Qt::UserRole));
    targetEdit()->setText(citem->m_regex);
}

void FakeVimExCommandsPage::targetIdentifierChanged()
{
    QTreeWidgetItem *current = commandList()->currentItem();
    if (!current)
        return;

    UniqueIDManager *uidm = UniqueIDManager::instance();
    CommandItem *citem = qVariantValue<CommandItem *>(current->data(0, Qt::UserRole));
    const QString name = uidm->stringForUniqueIdentifier(citem->m_cmd->id());

    if (current->data(0, Qt::UserRole).isValid()) {
        citem->m_regex = targetEdit()->text();
        current->setText(2, citem->m_regex);
        exCommandMap()[name] = QRegExp(citem->m_regex);
    }

    if (citem->m_regex != defaultExCommandMap()[name].pattern())
        setModified(current, true);
    else
        setModified(current, false);

}

void FakeVimExCommandsPage::setRegex(const QString &regex)
{
    targetEdit()->setText(regex);
}

void FakeVimExCommandsPage::resetTargetIdentifier()
{
    UniqueIDManager *uidm = UniqueIDManager::instance();
    QTreeWidgetItem *current = commandList()->currentItem();
    if (current && current->data(0, Qt::UserRole).isValid()) {
        CommandItem *citem = qVariantValue<CommandItem *>(current->data(0, Qt::UserRole));
        const QString &name = uidm->stringForUniqueIdentifier(citem->m_cmd->id());
        if (defaultExCommandMap().contains(name))
            setRegex(defaultExCommandMap()[name].pattern());
        else
            setRegex(QString());
    }
}

void FakeVimExCommandsPage::removeTargetIdentifier()
{
    targetEdit()->clear();
}

void FakeVimExCommandsPage::defaultAction()
{
    UniqueIDManager *uidm = UniqueIDManager::instance();
    foreach (CommandItem *item, m_citems) {
        const QString &name = uidm->stringForUniqueIdentifier(item->m_cmd->id());
        if (defaultExCommandMap().contains(name)) {
            item->m_regex = defaultExCommandMap()[name].pattern();
        } else {
            item->m_regex.clear();
        }
        setModified(item->m_item, false);
        item->m_item->setText(2, item->m_regex);
        if (item->m_item == commandList()->currentItem())
            commandChanged(item->m_item);
    }
}


///////////////////////////////////////////////////////////////////////
//
// WordCompletion
//
///////////////////////////////////////////////////////////////////////

class WordCompletion : public ICompletionCollector
{
    Q_OBJECT

public:
    WordCompletion()
    {
        m_editable = 0;
        m_editor = 0;
    }

    virtual bool shouldRestartCompletion()
    {
        //qDebug() << "SHOULD RESTART COMPLETION?";
        return false;
    }

    virtual ITextEditor *editor() const
    {
        //qDebug() << "NO EDITOR?";
        return m_editable;
    }

    virtual int startPosition() const
    {
        return m_startPosition;
    }

    virtual bool supportsEditor(ITextEditor *) const
    {
        return true;
    }

    virtual bool supportsPolicy(CompletionPolicy policy) const
    {
        return policy == TextCompletion;
    }

    virtual bool triggersCompletion(ITextEditor *editable)
    {
        //qDebug() << "TRIGGERS?";
        QTC_ASSERT(m_editable == editable, /**/);
        return true;
    }

    virtual int startCompletion(ITextEditor *editable)
    {
        //qDebug() << "START COMPLETION";
        QTC_ASSERT(m_editor, return -1);
        QTC_ASSERT(m_editable == editable, return -1);
        return m_editor->textCursor().position();
    }

    void setActive(const QString &needle, bool forward, FakeVimHandler *handler)
    {
        Q_UNUSED(forward);
        m_handler = handler;
        if (!m_handler)
            return;
        m_editor = qobject_cast<BaseTextEditorWidget *>(handler->widget());
        if (!m_editor)
            return;
        //qDebug() << "ACTIVATE: " << needle << forward;
        m_needle = needle;
        m_editable = m_editor->editor();
        m_startPosition = m_editor->textCursor().position() - needle.size();

        CompletionSupport::instance()->complete(m_editable, TextCompletion, false);
    }

    void setInactive()
    {
        m_needle.clear();
        m_editable = 0;
        m_editor = 0;
        m_handler = 0;
        m_startPosition = -1;
    }

    virtual void completions(QList<CompletionItem> *completions)
    {
        QTC_ASSERT(m_editor, return);
        QTC_ASSERT(completions, return);
        QTextCursor tc = m_editor->textCursor();
        tc.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);

        QSet<QString> seen;

        QTextDocument::FindFlags flags = QTextDocument::FindCaseSensitively;
        while (1) {
            tc = tc.document()->find(m_needle, tc.position(), flags);
            if (tc.isNull())
                break;
            QTextCursor sel = tc;
            sel.select(QTextCursor::WordUnderCursor);
            QString found = sel.selectedText();
            // Only add "real" completions.
            if (found.startsWith(m_needle)
                    && !seen.contains(found)
                    && sel.anchor() != m_startPosition) {
                seen.insert(found);
                CompletionItem item;
                item.collector = this;
                item.text = found;
                completions->append(item);
            }
            tc.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor);
        }
        //qDebug() << "COMPLETIONS" << completions->size();
    }

    virtual bool typedCharCompletes(const CompletionItem &item, QChar typedChar)
    {
        m_needle += typedChar;
        //qDebug() << "COMPLETE? " << typedChar << item.text << m_needle;
        return item.text == m_needle;
    }

    virtual void complete(const CompletionItem &item, QChar typedChar)
    {
        Q_UNUSED(typedChar);
        //qDebug() << "COMPLETE: " << item.text;
        QTC_ASSERT(m_handler, return);
        m_handler->handleReplay(item.text.mid(m_needle.size()));
        setInactive();
    }

    virtual bool partiallyComplete(const QList<CompletionItem> &completionItems)
    {
        //qDebug() << "PARTIALLY";
        Q_UNUSED(completionItems);
        return false;
    }

    virtual void cleanup() {}

private:
    int findStartOfName(int pos = -1) const;
    bool isInComment() const;

    FakeVimHandler *m_handler;
    BaseTextEditorWidget *m_editor;
    ITextEditor *m_editable;
    QString m_needle;
    QString m_currentPrefix;
    QList<CompletionItem> m_items;
    int m_startPosition;
};


///////////////////////////////////////////////////////////////////////
//
// FakeVimPluginPrivate
//
///////////////////////////////////////////////////////////////////////

class FakeVimPluginPrivate : public QObject
{
    Q_OBJECT

public:
    FakeVimPluginPrivate(FakeVimPlugin *);
    ~FakeVimPluginPrivate();
    friend class FakeVimPlugin;
    friend class FakeVimExCommandsPage;

    bool initialize();
    void aboutToShutdown();

private slots:
    void onCoreAboutToClose();
    void editorOpened(Core::IEditor *);
    void editorAboutToClose(Core::IEditor *);

    void setUseFakeVim(const QVariant &value);
    void quitFakeVim();
    void triggerCompletions();
    void triggerSimpleCompletions(const QString &needle, bool forward);
    void windowCommand(int key);
    void find(bool reverse);
    void findNext(bool reverse);
    void showSettingsDialog();
    void maybeReadVimRc();
    void setBlockSelection(bool);
    void hasBlockSelection(bool*);

    void showCommandBuffer(const QString &contents);
    void showExtraInformation(const QString &msg);
    void changeSelection(const QList<QTextEdit::ExtraSelection> &selections);
    void moveToMatchingParenthesis(bool *moved, bool *forward, QTextCursor *cursor);
    void checkForElectricCharacter(bool *result, QChar c);
    void indentRegion(int beginLine, int endLine, QChar typedChar);
    void handleExCommand(bool *handled, const ExCommand &cmd);

    void handleDelayedQuitAll(bool forced);
    void handleDelayedQuit(bool forced, Core::IEditor *editor);

    void switchToFile(int n);
    int currentFile() const;

signals:
    void delayedQuitRequested(bool forced, Core::IEditor *editor);
    void delayedQuitAllRequested(bool forced);

private:
    FakeVimPlugin *q;
    FakeVimOptionPage *m_fakeVimOptionsPage;
    FakeVimExCommandsPage *m_fakeVimExCommandsPage;
    QHash<Core::IEditor *, FakeVimHandler *> m_editorToHandler;
    QPointer<Core::ICore> m_core;
    QPointer<Core::EditorManager> m_editorManager;
    QPointer<Core::ActionManager> m_actionManager;
    ICore *core() const { return m_core; }
    EditorManager *editorManager() const { return m_editorManager; }
    ActionManager *actionManager() const { return m_actionManager; }

    void triggerAction(const QString &code);
    void setActionChecked(const QString &code, bool check);

    void readSettings(QSettings *settings);
    void writeSettings(QSettings *settings);

    typedef int (*DistFunction)(const QRect &cursor, const QRect &other);
    void moveSomewhere(DistFunction f);

    CommandMap &exCommandMap() { return m_exCommandMap; }
    CommandMap &defaultExCommandMap() { return m_exCommandMap; }
    CommandMap m_exCommandMap;
    CommandMap m_defaultExCommandMap;
    Core::StatusBarWidget *m_statusBar;
    WordCompletion *m_wordCompletion;
};

FakeVimPluginPrivate::FakeVimPluginPrivate(FakeVimPlugin *plugin)
{
    q = plugin;
    m_fakeVimOptionsPage = 0;
    m_fakeVimExCommandsPage = 0;
    defaultExCommandMap()[CppTools::Constants::SWITCH_HEADER_SOURCE] =
        QRegExp("^A$");
    defaultExCommandMap()["Coreplugin.OutputPane.previtem"] =
        QRegExp("^(cN(ext)?|cp(revious)?)!?( (.*))?$");
    defaultExCommandMap()["Coreplugin.OutputPane.nextitem"] =
        QRegExp("^cn(ext)?!?( (.*))?$");
    defaultExCommandMap()[CppEditor::Constants::JUMP_TO_DEFINITION] =
        QRegExp("^tag?$");
    defaultExCommandMap()[Core::Constants::GO_BACK] =
        QRegExp("^pop?$");
    defaultExCommandMap()[_("QtCreator.Locate")] =
        QRegExp("^e$");
    m_statusBar = 0;
}

FakeVimPluginPrivate::~FakeVimPluginPrivate()
{
    q->removeObject(m_fakeVimOptionsPage);
    delete m_fakeVimOptionsPage;
    m_fakeVimOptionsPage = 0;
    delete theFakeVimSettings();

    q->removeObject(m_fakeVimExCommandsPage);
    delete m_fakeVimExCommandsPage;
    m_fakeVimExCommandsPage = 0;
}

void FakeVimPluginPrivate::onCoreAboutToClose()
{
    // don't attach to editors any more
    disconnect(editorManager(), SIGNAL(editorOpened(Core::IEditor*)),
        this, SLOT(editorOpened(Core::IEditor*)));
}

void FakeVimPluginPrivate::aboutToShutdown()
{
    theFakeVimSettings()->writeSettings(ICore::instance()->settings());
    writeSettings(ICore::instance()->settings());
}

bool FakeVimPluginPrivate::initialize()
{
    m_core = Core::ICore::instance();
    m_editorManager = core()->editorManager();
    m_actionManager = core()->actionManager();
    QTC_ASSERT(actionManager(), return false);

    m_wordCompletion = new WordCompletion;
    q->addAutoReleasedObject(m_wordCompletion);
/*
    // Set completion settings and keep them up to date.
    TextEditorSettings *textEditorSettings = TextEditorSettings::instance();
    completion->setCompletionSettings(textEditorSettings->completionSettings());
    connect(textEditorSettings,
        SIGNAL(completionSettingsChanged(TextEditor::CompletionSettings)),
        completion,
        SLOT(setCompletionSettings(TextEditor::CompletionSettings)));
*/

    Context globalcontext(Core::Constants::C_GLOBAL);

    m_fakeVimOptionsPage = new FakeVimOptionPage;
    q->addObject(m_fakeVimOptionsPage);
    theFakeVimSettings()->readSettings(ICore::instance()->settings());

    m_fakeVimExCommandsPage = new FakeVimExCommandsPage(this);
    q->addObject(m_fakeVimExCommandsPage);
    readSettings(core()->settings());

    Core::Command *cmd = 0;
    cmd = actionManager()->registerAction(theFakeVimSetting(ConfigUseFakeVim),
        Constants::INSTALL_HANDLER, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::INSTALL_KEY));

    ActionContainer *advancedMenu =
        actionManager()->actionContainer(Core::Constants::M_EDIT_ADVANCED);
    advancedMenu->addAction(cmd, Core::Constants::G_EDIT_EDITOR);

    connect(m_core, SIGNAL(coreAboutToClose()), this, SLOT(onCoreAboutToClose()));

    // EditorManager
    connect(editorManager(), SIGNAL(editorAboutToClose(Core::IEditor*)),
        this, SLOT(editorAboutToClose(Core::IEditor*)));
    connect(editorManager(), SIGNAL(editorOpened(Core::IEditor*)),
        this, SLOT(editorOpened(Core::IEditor*)));

    connect(theFakeVimSetting(ConfigUseFakeVim), SIGNAL(valueChanged(QVariant)),
        this, SLOT(setUseFakeVim(QVariant)));
    connect(theFakeVimSetting(ConfigReadVimRc), SIGNAL(valueChanged(QVariant)),
        this, SLOT(maybeReadVimRc()));

    // Delayed operations.
    connect(this, SIGNAL(delayedQuitRequested(bool,Core::IEditor*)),
        this, SLOT(handleDelayedQuit(bool,Core::IEditor*)), Qt::QueuedConnection);
    connect(this, SIGNAL(delayedQuitAllRequested(bool)),
        this, SLOT(handleDelayedQuitAll(bool)), Qt::QueuedConnection);
    maybeReadVimRc();
    //    << "MODE: " << theFakeVimSetting(ConfigUseFakeVim)->value();

    return true;
}

static const char *exCommandMapGroup = "FakeVimExCommand";
static const char *reKey = "RegEx";
static const char *idKey = "Command";

void FakeVimPluginPrivate::writeSettings(QSettings *settings)
{
    settings->beginWriteArray(_(exCommandMapGroup));

    int count = 0;
    typedef CommandMap::const_iterator Iterator;
    const Iterator end = exCommandMap().constEnd();
    for (Iterator it = exCommandMap().constBegin(); it != end; ++it) {
        const QString &id = it.key();
        const QRegExp &re = it.value();

        if ((defaultExCommandMap().contains(id) && defaultExCommandMap()[id] != re)
            || (!defaultExCommandMap().contains(id) && !re.pattern().isEmpty())) {
            settings->setArrayIndex(count);
            settings->setValue(_(idKey), id);
            settings->setValue(_(reKey), re.pattern());
            ++count;
        }
    }

    settings->endArray();
}

void FakeVimPluginPrivate::readSettings(QSettings *settings)
{
    exCommandMap() = defaultExCommandMap();

    int size = settings->beginReadArray(_(exCommandMapGroup));
    for (int i = 0; i < size; ++i) {
        settings->setArrayIndex(i);
        const QString id = settings->value(_(idKey)).toString();
        const QString re = settings->value(_(reKey)).toString();
        exCommandMap()[id] = QRegExp(re);
    }
    settings->endArray();
}

void FakeVimPluginPrivate::maybeReadVimRc()
{
    //qDebug() << theFakeVimSetting(ConfigReadVimRc)
    //    << theFakeVimSetting(ConfigReadVimRc)->value();
    //qDebug() << theFakeVimSetting(ConfigShiftWidth)->value();
    if (!theFakeVimSetting(ConfigReadVimRc)->value().toBool())
        return;
    QString fileName =
        QDesktopServices::storageLocation(QDesktopServices::HomeLocation)
            + "/.vimrc";
    //qDebug() << "READING VIMRC: " << fileName;
    // Read it into a temporary handler for effects modifying global state.
    QPlainTextEdit editor;
    FakeVimHandler handler(&editor);
    handler.handleCommand("source " + fileName);
    theFakeVimSettings()->writeSettings(core()->settings());
    //qDebug() << theFakeVimSetting(ConfigShiftWidth)->value();
}

void FakeVimPluginPrivate::showSettingsDialog()
{
    core()->showOptionsDialog(
        _(Constants::SETTINGS_CATEGORY),
        _(Constants::SETTINGS_ID));
}

void FakeVimPluginPrivate::triggerAction(const QString &code)
{
    Core::ActionManager *am = actionManager();
    QTC_ASSERT(am, return);
    Core::Command *cmd = am->command(code);
    QTC_ASSERT(cmd, qDebug() << "UNKNOWN CODE: " << code; return);
    QAction *action = cmd->action();
    QTC_ASSERT(action, return);
    action->trigger();
}

void FakeVimPluginPrivate::setActionChecked(const QString &code, bool check)
{
    Core::ActionManager *am = actionManager();
    QTC_ASSERT(am, return);
    Core::Command *cmd = am->command(code);
    QTC_ASSERT(cmd, return);
    QAction *action = cmd->action();
    QTC_ASSERT(action, return);
    QTC_ASSERT(action->isCheckable(), return);
    action->setChecked(!check); // trigger negates the action's state
    action->trigger();
}

static int moveRightWeight(const QRect &cursor, const QRect &other)
{
    int dx = other.left() - cursor.right();
    if (dx < 0)
        return -1;
    int w = 10000 * dx;
    int dy1 = cursor.top() - other.bottom();
    int dy2 = cursor.bottom() - other.top();
    w += dy1 * (dy1 > 0);
    w += dy2 * (dy2 > 0);
    qDebug() << "      DX: " << dx << dy1 << dy2 << w;
    return w;
}

static int moveLeftWeight(const QRect &cursor, const QRect &other)
{
    int dx = other.right() - cursor.left();
    if (dx < 0)
        return -1;
    int w = 10000 * dx;
    int dy1 = cursor.top() - other.bottom();
    int dy2 = cursor.bottom() - other.top();
    w += dy1 * (dy1 > 0);
    w += dy2 * (dy2 > 0);
    return w;
}

static int moveUpWeight(const QRect &cursor, const QRect &other)
{
    int dy = other.bottom() - cursor.top();
    if (dy < 0)
        return -1;
    int w = 10000 * dy;
    int dx1 = cursor.left() - other.right();
    int dx2 = cursor.right() - other.left();
    w += dx1 * (dx1 > 0);
    w += dx2 * (dx2 > 0);
    return w;
}

static int moveDownWeight(const QRect &cursor, const QRect &other)
{
    int dy = other.top() - cursor.bottom();
    if (dy < 0)
        return -1;
    int w = 10000 * dy;
    int dx1 = cursor.left() - other.right();
    int dx2 = cursor.right() - other.left();
    w += dx1 * (dx1 > 0);
    w += dx2 * (dx2 > 0);
    return w;
}

void FakeVimPluginPrivate::windowCommand(int key)
{
#    define control(n) (256 + n)
    switch (key) {
        case 'c': case 'C': case control('c'):
            triggerAction(Core::Constants::CLOSE);
            break;
        case 'n': case 'N': case control('n'):
            triggerAction(Core::Constants::GOTONEXT);
            break;
        case 'o': case 'O': case control('o'):
            //triggerAction(Core::Constants::REMOVE_ALL_SPLITS);
            triggerAction(Core::Constants::REMOVE_CURRENT_SPLIT);
            break;
        case 'p': case 'P': case control('p'):
            triggerAction(Core::Constants::GOTOPREV);
            break;
        case 's': case 'S': case control('s'):
            triggerAction(Core::Constants::SPLIT);
            break;
        case 'w': case 'W': case control('w'):
            triggerAction(Core::Constants::GOTO_OTHER_SPLIT);
            break;
        case Qt::Key_Right:
            moveSomewhere(&moveRightWeight);
            break;
        case Qt::Key_Left:
            moveSomewhere(&moveLeftWeight);
            break;
        case Qt::Key_Up:
            moveSomewhere(&moveUpWeight);
            break;
        case Qt::Key_Down:
            moveSomewhere(&moveDownWeight);
            break;
        default:
            qDebug() << "UNKNOWN WINDOWS COMMAND: " << key;
            break;
    }
#    undef control
}

void FakeVimPluginPrivate::moveSomewhere(DistFunction f)
{
    IEditor *editor = editorManager()->currentEditor();
    QWidget *w = editor->widget();
    QPlainTextEdit *pe =
        qobject_cast<QPlainTextEdit *>(editor->widget());
    QTC_ASSERT(pe, return);
    QRect rc = pe->cursorRect();
    QRect cursorRect(w->mapToGlobal(rc.topLeft()),
            w->mapToGlobal(rc.bottomRight()));
    //qDebug() << "\nCURSOR: " << cursorRect;

    IEditor *bestEditor = 0;
    int bestValue = 1 << 30;

    QList<IEditor*> editors = editorManager()->visibleEditors();
    foreach (IEditor *editor, editors) {
        QWidget *w = editor->widget();
        QRect editorRect(w->mapToGlobal(w->geometry().topLeft()),
                w->mapToGlobal(w->geometry().bottomRight()));
        //qDebug() << "   EDITOR: " << editorRect << editor;

        int value = f(cursorRect, editorRect);
        if (value != -1 && value < bestValue) {
            bestValue = value;
            bestEditor = editor;
            //qDebug() << "          BEST SO FAR: " << bestValue << bestEditor;
        }
    }
    //qDebug() << "     BEST: " << bestValue << bestEditor;

    // FIME: This is know to fail as the EditorManager will fall back to
    // the current editor's view. Needs additional public API there.
    if (bestEditor)
        editorManager()->activateEditor(bestEditor);
}

void FakeVimPluginPrivate::find(bool reverse)
{
    if (Find::FindPlugin *plugin = Find::FindPlugin::instance()) {
        plugin->setUseFakeVim(true);
        plugin->openFindToolBar(reverse
                ? Find::FindPlugin::FindBackward
                : Find::FindPlugin::FindForward);
    }
}

void FakeVimPluginPrivate::findNext(bool reverse)
{
    if (reverse)
        triggerAction(Find::Constants::FIND_PREVIOUS);
    else
        triggerAction(Find::Constants::FIND_NEXT);
}

// This class defers deletion of a child FakeVimHandler using deleteLater().
class DeferredDeleter : public QObject
{
    Q_OBJECT

    FakeVimHandler *m_handler;

public:
    DeferredDeleter(QObject *parent, FakeVimHandler *handler)
        : QObject(parent), m_handler(handler)
    {}

    virtual ~DeferredDeleter()
    {
        if (m_handler) {
            m_handler->disconnectFromEditor();
            m_handler->deleteLater();
            m_handler = 0;
        }
    }
};

void FakeVimPluginPrivate::editorOpened(Core::IEditor *editor)
{
    if (!editor)
        return;

    QWidget *widget = editor->widget();
    if (!widget)
        return;

    // we can only handle QTextEdit and QPlainTextEdit
    if (!qobject_cast<QTextEdit *>(widget) && !qobject_cast<QPlainTextEdit *>(widget))
        return;

    //qDebug() << "OPENING: " << editor << editor->widget()
    //    << "MODE: " << theFakeVimSetting(ConfigUseFakeVim)->value();

    FakeVimHandler *handler = new FakeVimHandler(widget, 0);
    // the handler might have triggered the deletion of the editor:
    // make sure that it can return before being deleted itself
    new DeferredDeleter(widget, handler);
    m_editorToHandler[editor] = handler;

    connect(handler, SIGNAL(extraInformationChanged(QString)),
        this, SLOT(showExtraInformation(QString)));
    connect(handler, SIGNAL(commandBufferChanged(QString)),
        this, SLOT(showCommandBuffer(QString)));
    connect(handler, SIGNAL(selectionChanged(QList<QTextEdit::ExtraSelection>)),
        this, SLOT(changeSelection(QList<QTextEdit::ExtraSelection>)));
    connect(handler, SIGNAL(moveToMatchingParenthesis(bool*,bool*,QTextCursor*)),
        this, SLOT(moveToMatchingParenthesis(bool*,bool*,QTextCursor*)));
    connect(handler, SIGNAL(indentRegion(int,int,QChar)),
        this, SLOT(indentRegion(int,int,QChar)));
    connect(handler, SIGNAL(checkForElectricCharacter(bool*,QChar)),
        this, SLOT(checkForElectricCharacter(bool*,QChar)));
    connect(handler, SIGNAL(requestSetBlockSelection(bool)),
        this, SLOT(setBlockSelection(bool)));
    connect(handler, SIGNAL(requestHasBlockSelection(bool*)),
        this, SLOT(hasBlockSelection(bool*)));
    connect(handler, SIGNAL(completionRequested()),
        this, SLOT(triggerCompletions()));
    connect(handler, SIGNAL(simpleCompletionRequested(QString,bool)),
        this, SLOT(triggerSimpleCompletions(QString,bool)));
    connect(handler, SIGNAL(windowCommandRequested(int)),
        this, SLOT(windowCommand(int)));
    connect(handler, SIGNAL(findRequested(bool)),
        this, SLOT(find(bool)));
    connect(handler, SIGNAL(findNextRequested(bool)),
        this, SLOT(findNext(bool)));

    connect(handler, SIGNAL(handleExCommandRequested(bool*,ExCommand)),
        this, SLOT(handleExCommand(bool*,ExCommand)));

    handler->setCurrentFileName(editor->file()->fileName());
    handler->installEventFilter();

    // pop up the bar
    if (theFakeVimSetting(ConfigUseFakeVim)->value().toBool()) {
       showCommandBuffer(QString());
       handler->setupWidget();
    }
}

void FakeVimPluginPrivate::editorAboutToClose(Core::IEditor *editor)
{
    //qDebug() << "CLOSING: " << editor << editor->widget();
    m_editorToHandler.remove(editor);
}

void FakeVimPluginPrivate::setUseFakeVim(const QVariant &value)
{
    //qDebug() << "SET USE FAKEVIM" << value;
    bool on = value.toBool();
    if (Find::FindPlugin::instance())
        Find::FindPlugin::instance()->setUseFakeVim(on);
    if (on) {
        //ICore *core = ICore::instance();
        //core->updateAdditionalContexts(Core::Context(FAKEVIM_CONTEXT),
        // Core::Context());
        foreach (Core::IEditor *editor, m_editorToHandler.keys())
            m_editorToHandler[editor]->setupWidget();
    } else {
        //ICore *core = ICore::instance();
        //core->updateAdditionalContexts(Core::Context(),
        // Core::Context(FAKEVIM_CONTEXT));
        showCommandBuffer(QString());
        foreach (Core::IEditor *editor, m_editorToHandler.keys()) {
            if (TextEditor::BaseTextEditorWidget *textEditor =
                    qobject_cast<TextEditor::BaseTextEditorWidget *>(editor->widget())) {
                m_editorToHandler[editor]->restoreWidget(textEditor->tabSettings().m_tabSize);
            }
        }
    }
}

void FakeVimPluginPrivate::triggerCompletions()
{
    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (!handler)
        return;
    if (BaseTextEditorWidget *editor = qobject_cast<BaseTextEditorWidget *>(handler->widget()))
        CompletionSupport::instance()->
            complete(editor->editor(), TextCompletion, false);
   //     editor->triggerCompletions();
}

void FakeVimPluginPrivate::triggerSimpleCompletions(const QString &needle,
   bool forward)
{
    m_wordCompletion->setActive(needle, forward,
        qobject_cast<FakeVimHandler *>(sender()));
}

void FakeVimPluginPrivate::setBlockSelection(bool on)
{
    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (!handler)
        return;
    if (BaseTextEditorWidget *bt = qobject_cast<BaseTextEditorWidget *>(handler->widget()))
        bt->setBlockSelection(on);
}

void FakeVimPluginPrivate::hasBlockSelection(bool *on)
{
    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (!handler)
        return;
    if (BaseTextEditorWidget *bt = qobject_cast<BaseTextEditorWidget *>(handler->widget()))
        *on = bt->hasBlockSelection();
}

void FakeVimPluginPrivate::checkForElectricCharacter(bool *result, QChar c)
{
    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (!handler)
        return;
    if (BaseTextEditorWidget *bt = qobject_cast<BaseTextEditorWidget *>(handler->widget()))
        *result = bt->indenter()->isElectricCharacter(c);
}

void FakeVimPluginPrivate::handleExCommand(bool *handled, const ExCommand &cmd)
{
    using namespace Core;
    //qDebug() << "PLUGIN HANDLE: " << cmd.cmd << cmd.count;

    *handled = false;

    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (!handler)
        return;

    QTC_ASSERT(editorManager(), return);

    *handled = true;
    if (cmd.matches("w", "write") || cmd.cmd == "wq") {
        // :w[rite]
        Core::IEditor *editor = m_editorToHandler.key(handler);
        const QString fileName = handler->currentFileName();
        if (editor && editor->file()->fileName() == fileName) {
            // Handle that as a special case for nicer interaction with core
            Core::IFile *file = editor->file();
            Core::ICore::instance()->fileManager()->blockFileChange(file);
            file->save(fileName);
            Core::ICore::instance()->fileManager()->unblockFileChange(file);
            // Check result by reading back.
            QFile file3(fileName);
            file3.open(QIODevice::ReadOnly);
            QByteArray ba = file3.readAll();
            handler->showBlackMessage(FakeVimHandler::tr("\"%1\" %2 %3L, %4C written")
                .arg(fileName).arg(" ")
                .arg(ba.count('\n')).arg(ba.size()));
            if (cmd.cmd == "wq")
                delayedQuitRequested(cmd.hasBang, m_editorToHandler.key(handler));
        } else {
            handler->showRedMessage(tr("File not saved"));
        }
    } else if (cmd.matches("wa", "wall")) {
        // :w[all]
        FileManager *fm = ICore::instance()->fileManager();
        QList<IFile *> toSave = fm->modifiedFiles();
        QList<IFile *> failed = fm->saveModifiedFilesSilently(toSave);
        if (failed.isEmpty())
            handler->showBlackMessage(tr("Saving succeeded"));
        else
            handler->showRedMessage(tr("%n files not saved", 0, failed.size()));
    } else if (cmd.matches("q", "quit")) {
        // :q[uit]
        emit delayedQuitRequested(cmd.hasBang, m_editorToHandler.key(handler));
    } else if (cmd.matches("qa", "qall")) {
        // :qa[ll]
        emit delayedQuitAllRequested(cmd.hasBang);
    } else if (cmd.matches("sp", "split")) {
        // :sp[lit]
        triggerAction(Core::Constants::SPLIT);
    } else if (cmd.matches("vs", "vsplit")) {
        // :vs[plit]
        triggerAction(Core::Constants::SPLIT_SIDE_BY_SIDE);
    } else if (cmd.matches("mak", "make")) {
        // :mak[e][!] [arguments]
        triggerAction(ProjectExplorer::Constants::BUILD);
    } else if (cmd.matches("se", "set")) {
        if (cmd.args.isEmpty()) {
            // :se[t]
            showSettingsDialog();
        } else if (cmd.args == "ic" || cmd.args == "ignorecase") {
            // :set nc
            setActionChecked(Find::Constants::CASE_SENSITIVE, false);
        } else if (cmd.args == "noic" || cmd.args == "noignorecase") {
            // :set noic
            setActionChecked(Find::Constants::CASE_SENSITIVE, true);
        } else {
            *handled = false; // Let the handler see it as well.
        }
    } else if (cmd.matches("n", "next")) {
        // :n[ext]
        switchToFile(currentFile() + cmd.count);
    } else if (cmd.matches("prev", "previous") || cmd.matches("N", "Next")) {
        // :prev[ious], :N[ext]
        switchToFile(currentFile() - cmd.count);
    } else if (cmd.matches("bn", "bnext")) {
        // :bn[ext]
        switchToFile(currentFile() + cmd.count);
    } else if (cmd.matches("bp", "bprevious") || cmd.matches("bN", "bNext")) {
        // :bp[revious], :bN[ext]
        switchToFile(currentFile() - cmd.count);
    } else if (cmd.matches("on", "only")) {
        // :on[ly]
        //triggerAction(Core::Constants::REMOVE_ALL_SPLITS);
        triggerAction(Core::Constants::REMOVE_CURRENT_SPLIT);
    } else {
        // Check whether one of the configure commands matches.
        typedef CommandMap::const_iterator Iterator;
        const Iterator end = exCommandMap().constEnd();
        for (Iterator it = exCommandMap().constBegin(); it != end; ++it) {
            const QString &id = it.key();
            const QRegExp &re = it.value();
            if (!re.pattern().isEmpty() && re.indexIn(cmd.cmd) != -1) {
                triggerAction(id);
                return;
            }
        }
        *handled = false;
    }
}

void FakeVimPluginPrivate::handleDelayedQuit(bool forced, Core::IEditor *editor)
{
    // This tries to simulate vim behaviour. But the models of vim and
    // Qt Creator core do not match well...
    if (editorManager()->hasSplitter()) {
        triggerAction(Core::Constants::REMOVE_CURRENT_SPLIT);
    } else {
        QList<Core::IEditor *> editors;
        editors.append(editor);
        editorManager()->closeEditors(editors, !forced);
    }
}

void FakeVimPluginPrivate::handleDelayedQuitAll(bool forced)
{
    triggerAction(Core::Constants::REMOVE_ALL_SPLITS);
    editorManager()->closeAllEditors(!forced);
}

void FakeVimPluginPrivate::moveToMatchingParenthesis(bool *moved, bool *forward,
        QTextCursor *cursor)
{
    *moved = false;

    bool undoFakeEOL = false;
    if (cursor->atBlockEnd() && cursor->block().length() > 1) {
        cursor->movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
        undoFakeEOL = true;
    }
    TextBlockUserData::MatchType match
        = TextBlockUserData::matchCursorForward(cursor);
    if (match == TextBlockUserData::Match) {
        *moved = true;
        *forward = true;
    } else {
        if (undoFakeEOL)
            cursor->movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
        if (match == TextBlockUserData::NoMatch) {
            // Backward matching is according to the character before the cursor.
            bool undoMove = false;
            if (!cursor->atBlockEnd()) {
                cursor->movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
                undoMove = true;
            }
            match = TextBlockUserData::matchCursorBackward(cursor);
            if (match == TextBlockUserData::Match) {
                *moved = true;
                *forward = false;
            } else if (undoMove) {
                cursor->movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
            }
        }
    }
}

void FakeVimPluginPrivate::indentRegion(int beginLine, int endLine,
      QChar typedChar)
{
    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (!handler)
        return;

    BaseTextEditorWidget *bt = qobject_cast<BaseTextEditorWidget *>(handler->widget());
    if (!bt)
        return;

    const TabSettings oldTabSettings = bt->tabSettings();
    TabSettings tabSettings;
    tabSettings.m_indentSize = theFakeVimSetting(ConfigShiftWidth)->value().toInt();
    tabSettings.m_tabSize = theFakeVimSetting(ConfigTabStop)->value().toInt();
    tabSettings.m_spacesForTabs = theFakeVimSetting(ConfigExpandTab)->value().toBool();
    bt->setTabSettings(tabSettings);

    QTextDocument *doc = bt->document();
    QTextBlock startBlock = doc->findBlockByNumber(beginLine);

    // Record line lenghts for mark adjustments
    QVector<int> lineLengths(endLine - beginLine + 1);
    QTextBlock block = startBlock;

    for (int i = beginLine; i <= endLine; ++i) {
        lineLengths[i - beginLine] = block.text().length();
        if (typedChar == 0 && block.text().simplified().isEmpty()) {
            // clear empty lines
            QTextCursor cursor(block);
            while (!cursor.atBlockEnd())
                cursor.deleteChar();
        } else {
            bt->indenter()->indentBlock(doc, block, typedChar, bt);
        }
        block = block.next();
    }

    bt->setTabSettings(oldTabSettings);
}

void FakeVimPluginPrivate::quitFakeVim()
{
    theFakeVimSetting(ConfigUseFakeVim)->setValue(false);
}

void FakeVimPluginPrivate::showCommandBuffer(const QString &contents)
{
    //qDebug() << "SHOW COMMAND BUFFER" << contents;
    if (QLabel *label = qobject_cast<QLabel *>(m_statusBar->widget()))
        label->setText("  " + contents);
}

void FakeVimPluginPrivate::showExtraInformation(const QString &text)
{
    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (handler)
        QMessageBox::information(handler->widget(), tr("FakeVim Information"), text);
}

void FakeVimPluginPrivate::changeSelection
    (const QList<QTextEdit::ExtraSelection> &selection)
{
    if (FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender()))
        if (BaseTextEditorWidget *bt = qobject_cast<BaseTextEditorWidget *>(handler->widget()))
            bt->setExtraSelections(BaseTextEditorWidget::FakeVimSelection, selection);
}

int FakeVimPluginPrivate::currentFile() const
{
    Core::OpenEditorsModel *model = editorManager()->openedEditorsModel();
    IEditor *editor = editorManager()->currentEditor();
    return model->indexOf(editor).row();
}

void FakeVimPluginPrivate::switchToFile(int n)
{
    Core::OpenEditorsModel *model = editorManager()->openedEditorsModel();
    int size = model->rowCount();
    QTC_ASSERT(size, return);
    n = n % size;
    if (n < 0)
        n += size;
    editorManager()->activateEditorForIndex(model->index(n, 0));
}

CommandMap &FakeVimExCommandsPage::exCommandMap()
{
    return m_q->exCommandMap();
}

CommandMap &FakeVimExCommandsPage::defaultExCommandMap()
{
    return m_q->defaultExCommandMap();
}

///////////////////////////////////////////////////////////////////////
//
// FakeVimPlugin
//
///////////////////////////////////////////////////////////////////////

FakeVimPlugin::FakeVimPlugin()
    : d(new FakeVimPluginPrivate(this))
{}

FakeVimPlugin::~FakeVimPlugin()
{
    delete d;
}

bool FakeVimPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)
    return d->initialize();
}

ExtensionSystem::IPlugin::ShutdownFlag FakeVimPlugin::aboutToShutdown()
{
    d->aboutToShutdown();
    return SynchronousShutdown;
}

void FakeVimPlugin::extensionsInitialized()
{
    d->m_statusBar = new Core::StatusBarWidget;
    d->m_statusBar->setWidget(new QLabel);
    //d->m_statusBar->setContext(Context(FAKEVIM_CONTEXT));
    d->m_statusBar->setPosition(StatusBarWidget::Last);
    addAutoReleasedObject(d->m_statusBar);
}

} // namespace Internal
} // namespace FakeVim

#include "fakevimplugin.moc"

Q_EXPORT_PLUGIN(FakeVimPlugin)
