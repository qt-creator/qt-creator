/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "richtexteditor.h"
#include "ui_richtexteditor.h"
#include "hyperlinkdialog.h"

#include <functional>

#include <QToolButton>
#include <QAction>
#include <QStyle>
#include <QStyleFactory>
#include <QColorDialog>
#include <QWidgetAction>
#include <QTextTable>
#include <QScopeGuard>
#include <QPointer>

#include <utils/stylehelper.h>

namespace QmlDesigner {

template <class T>
class FontWidgetActions : public QWidgetAction {
public:
    FontWidgetActions(QObject *parent = nullptr)
        : QWidgetAction(parent) {}

    ~FontWidgetActions () override {}

    void setInitializer(std::function<void(T*)> func)
    {
        m_initializer = func;
    }

    QList<QWidget *> createdWidgets()
    {
        return QWidgetAction::createdWidgets();
    }

protected:
    QWidget *createWidget(QWidget *parent) override
    {
        T *w = new T(parent);
        if (m_initializer)
            m_initializer(w);
        return w;
    }

    void deleteWidget(QWidget *widget) override
    {
        widget->deleteLater();
    }

private:
    std::function<void(T*)> m_initializer;
};

static void cursorEditBlock(QTextCursor& cursor, std::function<void()> f) {
    cursor.beginEditBlock();
    f();
    cursor.endEditBlock();
}

RichTextEditor::RichTextEditor(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RichTextEditor)
    , m_linkDialog(new HyperlinkDialog(this))
{
    ui->setupUi(this);
    ui->textEdit->setTextInteractionFlags(Qt::TextEditorInteraction | Qt::LinksAccessibleByMouse);
    ui->tableBar->setVisible(false);

    setupEditActions();
    setupTextActions();
    setupHyperlinkActions();
    setupAlignActions();
    setupListActions();
    setupFontActions();
    setupTableActions();

    connect(ui->textEdit, &QTextEdit::currentCharFormatChanged,
            this, &RichTextEditor::currentCharFormatChanged);
    connect(ui->textEdit, &QTextEdit::cursorPositionChanged,
            this, &RichTextEditor::cursorPositionChanged);
    connect(m_linkDialog, &QDialog::accepted, [this]() {
        QTextCharFormat oldFormat = ui->textEdit->textCursor().charFormat();

        QTextCursor tcursor = ui->textEdit->textCursor();
        QTextCharFormat charFormat = tcursor.charFormat();

        charFormat.setForeground(QApplication::palette().color(QPalette::Link));
        charFormat.setFontUnderline(true);

        QString link = m_linkDialog->getLink();
        QString anchor = m_linkDialog->getAnchor();

        if (anchor.isEmpty())
            anchor = link;

        charFormat.setAnchor(true);
        charFormat.setAnchorHref(link);
        charFormat.setAnchorNames(QStringList(anchor));

        tcursor.insertText(anchor, charFormat);

        tcursor.insertText(" ", oldFormat);

        m_linkDialog->hide();
    });

    ui->textEdit->setFocus();
    m_linkDialog->hide();
}

RichTextEditor::~RichTextEditor()
{
}

void RichTextEditor::setPlainText(const QString &text)
{
    ui->textEdit->setPlainText(text);
}

QString RichTextEditor::plainText() const
{
    return ui->textEdit->toPlainText();
}

void RichTextEditor::setRichText(const QString &text)
{
    ui->textEdit->setHtml(text);
}

void RichTextEditor::setTabChangesFocus(bool change)
{
    ui->textEdit->setTabChangesFocus(change);
}

QIcon RichTextEditor::getIcon(Theme::Icon icon)
{
    const QString fontName = "qtds_propertyIconFont.ttf";

    return Utils::StyleHelper::getIconFromIconFont(fontName, Theme::getIconUnicode(icon), 20, 20);
}

QString RichTextEditor::richText() const
{
    return ui->textEdit->toHtml();
}

void RichTextEditor::currentCharFormatChanged(const QTextCharFormat &format)
{
    fontChanged(format.font());
    colorChanged(format.foreground().color());
}

void RichTextEditor::cursorPositionChanged()
{
    alignmentChanged(ui->textEdit->alignment());
    styleChanged(ui->textEdit->textCursor());
    tableChanged(ui->textEdit->textCursor());
}

void RichTextEditor::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = ui->textEdit->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);
    ui->textEdit->mergeCurrentCharFormat(format);
}

void RichTextEditor::fontChanged(const QFont &f)
{
    for (QWidget* w: m_fontNameAction->createdWidgets() ) {
        QFontComboBox* box = qobject_cast<QFontComboBox*>(w);
        if (box)
            box->setCurrentFont(f);
    }
    for (QWidget* w: m_fontSizeAction->createdWidgets() ) {
        QComboBox* box = qobject_cast<QComboBox*>(w);
        if (box)
            box->setCurrentText(QString::number(f.pointSize()));
    }

    m_actionTextBold->setChecked(f.bold());
    m_actionTextItalic->setChecked(f.italic());
    m_actionTextUnderline->setChecked(f.underline());
}

void RichTextEditor::colorChanged(const QColor &c)
{
    QPixmap colorBox(ui->tableBar->iconSize());
    colorBox.fill(c);
    m_actionTextColor->setIcon(colorBox);
}

void RichTextEditor::alignmentChanged(Qt::Alignment a)
{
    if (a & Qt::AlignLeft)
        m_actionAlignLeft->setChecked(true);
    else if (a & Qt::AlignHCenter)
        m_actionAlignCenter->setChecked(true);
    else if (a & Qt::AlignRight)
        m_actionAlignRight->setChecked(true);
    else if (a & Qt::AlignJustify)
        m_actionAlignJustify->setChecked(true);
}

void RichTextEditor::styleChanged(const QTextCursor &cursor)
{
    if (!m_actionBulletList || !m_actionNumberedList) return;

    QTextList *currentList = cursor.currentList();

    if (currentList) {
        if (currentList->format().style() == QTextListFormat::ListDisc) {
            m_actionBulletList->setChecked(true);
            m_actionNumberedList->setChecked(false);
        }
        else if (currentList->format().style() == QTextListFormat::ListDecimal) {
            m_actionBulletList->setChecked(false);
            m_actionNumberedList->setChecked(true);
        }
        else {
            m_actionBulletList->setChecked(false);
            m_actionNumberedList->setChecked(false);
        }
    }
    else {
        m_actionBulletList->setChecked(false);
        m_actionNumberedList->setChecked(false);
    }
}

void RichTextEditor::tableChanged(const QTextCursor &cursor)
{
    if (!m_actionTableSettings) return;

    QTextTable *currentTable = cursor.currentTable();

    if (currentTable) {
        m_actionTableSettings->setChecked(true);
        ui->tableBar->setVisible(true);

        setTableActionsActive(true);
    }
    else {
        setTableActionsActive(false);
    }
}

void RichTextEditor::setupEditActions()
{
    const QIcon undoIcon(getIcon(Theme::Icon::undo));
    QAction *actionUndo = ui->toolBar->addAction(undoIcon, tr("&Undo"), ui->textEdit, &QTextEdit::undo);
    actionUndo->setShortcut(QKeySequence::Undo);
    connect(ui->textEdit->document(), &QTextDocument::undoAvailable,
            actionUndo, &QAction::setEnabled);

    const QIcon redoIcon(getIcon(Theme::Icon::redo));
    QAction *actionRedo = ui->toolBar->addAction(redoIcon, tr("&Redo"), ui->textEdit, &QTextEdit::redo);
    actionRedo->setShortcut(QKeySequence::Redo);
    connect(ui->textEdit->document(), &QTextDocument::redoAvailable,
            actionRedo, &QAction::setEnabled);

    actionUndo->setEnabled(ui->textEdit->document()->isUndoAvailable());
    actionRedo->setEnabled(ui->textEdit->document()->isRedoAvailable());

    ui->toolBar->addSeparator();
}

void RichTextEditor::setupTextActions()
{
    const QIcon boldIcon(getIcon(Theme::Icon::fontStyleBold));
    m_actionTextBold = ui->toolBar->addAction(boldIcon, tr("&Bold"),
                                                     [this](bool checked) {
        QTextCharFormat fmt;
        fmt.setFontWeight(checked ? QFont::Bold : QFont::Normal);
        mergeFormatOnWordOrSelection(fmt);
    });
    m_actionTextBold->setShortcut(Qt::CTRL + Qt::Key_B);
    QFont bold;
    bold.setBold(true);
    m_actionTextBold->setFont(bold);
    m_actionTextBold->setCheckable(true);

    const QIcon italicIcon(getIcon(Theme::Icon::fontStyleItalic));
    m_actionTextItalic = ui->toolBar->addAction(italicIcon, tr("&Italic"),
                                                       [this](bool checked) {
        QTextCharFormat fmt;
        fmt.setFontItalic(checked);
        mergeFormatOnWordOrSelection(fmt);
    });
    m_actionTextItalic->setShortcut(Qt::CTRL + Qt::Key_I);
    QFont italic;
    italic.setItalic(true);
    m_actionTextItalic->setFont(italic);
    m_actionTextItalic->setCheckable(true);

    const QIcon underlineIcon(getIcon(Theme::Icon::fontStyleUnderline));
    m_actionTextUnderline = ui->toolBar->addAction(underlineIcon, tr("&Underline"),
                                                          [this](bool checked) {
        QTextCharFormat fmt;
        fmt.setFontUnderline(checked);
        mergeFormatOnWordOrSelection(fmt);
    });
    m_actionTextUnderline->setShortcut(Qt::CTRL + Qt::Key_U);
    QFont underline;
    underline.setUnderline(true);
    m_actionTextUnderline->setFont(underline);
    m_actionTextUnderline->setCheckable(true);

    ui->toolBar->addSeparator();
}

void RichTextEditor::setupHyperlinkActions()
{
    const QIcon bulletIcon(getIcon(Theme::Icon::actionIconBinding));
    m_actionHyperlink = ui->toolBar->addAction(bulletIcon, tr("Hyperlink Settings"), [this]() {
        QTextCursor cursor = ui->textEdit->textCursor();
        QTextCharFormat linkFormat = cursor.charFormat();
        if (linkFormat.isAnchor()) {
            m_linkDialog->setLink(linkFormat.anchorHref());
            m_linkDialog->setAnchor(linkFormat.anchorName());
        }
        else {
            m_linkDialog->setLink("http://");
            m_linkDialog->setAnchor("");
        }

        m_linkDialog->show();
    });
    m_actionHyperlink->setCheckable(false);

    ui->toolBar->addSeparator();
}

void RichTextEditor::setupAlignActions()
{
    const QIcon leftIcon(getIcon(Theme::Icon::textAlignLeft));
    m_actionAlignLeft = ui->toolBar->addAction(leftIcon, tr("&Left"), [this]() { ui->textEdit->setAlignment(Qt::AlignLeft | Qt::AlignAbsolute); });
    m_actionAlignLeft->setShortcut(Qt::CTRL + Qt::Key_L);
    m_actionAlignLeft->setCheckable(true);
    m_actionAlignLeft->setPriority(QAction::LowPriority);

    const QIcon centerIcon(getIcon(Theme::Icon::textAlignCenter));
    m_actionAlignCenter = ui->toolBar->addAction(centerIcon, tr("C&enter"), [this]() { ui->textEdit->setAlignment(Qt::AlignHCenter); });
    m_actionAlignCenter->setShortcut(Qt::CTRL + Qt::Key_E);
    m_actionAlignCenter->setCheckable(true);
    m_actionAlignCenter->setPriority(QAction::LowPriority);

    const QIcon rightIcon(getIcon(Theme::Icon::textAlignRight));
    m_actionAlignRight = ui->toolBar->addAction(rightIcon, tr("&Right"), [this]() { ui->textEdit->setAlignment(Qt::AlignRight | Qt::AlignAbsolute); });
    m_actionAlignRight->setShortcut(Qt::CTRL + Qt::Key_R);
    m_actionAlignRight->setCheckable(true);
    m_actionAlignRight->setPriority(QAction::LowPriority);

    const QIcon fillIcon(getIcon(Theme::Icon::textFullJustification));
    m_actionAlignJustify = ui->toolBar->addAction(fillIcon, tr("&Justify"), [this]() { ui->textEdit->setAlignment(Qt::AlignJustify); });
    m_actionAlignJustify->setShortcut(Qt::CTRL + Qt::Key_J);
    m_actionAlignJustify->setCheckable(true);
    m_actionAlignJustify->setPriority(QAction::LowPriority);

    // Make sure the alignLeft  is always left of the alignRight
    QActionGroup *alignGroup = new QActionGroup(ui->toolBar);

    if (QApplication::isLeftToRight()) {
        alignGroup->addAction(m_actionAlignLeft);
        alignGroup->addAction(m_actionAlignCenter);
        alignGroup->addAction(m_actionAlignRight);
    } else {
        alignGroup->addAction(m_actionAlignRight);
        alignGroup->addAction(m_actionAlignCenter);
        alignGroup->addAction(m_actionAlignLeft);
    }
    alignGroup->addAction(m_actionAlignJustify);

    ui->toolBar->addActions(alignGroup->actions());

    ui->toolBar->addSeparator();
}

void RichTextEditor::setupListActions()
{
    const QIcon bulletIcon(getIcon(Theme::Icon::textBulletList));
    m_actionBulletList = ui->toolBar->addAction(bulletIcon, tr("Bullet List"), [this](bool checked) {
        if (checked) {
            m_actionNumberedList->setChecked(false);
            textStyle(QTextListFormat::ListDisc);
        }
        else if (!m_actionNumberedList->isChecked()) {
            textStyle(QTextListFormat::ListStyleUndefined);
        }
    });
    m_actionBulletList->setCheckable(true);

    const QIcon numberedIcon(getIcon(Theme::Icon::textNumberedList));
    m_actionNumberedList = ui->toolBar->addAction(numberedIcon, tr("Numbered List"), [this](bool checked) {
        if (checked) {
            m_actionBulletList->setChecked(false);
            textStyle(QTextListFormat::ListDecimal);
        }
        else if (!m_actionBulletList->isChecked()) {
            textStyle(QTextListFormat::ListStyleUndefined);
        }
    });
    m_actionNumberedList->setCheckable(true);

    ui->toolBar->addSeparator();
}

void RichTextEditor::setupFontActions()
{
    QPixmap colorBox(ui->tableBar->iconSize());
    colorBox.fill(ui->textEdit->textColor());

    m_actionTextColor = ui->toolBar->addAction(colorBox, tr("&Color..."), [this]() {
        QColor col = QColorDialog::getColor(ui->textEdit->textColor(), this);
        if (!col.isValid())
            return;
        QTextCharFormat fmt;
        fmt.setForeground(col);
        mergeFormatOnWordOrSelection(fmt);
        colorChanged(col);
    });

    m_fontNameAction = new FontWidgetActions<QFontComboBox>(this);
    m_fontNameAction->setInitializer([this](QFontComboBox *w) {
        if (!w) return;

        w->setCurrentIndex(w->findText(ui->textEdit->currentCharFormat().font().family()));
        connect(w, QOverload<const QString &>::of(&QComboBox::activated), [this](const QString &f) {
            QTextCharFormat fmt;
            fmt.setFontFamily(f);
            mergeFormatOnWordOrSelection(fmt);
        });
    });

    m_fontNameAction->setDefaultWidget(new QFontComboBox);
    ui->toolBar->addAction(m_fontNameAction);

    m_fontSizeAction = new FontWidgetActions<QComboBox>(this);
    m_fontSizeAction->setInitializer([this](QComboBox *w) {
        if (!w) return;

        w->setEditable(true);

        const QList<int> standardSizes = QFontDatabase::standardSizes();
        foreach (int size, standardSizes)
            w->addItem(QString::number(size));
        w->setCurrentText(QString::number(ui->textEdit->currentCharFormat().font().pointSize()));
        connect(w, QOverload<const QString &>::of(&QComboBox::activated), [this](const QString &p) {
            qreal pointSize = p.toDouble();
            if (pointSize > 0.0) {
                QTextCharFormat fmt;
                fmt.setFontPointSize(pointSize);
                mergeFormatOnWordOrSelection(fmt);
            }
        });
    });

    m_fontSizeAction->setDefaultWidget(new QComboBox);
    ui->toolBar->addAction(m_fontSizeAction);


    ui->toolBar->addSeparator();
}

void RichTextEditor::setupTableActions()
{
    const QIcon tableIcon(getIcon(Theme::Icon::addTable));
    m_actionTableSettings = ui->toolBar->addAction(tableIcon, tr("&Table Settings"), [this](bool checked) {
        ui->tableBar->setVisible(checked);
    });
    m_actionTableSettings->setShortcut(Qt::CTRL + Qt::Key_T);
    m_actionTableSettings->setCheckable(true);
    m_actionTableSettings->setPriority(QAction::LowPriority);

//table bar:

    const QIcon createTableIcon(getIcon(Theme::Icon::addTable));
    m_actionCreateTable = ui->tableBar->addAction(createTableIcon, tr("Create Table"), [this]() {
        QTextCursor cursor = ui->textEdit->textCursor();
        cursorEditBlock(cursor, [&] () {
            cursor.insertTable(1,1);
        });
    });
    m_actionCreateTable->setCheckable(false);

    const QIcon removeTableIcon(getIcon(Theme::Icon::deleteTable));
    m_actionRemoveTable = ui->tableBar->addAction(removeTableIcon, tr("Remove Table"), [this]() {
        QTextCursor cursor = ui->textEdit->textCursor();
        if (QTextTable *currentTable = ui->textEdit->textCursor().currentTable()) {
            cursorEditBlock(cursor, [&] () {
                currentTable->removeRows(0, currentTable->rows());
            });
        }
    });
    m_actionRemoveTable->setCheckable(false);

    ui->tableBar->addSeparator();

    const QIcon addRowIcon(getIcon(Theme::Icon::addRowAfter)); //addRowAfter
    m_actionAddRow = ui->tableBar->addAction(addRowIcon, tr("Add Row"), [this]() {
        QTextCursor cursor = ui->textEdit->textCursor();
        if (QTextTable *currentTable = ui->textEdit->textCursor().currentTable()) {
            cursorEditBlock(cursor, [&] () {
                currentTable->insertRows(currentTable->cellAt(cursor).row()+1, 1);
            });
        }
    });
    m_actionAddRow->setCheckable(false);

    const QIcon addColumnIcon(getIcon(Theme::Icon::addColumnAfter)); //addColumnAfter
    m_actionAddColumn = ui->tableBar->addAction(addColumnIcon, tr("Add Column"), [this]() {
        QTextCursor cursor = ui->textEdit->textCursor();
        if (QTextTable *currentTable = ui->textEdit->textCursor().currentTable()) {
            cursorEditBlock(cursor, [&] () {
                currentTable->insertColumns(currentTable->cellAt(cursor).column()+1, 1);
            });
        }
    });
    m_actionAddColumn->setCheckable(false);

    const QIcon removeRowIcon(getIcon(Theme::Icon::deleteRow));
    m_actionRemoveRow = ui->tableBar->addAction(removeRowIcon, tr("Remove Row"), [this]() {
        QTextCursor cursor = ui->textEdit->textCursor();
        if (QTextTable *currentTable = cursor.currentTable()) {
            cursorEditBlock(cursor, [&] () {
                currentTable->insertColumns(currentTable->cellAt(cursor).column()+1, 1);

                int firstRow = 0;
                int numRows = 0;
                int firstColumn = 0;
                int numColumns = 0;

                if (cursor.hasSelection())
                    cursor.selectedTableCells(&firstRow, &numRows, &firstColumn, &numColumns);

                if (numRows < 1)
                    currentTable->removeRows(currentTable->cellAt(cursor).row(), 1);
                else
                    currentTable->removeRows(firstRow, numRows);
            });
        }
    });
    m_actionRemoveRow->setCheckable(false);

    const QIcon removeColumnIcon(getIcon(Theme::Icon::deleteColumn));
    m_actionRemoveColumn = ui->tableBar->addAction(removeColumnIcon, tr("Remove Column"), [this]() {
        QTextCursor cursor = ui->textEdit->textCursor();
        if (QTextTable *currentTable = cursor.currentTable()) {
            cursorEditBlock(cursor, [&] () {
                int firstRow = 0;
                int numRows = 0;
                int firstColumn = 0;
                int numColumns = 0;

                if (cursor.hasSelection())
                    cursor.selectedTableCells(&firstRow, &numRows, &firstColumn, &numColumns);

                if (numColumns < 1)
                    currentTable->removeColumns(currentTable->cellAt(cursor).column(), 1);
                else
                    currentTable->removeColumns(firstColumn, numColumns);
            });
        }
    });
    m_actionRemoveColumn->setCheckable(false);

    ui->tableBar->addSeparator();

    const QIcon mergeCellsIcon(getIcon(Theme::Icon::mergeCells));
    m_actionMergeCells = ui->tableBar->addAction(mergeCellsIcon, tr("Merge Cells"), [this]() {
        QTextCursor cursor = ui->textEdit->textCursor();
        if (QTextTable *currentTable = cursor.currentTable()) {
            if (cursor.hasSelection()) {
                cursorEditBlock(cursor, [&] () {
                    currentTable->mergeCells(cursor);
                });
            }
        }
    });
    m_actionMergeCells->setCheckable(false);

    const QIcon splitRowIcon(getIcon(Theme::Icon::splitRows));
    m_actionSplitRow = ui->tableBar->addAction(splitRowIcon, tr("Split Row"), [this]() {
        QTextCursor cursor = ui->textEdit->textCursor();
        if (QTextTable *currentTable = cursor.currentTable()) {
            cursorEditBlock(cursor, [&] () {
                currentTable->splitCell(currentTable->cellAt(cursor).row(),
                                        currentTable->cellAt(cursor).column(),
                                        2, 1);
            });
        }
    });
    m_actionSplitRow->setCheckable(false);

    const QIcon splitColumnIcon(getIcon(Theme::Icon::splitColumns));
    m_actionSplitColumn = ui->tableBar->addAction(splitRowIcon, tr("Split Column"), [this]() {
        QTextCursor cursor = ui->textEdit->textCursor();
        if (QTextTable *currentTable = cursor.currentTable()) {
            cursorEditBlock(cursor, [&] () {
                currentTable->splitCell(currentTable->cellAt(cursor).row(),
                                        currentTable->cellAt(cursor).column(),
                                        1, 2);
            });
        }
    });
    m_actionSplitColumn->setCheckable(false);
}

void RichTextEditor::textStyle(QTextListFormat::Style style)
{
    QTextCursor cursor = ui->textEdit->textCursor();
    cursorEditBlock(cursor, [&] () {
        if (style != QTextListFormat::ListStyleUndefined) {
            QTextBlockFormat blockFmt = cursor.blockFormat();
            QTextListFormat listFmt;

            if (cursor.currentList()) {
                listFmt = cursor.currentList()->format();
            } else {
                listFmt.setIndent(blockFmt.indent() + 1);
                blockFmt.setIndent(0);
                cursor.setBlockFormat(blockFmt);
            }

            listFmt.setStyle(style);

            cursor.createList(listFmt);
        } else {
            QTextList* currentList = cursor.currentList();
            QTextBlock currentBlock = cursor.block();
            currentList->remove(currentBlock);

            QTextBlockFormat blockFormat = cursor.blockFormat();
            blockFormat.setIndent(0);
            cursor.setBlockFormat(blockFormat);
        }
    });
}

void RichTextEditor::setTableActionsActive(bool active)
{
    m_actionCreateTable->setEnabled(!active);
    m_actionRemoveTable->setEnabled(active);

    m_actionAddRow->setEnabled(active);
    m_actionAddColumn->setEnabled(active);
    m_actionRemoveRow->setEnabled(active);
    m_actionRemoveColumn->setEnabled(active);

    m_actionMergeCells->setEnabled(active);
    m_actionSplitRow->setEnabled(active);
    m_actionSplitColumn->setEnabled(active);
}

}
