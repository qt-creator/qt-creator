/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "fakevimplugin.h"

#include "fakevimhandler.h"
#include "ui_fakevimoptions.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/openeditorsmodel.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/ifile.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/actionmanager/commandmappings.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/uniqueidmanager.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>

#include <texteditor/basetexteditor.h>
#include <texteditor/basetextmark.h>
#include <texteditor/completionsupport.h>
#include <texteditor/itexteditor.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textblockiterator.h>

#include <find/findplugin.h>
#include <find/textfindconstants.h>

#include <utils/qtcassert.h>
#include <utils/savedaction.h>
#include <utils/treewidgetcolumnstretcher.h>

#include <cppeditor/cppeditorconstants.h>

#include <cpptools/cpptoolsconstants.h>

#include <indenter.h>

#include <QtCore/QDebug>
#include <QtCore/QtPlugin>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QSettings>
#include <QtCore/QTextStream>

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
using namespace ProjectExplorer;


namespace FakeVim {
namespace Constants {

const char * const INSTALL_HANDLER        = "TextEditor.FakeVimHandler";
const char * const MINI_BUFFER            = "TextEditor.FakeVimMiniBuffer";
const char * const INSTALL_KEY            = "Alt+V,Alt+V";
const char * const SETTINGS_CATEGORY      = "D.FakeVim";
const char * const SETTINGS_ID            = "General";
const char * const SETTINGS_EX_CMDS_ID    = "ExCommands";
const char * const CMD_FILE_NEXT          = "FakeVim.SwitchFileNext";
const char * const CMD_FILE_PREV          = "FakeVim.SwitchFilePrev";

} // namespace Constants
} // namespace FakeVim


///////////////////////////////////////////////////////////////////////
//
// FakeVimOptionPage
//
///////////////////////////////////////////////////////////////////////

namespace FakeVim {
namespace Internal {

class FakeVimOptionPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    FakeVimOptionPage() {}

    // IOptionsPage
    QString id() const { return QLatin1String(Constants::SETTINGS_ID); }
    QString displayName() const { return tr("General"); }
    QString category() const { return QLatin1String(Constants::SETTINGS_CATEGORY); }
    QString displayCategory() const { return tr("FakeVim"); }
    QIcon categoryIcon() const { return QIcon(); } // TODO: Add an icon or move into another category

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

    m_group.insert(theFakeVimSetting(ConfigExpandTab),
        m_ui.checkBoxExpandTab);
    m_group.insert(theFakeVimSetting(ConfigHlSearch),
        m_ui.checkBoxHlSearch);
    m_group.insert(theFakeVimSetting(ConfigShiftWidth),
        m_ui.lineEditShiftWidth);

    m_group.insert(theFakeVimSetting(ConfigSmartTab),
        m_ui.checkBoxSmartTab);
    m_group.insert(theFakeVimSetting(ConfigStartOfLine),
        m_ui.checkBoxStartOfLine);
    m_group.insert(theFakeVimSetting(ConfigTabStop),
        m_ui.lineEditTabStop);
    m_group.insert(theFakeVimSetting(ConfigBackspace),
        m_ui.lineEditBackspace);

    m_group.insert(theFakeVimSetting(ConfigAutoIndent),
        m_ui.checkBoxAutoIndent);
    m_group.insert(theFakeVimSetting(ConfigSmartIndent),
        m_ui.checkBoxSmartIndent);
    m_group.insert(theFakeVimSetting(ConfigIncSearch),
        m_ui.checkBoxIncSearch);

    connect(m_ui.pushButtonCopyTextEditorSettings, SIGNAL(clicked()),
        this, SLOT(copyTextEditorSettings()));
    connect(m_ui.pushButtonSetQtStyle, SIGNAL(clicked()),
        this, SLOT(setQtStyle()));
    connect(m_ui.pushButtonSetPlainStyle, SIGNAL(clicked()),
        this, SLOT(setPlainStyle()));
    if (m_searchKeywords.isEmpty()) {
        QTextStream(&m_searchKeywords)
            << ' ' << m_ui.labelAutoIndent->text()
            << ' ' << m_ui.labelExpandTab->text()
            << ' ' << m_ui.labelSmartIndent->text()
            << ' ' << m_ui.labelExpandTab->text()
            << ' ' << m_ui.labelHlSearch->text()
            << ' ' << m_ui.labelIncSearch->text()
            << ' ' << m_ui.labelShiftWidth->text()
            << ' ' << m_ui.labelSmartTab->text()
            << ' ' << m_ui.labelStartOfLine->text()
            << ' ' << m_ui.tabulatorLabel->text()
            << ' ' << m_ui.labelBackspace->text();
        m_searchKeywords.remove(QLatin1Char('&'));
    }
    return w;
}

void FakeVimOptionPage::copyTextEditorSettings()
{
    TextEditor::TabSettings ts =
        TextEditor::TextEditorSettings::instance()->tabSettings();

    m_ui.checkBoxExpandTab->setChecked(ts.m_spacesForTabs);
    m_ui.lineEditTabStop->setText(QString::number(ts.m_tabSize));
    m_ui.lineEditShiftWidth->setText(QString::number(ts.m_indentSize));
    m_ui.checkBoxSmartTab->setChecked(ts.m_smartBackspace);
    m_ui.checkBoxAutoIndent->setChecked(true);
    m_ui.checkBoxSmartIndent->setChecked(ts.m_autoIndent);
    // FIXME: Not present in core
    //m_ui.checkBoxIncSearch->setChecked(ts.m_incSearch);
}

void FakeVimOptionPage::setQtStyle()
{
    m_ui.checkBoxExpandTab->setChecked(true);
    const QString four = QString(QLatin1Char('4'));
    m_ui.lineEditTabStop->setText(four);
    m_ui.lineEditShiftWidth->setText(four);
    m_ui.checkBoxSmartTab->setChecked(true);
    m_ui.checkBoxAutoIndent->setChecked(true);
    m_ui.checkBoxSmartIndent->setChecked(true);
    m_ui.checkBoxIncSearch->setChecked(true);
    m_ui.lineEditBackspace->setText(QLatin1String("indent,eol,start"));
}

void FakeVimOptionPage::setPlainStyle()
{
    m_ui.checkBoxExpandTab->setChecked(false);
    const QString eight = QString(QLatin1Char('4'));
    m_ui.lineEditTabStop->setText(eight);
    m_ui.lineEditShiftWidth->setText(eight);
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

} // namespace Internal
} // namespace FakeVim


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

Q_DECLARE_METATYPE(CommandItem*);

namespace FakeVim {
namespace Internal {

static QMap<QString, QRegExp> s_exCommandMap;
static QMap<QString, QRegExp> s_defaultExCommandMap;


class FakeVimExCommandsPage : public Core::CommandMappings
{
    Q_OBJECT

public:
    FakeVimExCommandsPage() {}

    // IOptionsPage
    QString id() const { return QLatin1String(Constants::SETTINGS_EX_CMDS_ID); }
    QString displayName() const { return tr("Ex Command Mapping"); }
    QString category() const { return QLatin1String(Constants::SETTINGS_CATEGORY); }
    QString displayCategory() const { return tr("FakeVim"); }
    QIcon categoryIcon() const { return QIcon(); } // TODO: Icon for FakeVim

    QWidget *createPage(QWidget *parent);
    void initialize();

public slots:
    void commandChanged(QTreeWidgetItem *current);
    void targetIdentifierChanged();
    void resetTargetIdentifier();
    void removeTargetIdentifier();
    void defaultAction();

private:
    void setRegex(const QString &regex);
    QList<CommandItem *> m_citems;
};

QWidget *FakeVimExCommandsPage::createPage(QWidget *parent)
{
    QWidget *w = CommandMappings::createPage(parent);
    setPageTitle(tr("Ex Command Mapping"));
    setTargetHeader(tr("Ex Trigger Expression"));
    setTargetLabelText(tr("Regular Expression:"));
    setTargetEditTitle(tr("Ex Command"));

    setImportExportEnabled(false);

    return w;
}

void FakeVimExCommandsPage::initialize()
{
    Core::ActionManager *am = Core::ICore::instance()->actionManager();
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
        m_citems << ci;

        const QString name = uidm->stringForUniqueIdentifier(c->id());
        const int pos = name.indexOf(QLatin1Char('.'));
        const QString section = name.left(pos);
        const QString subId = name.mid(pos+1);

        if (!sections.contains(section)) {
            QTreeWidgetItem *categoryItem = new QTreeWidgetItem(commandList(), QStringList() << section);
            QFont f = categoryItem->font(0);
            f.setBold(true);
            categoryItem->setFont(0, f);
            sections.insert(section, categoryItem);
            commandList()->expandItem(categoryItem);
        }
        sections[section]->addChild(item);

        item->setText(0, subId);

        if (c->action()) {
            QString text = c->hasAttribute(Command::CA_UpdateText) && !c->defaultText().isNull() ? c->defaultText() : c->action()->text();
            text.remove(QRegExp("&(?!&)"));
            item->setText(1, text);
        } else {
            item->setText(1, c->shortcut()->whatsThis());
        }
        if (s_exCommandMap.contains(name)) {
            ci->m_regex = s_exCommandMap[name].pattern();
        } else {
            ci->m_regex.clear();
        }

        item->setText(2, ci->m_regex);
        item->setData(0, Qt::UserRole, qVariantFromValue(ci));

        if (ci->m_regex != s_defaultExCommandMap[name].pattern()) {
            QFont f = item->font(0);
            f.setItalic(true);
            item->setFont(0, f);
            item->setFont(1, f);
            f.setBold(true);
            item->setFont(2, f);
        }

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
        s_exCommandMap[name] = QRegExp(citem->m_regex);
    }

    if (citem->m_regex != s_defaultExCommandMap[name].pattern()) {
        QFont f = current->font(0);
        f.setItalic(true);
        current->setFont(0, f);
        current->setFont(1, f);
        f.setBold(true);
        current->setFont(2, f);
    } else {
        QFont f = current->font(0);
        f.setItalic(false);
        f.setBold(false);
        current->setFont(0, f);
        current->setFont(1, f);
        current->setFont(2, f);
    }

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
        if (s_defaultExCommandMap.contains(name))
            setRegex(s_defaultExCommandMap[name].pattern());
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
        if (s_defaultExCommandMap.contains(name)) {
            item->m_regex = s_defaultExCommandMap[name].pattern();
        } else {
            item->m_regex.clear();
        }
        item->m_item->setText(2, item->m_regex);
        if (item->m_item == commandList()->currentItem())
            commandChanged(item->m_item);
    }
}

} // namespace Internal
} // namespace FakeVim


///////////////////////////////////////////////////////////////////////
//
// FakeVimPluginPrivate
//
///////////////////////////////////////////////////////////////////////

namespace FakeVim {
namespace Internal {

class FakeVimPluginPrivate : public QObject
{
    Q_OBJECT

public:
    FakeVimPluginPrivate(FakeVimPlugin *);
    ~FakeVimPluginPrivate();
    friend class FakeVimPlugin;

    bool initialize();
    void shutdown();

private slots:
    void editorOpened(Core::IEditor *);
    void editorAboutToClose(Core::IEditor *);

    void setUseFakeVim(const QVariant &value);
    void quitFakeVim();
    void triggerCompletions();
    void windowCommand(int key);
    void find(bool reverse);
    void findNext(bool reverse);
    void showSettingsDialog();

    void showCommandBuffer(const QString &contents);
    void showExtraInformation(const QString &msg);
    void changeSelection(const QList<QTextEdit::ExtraSelection> &selections);
    void writeFile(bool *handled, const QString &fileName, const QString &contents);
    void moveToMatchingParenthesis(bool *moved, bool *forward, QTextCursor *cursor);
    void checkForElectricCharacter(bool *result, QChar c);
    void indentRegion(int *amount, int beginLine, int endLine,  QChar typedChar);
    void handleExCommand(const QString &cmd);
    void handleSetCommand(bool *handled, QString cmd);

    void handleDelayedQuitAll(bool forced);
    void handleDelayedQuit(bool forced, Core::IEditor *editor);

    void switchFile(bool previous);
    void switchFileNext();
    void switchFilePrev();

signals:
    void delayedQuitRequested(bool forced, Core::IEditor *editor);
    void delayedQuitAllRequested(bool forced);

private:
    FakeVimPlugin *q;
    FakeVimOptionPage *m_fakeVimOptionsPage;
    FakeVimExCommandsPage *m_fakeVimExCommandsPage;
    QHash<Core::IEditor *, FakeVimHandler *> m_editorToHandler;

    void triggerAction(const QString& code);
    void setActionChecked(const QString& code, bool check);

    void readSettings(QSettings *settings);
    void writeSettings(QSettings *settings);
};

} // namespace Internal
} // namespace FakeVim

FakeVimPluginPrivate::FakeVimPluginPrivate(FakeVimPlugin *plugin)
{
    q = plugin;
    m_fakeVimOptionsPage = 0;
    m_fakeVimExCommandsPage = 0;

    s_defaultExCommandMap[Constants::CMD_FILE_NEXT] = QRegExp("^n(ext)?!?( (.*))?$");
    s_defaultExCommandMap[Constants::CMD_FILE_PREV] = QRegExp("^(N(ext)?|prev(ious)?)!?( (.*))?$");
    s_defaultExCommandMap[CppTools::Constants::SWITCH_HEADER_SOURCE] = QRegExp("^A$");
    s_defaultExCommandMap[ProjectExplorer::Constants::BUILD] = QRegExp("^make$");
    s_defaultExCommandMap["Coreplugin.OutputPane.previtem"] = QRegExp("^(cN(ext)?|cp(revious)?)!?( (.*))?$");
    s_defaultExCommandMap["Coreplugin.OutputPane.nextitem"] = QRegExp("^cn(ext)?!?( (.*))?$");
    s_defaultExCommandMap[CppEditor::Constants::JUMP_TO_DEFINITION] = QRegExp("^tag?$");
    s_defaultExCommandMap[Core::Constants::GO_BACK] = QRegExp("^pop?$");
}

FakeVimPluginPrivate::~FakeVimPluginPrivate()
{
}

void FakeVimPluginPrivate::shutdown()
{
    q->removeObject(m_fakeVimOptionsPage);
    delete m_fakeVimOptionsPage;
    m_fakeVimOptionsPage = 0;
    theFakeVimSettings()->writeSettings(Core::ICore::instance()->settings());
    delete theFakeVimSettings();

    q->removeObject(m_fakeVimExCommandsPage);
    delete m_fakeVimExCommandsPage;
    m_fakeVimExCommandsPage = 0;
    writeSettings(Core::ICore::instance()->settings());
}

bool FakeVimPluginPrivate::initialize()
{
    Core::ActionManager *actionManager = Core::ICore::instance()->actionManager();
    QTC_ASSERT(actionManager, return false);

    QList<int> globalcontext;
    globalcontext << Core::Constants::C_GLOBAL_ID;

    m_fakeVimOptionsPage = new FakeVimOptionPage;
    q->addObject(m_fakeVimOptionsPage);
    theFakeVimSettings()->readSettings(Core::ICore::instance()->settings());

    m_fakeVimExCommandsPage = new FakeVimExCommandsPage;
    q->addObject(m_fakeVimExCommandsPage);
    readSettings(Core::ICore::instance()->settings());

    Core::Command *cmd = 0;
    cmd = actionManager->registerAction(theFakeVimSetting(ConfigUseFakeVim),
        Constants::INSTALL_HANDLER, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::INSTALL_KEY));

    ActionContainer *advancedMenu =
        actionManager->actionContainer(Core::Constants::M_EDIT_ADVANCED);
    advancedMenu->addAction(cmd, Core::Constants::G_EDIT_EDITOR);

    // EditorManager
    QObject *editorManager = Core::ICore::instance()->editorManager();
    connect(editorManager, SIGNAL(editorAboutToClose(Core::IEditor*)),
        this, SLOT(editorAboutToClose(Core::IEditor*)));
    connect(editorManager, SIGNAL(editorOpened(Core::IEditor*)),
        this, SLOT(editorOpened(Core::IEditor*)));

    connect(theFakeVimSetting(SettingsDialog), SIGNAL(triggered()),
        this, SLOT(showSettingsDialog()));
    connect(theFakeVimSetting(ConfigUseFakeVim), SIGNAL(valueChanged(QVariant)),
        this, SLOT(setUseFakeVim(QVariant)));

    QAction *switchFileNextAction = new QAction(tr("Switch to next file"), this);
    cmd = actionManager->registerAction(switchFileNextAction, Constants::CMD_FILE_NEXT, globalcontext);
    cmd->setAttribute(Command::CA_Hide);
    connect(switchFileNextAction, SIGNAL(triggered()), this, SLOT(switchFileNext()));

    QAction *switchFilePrevAction = new QAction(tr("Switch to previous file"), this);
    cmd = actionManager->registerAction(switchFilePrevAction, Constants::CMD_FILE_PREV, globalcontext);
    cmd->setAttribute(Command::CA_Hide);
    connect(switchFilePrevAction, SIGNAL(triggered()), this, SLOT(switchFilePrev()));

    // Delayed operatiosn
    connect(this, SIGNAL(delayedQuitRequested(bool,Core::IEditor*)),
        this, SLOT(handleDelayedQuit(bool,Core::IEditor*)), Qt::QueuedConnection);
    connect(this, SIGNAL(delayedQuitAllRequested(bool)),
        this, SLOT(handleDelayedQuitAll(bool)), Qt::QueuedConnection);

    return true;
}

static const char *exCommandMapGroup = "FakeVimExCommand";
static const char *reKey = "RegEx";
static const char *idKey = "Command";

void FakeVimPluginPrivate::writeSettings(QSettings *settings)
{
    settings->beginWriteArray(QLatin1String(exCommandMapGroup));

    int count = 0;
    const QMap<QString, QRegExp>::const_iterator end = s_exCommandMap.constEnd();
    for (QMap<QString, QRegExp>::const_iterator it = s_exCommandMap.constBegin(); it != end; ++it) {
        const QString &id = it.key();
        const QRegExp &re = it.value();

        if ((s_defaultExCommandMap.contains(id) && s_defaultExCommandMap[id] != re)
            || (!s_defaultExCommandMap.contains(id) && !re.pattern().isEmpty())) {
            settings->setArrayIndex(count);
            settings->setValue(QLatin1String(idKey), id);
            settings->setValue(QLatin1String(reKey), re.pattern());
            ++count;
        }
    }

    settings->endArray();
}

void FakeVimPluginPrivate::readSettings(QSettings *settings)
{
    s_exCommandMap = s_defaultExCommandMap;

    int size = settings->beginReadArray(QLatin1String(exCommandMapGroup));
    for (int i=0; i<size; ++i) {
        settings->setArrayIndex(i);
        const QString id = settings->value(QLatin1String(idKey)).toString();
        const QString re = settings->value(QLatin1String(reKey)).toString();
        s_exCommandMap[id] = QRegExp(re);
    }
    settings->endArray();
}

void FakeVimPluginPrivate::showSettingsDialog()
{
    Core::ICore::instance()->showOptionsDialog(QLatin1String(Constants::SETTINGS_CATEGORY),
                                               QLatin1String(Constants::SETTINGS_ID));
}

void FakeVimPluginPrivate::triggerAction(const QString& code)
{
    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    QTC_ASSERT(am, return);
    Core::Command *cmd = am->command(code);
    QTC_ASSERT(cmd, return);
    QAction *action = cmd->action();
    QTC_ASSERT(action, return);
    action->trigger();
}

void FakeVimPluginPrivate::setActionChecked(const QString& code, bool check)
{
    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    QTC_ASSERT(am, return);
    Core::Command *cmd = am->command(code);
    QTC_ASSERT(cmd, return);
    QAction *action = cmd->action();
    QTC_ASSERT(action, return);
    QTC_ASSERT(action->isCheckable(), return);
    action->setChecked(check);
    action->trigger();
}

void FakeVimPluginPrivate::windowCommand(int key)
{
    #define control(n) (256 + n)
    QString code;
    switch (key) {
        case 'c': case 'C': case control('c'):
            code = Core::Constants::CLOSE;
            break;
        case 'n': case 'N': case control('n'):
            code = Core::Constants::GOTONEXT;
            break;
        case 'o': case 'O': case control('o'):
            code = Core::Constants::REMOVE_ALL_SPLITS;
            code = Core::Constants::REMOVE_CURRENT_SPLIT;
            break;
        case 'p': case 'P': case control('p'):
            code = Core::Constants::GOTOPREV;
            break;
        case 's': case 'S': case control('s'):
            code = Core::Constants::SPLIT;
            break;
        case 'w': case 'W': case control('w'):
            code = Core::Constants::GOTO_OTHER_SPLIT;
            break;
    }
    #undef control
    //qDebug() << "RUNNING WINDOW COMMAND: " << key << code;
    if (code.isEmpty()) {
        //qDebug() << "UNKNOWN WINDOWS COMMAND: " << key;
        return;
    }
    triggerAction(code);
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
    connect(handler, SIGNAL(writeFileRequested(bool*,QString,QString)),
        this, SLOT(writeFile(bool*,QString,QString)));
    connect(handler, SIGNAL(selectionChanged(QList<QTextEdit::ExtraSelection>)),
        this, SLOT(changeSelection(QList<QTextEdit::ExtraSelection>)));
    connect(handler, SIGNAL(moveToMatchingParenthesis(bool*,bool*,QTextCursor*)),
        this, SLOT(moveToMatchingParenthesis(bool*,bool*,QTextCursor*)));
    connect(handler, SIGNAL(indentRegion(int*,int,int,QChar)),
        this, SLOT(indentRegion(int*,int,int,QChar)));
    connect(handler, SIGNAL(checkForElectricCharacter(bool*,QChar)),
        this, SLOT(checkForElectricCharacter(bool*,QChar)));
    connect(handler, SIGNAL(completionRequested()),
        this, SLOT(triggerCompletions()));
    connect(handler, SIGNAL(windowCommandRequested(int)),
        this, SLOT(windowCommand(int)));
    connect(handler, SIGNAL(findRequested(bool)),
        this, SLOT(find(bool)));
    connect(handler, SIGNAL(findNextRequested(bool)),
        this, SLOT(findNext(bool)));

    connect(handler, SIGNAL(handleExCommandRequested(QString)),
        this, SLOT(handleExCommand(QString)));
    connect(handler, SIGNAL(handleSetCommandRequested(bool *,QString)),
        this, SLOT(handleSetCommand(bool *,QString)));

    handler->setCurrentFileName(editor->file()->fileName());
    handler->installEventFilter();

    // pop up the bar
    if (theFakeVimSetting(ConfigUseFakeVim)->value().toBool())
       showCommandBuffer(QString());
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
        Core::EditorManager::instance()->showEditorStatusBar(
            QLatin1String(Constants::MINI_BUFFER),
            "vi emulation mode. Type :q to leave. Use , Ctrl-R to trigger run.",
            tr("Quit FakeVim"), this, SLOT(quitFakeVim()));
        foreach (Core::IEditor *editor, m_editorToHandler.keys())
            m_editorToHandler[editor]->setupWidget();
    } else {
        Core::EditorManager::instance()->hideEditorStatusBar(
            QLatin1String(Constants::MINI_BUFFER));
        TextEditor::TabSettings ts =
            TextEditor::TextEditorSettings::instance()->tabSettings();
        foreach (Core::IEditor *editor, m_editorToHandler.keys())
            m_editorToHandler[editor]->restoreWidget(ts.m_tabSize);
    }
}

void FakeVimPluginPrivate::triggerCompletions()
{
    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (!handler)
        return;
    if (BaseTextEditor *bt = qobject_cast<BaseTextEditor *>(handler->widget()))
        TextEditor::Internal::CompletionSupport::instance()->
            autoComplete(bt->editableInterface(), false);
   //     bt->triggerCompletions();
}

void FakeVimPluginPrivate::checkForElectricCharacter(bool *result, QChar c)
{
    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (!handler)
        return;
    if (BaseTextEditor *bt = qobject_cast<BaseTextEditor *>(handler->widget()))
        *result = bt->isElectricCharacter(c);
}

void FakeVimPluginPrivate::writeFile(bool *handled,
    const QString &fileName, const QString &contents)
{
    Q_UNUSED(contents)

    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (!handler)
        return;

    Core::IEditor *editor = m_editorToHandler.key(handler);
    if (editor && editor->file()->fileName() == fileName) {
        // Handle that as a special case for nicer interaction with core
        Core::IFile *file = editor->file();
        Core::ICore::instance()->fileManager()->blockFileChange(file);
        file->save(fileName);
        Core::ICore::instance()->fileManager()->unblockFileChange(file);
        *handled = true;
    }
}

void FakeVimPluginPrivate::handleExCommand(const QString &cmd)
{
    static QRegExp reWriteAll("^wa(ll)?!?$");
    static QRegExp reQuit("^q!?$");
    static QRegExp reQuitAll("^qa!?$");

    using namespace Core;

    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (!handler)
        return;

    EditorManager *editorManager = EditorManager::instance();
    QTC_ASSERT(editorManager, return);

    if (reWriteAll.indexIn(cmd) != -1) {
        // :wa
        FileManager *fm = ICore::instance()->fileManager();
        QList<IFile *> toSave = fm->modifiedFiles();
        QList<IFile *> failed = fm->saveModifiedFilesSilently(toSave);
        if (failed.isEmpty())
            handler->showBlackMessage(tr("Saving succeeded"));
        else
            handler->showRedMessage(tr("%n files not saved", 0, failed.size()));
    } else if (reQuit.indexIn(cmd) != -1) {
        // :q
        bool forced = cmd.contains(QChar('!'));
        emit delayedQuitRequested(forced, m_editorToHandler.key(handler));
    } else if (reQuitAll.indexIn(cmd) != -1) {
        // :qa
        bool forced = cmd.contains(QChar('!'));
        emit delayedQuitAllRequested(forced);
    } else {
        const QMap<QString, QRegExp>::const_iterator end = s_exCommandMap.constEnd();
        for (QMap<QString, QRegExp>::const_iterator it = s_exCommandMap.constBegin(); it != end; ++it) {
            const QString &id = it.key();
            const QRegExp &re = it.value();

            if (!re.pattern().isEmpty() && re.indexIn(cmd) != -1) {
                triggerAction(id);
                return;
            }
        }

        handler->showRedMessage(tr("Not an editor command: %1").arg(cmd));
    }
}

void FakeVimPluginPrivate::handleSetCommand(bool *handled, QString cmd)
{
    *handled = false;
    bool value = true;
    if (cmd.startsWith("no")) {
        value = false;
        cmd = cmd.mid(2);
    }

    if (cmd == "ic" || cmd == "ignorecase") {
        setActionChecked(Find::Constants::CASE_SENSITIVE, value);
        *handled = true;
    }
}

void FakeVimPluginPrivate::handleDelayedQuit(bool forced, Core::IEditor *editor)
{
    QList<Core::IEditor *> editors;
    editors.append(editor);
    Core::EditorManager::instance()->closeEditors(editors, !forced);
}

void FakeVimPluginPrivate::handleDelayedQuitAll(bool forced)
{
    Core::EditorManager::instance()->closeAllEditors(!forced);
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
    TextEditor::TextBlockUserData::MatchType match
        = TextEditor::TextBlockUserData::matchCursorForward(cursor);
    if (match == TextEditor::TextBlockUserData::Match) {
        *moved = true;
        *forward = true;
    } else {
        if (undoFakeEOL)
            cursor->movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
        if (match == TextEditor::TextBlockUserData::NoMatch) {
            // backward matching is according to the character before the cursor
            bool undoMove = false;
            if (!cursor->atBlockEnd()) {
                cursor->movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
                undoMove = true;
            }
            match = TextEditor::TextBlockUserData::matchCursorBackward(cursor);
            if (match == TextEditor::TextBlockUserData::Match) {
                *moved = true;
                *forward = false;
            } else if (undoMove) {
                cursor->movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
            }
        }
    }
}

void FakeVimPluginPrivate::indentRegion(int *amount, int beginLine, int endLine,
      QChar typedChar)
{
    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (!handler)
        return;

    BaseTextEditor *bt = qobject_cast<BaseTextEditor *>(handler->widget());
    if (!bt)
        return;

    TextEditor::TabSettings tabSettings;
    tabSettings.m_indentSize = theFakeVimSetting(ConfigShiftWidth)->value().toInt();
    tabSettings.m_tabSize = theFakeVimSetting(ConfigTabStop)->value().toInt();
    tabSettings.m_spacesForTabs = theFakeVimSetting(ConfigExpandTab)->value().toBool();
    typedef SharedTools::Indenter<TextEditor::TextBlockIterator> Indenter;
    Indenter &indenter = Indenter::instance();
    indenter.setIndentSize(tabSettings.m_indentSize);
    indenter.setTabSize(tabSettings.m_tabSize);

    const QTextDocument *doc = bt->document();
    const TextEditor::TextBlockIterator docStart(doc->begin());
    QTextBlock cur = doc->findBlockByNumber(beginLine);
    for(int i = beginLine; i<= endLine; ++i)
    {
        if (typedChar == 0 && cur.text().simplified().isEmpty()) {
            // clear empty lines
            *amount = 0;
            QTextCursor cursor(cur);
            while (!cursor.atBlockEnd())
                cursor.deleteChar();
        } else {
            const TextEditor::TextBlockIterator current(cur);
            const TextEditor::TextBlockIterator next(cur.next());
            *amount = indenter.indentForBottomLine(current, docStart, next, typedChar);
            tabSettings.indentLine(cur, *amount);
        }
        cur = cur.next();
    }
}

void FakeVimPluginPrivate::quitFakeVim()
{
    theFakeVimSetting(ConfigUseFakeVim)->setValue(false);
}

void FakeVimPluginPrivate::showCommandBuffer(const QString &contents)
{
    //qDebug() << "SHOW COMMAND BUFFER" << contents;
    Core::EditorManager::instance()->showEditorStatusBar(
        QLatin1String(Constants::MINI_BUFFER), contents,
        tr("Quit FakeVim"), this, SLOT(quitFakeVim()));
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
        if (BaseTextEditor *bt = qobject_cast<BaseTextEditor *>(handler->widget()))
            bt->setExtraSelections(BaseTextEditor::FakeVimSelection, selection);
}

void FakeVimPluginPrivate::switchFile(bool previous)
{
    Core::EditorManager *em = Core::EditorManager::instance();
    Core::OpenEditorsModel *model = em->openedEditorsModel();
    IEditor *cur = Core::EditorManager::instance()->currentEditor();
    int curIdx = model->indexOf(cur).row();
    int nIdx = (curIdx + model->rowCount() + (previous ? -1 : 1)) % model->rowCount();
    em->activateEditor(model->index(nIdx, 0), 0);
}

void FakeVimPluginPrivate::switchFileNext()
{
    switchFile(false);
}

void FakeVimPluginPrivate::switchFilePrev()
{
    switchFile(true);
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

void FakeVimPlugin::shutdown()
{
    d->shutdown();
}

void FakeVimPlugin::extensionsInitialized()
{
}

#include "fakevimplugin.moc"

Q_EXPORT_PLUGIN(FakeVimPlugin)
