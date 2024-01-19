// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fakevimplugin.h"

#include "fakevimactions.h"
#include "fakevimhandler.h"
#include "fakevimtr.h"

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
#include <coreplugin/statusbarmanager.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/asyncprocessor.h>
#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/displaysettings.h>
#include <texteditor/icodestylepreferences.h>
#include <texteditor/indenter.h>
#include <texteditor/tabsettings.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textmark.h>
#include <texteditor/typingsettings.h>

#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/fancylineedit.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>

#include <cppeditor/cppeditorconstants.h>

#include <extensionsystem/pluginmanager.h>

#include <QAction>
#include <QAbstractTableModel>
#include <QDebug>
#include <QFile>
#include <QGridLayout>
#include <QGroupBox>
#include <QGuiApplication>
#include <QItemDelegate>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStyleHints>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>
#include <QTimer>
#include <QTreeWidgetItem>

#include <functional>

using namespace TextEditor;
using namespace Core;
using namespace Utils;

namespace FakeVim {
namespace Internal {

const char INSTALL_HANDLER[]                = "TextEditor.FakeVimHandler";
const char SETTINGS_CATEGORY[]              = "D.FakeVim";
const char SETTINGS_ID[]                    = "A.FakeVim.General";
const char SETTINGS_EX_CMDS_ID[]            = "B.FakeVim.ExCommands";
const char SETTINGS_USER_CMDS_ID[]          = "C.FakeVim.UserCommands";

static class FakeVimPluginPrivate *dd = nullptr;

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
            {
                QSignalBlocker blocker(m_edit);
                m_label->clear();
                m_edit->setText(contents);
                if (anchorPos != -1 && anchorPos != cursorPos)
                    m_edit->setSelection(anchorPos, cursorPos - anchorPos);
                else
                    m_edit->setCursorPosition(cursorPos);
            }
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
            if (m_eventFilter != nullptr) {
                m_edit->removeEventFilter(m_eventFilter);
                disconnect(this, &MiniBuffer::edited, nullptr, nullptr);
            }
            if (eventFilter != nullptr) {
                m_edit->installEventFilter(eventFilter);
                connect(this, &MiniBuffer::edited,
                        eventFilter, &FakeVimHandler::miniBufferTextEdited);
            }
            m_eventFilter = eventFilter;
        }

        m_lastMessageLevel = messageLevel;
    }

    QSize sizeHint() const override
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

        auto start = QOverload<>::of(&QTimer::start);
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
    void paintEvent(QPaintEvent *event) override
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
        const QColor bg = pal.color(QPalette::Window);
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

    bool eventFilter(QObject *, QEvent *event) override
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
        m_lineSpacing = m_editor->document()->documentLayout()->blockBoundingRect(tc.block()).height();
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

using ExCommandMap = QMap<QString, QRegularExpression>;
using UserCommandMap = QMap<int, QString>;


///////////////////////////////////////////////////////////////////////
//
// FakeVimPluginPrivate
//
///////////////////////////////////////////////////////////////////////

class FakeVimPluginRunData;

class FakeVimPluginPrivate : public QObject
{
    Q_OBJECT

public:
    FakeVimPluginPrivate();

    void initialize();

    void editorOpened(Core::IEditor *);
    void editorAboutToClose(Core::IEditor *);
    void currentEditorAboutToChange(Core::IEditor *);

    void allDocumentsRenamed(const FilePath &oldPath, const FilePath &newPath);
    void documentRenamed(Core::IDocument *document, const FilePath &oldPath, const FilePath &newPath);
    void renameFileNameInEditors(const FilePath &oldPath, const FilePath &newPath);

    void setUseFakeVim(bool on);
    void setUseFakeVimInternal(bool on);
    void quitFakeVim();
    void fold(FakeVimHandler *handler, int depth, bool fold);
    void maybeReadVimRc();
    void setShowRelativeLineNumbers(bool on);
    void setCursorBlinking(bool on);

    void resetCommandBuffer();
    void showCommandBuffer(FakeVimHandler *handler, const QString &contents,
                           int cursorPos, int anchorPos, int messageLevel);
    void handleExCommand(FakeVimHandler *handler, bool *handled, const ExCommand &cmd);

    void readSettings();

    void handleDelayedQuitAll(bool forced);
    void handleDelayedQuit(bool forced, Core::IEditor *editor);
    void userActionTriggered(int key);

    void updateAllHightLights();

    void switchToFile(int n);
    int currentFile() const;

    void createRelativeNumberWidget(IEditor *editor);

signals:
    void delayedQuitRequested(bool forced, Core::IEditor *editor);
    void delayedQuitAllRequested(bool forced);

public:
    struct HandlerAndData
    {
#ifdef Q_OS_WIN
        // We need to declare a constructor here, otherwise the MSVC 17.5.x compiler fails to parse
        // the "{nullptr}" initializer in the definition below.
        // This seems to be a compiler bug,
        // see: https://developercommunity.visualstudio.com/t/10351118
        HandlerAndData()
            : handler(nullptr)
        {}
#endif
        FakeVimHandler *handler{nullptr};
        TextEditorWidget::SuggestionBlocker suggestionBlocker;
    };

    QHash<IEditor *, HandlerAndData> m_editorToHandler;

    void setActionChecked(Id id, bool check);

    using DistFunction = int (*)(const QRect &, const QRect &);
    void moveSomewhere(FakeVimHandler *handler, DistFunction f, int count);

    void keepOnlyWindow(); // :only

    ExCommandMap m_exCommandMap;
    ExCommandMap m_defaultExCommandMap;

    UserCommandMap m_userCommandMap;
    UserCommandMap m_defaultUserCommandMap;

    MiniBuffer *m_miniBuffer = nullptr;
    FakeVimPluginRunData *runData = nullptr;

    QString m_lastHighlight;

    int m_savedCursorFlashTime = 0;
};

///////////////////////////////////////////////////////////////////////
//
// FakeVimExCommandsPage
//
///////////////////////////////////////////////////////////////////////

enum { CommandRole = Qt::UserRole };

const char exCommandMapGroup[] = "FakeVimExCommand";
const char userCommandMapGroup[] = "FakeVimUserCommand";
const char reKey[] = "RegEx";
const char cmdKey[] = "Cmd";
const char idKey[] = "Command";

class FakeVimExCommandsMappings : public CommandMappings
{
public:
    FakeVimExCommandsMappings();
    void apply();

protected:
    ExCommandMap exCommandMapFromWidget();

    void commandChanged();
    void resetToDefault();
    void defaultAction() override;

    void handleCurrentCommandChanged(QTreeWidgetItem *current);

private:
    QGroupBox *m_commandBox;
    FancyLineEdit *m_commandEdit;
};

FakeVimExCommandsMappings::FakeVimExCommandsMappings()
{
    setPageTitle(Tr::tr("Ex Command Mapping"));
    setTargetHeader(Tr::tr("Ex Trigger Expression"));
    setImportExportEnabled(false);

    connect(this, &FakeVimExCommandsMappings::currentCommandChanged,
            this, &FakeVimExCommandsMappings::handleCurrentCommandChanged);

    m_commandBox = new QGroupBox(Tr::tr("Ex Command"), this);
    m_commandBox->setEnabled(false);
    auto commandBoxLayout = new QVBoxLayout(m_commandBox);
    auto boxLayout = new QHBoxLayout;
    commandBoxLayout->addLayout(boxLayout);
    m_commandEdit = new FancyLineEdit(m_commandBox);
    m_commandEdit->setFiltering(true);
    m_commandEdit->setPlaceholderText(QString());
    connect(m_commandEdit, &FancyLineEdit::textChanged,
            this, &FakeVimExCommandsMappings::commandChanged);
    m_commandEdit->setValidationFunction([](FancyLineEdit *e, QString *){
        return QRegularExpression(e->text()).isValid();
    });
    auto resetButton = new QPushButton(Tr::tr("Reset"), m_commandBox);
    resetButton->setToolTip(Tr::tr("Reset to default."));
    connect(resetButton, &QPushButton::clicked,
            this, &FakeVimExCommandsMappings::resetToDefault);
    boxLayout->addWidget(new QLabel(Tr::tr("Regular expression:")));
    boxLayout->addWidget(m_commandEdit);
    boxLayout->addWidget(resetButton);

    auto infoLabel = new InfoLabel(Tr::tr("Invalid regular expression."), InfoLabel::Error);
    infoLabel->setVisible(false);
    connect(m_commandEdit, &FancyLineEdit::validChanged, this, [infoLabel](bool valid){
        infoLabel->setVisible(!valid);
    });
    commandBoxLayout->addWidget(infoLabel);
    layout()->addWidget(m_commandBox);

    QMap<QString, QTreeWidgetItem *> sections;

    const QList<Command *> commands = ActionManager::commands();
    for (Command *c : commands) {
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
        if (dd->m_exCommandMap.contains(name))
            regex = dd->m_exCommandMap[name].pattern();
        item->setText(2, regex);

        if (regex != dd->m_defaultExCommandMap[name].pattern())
            setModified(item, true);
    }

    handleCurrentCommandChanged(nullptr);
}

ExCommandMap FakeVimExCommandsMappings::exCommandMapFromWidget()
{
    ExCommandMap map;
    int n = commandList()->topLevelItemCount();
    for (int i = 0; i != n; ++i) {
        QTreeWidgetItem *section = commandList()->topLevelItem(i);
        int m = section->childCount();
        for (int j = 0; j != m; ++j) {
            QTreeWidgetItem *item = section->child(j);
            const QString name = item->data(0, CommandRole).toString();
            const QString regex = item->data(2, Qt::DisplayRole).toString();
            const QString pattern = dd->m_defaultExCommandMap.value(name).pattern();
            if ((regex.isEmpty() && pattern.isEmpty())
                || (!regex.isEmpty() && pattern == regex))
                continue;
            const QRegularExpression expression(regex);
            if (expression.isValid())
                map[name] = expression;
        }
    }
    return map;
}

void FakeVimExCommandsMappings::handleCurrentCommandChanged(QTreeWidgetItem *current)
{
    if (current) {
        m_commandEdit->setText(current->text(2));
        m_commandBox->setEnabled(true);
    } else {
        m_commandEdit->clear();
        m_commandBox->setEnabled(false);
    }
}

void FakeVimExCommandsMappings::commandChanged()
{
    QTreeWidgetItem *current = commandList()->currentItem();
    if (!current)
        return;

    const QString name =  current->data(0, CommandRole).toString();
    const QString regex = m_commandEdit->text();

    if (current->data(0, Qt::UserRole).isValid())
        current->setText(2, regex);

    setModified(current, regex != dd->m_defaultExCommandMap[name].pattern());
}

void FakeVimExCommandsMappings::resetToDefault()
{
    QTreeWidgetItem *current = commandList()->currentItem();
    if (!current)
        return;
    const QString name = current->data(0, CommandRole).toString();
    QString regex;
    if (dd->m_defaultExCommandMap.contains(name))
        regex = dd->m_defaultExCommandMap[name].pattern();
    m_commandEdit->setText(regex);
}

void FakeVimExCommandsMappings::defaultAction()
{
    const int n = commandList()->topLevelItemCount();
    for (int i = 0; i != n; ++i) {
        QTreeWidgetItem *section = commandList()->topLevelItem(i);
        const int m = section->childCount();
        for (int j = 0; j != m; ++j) {
            QTreeWidgetItem *item = section->child(j);
            const QString name = item->data(0, CommandRole).toString();
            QString regex;
            if (dd->m_defaultExCommandMap.contains(name))
                regex = dd->m_defaultExCommandMap[name].pattern();
            setModified(item, false);
            item->setText(2, regex);
            if (item == commandList()->currentItem())
                emit currentCommandChanged(item);
        }
    }
}

void FakeVimExCommandsMappings::apply()
{
    // now save the mappings if necessary
    const ExCommandMap &newMapping = exCommandMapFromWidget();
    ExCommandMap &globalCommandMapping = dd->m_exCommandMap;

    if (newMapping != globalCommandMapping) {
        const ExCommandMap &defaultMap = dd->m_defaultExCommandMap;
        QtcSettings *settings = ICore::settings();
        settings->beginWriteArray(exCommandMapGroup);
        int count = 0;
        using Iterator = ExCommandMap::const_iterator;
        const Iterator end = newMapping.constEnd();
        for (Iterator it = newMapping.constBegin(); it != end; ++it) {
            const QString id = it.key();
            const QRegularExpression re = it.value();

            if ((defaultMap.contains(id) && defaultMap[id] != re)
                || (!defaultMap.contains(id) && !re.pattern().isEmpty())) {
                settings->setArrayIndex(count);
                settings->setValue(idKey, id);
                settings->setValue(reKey, re.pattern());
                ++count;
            }
        }
        settings->endArray();
        globalCommandMapping.clear();
        globalCommandMapping.insert(defaultMap);
        globalCommandMapping.insert(newMapping);
    }
}

class FakeVimExCommandsPageWidget : public IOptionsPageWidget
{
public:
    FakeVimExCommandsPageWidget()
    {
        auto exCommands = new FakeVimExCommandsMappings;
        setOnApply([exCommands] { exCommands->apply(); });
        Layouting::Column { exCommands, Layouting::noMargin }.attachTo(this);
    }
};

class FakeVimExCommandsPage : public IOptionsPage
{
public:
    FakeVimExCommandsPage()
    {
        setId(SETTINGS_EX_CMDS_ID);
        setDisplayName(Tr::tr("Ex Command Mapping"));
        setCategory(SETTINGS_CATEGORY);
        setWidgetCreator([] { return new FakeVimExCommandsPageWidget; });
    }
};

const FakeVimExCommandsPage exCommandPage;

///////////////////////////////////////////////////////////////////////
//
// FakeVimUserCommandsPage
//
///////////////////////////////////////////////////////////////////////

class FakeVimUserCommandsModel : public QAbstractTableModel
{
public:
    FakeVimUserCommandsModel() { m_commandMap = dd->m_userCommandMap; }

    UserCommandMap commandMap() const { return m_commandMap; }
    int rowCount(const QModelIndex &parent) const final { return parent.isValid() ? 0 : 9; }
    int columnCount(const QModelIndex &parent) const final { return parent.isValid() ? 0 : 2; }
    QVariant data(const QModelIndex &index, int role) const final;
    bool setData(const QModelIndex &index, const QVariant &data, int role) final;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const final;
    Qt::ItemFlags flags(const QModelIndex &index) const final;

private:
    UserCommandMap m_commandMap;
};

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
        const QModelIndex &) const override
    {
        auto lineEdit = new QLineEdit(parent);
        lineEdit->setFrame(false);
        return lineEdit;
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override
    {
        auto lineEdit = qobject_cast<QLineEdit *>(editor);
        QTC_ASSERT(lineEdit, return);
        model->setData(index, lineEdit->text(), Qt::EditRole);
    }
};

class FakeVimUserCommandsPageWidget : public IOptionsPageWidget
{
public:
    FakeVimUserCommandsPageWidget()
    {
        auto widget = new QTreeView;
        widget->setModel(&m_model);
        widget->resizeColumnToContents(0);

        auto delegate = new FakeVimUserCommandsDelegate(widget);
        widget->setItemDelegateForColumn(1, delegate);

        auto layout = new QGridLayout(this);
        layout->addWidget(widget, 0, 0);
        setLayout(layout);
    }

private:
    void apply() final
    {
        // now save the mappings if necessary
        const UserCommandMap &current = m_model.commandMap();
        UserCommandMap &userMap = dd->m_userCommandMap;

        if (current != userMap) {
            QtcSettings *settings = ICore::settings();
            settings->beginWriteArray(userCommandMapGroup);
            int count = 0;
            using Iterator = UserCommandMap::const_iterator;
            const Iterator end = current.constEnd();
            for (Iterator it = current.constBegin(); it != end; ++it) {
                const int key = it.key();
                const QString cmd = it.value();

                if ((dd->m_defaultUserCommandMap.contains(key)
                     && dd->m_defaultUserCommandMap[key] != cmd)
                        || (!dd->m_defaultUserCommandMap.contains(key) && !cmd.isEmpty())) {
                    settings->setArrayIndex(count);
                    settings->setValue(idKey, key);
                    settings->setValue(cmdKey, cmd);
                    ++count;
                }
            }
            settings->endArray();
            userMap.clear();
            userMap.insert(dd->m_defaultUserCommandMap);
            userMap.insert(current);
        }
    }

    FakeVimUserCommandsModel m_model;
};

class FakeVimUserCommandsPage : public IOptionsPage
{
public:
    FakeVimUserCommandsPage()
    {
        setId(SETTINGS_USER_CMDS_ID);
        setDisplayName(Tr::tr("User Command Mapping"));
        setCategory(SETTINGS_CATEGORY);
        setWidgetCreator([] { return new FakeVimUserCommandsPageWidget; });
    }
};

const FakeVimUserCommandsPage userCommandsPage;

///////////////////////////////////////////////////////////////////////
//
// WordCompletion
//
///////////////////////////////////////////////////////////////////////

class FakeVimCompletionAssistProvider : public CompletionAssistProvider
{
public:
    IAssistProcessor *createProcessor(const AssistInterface *) const override;

    void setActive(const QString &needle, bool forward, FakeVimHandler *handler)
    {
        Q_UNUSED(forward)
        m_handler = handler;
        if (!m_handler)
            return;

        auto editor = qobject_cast<TextEditorWidget *>(handler->widget());
        if (!editor)
            return;

        //qDebug() << "ACTIVATE: " << needle << forward;
        m_needle = needle;
        editor->invokeAssist(Completion, this);
    }

    void setInactive()
    {
        m_needle.clear();
        m_handler = nullptr;
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

class FakeVimCompletionAssistProcessor : public AsyncProcessor
{
public:
    FakeVimCompletionAssistProcessor(const IAssistProvider *provider)
        : m_provider(static_cast<const FakeVimCompletionAssistProvider *>(provider))
    {}

    IAssistProposal *performAsync() override
    {
        const QString &needle = m_provider->needle();

        const int basePosition = interface()->position() - needle.size();

        QTextCursor tc(interface()->textDocument());
        tc.setPosition(interface()->position());
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
                    && sel.anchor() != basePosition
                    && Utils::insert(seen, found)) {
                auto item = new FakeVimAssistProposalItem(m_provider);
                item->setText(found);
                items.append(item);
            }
            tc.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor);
        }
        //qDebug() << "COMPLETIONS" << completions->size();

        return new GenericProposal(basePosition,
                                   GenericProposalModelPtr(new FakeVimAssistProposalModel(items)));
    }

private:
    const FakeVimCompletionAssistProvider *m_provider;
};

IAssistProcessor *FakeVimCompletionAssistProvider::createProcessor(const AssistInterface *) const
{
    return new FakeVimCompletionAssistProcessor(this);
}


///////////////////////////////////////////////////////////////////////
//
// FakeVimPluginRunData
//
///////////////////////////////////////////////////////////////////////

class FakeVimPluginRunData
{
public:
    FakeVimCompletionAssistProvider wordProvider;
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
            return m_commandMap.value(index.row() + 1);
        }
    }

    return QVariant();
}

bool FakeVimUserCommandsModel::setData(const QModelIndex &index,
    const QVariant &data, int role)
{
    if (role == Qt::DisplayRole || role == Qt::EditRole)
        if (index.column() == 1)
            m_commandMap[index.row() + 1] = data.toString();
    return true;
}

FakeVimPluginPrivate::FakeVimPluginPrivate()
{
    m_defaultExCommandMap[CppEditor::Constants::SWITCH_HEADER_SOURCE] =
        QRegularExpression("^A$");
    m_defaultExCommandMap["Coreplugin.OutputPane.previtem"] =
        QRegularExpression("^(cN(ext)?|cp(revious)?)!?( (.*))?$");
    m_defaultExCommandMap["Coreplugin.OutputPane.nextitem"] =
        QRegularExpression("^cn(ext)?!?( (.*))?$");
    m_defaultExCommandMap[TextEditor::Constants::FOLLOW_SYMBOL_UNDER_CURSOR] =
        QRegularExpression("^tag?$");
    m_defaultExCommandMap[Core::Constants::GO_BACK] =
        QRegularExpression("^pop?$");
    m_defaultExCommandMap["QtCreator.Locate"] =
        QRegularExpression("^e$");

    for (int i = 1; i < 10; ++i) {
        QString cmd = QString::fromLatin1(":echo User command %1 executed.<CR>");
        m_defaultUserCommandMap.insert(i, cmd.arg(i));
    }
}

void FakeVimPluginPrivate::initialize()
{
    runData = new FakeVimPluginRunData;
/*
    // Set completion settings and keep them up to date.
    TextEditorSettings *textEditorSettings = TextEditorSettings::instance();
    completion->setCompletionSettings(textEditorSettings->completionSettings());
    connect(textEditorSettings, &TextEditorSettings::completionSettingsChanged,
            completion, &TextEditorWidget::setCompletionSettings);
*/
    readSettings();

    // Vimrc can break test so don't source it if running tests.
    if (!ExtensionSystem::PluginManager::testRunRequested())
        maybeReadVimRc();

    Command *cmd = nullptr;

    cmd = ActionManager::registerAction(settings().useFakeVim.action(),
        INSTALL_HANDLER, Context(Core::Constants::C_GLOBAL), true);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+Shift+Y,Meta+Shift+Y")
                                                            : Tr::tr("Alt+Y,Alt+Y")));
    connect(cmd->action(), &QAction::triggered, [] { settings().writeSettings(); });

    ActionContainer *advancedMenu =
        ActionManager::actionContainer(Core::Constants::M_EDIT_ADVANCED);
    advancedMenu->addAction(cmd, Core::Constants::G_EDIT_EDITOR);

    const Id base = "FakeVim.UserAction";
    for (int i = 1; i < 10; ++i) {
        auto act = new QAction(this);
        act->setText(Tr::tr("Execute User Action #%1").arg(i));
        cmd = ActionManager::registerAction(act, base.withSuffix(i));
        cmd->setDefaultKeySequence(QKeySequence((useMacShortcuts ? Tr::tr("Meta+Shift+Y,%1")
                                                                 : Tr::tr("Alt+Y,%1")).arg(i)));
        connect(act, &QAction::triggered, this, [this, i] { userActionTriggered(i); });
    }

    connect(ICore::instance(), &ICore::coreAboutToClose, this, [] {
        // Don't attach to editors anymore.
        disconnect(EditorManager::instance(), &EditorManager::editorOpened,
                   dd, &FakeVimPluginPrivate::editorOpened);
    });

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

    FakeVimSettings &s = settings();
    connect(&s.useFakeVim, &FvBoolAspect::changed,
            this, [this, &s] { setUseFakeVim(s.useFakeVim()); });
    connect(&s.readVimRc, &FvBaseAspect::changed,
            this, &FakeVimPluginPrivate::maybeReadVimRc);
    connect(&s.vimRcPath, &FvBaseAspect::changed,
            this, &FakeVimPluginPrivate::maybeReadVimRc);
    connect(&s.relativeNumber, &FvBoolAspect::changed,
            this, [this, &s] { setShowRelativeLineNumbers(s.relativeNumber()); });
    connect(&s.blinkingCursor, &FvBoolAspect::changed,
            this, [this, &s] { setCursorBlinking(s.blinkingCursor()); });

    // Delayed operations.
    connect(this, &FakeVimPluginPrivate::delayedQuitRequested,
            this, &FakeVimPluginPrivate::handleDelayedQuit, Qt::QueuedConnection);
    connect(this, &FakeVimPluginPrivate::delayedQuitAllRequested,
            this, &FakeVimPluginPrivate::handleDelayedQuitAll, Qt::QueuedConnection);

    setCursorBlinking(s.blinkingCursor());
}

void FakeVimPluginPrivate::userActionTriggered(int key)
{
    IEditor *editor = EditorManager::currentEditor();
    FakeVimHandler *handler = m_editorToHandler[editor].handler;
    if (handler) {
        // If disabled, enable FakeVim mode just for single user command.
        bool enableFakeVim = !settings().useFakeVim();
        if (enableFakeVim)
            setUseFakeVimInternal(true);

        const QString cmd = m_userCommandMap.value(key);
        handler->handleInput(cmd);

        if (enableFakeVim)
            setUseFakeVimInternal(false);
    }
}

void FakeVimPluginPrivate::updateAllHightLights()
{
    const QList<IEditor *> editors = EditorManager::visibleEditors();
    for (IEditor *editor : editors) {
        QWidget *w = editor->widget();
        if (auto find = Aggregation::query<IFindSupport>(w))
            find->highlightAll(m_lastHighlight, FindRegularExpression | FindCaseSensitively);
    }
}

void FakeVimPluginPrivate::createRelativeNumberWidget(IEditor *editor)
{
    if (auto textEditor = TextEditorWidget::fromEditor(editor)) {
        auto relativeNumbers = new RelativeNumbersColumn(textEditor);
        connect(&settings().relativeNumber, &FvBaseAspect::changed,
                relativeNumbers, &QObject::deleteLater);
        connect(&settings().useFakeVim, &FvBaseAspect::changed,
                relativeNumbers, &QObject::deleteLater);
        relativeNumbers->show();
    }
}

void FakeVimPluginPrivate::readSettings()
{
    QtcSettings *settings = ICore::settings();

    m_exCommandMap = m_defaultExCommandMap;
    int size = settings->beginReadArray(exCommandMapGroup);
    for (int i = 0; i < size; ++i) {
        settings->setArrayIndex(i);
        const QString id = settings->value(idKey).toString();
        const QString re = settings->value(reKey).toString();
        const QRegularExpression regEx(re);
        if (regEx.isValid())
            m_exCommandMap[id] = regEx;
    }
    settings->endArray();

    m_userCommandMap = m_defaultUserCommandMap;
    size = settings->beginReadArray(userCommandMapGroup);
    for (int i = 0; i < size; ++i) {
        settings->setArrayIndex(i);
        const int id = settings->value(idKey).toInt();
        const QString cmd = settings->value(cmdKey).toString();
        m_userCommandMap[id] = cmd;
    }
    settings->endArray();
}

void FakeVimPluginPrivate::maybeReadVimRc()
{
    //qDebug() << theFakeVimSetting(ConfigReadVimRc)
    //    << theFakeVimSetting(ConfigReadVimRc)->value();
    //qDebug() << theFakeVimSetting(ConfigShiftWidth)->value();
    if (!settings().readVimRc())
        return;
    QString fileName = settings().vimRcPath().path();
    if (fileName.isEmpty()) {
        fileName = QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
            + QLatin1String(HostOsInfo::isWindowsHost() ? "/_vimrc" : "/.vimrc");
    }
    //qDebug() << "READING VIMRC: " << fileName;
    // Read it into a temporary handler for effects modifying global state.
    QPlainTextEdit editor;
    FakeVimHandler handler(&editor);
    handler.handleCommand("source " + fileName);
    //qDebug() << theFakeVimSetting(ConfigShiftWidth)->value();
}

static void triggerAction(Id id)
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

void FakeVimPluginPrivate::moveSomewhere(FakeVimHandler *handler, DistFunction f, int count)
{
    QTC_ASSERT(handler, return);
    QWidget *w = handler->widget();
    auto pe = qobject_cast<QPlainTextEdit *>(w);
    QTC_ASSERT(pe, return);
    QRect rc = pe->cursorRect();
    QRect cursorRect(w->mapToGlobal(rc.topLeft()), w->mapToGlobal(rc.bottomRight()));
    //qDebug() << "\nCURSOR: " << cursorRect;

    IEditor *bestEditor = nullptr;
    int repeat = count;

    IEditor *currentEditor = EditorManager::currentEditor();
    QList<IEditor *> editors = EditorManager::visibleEditors();
    while (repeat < 0 || repeat-- > 0) {
        editors.removeOne(currentEditor);
        int bestValue = -1;
        for (IEditor *editor : std::as_const(editors)) {
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

    for (IEditor *editor : std::as_const(editors)) {
        EditorManager::activateEditor(editor);
        triggerAction(Core::Constants::REMOVE_CURRENT_SPLIT);
    }
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

// This class defers deletion of a child FakeVimHandler using deleteLater().
class DeferredDeleter : public QObject
{
    Q_OBJECT

    FakeVimHandler *m_handler;

public:
    DeferredDeleter(QObject *parent, FakeVimHandler *handler)
        : QObject(parent), m_handler(handler)
    {}

    ~DeferredDeleter() override
    {
        if (m_handler) {
            m_handler->disconnectFromEditor();
            m_handler->deleteLater();
            m_handler = nullptr;
        }
    }
};

void FakeVimPluginPrivate::editorOpened(IEditor *editor)
{
    if (!editor)
        return;

    if (m_editorToHandler.contains(editor)) {
        // We get here via the call from the duplicated handler in case
        // it was triggered by triggerAction(Core::Constants::SPLIT).
        // On the other hand, we need the path from there to support the
        // case of manual calls to IEditor::duplicate().
        return;
    }

    QWidget *widget = editor->widget();
    if (!widget)
        return;

    // we can only handle QTextEdit and QPlainTextEdit
    if (auto edit = Aggregation::query<QTextEdit>(widget))
        widget = edit;
    else if (auto edit = Aggregation::query<QPlainTextEdit>(widget))
        widget = edit;
    else
        return;

    // Duplicated editors are not signalled by the EditorManager. Track them nevertheless.
    connect(editor, &IEditor::editorDuplicated, this, [this](IEditor *duplicate) {
        editorOpened(duplicate);
        connect(duplicate, &QObject::destroyed, this, [this, duplicate] {
            m_editorToHandler.remove(duplicate);
        });
    });

    auto tew = TextEditorWidget::fromEditor(editor);

    //qDebug() << "OPENING: " << editor << editor->widget()
    //    << "MODE: " << theFakeVimSetting(ConfigUseFakeVim)->value();

    auto handler = new FakeVimHandler(widget, nullptr);
    // the handler might have triggered the deletion of the editor:
    // make sure that it can return before being deleted itself
    new DeferredDeleter(widget, handler);
    m_editorToHandler[editor].handler = handler;

    handler->extraInformationChanged.set([this](const QString &text) {
        EditorManager::splitSideBySide();
        QString title = "stdout.txt";
        IEditor *iedit = EditorManager::openEditorWithContents(Id(), &title, text.toUtf8());
        EditorManager::activateEditor(iedit);
        FakeVimHandler *handler = m_editorToHandler.value(iedit, {}).handler;
        QTC_ASSERT(handler, return);
        handler->handleCommand("0");
    });

    handler->commandBufferChanged.set(
        [this, handler](const QString &contents, int cursorPos, int anchorPos, int messageLevel) {
            showCommandBuffer(handler, contents, cursorPos, anchorPos, messageLevel);
        });

    handler->selectionChanged.set([tew](const QList<QTextEdit::ExtraSelection> &selection) {
        if (tew)
            tew->setExtraSelections(TextEditorWidget::FakeVimSelection, selection);
    });

    handler->tabPressedInInsertMode.set([tew]() {
        auto suggestion = tew->currentSuggestion();
        if (suggestion) {
            suggestion->apply();
            return false;
        }

        return true;
    });

    handler->modeChanged.set([tew, this, editor](bool insertMode) {
        HandlerAndData &handlerAndData = m_editorToHandler[editor];
        if (!handlerAndData.handler->inFakeVimMode())
            return;

        // We don't want to show suggestions unless we are in insert mode.
        if (insertMode != (handlerAndData.suggestionBlocker == nullptr))
            handlerAndData.suggestionBlocker = insertMode ? nullptr : tew->blockSuggestions();

        if (tew)
            tew->clearSuggestion();
    });

    handler->highlightMatches.set([this](const QString &needle) {
        m_lastHighlight = needle;
        for (IEditor *editor : EditorManager::visibleEditors()) {
            QWidget *w = editor->widget();
            if (auto find = Aggregation::query<IFindSupport>(w))
                find->highlightAll(needle, FindRegularExpression | FindCaseSensitively);
        }
    });

    handler->moveToMatchingParenthesis.set([](bool *moved, bool *forward, QTextCursor *cursor) {
        *moved = false;

        bool undoFakeEOL = false;
        if (cursor->atBlockEnd() && cursor->block().length() > 1) {
            cursor->movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
            undoFakeEOL = true;
        }
        TextBlockUserData::MatchType match = TextBlockUserData::matchCursorForward(cursor);
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
    });

    handler->indentRegion.set([tew](int beginBlock, int endBlock, QChar typedChar) {
        if (!tew)
            return;

        TabSettings tabSettings;
        tabSettings.m_indentSize = settings().shiftWidth();
        tabSettings.m_tabSize = settings().tabStop();
        tabSettings.m_tabPolicy = settings().expandTab()
                ? TabSettings::SpacesOnlyTabPolicy : TabSettings::TabsOnlyTabPolicy;
        tabSettings.m_continuationAlignBehavior =
                tew->textDocument()->tabSettings().m_continuationAlignBehavior;

        QTextDocument *doc = tew->document();
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
                tew->textDocument()->indenter()->indentBlock(block, typedChar, tabSettings);
            }
            block = block.next();
        }
    });

    handler->checkForElectricCharacter.set([tew](bool *result, QChar c) {
        if (tew)
            *result = tew->textDocument()->indenter()->isElectricCharacter(c);
    });

    handler->requestDisableBlockSelection.set([tew] {
        if (tew)
            tew->setTextCursor(tew->textCursor());
    });

    handler->requestSetBlockSelection.set([tew](const QTextCursor &cursor) {
        if (tew) {
            const TabSettings &tabs = tew->textDocument()->tabSettings();
            MultiTextCursor mtc;
            const bool forwardSelection = cursor.anchor() < cursor.position();
            QTextBlock block = cursor.document()->findBlock(cursor.anchor());
            const QTextBlock end = forwardSelection ? cursor.block().next() : cursor.block().previous();
            const int anchor = tabs.columnAt(block.text(), cursor.anchor() - block.position());
            const int pos = tabs.columnAt(cursor.block().text(), cursor.positionInBlock());
            while (block.isValid() && block != end) {
                const int columns = tabs.columnCountForText(block.text());
                if (columns >= anchor || columns >= pos) {
                    QTextCursor c(block);
                    c.setPosition(block.position() + tabs.positionAtColumn(block.text(), anchor));
                    c.setPosition(block.position() + tabs.positionAtColumn(block.text(), pos),
                                  QTextCursor::KeepAnchor);
                    mtc.addCursor(c);
                }
                block = forwardSelection ? block.next() : block.previous();
            }
            tew->setMultiTextCursor(mtc);
        }
    });

    handler->requestBlockSelection.set([tew](QTextCursor *cursor) {
        if (tew && cursor) {
            MultiTextCursor mtc = tew->multiTextCursor();
            *cursor = mtc.cursors().first();
            cursor->setPosition(mtc.mainCursor().position(), QTextCursor::KeepAnchor);
        }
    });

    handler->requestHasBlockSelection.set([tew](bool *on) {
        if (tew && on)
            *on = tew->multiTextCursor().hasMultipleCursors();
    });

    handler->simpleCompletionRequested.set([this, handler](const QString &needle, bool forward) {
        runData->wordProvider.setActive(needle, forward, handler);
    });

    handler->windowCommandRequested.set([this, handler](const QString &map, int count) {
        // normalize mapping
        const QString key = map.toUpper();

        if (key == "C" || key == "<C-C>")
            triggerAction(Core::Constants::REMOVE_CURRENT_SPLIT);
        else if (key == "N" || key == "<C-N>")
            triggerAction(Core::Constants::GOTO_NEXT_SPLIT);
        else if (key == "O" || key == "<C-O>")
            keepOnlyWindow();
        else if (key == "P" || key == "<C-P>")
            triggerAction(Core::Constants::GOTO_PREV_SPLIT);
        else if (key == "S" || key == "<C-S>")
            triggerAction(Core::Constants::SPLIT);
        else if (key == "V" || key == "<C-V>")
            triggerAction(Core::Constants::SPLIT_SIDE_BY_SIDE);
        else if (key == "W" || key == "<C-W>")
            triggerAction(Core::Constants::GOTO_NEXT_SPLIT);
        else if (key.contains("RIGHT") || key == "L" || key == "<S-L>" || key == "<C-L>")
            moveSomewhere(handler, &moveRightWeight, key == "<S-L>" ? -1 : count);
        else if (key.contains("LEFT")  || key == "H" || key == "<S-H>" || key == "<C-H>")
            moveSomewhere(handler, &moveLeftWeight, key == "<S-H>" ? -1 : count);
        else if (key.contains("UP")    || key == "K" || key == "<S-K>" || key == "<C-K>")
            moveSomewhere(handler, &moveUpWeight, key == "<S-K>" ? -1 : count);
        else if (key.contains("DOWN")  || key == "J" || key == "<S-J>" || key == "<C-J>")
            moveSomewhere(handler, &moveDownWeight, key == "<S-J>" ? -1 : count);
        else
            qDebug() << "UNKNOWN WINDOW COMMAND: <C-W>" << map;
    });

    handler->findRequested.set([](bool reverse) {
        Find::setUseFakeVim(true);
        Find::openFindToolBar(reverse ? Find::FindBackwardDirection
                                      : Find::FindForwardDirection);
    });

    handler->findNextRequested.set([](bool reverse) {
        triggerAction(reverse ? Core::Constants::FIND_PREVIOUS : Core::Constants::FIND_NEXT);
    });

    handler->foldToggle.set([this, handler](int depth) {
        QTextBlock block = handler->textCursor().block();
        fold(handler, depth, !TextDocumentLayout::isFolded(block));
    });

    handler->foldAll.set([handler](bool fold) {
        QTextDocument *document = handler->textCursor().document();
        auto documentLayout = qobject_cast<TextDocumentLayout*>(document->documentLayout());
        QTC_ASSERT(documentLayout, return);

        QTextBlock block = document->firstBlock();
        while (block.isValid()) {
            TextDocumentLayout::doFoldOrUnfold(block, !fold);
            block = block.next();
        }

        documentLayout->requestUpdate();
        documentLayout->emitDocumentSizeChanged();
    });

    handler->fold.set([this, handler](int depth, bool dofold) { fold(handler, depth, dofold); });

    handler->foldGoTo.set([handler](int count, bool current) {
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
    });

    handler->requestJumpToGlobalMark.set(
        [this](QChar mark, bool backTickMode, const QString &fileName) {
            if (IEditor *iedit = EditorManager::openEditor(FilePath::fromString(fileName))) {
                if (FakeVimHandler *handler = m_editorToHandler.value(iedit, {}).handler)
                    handler->jumpToLocalMark(mark, backTickMode);
            }
        });

    handler->handleExCommandRequested.set([this, handler](bool *handled, const ExCommand &cmd) {
        handleExCommand(handler, handled, cmd);
    });

    handler->tabNextRequested.set([] { triggerAction(Core::Constants::GOTONEXTINHISTORY); });

    handler->tabPreviousRequested.set([] { triggerAction(Core::Constants::GOTOPREVINHISTORY); });

    handler->completionRequested.set([this, tew] {
        if (tew)
            tew->invokeAssist(Completion, &runData->wordProvider);
    });

    handler->processOutput.set([](const QString &command, const QString &input, QString *output) {
        Process proc;
        proc.setCommand(Utils::CommandLine::fromUserInput(command));
        proc.setWriteData(input.toLocal8Bit());
        proc.start();

        // FIXME: Process should be interruptable by user.
        //        Solution is to create a QObject for each process and emit finished state.
        proc.waitForFinished();
        *output = proc.cleanedStdOut();
    });

    handler->setCurrentFileName(editor->document()->filePath().toString());
    handler->installEventFilter();

    // pop up the bar
    if (settings().useFakeVim()) {
       resetCommandBuffer();
       handler->setupWidget();

       if (settings().relativeNumber())
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
    if (FakeVimHandler *handler = m_editorToHandler.value(editor, {}).handler)
        handler->enterCommandMode();
}

void FakeVimPluginPrivate::allDocumentsRenamed(const FilePath &oldPath, const FilePath &newPath)
{
    renameFileNameInEditors(oldPath, newPath);
    FakeVimHandler::updateGlobalMarksFilenames(oldPath.toString(), newPath.toString());
}

void FakeVimPluginPrivate::documentRenamed(
        IDocument *, const FilePath &oldPath, const FilePath &newPath)
{
    renameFileNameInEditors(oldPath, newPath);
}

void FakeVimPluginPrivate::renameFileNameInEditors(const FilePath &oldPath, const FilePath &newPath)
{
    for (const HandlerAndData &handlerAndData : m_editorToHandler) {
        if (handlerAndData.handler->currentFileName() == oldPath.toString())
            handlerAndData.handler->setCurrentFileName(newPath.toString());
    }
}

void FakeVimPluginPrivate::setUseFakeVim(bool on)
{
    //qDebug() << "SET USE FAKEVIM" << on;
    Find::setUseFakeVim(on);
    setUseFakeVimInternal(on);
    setShowRelativeLineNumbers(settings().relativeNumber());
    setCursorBlinking(settings().blinkingCursor());
}

void FakeVimPluginPrivate::setUseFakeVimInternal(bool on)
{
    if (on) {
        //ICore *core = ICore::instance();
        //core->updateAdditionalContexts(Context(FAKEVIM_CONTEXT),
        // Context());
        for (const HandlerAndData &handlerAndData : m_editorToHandler)
            handlerAndData.handler->setupWidget();
    } else {
        //ICore *core = ICore::instance();
        //core->updateAdditionalContexts(Context(),
        // Context(FAKEVIM_CONTEXT));
        resetCommandBuffer();
        for (auto it = m_editorToHandler.begin(); it != m_editorToHandler.end(); ++it) {
            if (auto textDocument = qobject_cast<const TextDocument *>(it.key()->document())) {
                HandlerAndData &handlerAndData = it.value();
                handlerAndData.handler->restoreWidget(textDocument->tabSettings().m_tabSize);
                handlerAndData.suggestionBlocker.reset();
            }
        }
    }
}

void FakeVimPluginPrivate::setShowRelativeLineNumbers(bool on)
{
    if (on && settings().useFakeVim()) {
        for (auto it = m_editorToHandler.constBegin(); it != m_editorToHandler.constEnd(); ++it)
            createRelativeNumberWidget(it.key());
    }
}

void FakeVimPluginPrivate::setCursorBlinking(bool on)
{
    if (m_savedCursorFlashTime == 0)
        m_savedCursorFlashTime = QGuiApplication::styleHints()->cursorFlashTime();

    const bool blink = on || !settings().useFakeVim();
    QGuiApplication::styleHints()->setCursorFlashTime(blink ? m_savedCursorFlashTime : 0);
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

    auto editorFromHandler = [this, handler]() -> Core::IEditor * {
        auto itEditor = std::find_if(m_editorToHandler.cbegin(),
                                     m_editorToHandler.cend(),
                                     [handler](const HandlerAndData &handlerAndData) {
                                         return handlerAndData.handler == handler;
                                     });
        if (itEditor != m_editorToHandler.cend())
            return itEditor.key();
        return nullptr;
    };

    *handled = true;
    if ((cmd.matches("w", "write") || cmd.cmd == "wq") && cmd.args.isEmpty()) {
        // :w[rite]
        bool saved = false;
        IEditor *editor = editorFromHandler();
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
                        emit delayedQuitRequested(cmd.hasBang, editor);
                }
            }
        }

        if (!saved)
            handler->showMessage(MessageError, Tr::tr("File not saved"));
    } else if (cmd.matches("wa", "wall") || cmd.matches("wqa", "wqall")) {
        // :wa[ll] :wqa[ll]
        triggerAction(Core::Constants::SAVEALL);
        const QList<IDocument *> failed = DocumentManager::modifiedDocuments();
        if (failed.isEmpty())
            handler->showMessage(MessageInfo, Tr::tr("Saving succeeded"));
        else
            handler->showMessage(MessageError, Tr::tr("%n files not saved", nullptr, failed.size()));
        if (cmd.matches("wqa", "wqall"))
            emit delayedQuitAllRequested(cmd.hasBang);
    } else if (cmd.matches("q", "quit")) {
        // :q[uit]
        emit delayedQuitRequested(cmd.hasBang, editorFromHandler());
    } else if (cmd.matches("qa", "qall")) {
        // :qa[ll]
        emit delayedQuitAllRequested(cmd.hasBang);
    } else if (cmd.matches("sp", "split")) {
        // :sp[lit]
        triggerAction(Core::Constants::SPLIT);
        updateAllHightLights();
    } else if (cmd.matches("vs", "vsplit")) {
        // :vs[plit]
        triggerAction(Core::Constants::SPLIT_SIDE_BY_SIDE);
        updateAllHightLights();
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
        triggerAction(CppEditor::Constants::SWITCH_HEADER_SOURCE);
    } else if (cmd.cmd == "AV") {
        triggerAction(Core::Constants::SPLIT_SIDE_BY_SIDE);
        triggerAction(CppEditor::Constants::SWITCH_HEADER_SOURCE);
    } else {
        // Check whether one of the configure commands matches.
        const auto end = m_exCommandMap.constEnd();
        for (auto it = m_exCommandMap.constBegin(); it != end; ++it) {
            const QString &id = it.key();
            QRegularExpression re = it.value();
            if (!re.pattern().isEmpty() && re.match(cmd.cmd).hasMatch()) {
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
        EditorManager::closeEditors({editor}, !forced);
}

void FakeVimPluginPrivate::handleDelayedQuitAll(bool forced)
{
    triggerAction(Core::Constants::REMOVE_ALL_SPLITS);
    EditorManager::closeAllEditors(!forced);
}

void FakeVimPluginPrivate::quitFakeVim()
{
    settings().useFakeVim.setValue(false);
}

void FakeVimPluginPrivate::resetCommandBuffer()
{
    showCommandBuffer(nullptr, QString(), -1, -1, 0);
}

void FakeVimPluginPrivate::showCommandBuffer(FakeVimHandler *handler, const QString &contents, int cursorPos, int anchorPos,
                                             int messageLevel)
{
    //qDebug() << "SHOW COMMAND BUFFER" << contents;
    QTC_ASSERT(m_miniBuffer, return);
    m_miniBuffer->setContents(contents, cursorPos, anchorPos, messageLevel, handler);
}

int FakeVimPluginPrivate::currentFile() const
{
    IEditor *editor = EditorManager::currentEditor();
    if (editor) {
        const std::optional<int> index = DocumentModel::indexOfDocument(editor->document());
        if (QTC_GUARD(index))
            return index.value();
    }
    return -1;
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


///////////////////////////////////////////////////////////////////////
//
// FakeVimPlugin
//
///////////////////////////////////////////////////////////////////////

FakeVimPlugin::FakeVimPlugin()
{
    dd = new FakeVimPluginPrivate;
}

FakeVimPlugin::~FakeVimPlugin()
{
    delete dd;
    dd = nullptr;
}

void FakeVimPlugin::initialize()
{
    dd->initialize();
}

ExtensionSystem::IPlugin::ShutdownFlag FakeVimPlugin::aboutToShutdown()
{
    delete dd->runData;
    dd->runData = nullptr;

    StatusBarManager::destroyStatusBarWidget(dd->m_miniBuffer);
    dd->m_miniBuffer = nullptr;
    return SynchronousShutdown;
}

void FakeVimPlugin::extensionsInitialized()
{
    dd->m_miniBuffer = new MiniBuffer;
    StatusBarManager::addStatusBarWidget(dd->m_miniBuffer, StatusBarManager::LastLeftAligned);
}

#ifdef WITH_TESTS
void FakeVimPlugin::setupTest(QString *title, FakeVimHandler **handler, QWidget **edit)
{
    *title = QString::fromLatin1("test.cpp");
    IEditor *iedit = EditorManager::openEditorWithContents(Id(), title);
    EditorManager::activateEditor(iedit);
    *edit = iedit->widget();
    *handler = dd->m_editorToHandler.value(iedit, {}).handler;
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
