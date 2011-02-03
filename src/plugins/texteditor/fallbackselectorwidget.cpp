#include "fallbackselectorwidget.h"
#include "ifallbackpreferences.h"

#include <QtGui/QComboBox>
#include <QtGui/QBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QCheckBox>
#include <QtCore/QTextStream>

using namespace TextEditor;

Q_DECLARE_METATYPE(TextEditor::IFallbackPreferences *)

FallbackSelectorWidget::FallbackSelectorWidget(QWidget *parent) :
    QWidget(parent),
    m_fallbackPreferences(0),
    m_layout(0),
    m_checkBox(0),
    m_comboBox(0),
    m_comboBoxLabel(0),
    m_fallbackWidgetVisible(true)
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

        if (fallbacks.count() == 1) {
            m_checkBox = new QCheckBox(this);
            m_layout->addWidget(m_checkBox);
            m_layout->addStretch();
            m_checkBox->setText(tr("Use %1 settings").arg(fallbacks.at(0)->displayName()));
            connect(m_checkBox, SIGNAL(clicked(bool)),
                    this, SLOT(slotCheckBoxClicked(bool)));
        } else {
            m_comboBoxLabel = new QLabel(tr("Settings:"), this);
            m_layout->addWidget(m_comboBoxLabel);
            m_comboBox = new QComboBox(this);
            m_layout->addWidget(m_comboBox);
            m_layout->setStretch(1, 1);
            m_comboBox->addItem(tr("Custom"), QVariant::fromValue<TextEditor::IFallbackPreferences *>(0));
            connect(m_comboBox, SIGNAL(activated(int)),
                    this, SLOT(slotComboBoxActivated(int)));

            for (int i = 0; i < fallbacks.count(); i++) {
                IFallbackPreferences *fallback = fallbacks.at(i);
                QString displayName = fallback->displayName();
                if (!displayName.isEmpty())
                    displayName[0] = displayName[0].toUpper();
                m_comboBox->insertItem(i, displayName, QVariant::fromValue(fallback));
            }
        }
        slotCurrentFallbackChanged(m_fallbackPreferences->currentFallback());

        connect(m_fallbackPreferences, SIGNAL(currentFallbackChanged(TextEditor::IFallbackPreferences*)),
                this, SLOT(slotCurrentFallbackChanged(TextEditor::IFallbackPreferences*)));
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

void FallbackSelectorWidget::slotCheckBoxClicked(bool checked)
{
    TextEditor::IFallbackPreferences *fallback = 0;
    if (checked && !m_fallbackPreferences->fallbacks().isEmpty())
        fallback = m_fallbackPreferences->fallbacks().first();

    const bool wasBlocked = blockSignals(true);
    m_fallbackPreferences->setCurrentFallback(fallback);
    blockSignals(wasBlocked);
}

void FallbackSelectorWidget::slotCurrentFallbackChanged(TextEditor::IFallbackPreferences *fallback)
{
    const bool wasBlocked = blockSignals(true);
    if (m_comboBox)
        m_comboBox->setCurrentIndex(m_comboBox->findData(QVariant::fromValue(fallback)));
    if (m_checkBox)
        m_checkBox->setChecked(fallback);
    blockSignals(wasBlocked);
}

void FallbackSelectorWidget::setFallbacksVisible(bool on)
{
    m_fallbackWidgetVisible = on;
    if (m_fallbackPreferences)
        setVisible(m_fallbackWidgetVisible && !m_fallbackPreferences->fallbacks().isEmpty());
}

QString FallbackSelectorWidget::searchKeywords() const
{
    // no useful keywords here
    return QString();
}
