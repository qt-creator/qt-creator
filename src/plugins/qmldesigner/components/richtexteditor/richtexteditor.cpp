// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "richtexteditor.h"
#include "hyperlinkdialog.h"

#include <utils/layoutbuilder.h>
#include <utils/stylehelper.h>

#include <functional>

#include <QAction>
#include <QActionGroup>
#include <QColorDialog>
#include <QFileDialog>
#include <QPainter>
#include <QPointer>
#include <QStyle>
#include <QStyleFactory>
#include <QTextTable>
#include <QTextTableFormat>
#include <QToolButton>
#include <QWidgetAction>
#include <QApplication>
#include <QTextEdit>
#include <QToolBar>

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

static QPixmap drawColorBox(const QColor& color, const QSize& size, int borderWidth = 4)
{
    if (size.isEmpty()) {
        return {};
    }

    QPixmap result(size);

    const QColor borderColor = QApplication::palette("QWidget").color(QPalette::Normal,
                                                                      QPalette::Button);

    result.fill(color);
    QPainter painter(&result);
    QPen pen(borderColor);
    pen.setWidth(borderWidth);
    painter.setPen(pen);
    painter.drawRect(QRect(QPoint(0,0), size));

    return result;
}

RichTextEditor::RichTextEditor(QWidget *parent)
    : QWidget(parent)
    , m_linkDialog(new HyperlinkDialog(this))
{

    resize(428, 283);
    QSizePolicy pol(QSizePolicy::Preferred, QSizePolicy::Expanding);
    pol.setHorizontalStretch(0);
    pol.setVerticalStretch(5);
    pol.setHeightForWidth(sizePolicy().hasHeightForWidth());
    setSizePolicy(pol);

    m_toolBar = new QToolBar(this);
    m_toolBar->setIconSize(QSize(20, 20));

    m_tableBar = new QToolBar(this);
    m_tableBar->setIconSize(QSize(20, 20));

    m_textEdit = new QTextEdit(this);
    m_textEdit->setTextInteractionFlags(Qt::TextEditorInteraction | Qt::LinksAccessibleByMouse);

    using namespace Layouting;
    Column {
        m_toolBar,
        m_tableBar,
        m_textEdit
    }.attachTo(this);

    m_tableBar->setVisible(false);

    const QColor backColor = Theme::getColor(Theme::DSpanelBackground);

    const QString toolBarStyleSheet =
            QString("QToolBar { background-color: %1; border-width: 1px }").arg(backColor.name());

    m_toolBar->setStyleSheet(toolBarStyleSheet);
    m_tableBar->setStyleSheet(toolBarStyleSheet);

    setupEditActions();
    setupTextActions();
    setupImageActions();
    setupHyperlinkActions();
    setupAlignActions();
    setupListActions();
    setupFontActions();
    setupTableActions();

    connect(m_textEdit, &QTextEdit::currentCharFormatChanged,
            this, &RichTextEditor::currentCharFormatChanged);
    connect(m_textEdit, &QTextEdit::cursorPositionChanged,
            this, &RichTextEditor::cursorPositionChanged);
    connect(m_textEdit, &QTextEdit::textChanged,
            this, &RichTextEditor::onTextChanged);
    connect(m_linkDialog, &QDialog::accepted, [this]() {
        QTextCharFormat oldFormat = m_textEdit->textCursor().charFormat();

        QTextCursor tcursor = m_textEdit->textCursor();
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

    m_textEdit->setFocus();
    m_linkDialog->hide();
}

RichTextEditor::~RichTextEditor()
{
}

void RichTextEditor::setPlainText(const QString &text)
{
    m_textEdit->setPlainText(text);
}

QString RichTextEditor::plainText() const
{
    return m_textEdit->toPlainText();
}

void RichTextEditor::setRichText(const QString &text)
{
    m_textEdit->setHtml(text);
}

void RichTextEditor::setTabChangesFocus(bool change)
{
    m_textEdit->setTabChangesFocus(change);
}

void RichTextEditor::setImageActionVisible(bool change)
{
    m_actionImage->setVisible(change);
}

void RichTextEditor::setDocumentBaseUrl(const QUrl& url)
{
    m_textEdit->document()->setBaseUrl(url);
}

QIcon RichTextEditor::getIcon(Theme::Icon icon)
{
    const QString fontName = "qtds_propertyIconFont.ttf";
    const QColor iconColorNormal(Theme::getColor(Theme::DStextColor));

    return Utils::StyleHelper::getIconFromIconFont(
                fontName, Theme::getIconUnicode(icon), 20, 20, iconColorNormal);
}

QString RichTextEditor::richText() const
{
    return m_textEdit->toHtml();
}

void RichTextEditor::currentCharFormatChanged(const QTextCharFormat &format)
{
    fontChanged(format.font());
    colorChanged(format.foreground().color());
}

void RichTextEditor::cursorPositionChanged()
{
    alignmentChanged(m_textEdit->alignment());
    styleChanged(m_textEdit->textCursor());
    tableChanged(m_textEdit->textCursor());
}

void RichTextEditor::onTextChanged() {
    emit textChanged(richText());
}

void RichTextEditor::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = m_textEdit->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);
    m_textEdit->mergeCurrentCharFormat(format);
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
    QPixmap colorBox(drawColorBox(c, m_tableBar->iconSize()));
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
        m_tableBar->setVisible(true);

        setTableActionsActive(true);
    }
    else {
        setTableActionsActive(false);
    }
}

void RichTextEditor::setupEditActions()
{
    const QIcon undoIcon(getIcon(Theme::Icon::undo));
    QAction *actionUndo = m_toolBar->addAction(undoIcon, tr("&Undo"), m_textEdit, &QTextEdit::undo);
    actionUndo->setShortcut(QKeySequence::Undo);
    connect(m_textEdit->document(), &QTextDocument::undoAvailable,
            actionUndo, &QAction::setEnabled);

    const QIcon redoIcon(getIcon(Theme::Icon::redo));
    QAction *actionRedo = m_toolBar->addAction(redoIcon, tr("&Redo"), m_textEdit, &QTextEdit::redo);
    actionRedo->setShortcut(QKeySequence::Redo);
    connect(m_textEdit->document(), &QTextDocument::redoAvailable,
            actionRedo, &QAction::setEnabled);

    actionUndo->setEnabled(m_textEdit->document()->isUndoAvailable());
    actionRedo->setEnabled(m_textEdit->document()->isRedoAvailable());

    m_toolBar->addSeparator();
}

void RichTextEditor::setupTextActions()
{
    const QIcon boldIcon(getIcon(Theme::Icon::fontStyleBold));
    m_actionTextBold = m_toolBar->addAction(boldIcon, tr("&Bold"),
                                                     [this](bool checked) {
        QTextCharFormat fmt;
        fmt.setFontWeight(checked ? QFont::Bold : QFont::Normal);
        mergeFormatOnWordOrSelection(fmt);
    });
    m_actionTextBold->setShortcut(Qt::CTRL | Qt::Key_B);
    QFont bold;
    bold.setBold(true);
    m_actionTextBold->setFont(bold);
    m_actionTextBold->setCheckable(true);

    const QIcon italicIcon(getIcon(Theme::Icon::fontStyleItalic));
    m_actionTextItalic = m_toolBar->addAction(italicIcon, tr("&Italic"),
                                                       [this](bool checked) {
        QTextCharFormat fmt;
        fmt.setFontItalic(checked);
        mergeFormatOnWordOrSelection(fmt);
    });
    m_actionTextItalic->setShortcut(Qt::CTRL | Qt::Key_I);
    QFont italic;
    italic.setItalic(true);
    m_actionTextItalic->setFont(italic);
    m_actionTextItalic->setCheckable(true);

    const QIcon underlineIcon(getIcon(Theme::Icon::fontStyleUnderline));
    m_actionTextUnderline = m_toolBar->addAction(underlineIcon, tr("&Underline"),
                                                          [this](bool checked) {
        QTextCharFormat fmt;
        fmt.setFontUnderline(checked);
        mergeFormatOnWordOrSelection(fmt);
    });
    m_actionTextUnderline->setShortcut(Qt::CTRL | Qt::Key_U);
    QFont underline;
    underline.setUnderline(true);
    m_actionTextUnderline->setFont(underline);
    m_actionTextUnderline->setCheckable(true);

    m_toolBar->addSeparator();
}

void RichTextEditor::setupImageActions()
{
    auto insertImage = [this]() {
        QFileDialog dialog(this);
        dialog.setFileMode(QFileDialog::ExistingFile);
        dialog.setWindowTitle(tr("Select Image"));
        dialog.setNameFilters({tr("Image files (*.png *.jpg)")});

        if (dialog.exec()) {
            QStringList files = dialog.selectedFiles();
            for (QString& filePath : files) {
                emit insertingImage(filePath);

                m_textEdit->insertHtml("<img src=\"" + filePath + "\" />");
            }
        }
    };

    m_actionImage = m_toolBar
                        ->addAction(getIcon(Theme::Icon::addFile), tr("Insert &Image"), insertImage);

    setImageActionVisible(false);
}

void RichTextEditor::setupHyperlinkActions()
{
    const QIcon bulletIcon(getIcon(Theme::Icon::actionIconBinding));
    m_actionHyperlink = m_toolBar->addAction(bulletIcon, tr("Hyperlink Settings"), [this]() {
        QTextCursor cursor = m_textEdit->textCursor();
        QTextCharFormat linkFormat = cursor.charFormat();
        if (linkFormat.isAnchor()) {
            m_linkDialog->setLink(linkFormat.anchorHref());
            m_linkDialog->setAnchor(
                linkFormat.anchorNames().isEmpty() ? QString() : linkFormat.anchorNames().constFirst());
        }
        else {
            m_linkDialog->setLink("http://");
            m_linkDialog->setAnchor("");
        }

        m_linkDialog->show();
    });
    m_actionHyperlink->setCheckable(false);

    m_toolBar->addSeparator();
}

void RichTextEditor::setupAlignActions()
{
    const QIcon leftIcon(getIcon(Theme::Icon::textAlignLeft));
    m_actionAlignLeft = m_toolBar->addAction(leftIcon, tr("&Left"), [this]() { m_textEdit->setAlignment(Qt::AlignLeft | Qt::AlignAbsolute); });
    m_actionAlignLeft->setShortcut(Qt::CTRL | Qt::Key_L);
    m_actionAlignLeft->setCheckable(true);
    m_actionAlignLeft->setPriority(QAction::LowPriority);

    const QIcon centerIcon(getIcon(Theme::Icon::textAlignCenter));
    m_actionAlignCenter = m_toolBar->addAction(centerIcon, tr("C&enter"), [this]() { m_textEdit->setAlignment(Qt::AlignHCenter); });
    m_actionAlignCenter->setShortcut(Qt::CTRL | Qt::Key_E);
    m_actionAlignCenter->setCheckable(true);
    m_actionAlignCenter->setPriority(QAction::LowPriority);

    const QIcon rightIcon(getIcon(Theme::Icon::textAlignRight));
    m_actionAlignRight = m_toolBar->addAction(rightIcon, tr("&Right"), [this]() { m_textEdit->setAlignment(Qt::AlignRight | Qt::AlignAbsolute); });
    m_actionAlignRight->setShortcut(Qt::CTRL | Qt::Key_R);
    m_actionAlignRight->setCheckable(true);
    m_actionAlignRight->setPriority(QAction::LowPriority);

    const QIcon fillIcon(getIcon(Theme::Icon::textFullJustification));
    m_actionAlignJustify = m_toolBar->addAction(fillIcon, tr("&Justify"), [this]() { m_textEdit->setAlignment(Qt::AlignJustify); });
    m_actionAlignJustify->setShortcut(Qt::CTRL | Qt::Key_J);
    m_actionAlignJustify->setCheckable(true);
    m_actionAlignJustify->setPriority(QAction::LowPriority);

    // Make sure the alignLeft  is always left of the alignRight
    QActionGroup *alignGroup = new QActionGroup(m_toolBar);

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

    m_toolBar->addActions(alignGroup->actions());

    m_toolBar->addSeparator();
}

void RichTextEditor::setupListActions()
{
    const QIcon bulletIcon(getIcon(Theme::Icon::textBulletList));
    m_actionBulletList = m_toolBar->addAction(bulletIcon, tr("Bullet List"), [this](bool checked) {
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
    m_actionNumberedList = m_toolBar->addAction(numberedIcon, tr("Numbered List"), [this](bool checked) {
        if (checked) {
            m_actionBulletList->setChecked(false);
            textStyle(QTextListFormat::ListDecimal);
        }
        else if (!m_actionBulletList->isChecked()) {
            textStyle(QTextListFormat::ListStyleUndefined);
        }
    });
    m_actionNumberedList->setCheckable(true);

    m_toolBar->addSeparator();
}

void RichTextEditor::setupFontActions()
{
    QPixmap colorBox(drawColorBox(m_textEdit->textColor(), m_tableBar->iconSize()));

    m_actionTextColor = m_toolBar->addAction(colorBox, tr("&Color..."), [this]() {
        QColor col = QColorDialog::getColor(m_textEdit->textColor(), this);
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

        w->setCurrentIndex(w->findText(m_textEdit->currentCharFormat().font().family()));
        connect(w, &QComboBox::textActivated, [this](const QString &f) {
            QTextCharFormat fmt;
            fmt.setFontFamily(f);
            mergeFormatOnWordOrSelection(fmt);
        });
    });

    m_fontNameAction->setDefaultWidget(new QFontComboBox);
    m_toolBar->addAction(m_fontNameAction);

    m_fontSizeAction = new FontWidgetActions<QComboBox>(this);
    m_fontSizeAction->setInitializer([this](QComboBox *w) {
        if (!w) return;

        w->setEditable(true);

        const QList<int> standardSizes = QFontDatabase::standardSizes();
        for (const int size : standardSizes)
            w->addItem(QString::number(size));
        w->setCurrentText(QString::number(m_textEdit->currentCharFormat().font().pointSize()));
        connect(w, &QComboBox::textActivated, [this](const QString &p) {
            qreal pointSize = p.toDouble();
            if (pointSize > 0.0) {
                QTextCharFormat fmt;
                fmt.setFontPointSize(pointSize);
                mergeFormatOnWordOrSelection(fmt);
            }
        });
    });

    m_fontSizeAction->setDefaultWidget(new QComboBox);
    m_toolBar->addAction(m_fontSizeAction);


    m_toolBar->addSeparator();
}

void RichTextEditor::setupTableActions()
{
    const QIcon tableIcon(getIcon(Theme::Icon::addTable));
    m_actionTableSettings = m_toolBar->addAction(tableIcon, tr("&Table Settings"), [this](bool checked) {
        m_tableBar->setVisible(checked);
    });
    m_actionTableSettings->setShortcut(Qt::CTRL | Qt::Key_T);
    m_actionTableSettings->setCheckable(true);
    m_actionTableSettings->setPriority(QAction::LowPriority);

//table bar:

    const QIcon createTableIcon(getIcon(Theme::Icon::addTable));
    m_actionCreateTable = m_tableBar->addAction(createTableIcon, tr("Create Table"), [this]() {
        QTextCursor cursor = m_textEdit->textCursor();
        cursorEditBlock(cursor, [&] () {
            //format table cells to look a bit better:
            QTextTableFormat tableFormat;
            tableFormat.setCellSpacing(2.0);
            tableFormat.setCellPadding(2.0);
            tableFormat.setBorder(1.0);
            tableFormat.setBorderCollapse(true);
            cursor.insertTable(1, 1, tableFormat);

            //move cursor into the first cell of the table:
            m_textEdit->setTextCursor(cursor);
        });
    });
    m_actionCreateTable->setCheckable(false);

    const QIcon removeTableIcon(getIcon(Theme::Icon::deleteTable));
    m_actionRemoveTable = m_tableBar->addAction(removeTableIcon, tr("Remove Table"), [this]() {
        QTextCursor cursor = m_textEdit->textCursor();
        if (QTextTable *currentTable = m_textEdit->textCursor().currentTable()) {
            cursorEditBlock(cursor, [&] () {
                currentTable->removeRows(0, currentTable->rows());
            });
        }
    });
    m_actionRemoveTable->setCheckable(false);

    m_tableBar->addSeparator();

    const QIcon addRowIcon(getIcon(Theme::Icon::addRowAfter)); //addRowAfter
    m_actionAddRow = m_tableBar->addAction(addRowIcon, tr("Add Row"), [this]() {
        QTextCursor cursor = m_textEdit->textCursor();
        if (QTextTable *currentTable = m_textEdit->textCursor().currentTable()) {
            cursorEditBlock(cursor, [&] () {
                currentTable->insertRows(currentTable->cellAt(cursor).row()+1, 1);
            });
        }
    });
    m_actionAddRow->setCheckable(false);

    const QIcon addColumnIcon(getIcon(Theme::Icon::addColumnAfter)); //addColumnAfter
    m_actionAddColumn = m_tableBar->addAction(addColumnIcon, tr("Add Column"), [this]() {
        QTextCursor cursor = m_textEdit->textCursor();
        if (QTextTable *currentTable = m_textEdit->textCursor().currentTable()) {
            cursorEditBlock(cursor, [&] () {
                currentTable->insertColumns(currentTable->cellAt(cursor).column()+1, 1);
            });
        }
    });
    m_actionAddColumn->setCheckable(false);

    const QIcon removeRowIcon(getIcon(Theme::Icon::deleteRow));
    m_actionRemoveRow = m_tableBar->addAction(removeRowIcon, tr("Remove Row"), [this]() {
        QTextCursor cursor = m_textEdit->textCursor();
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
    m_actionRemoveColumn = m_tableBar->addAction(removeColumnIcon, tr("Remove Column"), [this]() {
        QTextCursor cursor = m_textEdit->textCursor();
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

    m_tableBar->addSeparator();

    const QIcon mergeCellsIcon(getIcon(Theme::Icon::mergeCells));
    m_actionMergeCells = m_tableBar->addAction(mergeCellsIcon, tr("Merge Cells"), [this]() {
        QTextCursor cursor = m_textEdit->textCursor();
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
    m_actionSplitRow = m_tableBar->addAction(splitRowIcon, tr("Split Row"), [this]() {
        QTextCursor cursor = m_textEdit->textCursor();
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
    m_actionSplitColumn = m_tableBar->addAction(splitColumnIcon, tr("Split Column"), [this]() {
        QTextCursor cursor = m_textEdit->textCursor();
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
    QTextCursor cursor = m_textEdit->textCursor();
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
