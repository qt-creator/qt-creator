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

#include "diffshoweditor.h"
#include "diffeditorconstants.h"

#include <QToolBar>
#include <QToolButton>
#include <QCoreApplication>

#include <coreplugin/coreconstants.h>
#include <coreplugin/minisplitter.h>

#include <texteditor/basetexteditor.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/displaysettings.h>

using namespace TextEditor;

namespace DiffEditor {

namespace Internal {

class DiffShowEditorWidgetEditable : public BaseTextEditor
{
    Q_OBJECT
public:
    DiffShowEditorWidgetEditable(BaseTextEditorWidget *editorWidget) : BaseTextEditor(editorWidget) {}

    Core::Id id() const { return "DiffShowViewEditor"; }
    bool isTemporary() const { return false; }
};

class DiffShowEditorWidget : public BaseTextEditorWidget
{
    Q_OBJECT
public:
    DiffShowEditorWidget(QWidget *parent = 0);
    virtual QSize sizeHint() const;

public slots:
    void setDisplaySettings(const DisplaySettings &ds);

protected:
    BaseTextEditor *createEditor() { return new DiffShowEditorWidgetEditable(this); }

private:
};

DiffShowEditorWidget::DiffShowEditorWidget(QWidget *parent)
    : BaseTextEditorWidget(parent)
{
    DisplaySettings settings = displaySettings();
    settings.m_textWrapping = false;
    settings.m_displayLineNumbers = false;
    settings.m_highlightCurrentLine = false;
    settings.m_displayFoldingMarkers = false;
    settings.m_markTextChanges = false;
    settings.m_highlightBlocks = false;
    BaseTextEditorWidget::setDisplaySettings(settings);

    setCodeFoldingSupported(true);
    setFrameStyle(QFrame::NoFrame);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

QSize DiffShowEditorWidget::sizeHint() const
{
    QSize size = BaseTextEditorWidget::sizeHint();
    size.setHeight(size.height() / 5);
    return size;
}

void DiffShowEditorWidget::setDisplaySettings(const DisplaySettings &ds)
{
    DisplaySettings settings = displaySettings();
    settings.m_visualizeWhitespace = ds.m_visualizeWhitespace;
    BaseTextEditorWidget::setDisplaySettings(settings);
}

} // namespace Internal

DiffShowEditor::DiffShowEditor(DiffEditorWidget *editorWidget)
    : DiffEditor(editorWidget)
{
    QSplitter *splitter = new Core::MiniSplitter(Qt::Vertical);
    m_diffShowWidget = new Internal::DiffShowEditorWidget(splitter);
    m_diffShowWidget->setReadOnly(true);
    splitter->addWidget(m_diffShowWidget);
    splitter->addWidget(editorWidget);
    setWidget(splitter);

    TextEditorSettings *settings = TextEditorSettings::instance();
    connect(settings, SIGNAL(displaySettingsChanged(TextEditor::DisplaySettings)),
            m_diffShowWidget, SLOT(setDisplaySettings(TextEditor::DisplaySettings)));
    connect(settings, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
            m_diffShowWidget, SLOT(setFontSettings(TextEditor::FontSettings)));
    m_diffShowWidget->setDisplaySettings(settings->displaySettings());
    m_diffShowWidget->setCodeStyle(settings->codeStyle());
    m_diffShowWidget->setFontSettings(settings->fontSettings());
}

DiffShowEditor::~DiffShowEditor()
{
}

void DiffShowEditor::setDescription(const QString &description)
{
    m_diffShowWidget->setPlainText(description);
}

QString DiffShowEditor::displayName() const
{
    if (m_displayName.isEmpty())
        m_displayName = QCoreApplication::translate("DiffShowEditor", Constants::DIFF_SHOW_EDITOR_DISPLAY_NAME);
    return m_displayName;
}

void DiffShowEditor::setDisplayName(const QString &title)
{
    m_displayName = title;
    emit changed();
}

Core::Id DiffShowEditor::id() const
{
    return Constants::DIFF_SHOW_EDITOR_ID;
}

QWidget *DiffShowEditor::toolBar()
{
    if (m_toolWidget)
        return m_toolWidget;

    // Create
    DiffEditor::toolBar();

    m_toggleDescriptionButton = new QToolButton(m_toolWidget);
    m_toggleDescriptionButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_TOGGLE_TOPBAR)));
    m_toggleDescriptionButton->setCheckable(true);
    m_toggleDescriptionButton->setChecked(true);
    connect(m_toggleDescriptionButton, SIGNAL(clicked(bool)),
            this, SLOT(setDescriptionVisible(bool)));
    m_toolWidget->addWidget(m_toggleDescriptionButton);
    setDescriptionVisible(true);

    return m_toolWidget;
}

void DiffShowEditor::setDescriptionVisible(bool visible)
{
    if (visible) {
        m_toggleDescriptionButton->setToolTip(tr("Hide Change Description"));
    } else {
        m_toggleDescriptionButton->setToolTip(tr("Show Change Description"));
    }
    m_diffShowWidget->setVisible(visible);
}

} // namespace DiffEditor

#include "diffshoweditor.moc"
