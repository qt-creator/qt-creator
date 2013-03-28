/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "variablechooser.h"
#include "ui_variablechooser.h"
#include "variablemanager.h"
#include "coreconstants.h"

#include <utils/fancylineedit.h> // IconButton
#include <utils/qtcassert.h>

#include <QTimer>
#include <QLineEdit>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QListWidgetItem>

using namespace Core;

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
 * either through the addVariableSupport() method or by manually setting the
 * custom kVariableSupportProperty on the control. If the control supports variables,
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
 * \variable VariableChooser::kVariableSupportProperty
 * Property name that is checked for deciding if a widget supports \QC variables.
 * Can be manually set with
 * \c{textcontrol->setProperty(VariableChooser::kVariableSupportProperty, true)}
 * \sa addVariableSupport()
 */
const char VariableChooser::kVariableSupportProperty[] = "QtCreator.VariableSupport";

/*!
 * Creates a variable chooser that tracks all children of \a parent for variable support.
 * Ownership is also transferred to \a parent.
 * \sa addVariableSupport()
 */
VariableChooser::VariableChooser(QWidget *parent) :
    QWidget(parent),
    ui(new Internal::Ui::VariableChooser),
    m_lineEdit(0),
    m_textEdit(0),
    m_plainTextEdit(0)
{
    ui->setupUi(this);
    m_defaultDescription = ui->variableDescription->text();
    ui->variableList->setAttribute(Qt::WA_MacSmallSize);
    ui->variableList->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->variableDescription->setAttribute(Qt::WA_MacSmallSize);
    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
    setFocusPolicy(Qt::StrongFocus);
    setFocusProxy(ui->variableList);

    foreach (const QByteArray &variable, VariableManager::variables())
        ui->variableList->addItem(QString::fromLatin1(variable));

    connect(ui->variableList, SIGNAL(currentTextChanged(QString)),
            this, SLOT(updateDescription(QString)));
    connect(ui->variableList, SIGNAL(itemActivated(QListWidgetItem*)),
            this, SLOT(handleItemActivated(QListWidgetItem*)));
    connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)),
            this, SLOT(updateCurrentEditor(QWidget*,QWidget*)));
    updateCurrentEditor(0, qApp->focusWidget());
}

/*!
 * \internal
 */
VariableChooser::~VariableChooser()
{
    delete m_iconButton;
    delete ui;
}

/*!
 * Marks the control as supporting variables.
 * \sa kVariableSupportProperty
 */
void VariableChooser::addVariableSupport(QWidget *textcontrol)
{
    QTC_ASSERT(textcontrol, return);
    textcontrol->setProperty(kVariableSupportProperty, true);
}

/*!
 * \internal
 */
void VariableChooser::updateDescription(const QString &variable)
{
    if (variable.isNull())
        ui->variableDescription->setText(m_defaultDescription);
    else
        ui->variableDescription->setText(VariableManager::variableDescription(variable.toUtf8()));
}

/*!
 * \internal
 */
void VariableChooser::updateCurrentEditor(QWidget *old, QWidget *widget)
{
    if (old)
        old->removeEventFilter(this);
    if (!widget) // we might loose focus, but then keep the previous state
        return;
    // prevent children of the chooser itself, and limit to children of chooser's parent
    bool handle = false;
    QWidget *parent = widget;
    while (parent) {
        if (parent == this)
            return;
        if (parent == this->parentWidget()) {
            handle = true;
            break;
        }
        parent = parent->parentWidget();
    }
    if (!handle)
        return;
    widget->installEventFilter(this); // for intercepting escape key presses
    QLineEdit *previousLineEdit = m_lineEdit;
    m_lineEdit = 0;
    m_textEdit = 0;
    m_plainTextEdit = 0;
    QVariant variablesSupportProperty = widget->property(kVariableSupportProperty);
    bool supportsVariables = (variablesSupportProperty.isValid()
                              ? variablesSupportProperty.toBool() : false);
    if (QLineEdit *lineEdit = qobject_cast<QLineEdit *>(widget))
        m_lineEdit = (supportsVariables ? lineEdit : 0);
    else if (QTextEdit *textEdit = qobject_cast<QTextEdit *>(widget))
        m_textEdit = (supportsVariables ? textEdit : 0);
    else if (QPlainTextEdit *plainTextEdit = qobject_cast<QPlainTextEdit *>(widget))
        m_plainTextEdit = (supportsVariables ? plainTextEdit : 0);
    if (!(m_lineEdit || m_textEdit || m_plainTextEdit))
        hide();
    if (m_lineEdit != previousLineEdit) {
        if (previousLineEdit)
            previousLineEdit->setTextMargins(0, 0, 0, 0);
        if (m_iconButton) {
            m_iconButton->hide();
            m_iconButton->setParent(0);
        }
        if (m_lineEdit) {
            if (!m_iconButton)
                createIconButton();
            int margin = m_iconButton->pixmap().width() + 8;
            if (style()->inherits("OxygenStyle"))
                margin = qMax(24, margin);
            m_lineEdit->setTextMargins(0, 0, margin, 0);
            m_iconButton->setParent(m_lineEdit);
            m_iconButton->setGeometry(m_lineEdit->rect().adjusted(
                                          m_lineEdit->width() - (margin + 4), 0, 0, 0));
            m_iconButton->show();
        }
    }
}

/*!
 * \internal
 */
void VariableChooser::createIconButton()
{
    m_iconButton = new Utils::IconButton;
    m_iconButton->setPixmap(QPixmap(QLatin1String(":/core/images/replace.png")));
    m_iconButton->setToolTip(tr("Insert variable"));
    m_iconButton->hide();
    connect(m_iconButton, SIGNAL(clicked()), this, SLOT(updatePositionAndShow()));
}

/*!
 * \internal
 */
void VariableChooser::updatePositionAndShow()
{
    if (parentWidget()) {
        QPoint parentCenter = parentWidget()->mapToGlobal(parentWidget()->geometry().center());
        move(parentCenter.x() - width()/2, parentCenter.y() - height()/2);
    }
    show();
    raise();
    activateWindow();
}

/*!
 * \internal
 */
void VariableChooser::handleItemActivated(QListWidgetItem *item)
{
    if (item)
        insertVariable(item->text());
}

/*!
 * \internal
 */
void VariableChooser::insertVariable(const QString &variable)
{
    const QString &text = QLatin1String("%{") + variable + QLatin1String("}");
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
