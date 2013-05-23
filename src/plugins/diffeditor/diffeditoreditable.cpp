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

#include "diffeditoreditable.h"
#include "diffeditorfile.h"
#include "diffeditorwidget.h"
#include "diffeditorconstants.h"

#include <coreplugin/icore.h>
#include <QCoreApplication>
#include <QToolButton>
#include <QSpinBox>
#include <QStyle>
#include <QLabel>
#include <QHBoxLayout>

namespace DiffEditor {

///////////////////////////////// DiffEditorEditable //////////////////////////////////

DiffEditorEditable::DiffEditorEditable(DiffEditorWidget *editorWidget)
    : IEditor(0),
      m_file(new Internal::DiffEditorFile(QLatin1String(Constants::DIFF_EDITOR_MIMETYPE), this)),
      m_editorWidget(editorWidget),
      m_toolWidget(0)
{
    setWidget(editorWidget);
}

DiffEditorEditable::~DiffEditorEditable()
{
    delete m_toolWidget;
    if (m_widget)
        delete m_widget;
}

bool DiffEditorEditable::createNew(const QString &contents)
{
    Q_UNUSED(contents)
    return true;
}

bool DiffEditorEditable::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    Q_UNUSED(errorString)
    Q_UNUSED(fileName)
    Q_UNUSED(realFileName)
    return true;
}

Core::IDocument *DiffEditorEditable::document()
{
    return m_file;
}

QString DiffEditorEditable::displayName() const
{
    if (m_displayName.isEmpty())
        m_displayName = QCoreApplication::translate("DiffEditor", Constants::DIFF_EDITOR_DISPLAY_NAME);
    return m_displayName;
}

void DiffEditorEditable::setDisplayName(const QString &title)
{
    m_displayName = title;
    emit changed();
}

bool DiffEditorEditable::duplicateSupported() const
{
    return false;
}

Core::IEditor *DiffEditorEditable::duplicate(QWidget *parent)
{
    Q_UNUSED(parent)
    return 0;
}

Core::Id DiffEditorEditable::id() const
{
    return Constants::DIFF_EDITOR_ID;
}

static QToolBar *createToolBar(const QWidget *someWidget)
{
    // Create
    QToolBar *toolBar = new QToolBar;
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    const int size = someWidget->style()->pixelMetric(QStyle::PM_SmallIconSize);
    toolBar->setIconSize(QSize(size, size));
    toolBar->addSeparator();

    return toolBar;
}

QWidget *DiffEditorEditable::toolBar()
{
    if (m_toolWidget)
        return m_toolWidget;

    // Create
    m_toolWidget = createToolBar(m_editorWidget);

    QWidget *spacerWidget = new QWidget();
    QLayout *spacerLayout = new QHBoxLayout();
    spacerLayout->setMargin(0);
    spacerLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
    spacerWidget->setLayout(spacerLayout);
    m_toolWidget->addWidget(spacerWidget);

    QToolButton *whitespaceButton = new QToolButton(m_toolWidget);
    whitespaceButton->setText(tr("Ignore Whitespace"));
    whitespaceButton->setCheckable(true);
    whitespaceButton->setChecked(true);
    connect(whitespaceButton, SIGNAL(clicked(bool)),
            m_editorWidget, SLOT(setIgnoreWhitespaces(bool)));
    m_toolWidget->addWidget(whitespaceButton);

    QLabel *contextLabel = new QLabel(tr("Context lines:"), m_toolWidget);
    m_toolWidget->addWidget(contextLabel);

    QSpinBox *contextSpinBox = new QSpinBox(m_toolWidget);
    contextSpinBox->setRange(-1, 100);
    contextSpinBox->setValue(3);
    connect(contextSpinBox, SIGNAL(valueChanged(int)),
            m_editorWidget, SLOT(setContextLinesNumber(int)));
    m_toolWidget->addWidget(contextSpinBox);

    return m_toolWidget;
}

QByteArray DiffEditorEditable::saveState() const
{
    return QByteArray();
}

bool DiffEditorEditable::restoreState(const QByteArray &state)
{
    Q_UNUSED(state)
    return true;
}

} // namespace DiffEditor
