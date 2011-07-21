/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "fallbackselectorwidget.h"
#include "ifallbackpreferences.h"

#include <QtGui/QComboBox>
#include <QtGui/QBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QCheckBox>
#include <QtGui/QPushButton>
#include <QtGui/QMenu>
#include <QtCore/QTextStream>
#include <QtCore/QSignalMapper>

using namespace TextEditor;

Q_DECLARE_METATYPE(TextEditor::IFallbackPreferences *)

FallbackSelectorWidget::FallbackSelectorWidget(QWidget *parent) :
    QWidget(parent),
    m_fallbackPreferences(0),
    m_layout(0),
    m_comboBox(0),
    m_comboBoxLabel(0),
    m_restoreButton(0),
    m_fallbackWidgetVisible(true),
    m_labelText(tr("Settings:"))
{
    hide();
}

void FallbackSelectorWidget::setFallbackPreferences(TextEditor::IFallbackPreferences *fallbackPreferences)
{
    if (m_fallbackPreferences == fallbackPreferences)
        return; // nothing changes

    // cleanup old
    if (m_fallbackPreferences) {
        disconnect(m_fallbackPreferences, SIGNAL(currentFallbackChanged(IFallbackPreferences*)),
                this, SLOT(slotCurrentFallbackChanged(IFallbackPreferences*)));
        hide();

        if (m_layout)
            delete m_layout;
    }
    m_fallbackPreferences = fallbackPreferences;
    // fillup new
    if (m_fallbackPreferences) {
        const QList<IFallbackPreferences *> fallbacks = m_fallbackPreferences->fallbacks();
        setVisible(m_fallbackWidgetVisible && !fallbacks.isEmpty());

        m_layout = new QHBoxLayout(this);
        m_layout->setContentsMargins(QMargins());
        m_restoreButton = new QPushButton(this);
        QSignalMapper *mapper = new QSignalMapper(this);

        m_comboBoxLabel = new QLabel(m_labelText, this);
        m_comboBoxLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_layout->addWidget(m_comboBoxLabel);
        m_comboBox = new QComboBox(this);
        m_layout->addWidget(m_comboBox);
        m_comboBox->addItem(tr("Custom"), QVariant::fromValue<TextEditor::IFallbackPreferences *>(0));
        connect(m_comboBox, SIGNAL(activated(int)),
                this, SLOT(slotComboBoxActivated(int)));

        QMenu *menu = new QMenu(this);
        if (fallbacks.count() == 1) {
            IFallbackPreferences *fallback = fallbacks.first();
            m_restoreButton->setText(tr("Restore %1", "%1 is settings name (e.g. Global C++)").arg(fallback->displayName()));
            connect(m_restoreButton, SIGNAL(clicked()), mapper, SLOT(map()));
            mapper->setMapping(m_restoreButton, fallback);
        } else {
            m_restoreButton->setText(tr("Restore"));
            m_restoreButton->setMenu(menu);
        }

        for (int i = 0; i < fallbacks.count(); i++) {
            IFallbackPreferences *fallback = fallbacks.at(i);
            const QString displayName = fallback->displayName();
            const QVariant data = QVariant::fromValue(fallback);
            m_comboBox->insertItem(i, displayName, data);
            QAction *restoreAction = new QAction(displayName, this);
            menu->addAction(restoreAction);
            connect(restoreAction, SIGNAL(triggered()), mapper, SLOT(map()));
            mapper->setMapping(restoreAction, fallback);
        }
        m_layout->addWidget(m_restoreButton);

        slotCurrentFallbackChanged(m_fallbackPreferences->currentFallback());

        connect(m_fallbackPreferences, SIGNAL(currentFallbackChanged(TextEditor::IFallbackPreferences*)),
                this, SLOT(slotCurrentFallbackChanged(TextEditor::IFallbackPreferences*)));
        connect(mapper, SIGNAL(mapped(QObject*)), this, SLOT(slotRestoreValues(QObject*)));
    }
}

void FallbackSelectorWidget::slotComboBoxActivated(int index)
{
    if (!m_comboBox || index < 0 || index >= m_comboBox->count())
        return;
    TextEditor::IFallbackPreferences *fallback =
            m_comboBox->itemData(index).value<TextEditor::IFallbackPreferences *>();

    const bool wasBlocked = blockSignals(true);
    m_fallbackPreferences->setCurrentFallback(fallback);
    blockSignals(wasBlocked);
}

void FallbackSelectorWidget::slotCurrentFallbackChanged(TextEditor::IFallbackPreferences *fallback)
{
    const bool wasBlocked = blockSignals(true);
    if (m_comboBox)
        m_comboBox->setCurrentIndex(m_comboBox->findData(QVariant::fromValue(fallback)));
    if (m_restoreButton)
        m_restoreButton->setEnabled(!fallback);
    blockSignals(wasBlocked);
}

void FallbackSelectorWidget::slotRestoreValues(QObject *fallbackObject)
{
    TextEditor::IFallbackPreferences *fallback
            = qobject_cast<TextEditor::IFallbackPreferences *>(fallbackObject);
    if (!fallback)
        return;
    m_fallbackPreferences->setValue(fallback->currentValue());
}

void FallbackSelectorWidget::setFallbacksVisible(bool on)
{
    m_fallbackWidgetVisible = on;
    if (m_fallbackPreferences)
        setVisible(m_fallbackWidgetVisible && !m_fallbackPreferences->fallbacks().isEmpty());
}

void FallbackSelectorWidget::setLabelText(const QString &text)
{
    m_labelText = text;
    if (m_comboBoxLabel)
        m_comboBoxLabel->setText(text);
}

QString FallbackSelectorWidget::searchKeywords() const
{
    // no useful keywords here
    return QString();
}
