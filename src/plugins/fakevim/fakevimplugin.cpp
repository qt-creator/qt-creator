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

#include "fakevimplugin.h"

#include "fakevimactions.h"
#include "fakevimhandler.h"
#include "fakevimtr.h"
#include "ui_fakevimoptions.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/commandmappings.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/find/findplugin.h>
#include <coreplugin/find/textfindconstants.h>
#include <coreplugin/find/ifindsupport.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/id.h>
#include <coreplugin/statusbarwidget.h>
#include <coreplugin/statusbarmanager.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <texteditor/displaysettings.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditor.h>
#include <texteditor/textmark.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/typingsettings.h>
#include <texteditor/tabsettings.h>
#include <texteditor/icodestylepreferences.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/indenter.h>
#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/genericproposal.h>

#include <utils/fancylineedit.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/pathchooser.h>
#include <utils/savedaction.h>
#include <utils/stylehelper.h>

#include <cpptools/cpptoolsconstants.h>

#include <extensionsystem/pluginmanager.h>

#include <QAbstractTableModel>
#include <QDebug>
#include <QFile>
#include <QItemDelegate>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPointer>
#include <QScrollBar>
#include <QSettings>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>
#include <QTimer>
#include <QTreeWidgetItem>

using namespace TextEditor;
using namespace Core;
using namespace Utils;

namespace FakeVim {
namespace Internal {

const char INSTALL_HANDLER[]                = "TextEditor.FakeVimHandler";
const char SETTINGS_CATEGORY[]              = "D.FakeVim";
const char SETTINGS_CATEGORY_FAKEVIM_ICON[] = ":/fakevim/images/category_fakevim.png";
const char SETTINGS_ID[]                    = "A.FakeVim.General";
const char SETTINGS_EX_CMDS_ID[]            = "B.FakeVim.ExCommands";
const char SETTINGS_USER_CMDS_ID[]          = "C.FakeVim.UserCommands";

class MiniBuffer : public QStackedWidget
{
    Q_OBJECT

public:
    MiniBuffer()
        : m_label(new QLabel(this))
        , m_edit(new QLineEdit(this))
    {
        connect(m_edit, &QLineEdit::textEdited, this, &MiniBuffer::changed);
        connect(m_edit, &QLineEdit::cursorPositionChanged, this, &MiniBuffer::changed);
        connect(m_edit, &QLineEdit::selectionChanged, this, &MiniBuffer::changed);
        m_label->setTextInteractionFlags(Qt::TextSelectableByMouse);

        addWidget(m_label);
        addWidget(m_edit);

        m_hideTimer.setSingleShot(true);
        m_hideTimer.setInterval(8000);
        connect(&m_hideTimer, &QTimer::timeout, this, &QWidget::hide);
    }

    void setContents(const QString &contents, int cursorPos, int anchorPos,
                     int messageLevel, FakeVimHandler *eventFilter)
    {
        if (cursorPos != -1) {
            m_edit->blockSignals(true);
            m_label->clear();
            m_edit->setText(contents);
            if (anchorPos != -1 && anchorPos != cursorPos)
                m_edit->setSelection(anchorPos, cursorPos - anchorPos);
            else
                m_edit->setCursorPosition(cursorPos);
            m_edit->blockSignals(false);
            setCurrentWidget(m_edit);
            m_edit->setFocus();
        } else {
            if (contents.isEmpty()) {
                if (m_lastMessageLevel == MessageMode)
                    hide();
                else
                    m_hideTimer.start();
            } else {
                m_hideTimer.stop();
                show();

                m_label->setText(contents);

                QString css;
                if (messageLevel == MessageError) {
                    css = "border:1px solid rgba(255,255,255,150);"
                          "background-color:rgba(255,0,0,100);";
                } else if (messageLevel == MessageWarning) {
                    css = "border:1px solid rgba(255,255,255,120);"
                            "background-color:rgba(255,255,0,20);";
                } else if (messageLevel == MessageShowCmd) {
                    css = "border:1px solid rgba(255,255,255,120);"
                            "background-color:rgba(100,255,100,30);";
                }
                m_label->setStyleSheet(QString::fromLatin1(
                    "*{border-radius:2px;padding-left:4px;padding-right:4px;%1}").arg(css));
            }

            if (m_edit->hasFocus())
                emit edited(QString(), -1, -1);

            setCurrentWidget(m_label);
        }

        if (m_eventFilter != eventFilter) {
            if (m_eventFilter != 0) {
                m_edit->removeEventFilter(m_eventFilter);
                disconnect(this, &MiniBuffer::edited, 0, 0);
            }
            if (eventFilter != 0) {
                m_edit->installEventFilter(eventFilter);
                connect(this, &MiniBuffer::edited,
                        eventFilter, &FakeVimHandler::miniBufferTextEdited);
            }
            m_eventFilter = eventFilter;
        }

        m_lastMessageLevel = messageLevel;
    }

    QSize sizeHint() const
    {
        QSize size = QWidget::sizeHint();
        // reserve maximal width for line edit widget
        return currentWidget() == m_edit ? QSize(maximumWidth(), size.height()) : size;
    }

signals:
    void edited(const QString &text, int cursorPos, int anchorPos);

private:
    void changed()
    {
        const int cursorPos = m_edit->cursorPosition();
        int anchorPos = m_edit->selectionStart();
        if (anchorPos == cursorPos)
            anchorPos = cursorPos + m_edit->selectedText().length();
        emit edited(m_edit->text(), cursorPos, anchorPos);
    }

    QLabel *m_label;
    QLineEdit *m_edit;
    QObject *m_eventFilter = nullptr;
    QTimer m_hideTimer;
    int m_lastMessageLevel = MessageMode;
};

class RelativeNumbersColumn : public QWidget
{
    Q_OBJECT

public:
    RelativeNumbersColumn(TextEditorWidget *baseTextEditor)
        : QWidget(baseTextEditor)
        , m_editor(baseTextEditor)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);

        m_timerUpdate.setSingleShot(true);
        m_timerUpdate.setInterval(0);
        connect(&m_timerUpdate, &QTimer::timeout,
                this, &RelativeNumbersColumn::followEditorLayout);

        auto start = static_cast<void(QTimer::*)()>(&QTimer::start);
        connect(m_editor, &QPlainTextEdit::cursorPositionChanged,
                &m_timerUpdate, start);
        connect(m_editor->verticalScrollBar(), &QAbstractSlider::valueChanged,
                &m_timerUpdate, start);
        connect(m_editor->document(), &QTextDocument::contentsChanged,
                &m_timerUpdate, start);
        connect(TextEditorSettings::instance(), &TextEditorSettings::displaySettingsChanged,
                &m_timerUpdate, start);

        m_editor->installEventFilter(this);

        followEditorLayout();
    }

protected:
    void paintEvent(QPaintEvent *event)
    {
        QTextCursor firstVisibleCursor = m_editor->cursorForPosition(QPoint(0, 0));
        QTextBlock firstVisibleBlock = firstVisibleCursor.block();
        if (firstVisibleCursor.positionInBlock() > 0) {
            firstVisibleBlock = firstVisibleBlock.next();
            firstVisibleCursor.setPosition(firstVisibleBlock.position());
        }

        // Find relative number for the first visible line.
        QTextBlock block = m_editor->textCursor().block();
        bool forward = firstVisibleBlock.blockNumber() > block.blockNumber();
        int n = 0;
        while (block.isValid() && block != firstVisibleBlock) {
            block = forward ? block.next() : block.previous();
            if (block.isVisible())
                n += forward ? 1 : -1;
        }

        // Copy colors from extra area palette.
        QPainter p(this);
        QPalette pal = m_editor->extraArea()->palette();
        const QColor fg = pal.color(QPalette::Dark);
        const QColor bg = pal.color(QPalette::Background);
        p.setPen(fg);

        // Draw relative line numbers.
        QRect rect(0, m_editor->cursorRect(firstVisibleCursor).y(), width(), m_lineSpacing);
        bool hideLineNumbers = m_editor->lineNumbersVisible();
        while (block.isValid()) {
            if (block.isVisible()) {
                if (n != 0 && rect.intersects(event->rect())) {
                    const int line = qAbs(n);
                    const QString number = QString::number(line);
                    if (hideLineNumbers)
                        p.fillRect(rect, bg);
                    if (hideLineNumbers || line < 100)
                        p.drawText(rect, Qt::AlignRight | Qt::AlignVCenter, number);
                }

                rect.translate(0, m_lineSpacing * block.lineCount());
                if (rect.y() > height())
                    break;

                ++n;
            }

            block = block.next();
        }
    }

    bool eventFilter(QObject *, QEvent *event)
    {
        if (event->type() == QEvent::Resize || event->type() == QEvent::Move)
            m_timerUpdate.start();
        return false;
    }

private:
    void followEditorLayout()
    {
        QTextCursor tc = m_editor->textCursor();
        m_currentPos = tc.position();
        m_lineSpacing = m_editor->cursorRect(tc).height();
        setFont(m_editor->extraArea()->font());

        // Follow geometry of normal line numbers if visible,
        // otherwise follow geometry of marks (breakpoints etc.).
        QRect rect = m_editor->extraArea()->geometry().adjusted(0, 0, -3, 0);
        bool marksVisible = m_editor->marksVisible();
        bool lineNumbersVisible = m_editor->lineNumbersVisible();
        bool foldMarksVisible = m_editor->codeFoldingVisible();
        if (marksVisible && lineNumbersVisible)
            rect.setLeft(m_lineSpacing);
        if (foldMarksVisible && (marksVisible || lineNumbersVisible))
            rect.setRight(rect.right() - (m_lineSpacing + m_lineSpacing % 2));
        setGeometry(rect);

        update();
    }

    int m_currentPos = 0;
    int m_lineSpacing = 0;
    TextEditorWidget *m_editor;
    QTimer m_timerUpdate;
};

///////////////////////////////////////////////////////////////////////
//
// FakeVimOptionPage
//
///////////////////////////////////////////////////////////////////////

typedef QMap<QString, QRegExp> ExCommandMap;
typedef QMap<int, QString> UserCommandMap;

class FakeVimOptionPage : public IOptionsPage
{
    Q_OBJECT

public:
    FakeVimOptionPage()
    {
        setId(SETTINGS_ID);
        setDisplayName(Tr::tr("General"));
        setCategory(SETTINGS_CATEGORY);
        setDisplayCategory(Tr::tr("FakeVim"));
        setCategoryIcon(Utils::Icon(SETTINGS_CATEGORY_FAKEVIM_ICON));
    }

    QWidget *widget();
    void apply();
    void finish();

private:
    void copyTextEditorSettings();
    void setQtStyle();
    void setPlainStyle();
    void updateVimRcWidgets();

    QPointer<QWidget> m_widget;
    Ui::FakeVimOptionPage m_ui;
    SavedActionSet m_group;
};

QWidget *FakeVimOptionPage::widget()
{
    if (!m_widget) {
        m_widget = new QWidget;
        m_ui.setupUi(m_widget);
        const QString vimrcDefault = QLatin1String(HostOsInfo::isAnyUnixHost()
                ? "$HOME/.vimrc" : "%USERPROFILE%\\_vimrc");
        m_ui.pathChooserVimRcPath->setExpectedKind(PathChooser::File);
        m_ui.pathChooserVimRcPath->lineEdit()->setToolTip(Tr::tr("Keep empty to use the default path, i.e. "
                                                             "%USERPROFILE%\\_vimrc on Windows, ~/.vimrc otherwise."));
        m_ui.pathChooserVimRcPath->lineEdit()->setPlaceholderText(Tr::tr("Default: %1").arg(vimrcDefault));

        m_group.clear();
        m_group.insert(theFakeVimSetting(ConfigUseFakeVim),
                       m_ui.checkBoxUseFakeVim);
        m_group.insert(theFakeVimSetting(ConfigReadVimRc),
                       m_ui.checkBoxReadVimRc);
        m_group.insert(theFakeVimSetting(ConfigVimRcPath),
                       m_ui.pathChooserVimRcPath);

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
        m_group.insert(theFakeVimSetting(ConfigPassKeys),
                       m_ui.checkBoxPassKeys);
        m_group.insert(theFakeVimSetting(ConfigTabStop),
                       m_ui.spinBoxTabStop);
        m_group.insert(theFakeVimSetting(ConfigScrollOff),
                       m_ui.spinBoxScrollOff);
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
        m_group.insert(theFakeVimSetting(ConfigSmartCase),
                       m_ui.checkBoxSmartCase);
        m_group.insert(theFakeVimSetting(ConfigIgnoreCase),
                       m_ui.checkBoxIgnoreCase);
        m_group.insert(theFakeVimSetting(ConfigWrapScan),
                       m_ui.checkBoxWrapScan);

        m_group.insert(theFakeVimSetting(ConfigShowCmd),
                       m_ui.checkBoxShowCmd);

        m_group.insert(theFakeVimSetting(ConfigRelativeNumber),
                       m_ui.checkBoxRelativeNumber);

        connect(m_ui.pushButtonCopyTextEditorSettings, &QAbstractButton::clicked,
                this, &FakeVimOptionPage::copyTextEditorSettings);
        connect(m_ui.pushButtonSetQtStyle, &QAbstractButton::clicked,
                this, &FakeVimOptionPage::setQtStyle);
        connect(m_ui.pushButtonSetPlainStyle, &QAbstractButton::clicked,
                this, &FakeVimOptionPage::setPlainStyle);
        connect(m_ui.checkBoxReadVimRc, &QCheckBox::stateChanged,
                this, &FakeVimOptionPage::updateVimRcWidgets);
        updateVimRcWidgets();

    }
    return m_widget;
}

void FakeVimOptionPage::apply()
{
    m_group.apply(ICore::settings());
}

void FakeVimOptionPage::finish()
{
    m_group.finish();
    delete m_widget;
}

void FakeVimOptionPage::copyTextEditorSettings()
{
    TabSettings ts = TextEditorSettings::codeStyle()->tabSettings();
    TypingSettings tps = TextEditorSettings::typingSettings();
    m_ui.checkBoxExpandTab->setChecked(ts.m_tabPolicy != TabSettings::TabsOnlyTabPolicy);
    m_ui.spinBoxTabStop->setValue(ts.m_tabSize);
    m_ui.spinBoxShiftWidth->setValue(ts.m_indentSize);
    m_ui.checkBoxSmartTab->setChecked(
        tps.m_smartBackspaceBehavior == TypingSettings::BackspaceFollowsPreviousIndents);
    m_ui.checkBoxAutoIndent->setChecked(true);
    m_ui.checkBoxSmartIndent->setChecked(tps.m_autoIndent);
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
    m_ui.lineEditBackspace->setText("indent,eol,start");
    m_ui.checkBoxPassKeys->setChecked(true);
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
    m_ui.lineEditBackspace->clear();
    m_ui.checkBoxPassKeys->setChecked(false);
}

void FakeVimOptionPage::updateVimRcWidgets()
{
    m_ui.pathChooserVimRcPath->setEnabled(m_ui.checkBoxReadVimRc->isChecked());
}

//const char *FAKEVIM_CONTEXT = "FakeVim";

///////////////////////////////////////////////////////////////////////
//
// FakeVimExCommandsPage
//
///////////////////////////////////////////////////////////////////////

enum { CommandRole = Qt::UserRole };

class FakeVimExCommandsWidget : public CommandMappings
{
    Q_OBJECT

public:
    FakeVimExCommandsWidget(FakeVimPluginPrivate *q, QWidget *parent = 0);

protected:
    void commandChanged();
    void resetToDefault();
    void defaultAction() override;

    void handleCurrentCommandChanged(QTreeWidgetItem *current);

private:
    void initialize();

    ExCommandMap &exCommandMap();
    ExCommandMap &defaultExCommandMap();

    FakeVimPluginPrivate *m_q;
    QGroupBox *m_commandBox;
    FancyLineEdit *m_commandEdit;
};

FakeVimExCommandsWidget::FakeVimExCommandsWidget(FakeVimPluginPrivate *q, QWidget *parent)
    : CommandMappings(parent), m_q(q)
{
    setPageTitle(Tr::tr("Ex Command Mapping"));
    setTargetHeader(Tr::tr("Ex Trigger Expression"));
    setImportExportEnabled(false);

    connect(this, &FakeVimExCommandsWidget::currentCommandChanged,
            this, &FakeVimExCommandsWidget::handleCurrentCommandChanged);

    m_commandBox = new QGroupBox(Tr::tr("Ex Command"), this);
    m_commandBox->setEnabled(false);
    auto boxLayout = new QHBoxLayout(m_commandBox);
    m_commandEdit = new FancyLineEdit(m_commandBox);
    m_commandEdit->setFiltering(true);
    m_commandEdit->setPlaceholderText(QString());
    connect(m_commandEdit, &FancyLineEdit::textChanged,
            this, &FakeVimExCommandsWidget::commandChanged);
    auto resetButton = new QPushButton(Tr::tr("Reset"), m_commandBox);
    resetButton->setToolTip(Tr::tr("Reset to default."));
    connect(resetButton, &QPushButton::clicked,
            this, &FakeVimExCommandsWidget::resetToDefault);
    boxLayout->addWidget(new QLabel(Tr::tr("Regular expression:")));
    boxLayout->addWidget(m_commandEdit);
    boxLayout->addWidget(resetButton);

    layout()->addWidget(m_commandBox);

    initialize();
}

class FakeVimExCommandsPage : public IOptionsPage
{
    Q_OBJECT

public:
    FakeVimExCommandsPage(FakeVimPluginPrivate *q)
        : m_q(q)
    {
        setId(SETTINGS_EX_CMDS_ID);
        setDisplayName(Tr::tr("Ex Command Mapping"));
        setCategory(SETTINGS_CATEGORY);
        setDisplayCategory(Tr::tr("FakeVim"));
        setCategoryIcon(Utils::Icon(SETTINGS_CATEGORY_FAKEVIM_ICON));
    }

    QWidget *widget() override;
    void apply() override {}
    void finish() override;

private:
    FakeVimPluginPrivate *m_q;
    QPointer<QWidget> m_widget;
};

QWidget *FakeVimExCommandsPage::widget()
{
    if (!m_widget)
        m_widget = new FakeVimExCommandsWidget(m_q);
    return m_widget;
}

void FakeVimExCommandsPage::finish()
{
    delete m_widget;
}

void FakeVimExCommandsWidget::initialize()
{
    QMap<QString, QTreeWidgetItem *> sections;

    foreach (Command *c, ActionManager::commands()) {
        if (c->action() && c->action()->isSeparator())
            continue;

        auto item = new QTreeWidgetItem;
        const QString name = c->id().toString();
        const int pos = name.indexOf('.');
        const QString section = name.left(pos);
        const QString subId = name.mid(pos + 1);
        item->setData(0, CommandRole, name);

        if (!sections.contains(section)) {
            auto categoryItem = new QTreeWidgetItem(commandList(), { section });
            QFont f = categoryItem->font(0);
            f.setBold(true);
            categoryItem->setFont(0, f);
            sections.insert(section, categoryItem);
            commandList()->expandItem(categoryItem);
        }
        sections[section]->addChild(item);

        item->setText(0, subId);
        item->setText(1, c->description());

        QString regex;
        if (exCommandMap().contains(name))
            regex = exCommandMap()[name].pattern();
        item->setText(2, regex);

        if (regex != defaultExCommandMap()[name].pattern())
            setModified(item, true);
    }

    handleCurrentCommandChanged(0);
}

void FakeVimExCommandsWidget::handleCurrentCommandChanged(QTreeWidgetItem *current)
{
    if (current) {
        m_commandEdit->setText(current->text(2));
        m_commandBox->setEnabled(true);
    } else {
        m_commandEdit->clear();
        m_commandBox->setEnabled(false);
    }
}

void FakeVimExCommandsWidget::commandChanged()
{
    QTreeWidgetItem *current = commandList()->currentItem();
    if (!current)
        return;

    const QString name =  current->data(0, CommandRole).toString();
    const QString regex = m_commandEdit->text();

    if (current->data(0, Qt::UserRole).isValid()) {
        current->setText(2, regex);
        exCommandMap()[name] = QRegExp(regex);
    }

    setModified(current, regex != defaultExCommandMap()[name].pattern());
}

void FakeVimExCommandsWidget::resetToDefault()
{
    QTreeWidgetItem *current = commandList()->currentItem();
    if (!current)
        return;
    const QString name = current->data(0, CommandRole).toString();
    QString regex;
    if (defaultExCommandMap().contains(name))
        regex = defaultExCommandMap()[name].pattern();
    m_commandEdit->setText(regex);
}

void FakeVimExCommandsWidget::defaultAction()
{
    int n = commandList()->topLevelItemCount();
    for (int i = 0; i != n; ++i) {
        QTreeWidgetItem *section = commandList()->topLevelItem(i);
        int m = section->childCount();
        for (int j = 0; j != m; ++j) {
            QTreeWidgetItem *item = section->child(j);
            const QString name = item->data(0, CommandRole).toString();
            QString regex;
            if (defaultExCommandMap().contains(name))
                regex = defaultExCommandMap()[name].pattern();
            setModified(item, false);
            item->setText(2, regex);
            if (item == commandList()->currentItem())
                currentCommandChanged(item);
        }
    }
}

///////////////////////////////////////////////////////////////////////
//
// FakeVimUserCommandsPage
//
///////////////////////////////////////////////////////////////////////

class FakeVimUserCommandsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    FakeVimUserCommandsModel(FakeVimPluginPrivate *q) : m_q(q) {}
    ~FakeVimUserCommandsModel() {}

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &data, int role);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

private:
    FakeVimPluginPrivate *m_q;
};

int FakeVimUserCommandsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 9;
}

int FakeVimUserCommandsModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 2;
}


QVariant FakeVimUserCommandsModel::headerData(int section,
    Qt::Orientation orient, int role) const
{
    if (orient == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0: return Tr::tr("Action");
            case 1: return Tr::tr("Command");
        };
    }
    return QVariant();
}

Qt::ItemFlags FakeVimUserCommandsModel::flags(const QModelIndex &index) const
{
    if (index.column() == 1)
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
    return QAbstractTableModel::flags(index);
}

class FakeVimUserCommandsDelegate : public QItemDelegate
{
public:
    explicit FakeVimUserCommandsDelegate(QObject *parent)
        : QItemDelegate(parent)
    {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
        const QModelIndex &) const
    {
        auto lineEdit = new QLineEdit(parent);
        lineEdit->setFrame(false);
        return lineEdit;
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const
    {
        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor);
        QTC_ASSERT(lineEdit, return);
        model->setData(index, lineEdit->text(), Qt::EditRole);
    }
};

class FakeVimUserCommandsPage : public IOptionsPage
{
    Q_OBJECT

public:
    FakeVimUserCommandsPage(FakeVimPluginPrivate *q)
        : m_q(q)
    {
        setId(SETTINGS_USER_CMDS_ID);
        setDisplayName(Tr::tr("User Command Mapping"));
        setCategory(SETTINGS_CATEGORY);
        setDisplayCategory(Tr::tr("FakeVim"));
        setCategoryIcon(Utils::Icon(SETTINGS_CATEGORY_FAKEVIM_ICON));
    }

    void apply();
    void finish() {}

    QWidget *widget();
    void initialize() {}
    UserCommandMap &userCommandMap();
    UserCommandMap &defaultUserCommandMap();

private:
    FakeVimPluginPrivate *m_q;
    QPointer<QWidget> m_widget;
};

QWidget *FakeVimUserCommandsPage::widget()
{
    if (!m_widget) {
        m_widget = new QWidget;

        auto model = new FakeVimUserCommandsModel(m_q);
        auto widget = new QTreeView;
        model->setParent(widget);
        widget->setModel(model);
        widget->resizeColumnToContents(0);

        auto delegate = new FakeVimUserCommandsDelegate(widget);
        widget->setItemDelegateForColumn(1, delegate);

        auto layout = new QGridLayout(m_widget);
        layout->addWidget(widget, 0, 0);
        m_widget->setLayout(layout);
    }
    return m_widget;
}

void FakeVimUserCommandsPage::apply()
{
    //m_q->writeSettings();
    delete m_widget;
}


///////////////////////////////////////////////////////////////////////
//
// WordCompletion
//
///////////////////////////////////////////////////////////////////////

class FakeVimCompletionAssistProvider : public CompletionAssistProvider
{
public:
    bool supportsEditor(Id ) const
    {
        return false;
    }

    IAssistProcessor *createProcessor() const;

    void setActive(const QString &needle, bool forward, FakeVimHandler *handler)
    {
        Q_UNUSED(forward);
        m_handler = handler;
        if (!m_handler)
            return;

        TextEditorWidget *editor = qobject_cast<TextEditorWidget *>(handler->widget());
        if (!editor)
            return;

        //qDebug() << "ACTIVATE: " << needle << forward;
        m_needle = needle;
        editor->invokeAssist(Completion, this);
    }

    void setInactive()
    {
        m_needle.clear();
        m_handler = 0;
    }

    const QString &needle() const
    {
        return m_needle;
    }

    void appendNeedle(const QChar &c)
    {
        m_needle.append(c);
    }

    FakeVimHandler *handler() const
    {
        return m_handler;
    }

private:
    FakeVimHandler *m_handler = nullptr;
    QString m_needle;
};

class FakeVimAssistProposalItem final : public AssistProposalItem
{
public:
    FakeVimAssistProposalItem(const FakeVimCompletionAssistProvider *provider)
        : m_provider(const_cast<FakeVimCompletionAssistProvider *>(provider))
    {}

    bool implicitlyApplies() const override
    {
        return false;
    }

    bool prematurelyApplies(const QChar &c) const override
    {
        m_provider->appendNeedle(c);
        return text() == m_provider->needle();
    }

    void applyContextualContent(TextDocumentManipulatorInterface &, int) const override
    {
        QTC_ASSERT(m_provider->handler(), return);
        m_provider->handler()->handleReplay(text().mid(m_provider->needle().size()));
        const_cast<FakeVimCompletionAssistProvider *>(m_provider)->setInactive();
    }

private:
    FakeVimCompletionAssistProvider *m_provider;
};


class FakeVimAssistProposalModel : public GenericProposalModel
{
public:
    FakeVimAssistProposalModel(const QList<AssistProposalItemInterface *> &items)
    {
        loadContent(items);
    }

    bool supportsPrefixExpansion() const override
    {
        return false;
    }
};

class FakeVimCompletionAssistProcessor : public IAssistProcessor
{
public:
    FakeVimCompletionAssistProcessor(const IAssistProvider *provider)
        : m_provider(static_cast<const FakeVimCompletionAssistProvider *>(provider))
    {}

    IAssistProposal *perform(const AssistInterface *interface) override
    {
        const QString &needle = m_provider->needle();

        const int basePosition = interface->position() - needle.size();

        QTextCursor tc(interface->textDocument());
        tc.setPosition(interface->position());
        tc.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);

        QList<AssistProposalItemInterface *> items;
        QSet<QString> seen;
        QTextDocument::FindFlags flags = QTextDocument::FindCaseSensitively;
        while (1) {
            tc = tc.document()->find(needle, tc.position(), flags);
            if (tc.isNull())
                break;
            QTextCursor sel = tc;
            sel.select(QTextCursor::WordUnderCursor);
            QString found = sel.selectedText();
            // Only add "real" completions.
            if (found.startsWith(needle)
                    && !seen.contains(found)
                    && sel.anchor() != basePosition) {
                seen.insert(found);
                auto item = new FakeVimAssistProposalItem(m_provider);
                item->setText(found);
                items.append(item);
            }
            tc.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor);
        }
        //qDebug() << "COMPLETIONS" << completions->size();

        delete interface;
        return new GenericProposal(basePosition, new FakeVimAssistProposalModel(items));
    }

private:
    const FakeVimCompletionAssistProvider *m_provider;
};

IAssistProcessor *FakeVimCompletionAssistProvider::createProcessor() const
{
    return new FakeVimCompletionAssistProcessor(this);
}


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

    friend class FakeVimPlugin;
    friend class FakeVimExCommandsWidget;
    friend class FakeVimUserCommandsPage;
    friend class FakeVimUserCommandsModel;

    bool initialize();
    void aboutToShutdown();

private:
    void onCoreAboutToClose();
    void editorOpened(Core::IEditor *);
    void editorAboutToClose(Core::IEditor *);
    void currentEditorAboutToChange(Core::IEditor *);

    void allDocumentsRenamed(const QString &oldName, const QString &newName);
    void documentRenamed(Core::IDocument *document, const QString &oldName, const QString &newName);
    void renameFileNameInEditors(const QString &oldName, const QString &newName);

    void setUseFakeVim(const QVariant &value);
    void setUseFakeVimInternal(bool on);
    void quitFakeVim();
    void triggerCompletions(FakeVimHandler *handler);
    void triggerSimpleCompletions(FakeVimHandler *handler, const QString &needle, bool forward);
    void windowCommand(FakeVimHandler *handler, const QString &key, int count);
    void find(bool reverse);
    void findNext(bool reverse);
    void foldToggle(FakeVimHandler *handler, int depth);
    void foldAll(FakeVimHandler *handler, bool fold);
    void fold(FakeVimHandler *handler, int depth, bool fold);
    void foldGoTo(FakeVimHandler *handler, int count, bool current);
    void jumpToGlobalMark(FakeVimHandler *, QChar mark, bool backTickMode, const QString &fileName);
    void maybeReadVimRc();
    void disableBlockSelection(FakeVimHandler *handler);
    void setBlockSelection(FakeVimHandler *handler, const QTextCursor &cursor);
    void blockSelection(FakeVimHandler *handler, QTextCursor *cursor);
    void hasBlockSelection(FakeVimHandler *handler, bool *on);
    void setShowRelativeLineNumbers(const QVariant &value);

    void resetCommandBuffer();
    void showCommandBuffer(FakeVimHandler *handler, const QString &contents,
                           int cursorPos, int anchorPos, int messageLevel);
    void showExtraInformation(FakeVimHandler *handler, const QString &msg);
    void changeSelection(FakeVimHandler *handler, const QList<QTextEdit::ExtraSelection> &selections);
    void highlightMatches(FakeVimHandler *handler, const QString &needle);
    void moveToMatchingParenthesis(FakeVimHandler *handler, bool *moved, bool *forward, QTextCursor *cursor);
    void checkForElectricCharacter(FakeVimHandler *handler, bool *result, QChar c);
    void indentRegion(FakeVimHandler *handler, int beginBlock, int endBlock, QChar typedChar);
    void handleExCommand(FakeVimHandler *handler, bool *handled, const ExCommand &cmd);

    void writeSettings();
    void readSettings();

    void handleDelayedQuitAll(bool forced);
    void handleDelayedQuit(bool forced, Core::IEditor *editor);
    void userActionTriggered(int key);

    void switchToFile(int n);
    int currentFile() const;

    void createRelativeNumberWidget(IEditor *editor);

signals:
    void delayedQuitRequested(bool forced, Core::IEditor *editor);
    void delayedQuitAllRequested(bool forced);

private:
    FakeVimPlugin *q;
    FakeVimOptionPage *m_fakeVimOptionsPage = nullptr;
    FakeVimExCommandsPage *m_fakeVimExCommandsPage = nullptr;
    FakeVimUserCommandsPage *m_fakeVimUserCommandsPage = nullptr;
    QHash<IEditor *, FakeVimHandler *> m_editorToHandler;

    void triggerAction(Id id);
    void setActionChecked(Id id, bool check);

    typedef int (*DistFunction)(const QRect &cursor, const QRect &other);
    void moveSomewhere(FakeVimHandler *handler, DistFunction f, int count);

    void keepOnlyWindow(); // :only

    ExCommandMap &exCommandMap() { return m_exCommandMap; }
    ExCommandMap &defaultExCommandMap() { return m_defaultExCommandMap; }
    ExCommandMap m_exCommandMap;
    ExCommandMap m_defaultExCommandMap;

    UserCommandMap &userCommandMap() { return m_userCommandMap; }
    UserCommandMap &defaultUserCommandMap() { return m_defaultUserCommandMap; }
    UserCommandMap m_userCommandMap;
    UserCommandMap m_defaultUserCommandMap;

    StatusBarWidget *m_statusBar;
    // @TODO: Delete
    //WordCompletion *m_wordCompletion;
    FakeVimCompletionAssistProvider *m_wordProvider = nullptr;
};

QVariant FakeVimUserCommandsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case 0: // Action
            return Tr::tr("User command #%1").arg(index.row() + 1);
        case 1: // Command
            return m_q->userCommandMap().value(index.row() + 1);
        }
    }

    return QVariant();
}

bool FakeVimUserCommandsModel::setData(const QModelIndex &index,
    const QVariant &data, int role)
{
    if (role == Qt::DisplayRole || role == Qt::EditRole)
        if (index.column() == 1)
            m_q->userCommandMap()[index.row() + 1] = data.toString();
    return true;
}

FakeVimPluginPrivate::FakeVimPluginPrivate(FakeVimPlugin *plugin)
    : q(plugin)
{
    defaultExCommandMap()[CppTools::Constants::SWITCH_HEADER_SOURCE] =
        QRegExp("^A$");
    defaultExCommandMap()["Coreplugin.OutputPane.previtem"] =
        QRegExp("^(cN(ext)?|cp(revious)?)!?( (.*))?$");
    defaultExCommandMap()["Coreplugin.OutputPane.nextitem"] =
        QRegExp("^cn(ext)?!?( (.*))?$");
    defaultExCommandMap()[TextEditor::Constants::FOLLOW_SYMBOL_UNDER_CURSOR] =
        QRegExp("^tag?$");
    defaultExCommandMap()[Core::Constants::GO_BACK] =
        QRegExp("^pop?$");
    defaultExCommandMap()["QtCreator.Locate"] =
        QRegExp("^e$");

    for (int i = 1; i < 10; ++i) {
        QString cmd = QString::fromLatin1(":echo User command %1 executed.<CR>");
        defaultUserCommandMap().insert(i, cmd.arg(i));
    }

    m_statusBar = 0;
}

void FakeVimPluginPrivate::onCoreAboutToClose()
{
    // Don't attach to editors anymore.
    disconnect(EditorManager::instance(), &EditorManager::editorOpened,
               this, &FakeVimPluginPrivate::editorOpened);
}

void FakeVimPluginPrivate::aboutToShutdown()
{
    q->removeObject(m_fakeVimOptionsPage);
    delete m_fakeVimOptionsPage;
    m_fakeVimOptionsPage = 0;

    q->removeObject(m_fakeVimExCommandsPage);
    delete m_fakeVimExCommandsPage;
    m_fakeVimExCommandsPage = 0;

    q->removeObject(m_fakeVimUserCommandsPage);
    delete m_fakeVimUserCommandsPage;
    m_fakeVimUserCommandsPage = 0;

    delete m_wordProvider;
    m_wordProvider = 0;
}

bool FakeVimPluginPrivate::initialize()
{
    //m_wordCompletion = new WordCompletion;
    //q->addAutoReleasedObject(m_wordCompletion);
    m_wordProvider = new FakeVimCompletionAssistProvider;

/*
    // Set completion settings and keep them up to date.
    TextEditorSettings *textEditorSettings = TextEditorSettings::instance();
    completion->setCompletionSettings(textEditorSettings->completionSettings());
    connect(textEditorSettings, &TextEditorSettings::completionSettingsChanged,
            completion, &TextEditorWidget::setCompletionSettings);
*/

    Context globalcontext(Core::Constants::C_GLOBAL);

    m_fakeVimOptionsPage = new FakeVimOptionPage;
    q->addObject(m_fakeVimOptionsPage);

    m_fakeVimExCommandsPage = new FakeVimExCommandsPage(this);
    q->addObject(m_fakeVimExCommandsPage);

    m_fakeVimUserCommandsPage = new FakeVimUserCommandsPage(this);
    q->addObject(m_fakeVimUserCommandsPage);

    readSettings();

    Command *cmd = 0;
    cmd = ActionManager::registerAction(theFakeVimSetting(ConfigUseFakeVim),
        INSTALL_HANDLER, globalcontext, true);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? Tr::tr("Meta+Shift+V,Meta+Shift+V") : Tr::tr("Alt+V,Alt+V")));

    ActionContainer *advancedMenu =
        ActionManager::actionContainer(Core::Constants::M_EDIT_ADVANCED);
    advancedMenu->addAction(cmd, Core::Constants::G_EDIT_EDITOR);

    const Id base = "FakeVim.UserAction";
    for (int i = 1; i < 10; ++i) {
        auto act = new QAction(this);
        act->setText(Tr::tr("Execute User Action #%1").arg(i));
        cmd = ActionManager::registerAction(act, base.withSuffix(i));
        cmd->setDefaultKeySequence(QKeySequence((UseMacShortcuts ? Tr::tr("Meta+Shift+V,%1") : Tr::tr("Alt+V,%1")).arg(i)));
        connect(act, &QAction::triggered, this, [this, i] { userActionTriggered(i); });
    }

    connect(ICore::instance(), &ICore::coreAboutToClose,
            this, &FakeVimPluginPrivate::onCoreAboutToClose);

    // EditorManager
    connect(EditorManager::instance(), &EditorManager::editorAboutToClose,
            this, &FakeVimPluginPrivate::editorAboutToClose);
    connect(EditorManager::instance(), &EditorManager::editorOpened,
            this, &FakeVimPluginPrivate::editorOpened);
    connect(EditorManager::instance(), &EditorManager::currentEditorAboutToChange,
            this, &FakeVimPluginPrivate::currentEditorAboutToChange);

    connect(DocumentManager::instance(), &DocumentManager::allDocumentsRenamed,
            this, &FakeVimPluginPrivate::allDocumentsRenamed);
    connect(DocumentManager::instance(), &DocumentManager::documentRenamed,
            this, &FakeVimPluginPrivate::documentRenamed);

    connect(theFakeVimSetting(ConfigUseFakeVim), &SavedAction::valueChanged,
            this, &FakeVimPluginPrivate::setUseFakeVim);
    connect(theFakeVimSetting(ConfigReadVimRc), &SavedAction::valueChanged,
            this, &FakeVimPluginPrivate::maybeReadVimRc);
    connect(theFakeVimSetting(ConfigVimRcPath), &SavedAction::valueChanged,
            this, &FakeVimPluginPrivate::maybeReadVimRc);
    connect(theFakeVimSetting(ConfigRelativeNumber), &SavedAction::valueChanged,
            this, &FakeVimPluginPrivate::setShowRelativeLineNumbers);

    // Delayed operations.
    connect(this, &FakeVimPluginPrivate::delayedQuitRequested,
            this, &FakeVimPluginPrivate::handleDelayedQuit, Qt::QueuedConnection);
    connect(this, &FakeVimPluginPrivate::delayedQuitAllRequested,
            this, &FakeVimPluginPrivate::handleDelayedQuitAll, Qt::QueuedConnection);

    // Vimrc can break test so don't source it if running tests.
    if (!ExtensionSystem::PluginManager::testRunRequested())
        maybeReadVimRc();
    //    << "MODE: " << theFakeVimSetting(ConfigUseFakeVim)->value();

    return true;
}

void FakeVimPluginPrivate::userActionTriggered(int key)
{
    IEditor *editor = EditorManager::currentEditor();
    FakeVimHandler *handler = m_editorToHandler[editor];
    if (handler) {
        // If disabled, enable FakeVim mode just for single user command.
        bool enableFakeVim = !theFakeVimSetting(ConfigUseFakeVim)->value().toBool();
        if (enableFakeVim)
            setUseFakeVimInternal(true);

        const QString cmd = userCommandMap().value(key);
        handler->handleInput(cmd);

        if (enableFakeVim)
            setUseFakeVimInternal(false);
    }
}

void FakeVimPluginPrivate::createRelativeNumberWidget(IEditor *editor)
{
    if (TextEditorWidget *textEditor = qobject_cast<TextEditorWidget *>(editor->widget())) {
        auto relativeNumbers = new RelativeNumbersColumn(textEditor);
        connect(theFakeVimSetting(ConfigRelativeNumber), &SavedAction::valueChanged,
                relativeNumbers, &QObject::deleteLater);
        connect(theFakeVimSetting(ConfigUseFakeVim), &SavedAction::valueChanged,
                relativeNumbers, &QObject::deleteLater);
        relativeNumbers->show();
    }
}

const char exCommandMapGroup[] = "FakeVimExCommand";
const char userCommandMapGroup[] = "FakeVimUserCommand";
const char reKey[] = "RegEx";
const char cmdKey[] = "Cmd";
const char idKey[] = "Command";

void FakeVimPluginPrivate::writeSettings()
{
    QSettings *settings = ICore::settings();

    theFakeVimSettings()->writeSettings(settings);

    { // block
    settings->beginWriteArray(exCommandMapGroup);
    int count = 0;
    typedef ExCommandMap::const_iterator Iterator;
    const Iterator end = exCommandMap().constEnd();
    for (Iterator it = exCommandMap().constBegin(); it != end; ++it) {
        const QString id = it.key();
        const QRegExp re = it.value();

        if ((defaultExCommandMap().contains(id) && defaultExCommandMap()[id] != re)
            || (!defaultExCommandMap().contains(id) && !re.pattern().isEmpty())) {
            settings->setArrayIndex(count);
            settings->setValue(idKey, id);
            settings->setValue(reKey, re.pattern());
            ++count;
        }
    }
    settings->endArray();
    } // block

    { // block
    settings->beginWriteArray(userCommandMapGroup);
    int count = 0;
    typedef UserCommandMap::const_iterator Iterator;
    const Iterator end = userCommandMap().constEnd();
    for (Iterator it = userCommandMap().constBegin(); it != end; ++it) {
        const int key = it.key();
        const QString cmd = it.value();

        if ((defaultUserCommandMap().contains(key)
                && defaultUserCommandMap()[key] != cmd)
            || (!defaultUserCommandMap().contains(key) && !cmd.isEmpty())) {
            settings->setArrayIndex(count);
            settings->setValue(idKey, key);
            settings->setValue(cmdKey, cmd);
            ++count;
        }
    }
    settings->endArray();
    } // block
}

void FakeVimPluginPrivate::readSettings()
{
    QSettings *settings = ICore::settings();

    theFakeVimSettings()->readSettings(settings);

    exCommandMap() = defaultExCommandMap();
    int size = settings->beginReadArray(exCommandMapGroup);
    for (int i = 0; i < size; ++i) {
        settings->setArrayIndex(i);
        const QString id = settings->value(idKey).toString();
        const QString re = settings->value(reKey).toString();
        exCommandMap()[id] = QRegExp(re);
    }
    settings->endArray();

    userCommandMap() = defaultUserCommandMap();
    size = settings->beginReadArray(userCommandMapGroup);
    for (int i = 0; i < size; ++i) {
        settings->setArrayIndex(i);
        const int id = settings->value(idKey).toInt();
        const QString cmd = settings->value(cmdKey).toString();
        userCommandMap()[id] = cmd;
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
    QString fileName = theFakeVimSetting(ConfigVimRcPath)->value().toString();
    if (fileName.isEmpty()) {
        fileName = QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
            + QLatin1String(HostOsInfo::isWindowsHost() ? "/_vimrc" : "/.vimrc");
    }
    //qDebug() << "READING VIMRC: " << fileName;
    // Read it into a temporary handler for effects modifying global state.
    QPlainTextEdit editor;
    FakeVimHandler handler(&editor);
    handler.handleCommand("source " + fileName);
    //writeSettings();
    //qDebug() << theFakeVimSetting(ConfigShiftWidth)->value();
}

void FakeVimPluginPrivate::triggerAction(Id id)
{
    Command *cmd = ActionManager::command(id);
    QTC_ASSERT(cmd, qDebug() << "UNKNOWN CODE: " << id.name(); return);
    QAction *action = cmd->action();
    QTC_ASSERT(action, return);
    action->trigger();
}

void FakeVimPluginPrivate::setActionChecked(Id id, bool check)
{
    Command *cmd = ActionManager::command(id);
    QTC_ASSERT(cmd, return);
    QAction *action = cmd->action();
    QTC_ASSERT(action, return);
    QTC_ASSERT(action->isCheckable(), return);
    action->setChecked(!check); // trigger negates the action's state
    action->trigger();
}

static int moveRightWeight(const QRect &cursor, const QRect &other)
{
    if (!cursor.adjusted(999999, 0, 0, 0).intersects(other))
        return -1;
    const int dx = other.left() - cursor.right();
    const int dy = qAbs(cursor.center().y() - other.center().y());
    const int w = 10000 * dx + dy;
    return w;
}

static int moveLeftWeight(const QRect &cursor, const QRect &other)
{
    if (!cursor.adjusted(-999999, 0, 0, 0).intersects(other))
        return -1;
    const int dx = cursor.left() - other.right();
    const int dy = qAbs(cursor.center().y() -other.center().y());
    const int w = 10000 * dx + dy;
    return w;
}

static int moveUpWeight(const QRect &cursor, const QRect &other)
{
    if (!cursor.adjusted(0, 0, 0, -999999).intersects(other))
        return -1;
    const int dy = cursor.top() - other.bottom();
    const int dx = qAbs(cursor.center().x() - other.center().x());
    const int w = 10000 * dy + dx;
    return w;
}

static int moveDownWeight(const QRect &cursor, const QRect &other)
{
    if (!cursor.adjusted(0, 0, 0, 999999).intersects(other))
        return -1;
    const int dy = other.top() - cursor.bottom();
    const int dx = qAbs(cursor.center().x() - other.center().x());
    const int w = 10000 * dy + dx;
    return w;
}

void FakeVimPluginPrivate::windowCommand(FakeVimHandler *handler, const QString &map, int count)
{
    // normalize mapping
    const QString key = map.toUpper();

    if (key == "C" || key == "<C-C>")
        triggerAction(Core::Constants::REMOVE_CURRENT_SPLIT);
    else if (key == "N" || key == "<C-N>")
        triggerAction(Core::Constants::GOTONEXT);
    else if (key == "O" || key == "<C-O>")
        keepOnlyWindow();
    else if (key == "P" || key == "<C-P>")
        triggerAction(Core::Constants::GOTOPREV);
    else if (key == "S" || key == "<C-S>")
        triggerAction(Core::Constants::SPLIT);
    else if (key == "V" || key == "<C-V>")
        triggerAction(Core::Constants::SPLIT_SIDE_BY_SIDE);
    else if (key == "W" || key == "<C-W>")
        triggerAction(Core::Constants::GOTO_NEXT_SPLIT);
    else if (key.contains("RIGHT") || key == "L" || key == "<S-L>")
        moveSomewhere(handler, &moveRightWeight, key == "<S-L>" ? -1 : count);
    else if (key.contains("LEFT")  || key == "H" || key == "<S-H>")
        moveSomewhere(handler, &moveLeftWeight, key == "<S-H>" ? -1 : count);
    else if (key.contains("UP")    || key == "K" || key == "<S-K>")
        moveSomewhere(handler, &moveUpWeight, key == "<S-K>" ? -1 : count);
    else if (key.contains("DOWN")  || key == "J" || key == "<S-J>")
        moveSomewhere(handler, &moveDownWeight, key == "<S-J>" ? -1 : count);
    else
        qDebug() << "UNKNOWN WINDOW COMMAND: <C-W>" << map;
}

void FakeVimPluginPrivate::moveSomewhere(FakeVimHandler *handler, DistFunction f, int count)
{
    QTC_ASSERT(handler, return);
    QWidget *w = handler->widget();
    QPlainTextEdit *pe = qobject_cast<QPlainTextEdit *>(w);
    QTC_ASSERT(pe, return);
    QRect rc = pe->cursorRect();
    QRect cursorRect(w->mapToGlobal(rc.topLeft()), w->mapToGlobal(rc.bottomRight()));
    //qDebug() << "\nCURSOR: " << cursorRect;

    IEditor *bestEditor = 0;
    int repeat = count;

    IEditor *currentEditor = EditorManager::currentEditor();
    QList<IEditor *> editors = EditorManager::visibleEditors();
    while (repeat < 0 || repeat-- > 0) {
        editors.removeOne(currentEditor);
        int bestValue = -1;
        foreach (IEditor *editor, editors) {
            QWidget *w = editor->widget();
            QRect editorRect(w->mapToGlobal(w->geometry().topLeft()),
                    w->mapToGlobal(w->geometry().bottomRight()));
            //qDebug() << "   EDITOR: " << editorRect << editor;

            int value = f(cursorRect, editorRect);
            if (value != -1 && (bestValue == -1 || value < bestValue)) {
                bestValue = value;
                bestEditor = editor;
                //qDebug() << "          BEST SO FAR: " << bestValue << bestEditor;
            }
        }
        if (bestValue == -1)
            break;

        currentEditor = bestEditor;
        //qDebug() << "     BEST: " << bestValue << bestEditor;
    }

    // FIME: This is know to fail as the EditorManager will fall back to
    // the current editor's view. Needs additional public API there.
    if (bestEditor)
        EditorManager::activateEditor(bestEditor);
}

void FakeVimPluginPrivate::keepOnlyWindow()
{
    IEditor *currentEditor = EditorManager::currentEditor();
    QList<IEditor *> editors = EditorManager::visibleEditors();
    editors.removeOne(currentEditor);

    foreach (IEditor *editor, editors) {
        EditorManager::activateEditor(editor);
        triggerAction(Core::Constants::REMOVE_CURRENT_SPLIT);
    }
}

void FakeVimPluginPrivate::find(bool reverse)
{
    Find::setUseFakeVim(true);
    Find::openFindToolBar(reverse ? Find::FindBackwardDirection
                                  : Find::FindForwardDirection);
}

void FakeVimPluginPrivate::findNext(bool reverse)
{
    if (reverse)
        triggerAction(Core::Constants::FIND_PREVIOUS);
    else
        triggerAction(Core::Constants::FIND_NEXT);
}

void FakeVimPluginPrivate::foldToggle(FakeVimHandler *handler, int depth)
{
    QTC_ASSERT(handler, return);
    QTextBlock block = handler->textCursor().block();
    fold(handler, depth, !TextDocumentLayout::isFolded(block));
}

void FakeVimPluginPrivate::foldAll(FakeVimHandler *handler, bool fold)
{
    QTC_ASSERT(handler, return);
    QTextDocument *document = handler->textCursor().document();
    auto documentLayout = qobject_cast<TextDocumentLayout*>(document->documentLayout());
    QTC_ASSERT(documentLayout != 0, return);

    QTextBlock block = document->firstBlock();
    while (block.isValid()) {
        TextDocumentLayout::doFoldOrUnfold(block, !fold);
        block = block.next();
    }

    documentLayout->requestUpdate();
    documentLayout->emitDocumentSizeChanged();
}

void FakeVimPluginPrivate::fold(FakeVimHandler *handler, int depth, bool fold)
{
    QTC_ASSERT(handler, return);
    QTextDocument *doc = handler->textCursor().document();
    QTC_ASSERT(doc, return);
    auto documentLayout = qobject_cast<TextDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);

    QTextBlock block = handler->textCursor().block();
    int indent = TextDocumentLayout::foldingIndent(block);
    if (fold) {
        if (TextDocumentLayout::isFolded(block)) {
            while (block.isValid() && (TextDocumentLayout::foldingIndent(block) >= indent
                || !block.isVisible())) {
                block = block.previous();
            }
        }
        if (TextDocumentLayout::canFold(block))
            ++indent;
        while (depth != 0 && block.isValid()) {
            const int indent2 = TextDocumentLayout::foldingIndent(block);
            if (TextDocumentLayout::canFold(block) && indent2 < indent) {
                TextDocumentLayout::doFoldOrUnfold(block, false);
                if (depth > 0)
                    --depth;
                indent = indent2;
            }
            block = block.previous();
        }
    } else {
        if (TextDocumentLayout::isFolded(block)) {
            if (depth < 0) {
                // recursively open fold
                while (block.isValid()
                    && TextDocumentLayout::foldingIndent(block) >= indent) {
                    if (TextDocumentLayout::canFold(block))
                        TextDocumentLayout::doFoldOrUnfold(block, true);
                    block = block.next();
                }
            } else {
                if (TextDocumentLayout::canFold(block)) {
                    TextDocumentLayout::doFoldOrUnfold(block, true);
                    if (depth > 0)
                        --depth;
                }
            }
        }
    }

    documentLayout->requestUpdate();
    documentLayout->emitDocumentSizeChanged();
}

void FakeVimPluginPrivate::foldGoTo(FakeVimHandler *handler, int count, bool current)
{
    QTC_ASSERT(handler, return);
    QTextCursor tc = handler->textCursor();
    QTextBlock block = tc.block();

    int pos = -1;
    if (count > 0) {
        int repeat = count;
        block = block.next();
        QTextBlock prevBlock = block;
        int indent = TextDocumentLayout::foldingIndent(block);
        block = block.next();
        while (block.isValid()) {
            int newIndent = TextDocumentLayout::foldingIndent(block);
            if (current ? indent > newIndent : indent < newIndent) {
                if (prevBlock.isVisible()) {
                    pos = prevBlock.position();
                    if (--repeat <= 0)
                        break;
                } else if (current) {
                    indent = newIndent;
                }
            }
            if (!current)
                indent = newIndent;
            prevBlock = block;
            block = block.next();
        }
    } else if (count < 0) {
        int repeat = -count;
        int indent = TextDocumentLayout::foldingIndent(block);
        block = block.previous();
        while (block.isValid()) {
            int newIndent = TextDocumentLayout::foldingIndent(block);
            if (current ? indent > newIndent : indent < newIndent) {
                while (block.isValid() && !block.isVisible())
                    block = block.previous();
                pos = block.position();
                if (--repeat <= 0)
                    break;
            }
            if (!current)
                indent = newIndent;
            block = block.previous();
        }
    }

    if (pos != -1) {
        tc.setPosition(pos, QTextCursor::KeepAnchor);
        handler->setTextCursor(tc);
    }
}

void FakeVimPluginPrivate::jumpToGlobalMark(FakeVimHandler *,
    QChar mark, bool backTickMode, const QString &fileName)
{
    IEditor *iedit = EditorManager::openEditor(fileName);
    if (!iedit)
        return;
    FakeVimHandler *handler = m_editorToHandler.value(iedit, 0);
    if (handler)
        handler->jumpToLocalMark(mark, backTickMode);
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

    ~DeferredDeleter()
    {
        if (m_handler) {
            m_handler->disconnectFromEditor();
            m_handler->deleteLater();
            m_handler = 0;
        }
    }
};

void FakeVimPluginPrivate::editorOpened(IEditor *editor)
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

    auto handler = new FakeVimHandler(widget, 0);
    // the handler might have triggered the deletion of the editor:
    // make sure that it can return before being deleted itself
    new DeferredDeleter(widget, handler);
    m_editorToHandler[editor] = handler;

    connect(handler, &FakeVimHandler::extraInformationChanged,
            this, &FakeVimPluginPrivate::showExtraInformation);
    connect(handler, &FakeVimHandler::commandBufferChanged,
            this, &FakeVimPluginPrivate::showCommandBuffer);
    connect(handler, &FakeVimHandler::selectionChanged,
            this, &FakeVimPluginPrivate::changeSelection);
    connect(handler, &FakeVimHandler::highlightMatches,
            this, &FakeVimPluginPrivate::highlightMatches);
    connect(handler, &FakeVimHandler::moveToMatchingParenthesis,
            this, &FakeVimPluginPrivate::moveToMatchingParenthesis, Qt::DirectConnection);
    connect(handler, &FakeVimHandler::indentRegion,
            this, &FakeVimPluginPrivate::indentRegion);
    connect(handler, &FakeVimHandler::checkForElectricCharacter,
            this, &FakeVimPluginPrivate::checkForElectricCharacter, Qt::DirectConnection);
    connect(handler, &FakeVimHandler::requestDisableBlockSelection,
            this, &FakeVimPluginPrivate::disableBlockSelection);
    connect(handler, &FakeVimHandler::requestSetBlockSelection,
            this, &FakeVimPluginPrivate::setBlockSelection);
    connect(handler, &FakeVimHandler::requestBlockSelection,
            this, &FakeVimPluginPrivate::blockSelection, Qt::DirectConnection);
    connect(handler, &FakeVimHandler::requestHasBlockSelection,
            this, &FakeVimPluginPrivate::hasBlockSelection, Qt::DirectConnection);
    connect(handler, &FakeVimHandler::completionRequested,
            this, &FakeVimPluginPrivate::triggerCompletions);
    connect(handler, &FakeVimHandler::simpleCompletionRequested,
            this, &FakeVimPluginPrivate::triggerSimpleCompletions);
    connect(handler, &FakeVimHandler::windowCommandRequested,
            this, &FakeVimPluginPrivate::windowCommand);
    connect(handler, &FakeVimHandler::findRequested,
            this, &FakeVimPluginPrivate::find);
    connect(handler, &FakeVimHandler::findNextRequested,
            this, &FakeVimPluginPrivate::findNext);
    connect(handler, &FakeVimHandler::foldToggle,
            this, &FakeVimPluginPrivate::foldToggle);
    connect(handler, &FakeVimHandler::foldAll,
            this, &FakeVimPluginPrivate::foldAll);
    connect(handler, &FakeVimHandler::fold,
            this, &FakeVimPluginPrivate::fold);
    connect(handler, &FakeVimHandler::foldGoTo,
            this, &FakeVimPluginPrivate::foldGoTo);
    connect(handler, &FakeVimHandler::jumpToGlobalMark,
            this, &FakeVimPluginPrivate::jumpToGlobalMark);

    connect(handler, &FakeVimHandler::handleExCommandRequested,
            this, &FakeVimPluginPrivate::handleExCommand, Qt::DirectConnection);

    connect(ICore::instance(), &ICore::saveSettingsRequested,
            this, &FakeVimPluginPrivate::writeSettings);

    handler->setCurrentFileName(editor->document()->filePath().toString());
    handler->installEventFilter();

    // pop up the bar
    if (theFakeVimSetting(ConfigUseFakeVim)->value().toBool()) {
       resetCommandBuffer();
       handler->setupWidget();

       if (theFakeVimSetting(ConfigRelativeNumber)->value().toBool())
           createRelativeNumberWidget(editor);
    }
}

void FakeVimPluginPrivate::editorAboutToClose(IEditor *editor)
{
    //qDebug() << "CLOSING: " << editor << editor->widget();
    m_editorToHandler.remove(editor);
}

void FakeVimPluginPrivate::currentEditorAboutToChange(IEditor *editor)
{
    if (FakeVimHandler *handler = m_editorToHandler.value(editor, 0))
        handler->enterCommandMode();
}

void FakeVimPluginPrivate::allDocumentsRenamed(const QString &oldName, const QString &newName)
{
    renameFileNameInEditors(oldName, newName);
    FakeVimHandler::updateGlobalMarksFilenames(oldName, newName);
}

void FakeVimPluginPrivate::documentRenamed(
        IDocument *, const QString &oldName, const QString &newName)
{
    renameFileNameInEditors(oldName, newName);
}

void FakeVimPluginPrivate::renameFileNameInEditors(const QString &oldName, const QString &newName)
{
    foreach (FakeVimHandler *handler, m_editorToHandler.values()) {
        if (handler->currentFileName() == oldName)
            handler->setCurrentFileName(newName);
    }
}

void FakeVimPluginPrivate::setUseFakeVim(const QVariant &value)
{
    //qDebug() << "SET USE FAKEVIM" << value;
    bool on = value.toBool();
    Find::setUseFakeVim(on);
    setUseFakeVimInternal(on);
    setShowRelativeLineNumbers(theFakeVimSetting(ConfigRelativeNumber)->value());
}

void FakeVimPluginPrivate::setUseFakeVimInternal(bool on)
{
    if (on) {
        //ICore *core = ICore::instance();
        //core->updateAdditionalContexts(Context(FAKEVIM_CONTEXT),
        // Context());
        foreach (IEditor *editor, m_editorToHandler.keys())
            m_editorToHandler[editor]->setupWidget();
    } else {
        //ICore *core = ICore::instance();
        //core->updateAdditionalContexts(Context(),
        // Context(FAKEVIM_CONTEXT));
        resetCommandBuffer();
        foreach (IEditor *editor, m_editorToHandler.keys()) {
            if (TextDocument *textDocument = qobject_cast<TextDocument *>(editor->document()))
                m_editorToHandler[editor]->restoreWidget(textDocument->tabSettings().m_tabSize);
        }
    }
}

void FakeVimPluginPrivate::triggerCompletions(FakeVimHandler *handler)
{
    QTC_ASSERT(handler, return);
    if (TextEditorWidget *editor = qobject_cast<TextEditorWidget *>(handler->widget()))
        editor->invokeAssist(Completion, m_wordProvider);
//        CompletionSupport::instance()->complete(editor->editor(), TextCompletion, false);
}

void FakeVimPluginPrivate::triggerSimpleCompletions(FakeVimHandler *handler, const QString &needle, bool forward)
{
    QTC_ASSERT(handler, return);
//    m_wordCompletion->setActive(needle, forward, handler);
    m_wordProvider->setActive(needle, forward, handler);
}

void FakeVimPluginPrivate::disableBlockSelection(FakeVimHandler *handler)
{
    QTC_ASSERT(handler, return);
    if (TextEditorWidget *bt = qobject_cast<TextEditorWidget *>(handler->widget()))
        bt->setBlockSelection(false);
}

void FakeVimPluginPrivate::setBlockSelection(FakeVimHandler *handler, const QTextCursor &cursor)
{
    QTC_ASSERT(handler, return);
    if (TextEditorWidget *bt = qobject_cast<TextEditorWidget *>(handler->widget()))
        bt->setBlockSelection(cursor);
}

void FakeVimPluginPrivate::blockSelection(FakeVimHandler *handler, QTextCursor *cursor)
{
    QTC_ASSERT(handler, return);
    if (TextEditorWidget *bt = qobject_cast<TextEditorWidget *>(handler->widget()))
        if (cursor)
            *cursor = bt->blockSelection();
}

void FakeVimPluginPrivate::hasBlockSelection(FakeVimHandler *handler, bool *on)
{
    QTC_ASSERT(handler, return);
    if (TextEditorWidget *bt = qobject_cast<TextEditorWidget *>(handler->widget()))
        *on = bt->hasBlockSelection();
}

void FakeVimPluginPrivate::setShowRelativeLineNumbers(const QVariant &value)
{
    if (value.toBool() && theFakeVimSetting(ConfigUseFakeVim)->value().toBool()) {
        foreach (IEditor *editor, m_editorToHandler.keys())
            createRelativeNumberWidget(editor);
    }
}

void FakeVimPluginPrivate::checkForElectricCharacter(FakeVimHandler *handler, bool *result, QChar c)
{
    QTC_ASSERT(handler, return);
    if (TextEditorWidget *bt = qobject_cast<TextEditorWidget *>(handler->widget()))
        *result = bt->textDocument()->indenter()->isElectricCharacter(c);
}

void FakeVimPluginPrivate::handleExCommand(FakeVimHandler *handler, bool *handled, const ExCommand &cmd)
{
    QTC_ASSERT(handler, return);
    using namespace Core;
    //qDebug() << "PLUGIN HANDLE: " << cmd.cmd << cmd.count;

    *handled = false;

    // Focus editor first so actions can be executed in correct context.
    QWidget *editor = handler->widget();
    if (editor)
        editor->setFocus();

    *handled = true;
    if ((cmd.matches("w", "write") || cmd.cmd == "wq") && cmd.args.isEmpty()) {
        // :w[rite]
        bool saved = false;
        IEditor *editor = m_editorToHandler.key(handler);
        const QString fileName = handler->currentFileName();
        if (editor && editor->document()->filePath().toString() == fileName) {
            triggerAction(Core::Constants::SAVE);
            saved = !editor->document()->isModified();
            if (saved) {
                QFile file3(fileName);
                if (file3.open(QIODevice::ReadOnly)) {
                    const QByteArray ba = file3.readAll();
                    handler->showMessage(MessageInfo, Tr::tr("\"%1\" %2 %3L, %4C written")
                        .arg(fileName).arg(' ').arg(ba.count('\n')).arg(ba.size()));
                    if (cmd.cmd == "wq")
                        delayedQuitRequested(cmd.hasBang, m_editorToHandler.key(handler));
                }
            }
        }

        if (!saved)
            handler->showMessage(MessageError, Tr::tr("File not saved"));
    } else if (cmd.matches("wa", "wall")) {
        // :w[all]
        triggerAction(Core::Constants::SAVEALL);
        const QList<IDocument *> failed = DocumentManager::modifiedDocuments();
        if (failed.isEmpty())
            handler->showMessage(MessageInfo, Tr::tr("Saving succeeded"));
        else
            handler->showMessage(MessageError, Tr::tr("%n files not saved", 0, failed.size()));
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
            ICore::showOptionsDialog(SETTINGS_ID);
        } else if (cmd.args == "ic" || cmd.args == "ignorecase") {
            // :set nc
            setActionChecked(Core::Constants::CASE_SENSITIVE, false);
        } else if (cmd.args == "noic" || cmd.args == "noignorecase") {
            // :set noic
            setActionChecked(Core::Constants::CASE_SENSITIVE, true);
        }
        *handled = false; // Let the handler see it as well.
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
        keepOnlyWindow();
    } else if (cmd.cmd == "AS") {
        triggerAction(Core::Constants::SPLIT);
        triggerAction(CppTools::Constants::SWITCH_HEADER_SOURCE);
    } else if (cmd.cmd == "AV") {
        triggerAction(Core::Constants::SPLIT_SIDE_BY_SIDE);
        triggerAction(CppTools::Constants::SWITCH_HEADER_SOURCE);
    } else {
        // Check whether one of the configure commands matches.
        typedef ExCommandMap::const_iterator Iterator;
        const Iterator end = exCommandMap().constEnd();
        for (Iterator it = exCommandMap().constBegin(); it != end; ++it) {
            const QString &id = it.key();
            QRegExp re = it.value();
            if (!re.pattern().isEmpty() && re.indexIn(cmd.cmd) != -1) {
                triggerAction(Id::fromString(id));
                return;
            }
        }
        *handled = false;
    }
}

void FakeVimPluginPrivate::handleDelayedQuit(bool forced, IEditor *editor)
{
    // This tries to simulate vim behaviour. But the models of vim and
    // Qt Creator core do not match well...
    if (EditorManager::hasSplitter())
        triggerAction(Core::Constants::REMOVE_CURRENT_SPLIT);
    else
        EditorManager::closeEditor(editor, !forced);
}

void FakeVimPluginPrivate::handleDelayedQuitAll(bool forced)
{
    triggerAction(Core::Constants::REMOVE_ALL_SPLITS);
    EditorManager::closeAllEditors(!forced);
}

void FakeVimPluginPrivate::moveToMatchingParenthesis(FakeVimHandler *, bool *moved, bool *forward,
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

void FakeVimPluginPrivate::indentRegion(FakeVimHandler *handler,
        int beginBlock, int endBlock, QChar typedChar)
{
    QTC_ASSERT(handler, return);

    TextEditorWidget *bt = qobject_cast<TextEditorWidget *>(handler->widget());
    if (!bt)
        return;

    TabSettings tabSettings;
    tabSettings.m_indentSize = theFakeVimSetting(ConfigShiftWidth)->value().toInt();
    tabSettings.m_tabSize = theFakeVimSetting(ConfigTabStop)->value().toInt();
    tabSettings.m_tabPolicy = theFakeVimSetting(ConfigExpandTab)->value().toBool()
            ? TabSettings::SpacesOnlyTabPolicy : TabSettings::TabsOnlyTabPolicy;

    QTextDocument *doc = bt->document();
    QTextBlock startBlock = doc->findBlockByNumber(beginBlock);

    // Record line lenghts for mark adjustments
    QVector<int> lineLengths(endBlock - beginBlock + 1);
    QTextBlock block = startBlock;

    for (int i = beginBlock; i <= endBlock; ++i) {
        lineLengths[i - beginBlock] = block.text().length();
        if (typedChar.unicode() == 0 && block.text().simplified().isEmpty()) {
            // clear empty lines
            QTextCursor cursor(block);
            while (!cursor.atBlockEnd())
                cursor.deleteChar();
        } else {
            bt->textDocument()->indenter()->indentBlock(doc, block, typedChar, tabSettings);
        }
        block = block.next();
    }
}

void FakeVimPluginPrivate::quitFakeVim()
{
    theFakeVimSetting(ConfigUseFakeVim)->setValue(false);
}

void FakeVimPluginPrivate::resetCommandBuffer()
{
    showCommandBuffer(nullptr, QString(), -1, -1, 0);
}

void FakeVimPluginPrivate::showCommandBuffer(FakeVimHandler *handler, const QString &contents, int cursorPos, int anchorPos,
                                             int messageLevel)
{
    //qDebug() << "SHOW COMMAND BUFFER" << contents;
    if (MiniBuffer *w = qobject_cast<MiniBuffer *>(m_statusBar->widget()))
        w->setContents(contents, cursorPos, anchorPos, messageLevel, handler);
}

void FakeVimPluginPrivate::showExtraInformation(FakeVimHandler *, const QString &text)
{
    EditorManager::splitSideBySide();
    QString title = "stdout.txt";
    IEditor *iedit = EditorManager::openEditorWithContents(Id(), &title, text.toUtf8());
    EditorManager::activateEditor(iedit);
    FakeVimHandler *handler = m_editorToHandler.value(iedit, 0);
    QTC_ASSERT(handler, return);
    handler->handleCommand("0");
}

void FakeVimPluginPrivate::changeSelection(FakeVimHandler *handler, const QList<QTextEdit::ExtraSelection> &selection)
{
    QTC_ASSERT(handler, return);
    if (TextEditorWidget *bt = qobject_cast<TextEditorWidget *>(handler->widget()))
        bt->setExtraSelections(TextEditorWidget::FakeVimSelection, selection);
}

void FakeVimPluginPrivate::highlightMatches(FakeVimHandler *, const QString &needle)
{
    foreach (IEditor *editor, EditorManager::visibleEditors()) {
        QWidget *w = editor->widget();
        IFindSupport *find = Aggregation::query<IFindSupport>(w);
        if (find != 0)
            find->highlightAll(needle, FindRegularExpression | FindCaseSensitively);
    }
}

int FakeVimPluginPrivate::currentFile() const
{
    IEditor *editor = EditorManager::currentEditor();
    if (!editor)
        return -1;
    return DocumentModel::indexOfDocument(editor->document());
}

void FakeVimPluginPrivate::switchToFile(int n)
{
    int size = DocumentModel::entryCount();
    QTC_ASSERT(size, return);
    n = n % size;
    if (n < 0)
        n += size;
    EditorManager::activateEditorForEntry(DocumentModel::entries().at(n));
}

ExCommandMap &FakeVimExCommandsWidget::exCommandMap()
{
    return m_q->exCommandMap();
}

ExCommandMap &FakeVimExCommandsWidget::defaultExCommandMap()
{
    return m_q->defaultExCommandMap();
}

UserCommandMap &FakeVimUserCommandsPage::userCommandMap()
{
    return m_q->userCommandMap();
}

UserCommandMap &FakeVimUserCommandsPage::defaultUserCommandMap()
{
    return m_q->defaultUserCommandMap();
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
    d->m_statusBar = new StatusBarWidget;
    d->m_statusBar->setWidget(new MiniBuffer);
    d->m_statusBar->setPosition(StatusBarWidget::LastLeftAligned);
    addAutoReleasedObject(d->m_statusBar);
}

#ifdef WITH_TESTS
void FakeVimPlugin::setupTest(QString *title, FakeVimHandler **handler, QWidget **edit)
{
    *title = QString::fromLatin1("test.cpp");
    IEditor *iedit = EditorManager::openEditorWithContents(Id(), title);
    EditorManager::activateEditor(iedit);
    *edit = iedit->widget();
    *handler = d->m_editorToHandler.value(iedit, 0);
    (*handler)->setupWidget();
    (*handler)->handleCommand("set startofline");

//    *handler = 0;
//    m_statusMessage.clear();
//    m_statusData.clear();
//    m_infoMessage.clear();
//    if (m_textedit) {
//        m_textedit->setPlainText(lines);
//        QTextCursor tc = m_textedit->textCursor();
//        tc.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
//        m_textedit->setTextCursor(tc);
//        m_textedit->setPlainText(lines);
//        *handler = new FakeVimHandler(m_textedit);
//    } else {
//        m_plaintextedit->setPlainText(lines);
//        QTextCursor tc = m_plaintextedit->textCursor();
//        tc.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
//        m_plaintextedit->setTextCursor(tc);
//        m_plaintextedit->setPlainText(lines);
//        *handler = new FakeVimHandler(m_plaintextedit);
//    }

//    connect(*handler, &FakeVimHandler::commandBufferChanged,
//            this, &FakeVimPlugin::changeStatusMessage);
//    connect(*handler, &FakeVimHandler::extraInformationChanged,
//            this, &FakeVimPlugin::changeExtraInformation);
//    connect(*handler, &FakeVimHandler::statusDataChanged,
//            this, &FakeVimPlugin::changeStatusData);

//    QCOMPARE(EDITOR(toPlainText()), lines);
    (*handler)->handleCommand("set iskeyword=@,48-57,_,192-255,a-z,A-Z");
}
#endif

} // namespace Internal
} // namespace FakeVim

#include "fakevimplugin.moc"
