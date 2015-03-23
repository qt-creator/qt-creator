/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "variablechooser.h"
#include "coreconstants.h"

#include <utils/fancylineedit.h> // IconButton
#include <utils/headerviewstretcher.h> // IconButton
#include <utils/macroexpander.h>
#include <utils/treemodel.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QAbstractItemModel>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidgetItem>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPointer>
#include <QTextEdit>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>
#include <QVector>

using namespace Utils;

namespace Core {
namespace Internal {

enum {
    UnexpandedTextRole = Qt::UserRole,
    ExpandedTextRole
};

class VariableTreeView : public QTreeView
{
public:
    VariableTreeView(QWidget *parent, VariableChooserPrivate *target)
        : QTreeView(parent), m_target(target)
    {
        setAttribute(Qt::WA_MacSmallSize);
        setAttribute(Qt::WA_MacShowFocusRect, false);
        setIndentation(indentation() * 7/10);
        header()->hide();
        new HeaderViewStretcher(header(), 0);
    }

    void contextMenuEvent(QContextMenuEvent *ev);

private:
    VariableChooserPrivate *m_target;
};


class VariableChooserPrivate : public QObject
{
public:
    VariableChooserPrivate(VariableChooser *parent);

    void createIconButton()
    {
        m_iconButton = new IconButton;
        m_iconButton->setPixmap(QPixmap(QLatin1String(":/core/images/replace.png")));
        m_iconButton->setToolTip(VariableChooser::tr("Insert variable"));
        m_iconButton->hide();
        connect(m_iconButton.data(), static_cast<void(QAbstractButton::*)(bool)>(&QAbstractButton::clicked),
                this, &VariableChooserPrivate::updatePositionAndShow);
    }

    void updateDescription(const QModelIndex &index);
    void updateCurrentEditor(QWidget *old, QWidget *widget);
    void handleItemActivated(const QModelIndex &index);
    void insertText(const QString &variable);
    void updatePositionAndShow(bool);

    QWidget *currentWidget();

    int buttonMargin() const;
    void updateButtonGeometry();

public:
    VariableChooser *q;
    TreeModel m_model;

    QPointer<QLineEdit> m_lineEdit;
    QPointer<QTextEdit> m_textEdit;
    QPointer<QPlainTextEdit> m_plainTextEdit;
    QPointer<IconButton> m_iconButton;

    VariableTreeView *m_variableTree;
    QLabel *m_variableDescription;
    QString m_defaultDescription;
    QByteArray m_currentVariableName; // Prevent recursive insertion of currently expanded item
};

class VariableGroupItem : public TreeItem
{
public:
    VariableGroupItem()
        : m_chooser(0)
    {
        setLazy(true);
    }

    QVariant data(int column, int role) const
    {
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            if (column == 0)
                if (MacroExpander *expander = m_provider())
                    return expander->displayName();
        }

        return QVariant();
    }

    void populate()
    {
        if (MacroExpander *expander = m_provider())
            populateGroup(expander);
    }

    void populateGroup(MacroExpander *expander);

public:
    VariableChooserPrivate *m_chooser; // Not owned.
    MacroExpanderProvider m_provider;
};

class VariableItem : public TreeItem
{
public:
    VariableItem() {}

    QVariant data(int column, int role) const
    {
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            if (column == 0)
                return m_variable;
        }

        if (role == Qt::ToolTipRole)
            return m_expander->variableDescription(m_variable.toUtf8());

        if (role == UnexpandedTextRole)
            return QString(QLatin1String("%{") + m_variable + QLatin1Char('}'));

        if (role == ExpandedTextRole)
            return m_expander->expand(QLatin1String("%{") + m_variable + QLatin1Char('}'));

        return QVariant();
    }

public:
    MacroExpander *m_expander;
    QString m_variable;
};

void VariableTreeView::contextMenuEvent(QContextMenuEvent *ev)
{
    const QModelIndex index = indexAt(ev->pos());

    QString unexpandedText = index.data(UnexpandedTextRole).toString();
    QString expandedText = index.data(ExpandedTextRole).toString();

    QMenu menu;
    QAction *insertUnexpandedAction = 0;
    QAction *insertExpandedAction = 0;

    if (unexpandedText.isEmpty()) {
        insertUnexpandedAction = menu.addAction(VariableChooser::tr("Insert unexpanded value"));
        insertUnexpandedAction->setEnabled(false);
    } else {
        insertUnexpandedAction = menu.addAction(VariableChooser::tr("Insert \"%1\"").arg(unexpandedText));
    }

    if (expandedText.isEmpty()) {
        insertExpandedAction = menu.addAction(VariableChooser::tr("Insert expanded value"));
        insertExpandedAction->setEnabled(false);
    } else {
        insertExpandedAction = menu.addAction(VariableChooser::tr("Insert \"%1\"").arg(expandedText));
    }


    QAction *act = menu.exec(ev->globalPos());

    if (act == insertUnexpandedAction)
        m_target->insertText(unexpandedText);
    else if (act == insertExpandedAction)
        m_target->insertText(expandedText);
}

VariableChooserPrivate::VariableChooserPrivate(VariableChooser *parent)
    : q(parent),
      m_lineEdit(0),
      m_textEdit(0),
      m_plainTextEdit(0)
{
    m_defaultDescription = VariableChooser::tr("Select a variable to insert.");

    m_variableTree = new VariableTreeView(q, this);
    m_variableTree->setModel(&m_model);

    m_variableDescription = new QLabel(q);
    m_variableDescription->setText(m_defaultDescription);
    m_variableDescription->setMinimumSize(QSize(0, 60));
    m_variableDescription->setAlignment(Qt::AlignLeft|Qt::AlignTop);
    m_variableDescription->setWordWrap(true);
    m_variableDescription->setAttribute(Qt::WA_MacSmallSize);

    QVBoxLayout *verticalLayout = new QVBoxLayout(q);
    verticalLayout->setContentsMargins(3, 3, 3, 12);
    verticalLayout->addWidget(m_variableTree);
    verticalLayout->addWidget(m_variableDescription);

    connect(m_variableTree, &QTreeView::clicked,
            this, &VariableChooserPrivate::updateDescription);
    connect(m_variableTree, &QTreeView::activated,
            this, &VariableChooserPrivate::handleItemActivated);
    connect(qobject_cast<QApplication *>(qApp), &QApplication::focusChanged,
            this, &VariableChooserPrivate::updateCurrentEditor);
    updateCurrentEditor(0, qApp->focusWidget());
}

void VariableGroupItem::populateGroup(MacroExpander *expander)
{
    if (!expander)
        return;

    foreach (const QByteArray &variable, expander->visibleVariables()) {
        auto item = new VariableItem;
        item->m_variable = QString::fromUtf8(variable);
        item->m_expander = expander;
        if (variable == m_chooser->m_currentVariableName)
            item->setFlags(Qt::ItemIsSelectable); // not ItemIsEnabled
        appendChild(item);
    }

    foreach (const MacroExpanderProvider &subProvider, expander->subProviders()) {
        if (!subProvider)
            continue;
        if (expander->isAccumulating()) {
            populateGroup(subProvider());
        } else {
            auto item = new VariableGroupItem;
            item->m_chooser = m_chooser;
            item->m_provider = subProvider;
            appendChild(item);
        }
    }
}

} // namespace Internal

using namespace Internal;

/*!
 * \class Core::VariableChooser
 * \brief The VariableChooser class is used to add a tool window for selecting \QC variables
 * to line edits, text edits or plain text edits.
 *
 * If you allow users to add \QC variables to strings that are specified in your UI, for example
 * when users can provide a string through a text control, you should add a variable chooser to it.
 * The variable chooser allows users to open a tool window that contains the list of
 * all available variables together with a description. Double-clicking a variable inserts the
 * corresponding string into the corresponding text control like a line edit.
 *
 * \image variablechooser.png "External Tools Preferences with Variable Chooser"
 *
 * The variable chooser monitors focus changes of all children of its parent widget.
 * When a text control gets focus, the variable chooser checks if it has variable support set,
 * either through the addVariableSupport() function. If the control supports variables,
 * a tool button which opens the variable chooser is shown in it while it has focus.
 *
 * Supported text controls are QLineEdit, QTextEdit and QPlainTextEdit.
 *
 * The variable chooser is deleted when its parent widget is deleted.
 *
 * Example:
 * \code
 * QWidget *myOptionsContainerWidget = new QWidget;
 * new Core::VariableChooser(myOptionsContainerWidget)
 * QLineEdit *myLineEditOption = new QLineEdit(myOptionsContainerWidget);
 * myOptionsContainerWidget->layout()->addWidget(myLineEditOption);
 * Core::VariableChooser::addVariableSupport(myLineEditOption);
 * \endcode
 */

/*!
 * \internal
 * \variable VariableChooser::kVariableSupportProperty
 * Property name that is checked for deciding if a widget supports \QC variables.
 * Can be manually set with
 * \c{textcontrol->setProperty(VariableChooser::kVariableSupportProperty, true)}
 * \sa addVariableSupport()
 */
const char kVariableSupportProperty[] = "QtCreator.VariableSupport";
const char kVariableNameProperty[] = "QtCreator.VariableName";

/*!
 * Creates a variable chooser that tracks all children of \a parent for variable support.
 * Ownership is also transferred to \a parent.
 * \sa addVariableSupport()
 */
VariableChooser::VariableChooser(QWidget *parent) :
    QWidget(parent),
    d(new VariableChooserPrivate(this))
{
    setWindowTitle(tr("Variables"));
    setWindowFlags(Qt::Tool);
    setFocusPolicy(Qt::StrongFocus);
    setFocusProxy(d->m_variableTree);
    addMacroExpanderProvider([]() { return globalMacroExpander(); });
}

/*!
 * \internal
 */
VariableChooser::~VariableChooser()
{
    delete d->m_iconButton;
    delete d;
}

void VariableChooser::addMacroExpanderProvider(const MacroExpanderProvider &provider)
{
    auto item = new VariableGroupItem;
    item->m_chooser = d;
    item->m_provider = provider;
    d->m_model.rootItem()->prependChild(item);
}

/*!
 * Marks the control as supporting variables.
 * \sa kVariableSupportProperty
 */
void VariableChooser::addSupportedWidget(QWidget *textcontrol, const QByteArray &ownName)
{
    QTC_ASSERT(textcontrol, return);
    textcontrol->setProperty(kVariableSupportProperty, QVariant::fromValue<QWidget *>(this));
    textcontrol->setProperty(kVariableNameProperty, ownName);
}

void VariableChooser::addSupportForChildWidgets(QWidget *parent, MacroExpander *expander)
{
     auto chooser = new VariableChooser(parent);
     chooser->addMacroExpanderProvider([expander] { return expander; });
     foreach (QWidget *child, parent->findChildren<QWidget *>()) {
         if (qobject_cast<QLineEdit *>(child)
                 || qobject_cast<QTextEdit *>(child)
                 || qobject_cast<QPlainTextEdit *>(child))
             chooser->addSupportedWidget(child);
     }
}

/*!
 * \internal
 */
void VariableChooserPrivate::updateDescription(const QModelIndex &index)
{
    m_variableDescription->setText(m_model.data(index, Qt::ToolTipRole).toString());
}

/*!
 * \internal
 */
int VariableChooserPrivate::buttonMargin() const
{
    int margin = m_iconButton->pixmap().width() + 8;
    if (q->style()->inherits("OxygenStyle"))
        margin = qMax(24, margin);

    return margin;
}

void VariableChooserPrivate::updateButtonGeometry()
{
    QWidget *current = currentWidget();
    int margin = buttonMargin();
    m_iconButton->setGeometry(current->rect().adjusted(
                                  current->width() - (margin + 4), 0,
                                  0, -qMax(0, current->height() - (margin + 4))));
}

void VariableChooserPrivate::updateCurrentEditor(QWidget *old, QWidget *widget)
{
    Q_UNUSED(old);
    if (!widget) // we might loose focus, but then keep the previous state
        return;
    // prevent children of the chooser itself, and limit to children of chooser's parent
    bool handle = false;
    QWidget *parent = widget;
    while (parent) {
        if (parent == q)
            return;
        if (parent == q->parentWidget()) {
            handle = true;
            break;
        }
        parent = parent->parentWidget();
    }
    if (!handle)
        return;

    QLineEdit *previousLineEdit = m_lineEdit;
    QWidget *previousWidget = currentWidget();
    m_lineEdit = 0;
    m_textEdit = 0;
    m_plainTextEdit = 0;
    QWidget *chooser = widget->property(kVariableSupportProperty).value<QWidget *>();
    m_currentVariableName = widget->property(kVariableNameProperty).value<QByteArray>();
    bool supportsVariables = chooser == q;
    if (QLineEdit *lineEdit = qobject_cast<QLineEdit *>(widget))
        m_lineEdit = (supportsVariables ? lineEdit : 0);
    else if (QTextEdit *textEdit = qobject_cast<QTextEdit *>(widget))
        m_textEdit = (supportsVariables ? textEdit : 0);
    else if (QPlainTextEdit *plainTextEdit = qobject_cast<QPlainTextEdit *>(widget))
        m_plainTextEdit = (supportsVariables ? plainTextEdit : 0);

    QWidget *current = currentWidget();
    if (current != previousWidget) {
        if (previousWidget)
            previousWidget->removeEventFilter(q);
        if (previousLineEdit)
            previousLineEdit->setTextMargins(0, 0, 0, 0);
        if (m_iconButton) {
            m_iconButton->hide();
            m_iconButton->setParent(0);
        }
        if (current) {
            current->installEventFilter(q); // escape key handling and geometry changes
            if (!m_iconButton)
                createIconButton();
            int margin = buttonMargin();
            if (m_lineEdit)
                m_lineEdit->setTextMargins(0, 0, margin, 0);
            m_iconButton->setParent(current);
            updateButtonGeometry();
            m_iconButton->show();
        } else {
            q->hide();
        }
    }
}


/*!
 * \internal
 */
void VariableChooserPrivate::updatePositionAndShow(bool)
{
    if (QWidget *w = q->parentWidget()) {
        QPoint parentCenter = w->mapToGlobal(w->geometry().center());
        q->move(parentCenter.x() - q->width()/2, parentCenter.y() - q->height()/2);
    }
    q->show();
    q->raise();
    q->activateWindow();
    m_variableTree->expandAll();
}

/*!
 * \internal
 */
QWidget *VariableChooserPrivate::currentWidget()
{
    if (m_lineEdit)
        return m_lineEdit;
    if (m_textEdit)
        return m_textEdit;
    return m_plainTextEdit;
}

/*!
 * \internal
 */
void VariableChooserPrivate::handleItemActivated(const QModelIndex &index)
{
    QString text = m_model.data(index, UnexpandedTextRole).toString();
    if (!text.isEmpty())
        insertText(text);
}

/*!
 * \internal
 */
void VariableChooserPrivate::insertText(const QString &text)
{
    if (m_lineEdit) {
        m_lineEdit->insert(text);
        m_lineEdit->activateWindow();
    } else if (m_textEdit) {
        m_textEdit->insertPlainText(text);
        m_textEdit->activateWindow();
    } else if (m_plainTextEdit) {
        m_plainTextEdit->insertPlainText(text);
        m_plainTextEdit->activateWindow();
    }
}

/*!
 * \internal
 */
static bool handleEscapePressed(QKeyEvent *ke, QWidget *widget)
{
    if (ke->key() == Qt::Key_Escape && !ke->modifiers()) {
        ke->accept();
        QTimer::singleShot(0, widget, SLOT(close()));
        return true;
    }
    return false;
}

/*!
 * \internal
 */
bool VariableChooser::event(QEvent *ev)
{
    if (ev->type() == QEvent::KeyPress || ev->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(ev);
        if (handleEscapePressed(ke, this))
            return true;
    }
    return QWidget::event(ev);
}

/*!
 * \internal
 */
bool VariableChooser::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != d->currentWidget())
        return false;
    if ((event->type() == QEvent::KeyPress || event->type() == QEvent::ShortcutOverride) && isVisible()) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        return handleEscapePressed(ke, this);
    } else if (event->type() == QEvent::Resize) {
        d->updateButtonGeometry();
    } else if (event->type() == QEvent::Hide) {
        close();
    }
    return false;
}

} // namespace Internal
