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
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "variablechooser.h"
#include "coreconstants.h"

#include <utils/fancylineedit.h> // IconButton
#include <utils/macroexpander.h>
#include <utils/treemodel.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QAbstractItemModel>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidgetItem>
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

class VariableChooserPrivate : public QObject
{
public:
    VariableChooserPrivate(VariableChooser *parent);

    void createIconButton()
    {
        m_iconButton = new Utils::IconButton;
        m_iconButton->setPixmap(QPixmap(QLatin1String(":/core/images/replace.png")));
        m_iconButton->setToolTip(tr("Insert variable"));
        m_iconButton->hide();
        connect(m_iconButton.data(), static_cast<void(QAbstractButton::*)(bool)>(&QAbstractButton::clicked),
                this, &VariableChooserPrivate::updatePositionAndShow);
    }

    void updateDescription(const QModelIndex &index);
    void updateCurrentEditor(QWidget *old, QWidget *widget);
    void handleItemActivated(const QModelIndex &index);
    void insertVariable(const QString &variable);
    void updatePositionAndShow(bool);

    QWidget *currentWidget();

public:
    VariableChooser *q;
    TreeModel m_model;

    QPointer<QLineEdit> m_lineEdit;
    QPointer<QTextEdit> m_textEdit;
    QPointer<QPlainTextEdit> m_plainTextEdit;
    QPointer<Utils::IconButton> m_iconButton;

    QTreeView *m_variableTree;
    QLabel *m_variableDescription;
    QString m_defaultDescription;
    QByteArray m_currentVariableName; // Prevent recursive insertion of currently expanded item
};

class VariableItem : public TreeItem
{
public:
    VariableItem()
    {}

    QVariant data(int column, int role) const
    {
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            if (column == 0)
                return m_display;
        }
        if (role == Qt::ToolTipRole)
            return m_description;

        return QVariant();
    }

public:
    QString m_display;
    QString m_description;
};

class VariableGroupItem : public TreeItem
{
public:
    VariableGroupItem(VariableChooserPrivate *chooser)
        : m_chooser(chooser), m_expander(0)
    {
        setLazy(true);
    }

    bool ensureExpander() const
    {
        if (!m_expander)
            m_expander = m_provider();
        return m_expander != 0;
    }

    QVariant data(int column, int role) const
    {
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            if (column == 0 && ensureExpander())
                return m_expander->displayName();
        }
        return QVariant();
    }

    void populate()
    {
        if (ensureExpander()) {
            foreach (const QByteArray &variable, m_expander->variables()) {
                auto item = new VariableItem;
                item->m_display = QString::fromLatin1(variable);
                item->m_description = m_expander->variableDescription(variable);
                if (variable == m_chooser->m_currentVariableName)
                    item->setFlags(Qt::ItemIsSelectable); // not ItemIsEnabled
                appendChild(item);
            }
        }
    }

public:
    VariableChooserPrivate *m_chooser; // Not owned.
    MacroExpanderProvider m_provider;
    mutable MacroExpander *m_expander; // Not owned.
    QString m_displayName;
};

VariableChooserPrivate::VariableChooserPrivate(VariableChooser *parent)
    : q(parent),
      m_lineEdit(0),
      m_textEdit(0),
      m_plainTextEdit(0)
{
    m_defaultDescription = VariableChooser::tr("Select a variable to insert.");

    m_variableTree = new QTreeView(q);
    m_variableTree->setAttribute(Qt::WA_MacSmallSize);
    m_variableTree->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_variableTree->setModel(&m_model);
    m_variableTree->header()->hide();
    m_variableTree->header()->setStretchLastSection(true);

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

    //        connect(m_variableList, &QTreeView::currentChanged,
    //            this, &VariableChooserPrivate::updateDescription);
    connect(m_variableTree, &QTreeView::clicked,
            this, &VariableChooserPrivate::updateDescription);
    connect(m_variableTree, &QTreeView::activated,
            this, &VariableChooserPrivate::handleItemActivated);
    connect(qobject_cast<QApplication *>(qApp), &QApplication::focusChanged,
            this, &VariableChooserPrivate::updateCurrentEditor);
    updateCurrentEditor(0, qApp->focusWidget());
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
    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
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
    auto *item = new VariableGroupItem(d);
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
void VariableChooserPrivate::updateCurrentEditor(QWidget *old, QWidget *widget)
{
    if (old)
        old->removeEventFilter(this);
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

    widget->installEventFilter(this); // for intercepting escape key presses
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
    if (!(m_lineEdit || m_textEdit || m_plainTextEdit))
        q->hide();

    QWidget *current = currentWidget();
    if (current != previousWidget) {
        if (previousLineEdit)
            previousLineEdit->setTextMargins(0, 0, 0, 0);
        if (m_iconButton) {
            m_iconButton->hide();
            m_iconButton->setParent(0);
        }
        if (current) {
            if (!m_iconButton)
                createIconButton();
            int margin = m_iconButton->pixmap().width() + 8;
            if (q->style()->inherits("OxygenStyle"))
                margin = qMax(24, margin);
            if (m_lineEdit)
                m_lineEdit->setTextMargins(0, 0, margin, 0);
            m_iconButton->setParent(current);
            m_iconButton->setGeometry(current->rect().adjusted(
                                          current->width() - (margin + 4), 0,
                                          0, -qMax(0, current->height() - (margin + 4))));
            m_iconButton->show();
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
    insertVariable(m_model.data(index, Qt::DisplayRole).toString());
}

/*!
 * \internal
 */
void VariableChooserPrivate::insertVariable(const QString &variable)
{
    const QString text = QLatin1String("%{") + variable + QLatin1Char('}');
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
void VariableChooser::keyPressEvent(QKeyEvent *ke)
{
    handleEscapePressed(ke, this);
}

/*!
 * \internal
 */
bool VariableChooser::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::KeyPress && isVisible()) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        return handleEscapePressed(ke, this);
    }
    return false;
}

} // namespace Internal
