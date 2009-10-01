/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "findincurrentfile.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QtDebug>
#include <QtCore/QSettings>
#include <QtCore/QDirIterator>
#include <QtGui/QPushButton>
#include <QtGui/QFileDialog>
#include <QtGui/QVBoxLayout>

using namespace Find;
using namespace TextEditor::Internal;

FindInCurrentFile::FindInCurrentFile(SearchResultWindow *resultWindow)
  : BaseFileFind(resultWindow),
    m_configWidget(0),
    m_currentFile(0)
{
    connect(Core::ICore::instance()->editorManager(), SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(handleFileChange(Core::IEditor*)));
    handleFileChange(Core::ICore::instance()->editorManager()->currentEditor());
}

QString FindInCurrentFile::id() const
{
    return "Current File";
}

QString FindInCurrentFile::name() const
{
    return tr("Current File");
}

QKeySequence FindInCurrentFile::defaultShortcut() const
{
    return QKeySequence();
}

QStringList FindInCurrentFile::files()
{
    QStringList fileList;
    if (isEnabled())
        fileList << m_currentFile->fileName();
    return fileList;
}

bool FindInCurrentFile::isEnabled() const
{
    return m_currentFile && !m_currentFile->fileName().isEmpty();
}

void FindInCurrentFile::handleFileChange(Core::IEditor *editor)
{
    if (!editor) {
        if (m_currentFile) {
            m_currentFile = 0;
            emit changed();
        }
    } else {
        Core::IFile *file = editor->file();
        if (file != m_currentFile) {
            m_currentFile = file;
            emit changed();
        }
    }
}


QWidget *FindInCurrentFile::createConfigWidget()
{
    if (!m_configWidget) {
        m_configWidget = new QWidget;
        QGridLayout * const gridLayout = new QGridLayout(m_configWidget);
        gridLayout->setMargin(0);
        m_configWidget->setLayout(gridLayout);
        gridLayout->addWidget(createRegExpWidget(), 0, 1, 1, 2);
        // just for the layout HACK
        QLabel * const filePatternLabel = new QLabel;
        filePatternLabel->setMinimumWidth(80);
        filePatternLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        gridLayout->addWidget(filePatternLabel, 0, 0);
    }
    return m_configWidget;
}

void FindInCurrentFile::writeSettings(QSettings *settings)
{
    settings->beginGroup("FindInCurrentFile");
    writeCommonSettings(settings);
    settings->endGroup();
}

void FindInCurrentFile::readSettings(QSettings *settings)
{
    settings->beginGroup("FindInCurrentFile");
    readCommonSettings(settings, "*.cpp,*.h");
    settings->endGroup();
}
