#include "fallbackselectorwidget.h"
#include "ifallbackpreferences.h"

#include <QtGui/QComboBox>
#include <QtGui/QBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QCheckBox>
#include <QtGui/QToolButton>
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
        m_restoreButton = new QToolButton(this);
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
            m_restoreButton->setPopupMode(QToolButton::InstantPopup);
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
