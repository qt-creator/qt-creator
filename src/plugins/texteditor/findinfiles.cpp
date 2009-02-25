/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "findinfiles.h"

#include <QtDebug>
#include <QtCore/QDirIterator>
#include <QtGui/QPushButton>
#include <QtGui/QFileDialog>
#include <QtGui/QVBoxLayout>

using namespace Find;
using namespace TextEditor::Internal;

FindInFiles::FindInFiles(SearchResultWindow *resultWindow)
  : BaseFileFind(resultWindow),
    m_configWidget(0),
    m_directory(0)
{
}

QString FindInFiles::name() const
{
    return tr("Files on Disk");
}

QKeySequence FindInFiles::defaultShortcut() const
{
    return QKeySequence();
}

void FindInFiles::findAll(const QString &txt, QTextDocument::FindFlags findFlags)
{
    updateComboEntries(m_directory, true);
    BaseFileFind::findAll(txt, findFlags);
}

QStringList FindInFiles::files()
{
    QStringList fileList;
    QDirIterator it(m_directory->currentText(),
                    fileNameFilters(),
                    QDir::Files|QDir::Readable,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        fileList << it.filePath();
    }
    return fileList;
}

QWidget *FindInFiles::createConfigWidget()
{
    if (!m_configWidget) {
        m_configWidget = new QWidget;
        QGridLayout * const gridLayout = new QGridLayout(m_configWidget);
        gridLayout->setMargin(0);
        m_configWidget->setLayout(gridLayout);
        gridLayout->addWidget(createRegExpWidget(), 0, 1, 1, 2);

        QLabel *dirLabel = new QLabel(tr("&Directory:"));
        gridLayout->addWidget(dirLabel, 1, 0, Qt::AlignRight);
        m_directory = new QComboBox;
        m_directory->setEditable(true);
        m_directory->setMaxCount(30);
        m_directory->setMinimumContentsLength(10);
        m_directory->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        m_directory->setInsertPolicy(QComboBox::InsertAtTop);
        m_directory->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_directory->setModel(&m_directoryStrings);
        syncComboWithSettings(m_directory, m_directorySetting);
        dirLabel->setBuddy(m_directory);
        gridLayout->addWidget(m_directory, 1, 1);
        QPushButton *browseButton = new QPushButton(tr("&Browse"));
        gridLayout->addWidget(browseButton, 1, 2);
        connect(browseButton, SIGNAL(clicked()), this, SLOT(openFileBrowser()));

        QLabel * const filePatternLabel = new QLabel(tr("File &pattern:"));
        filePatternLabel->setMinimumWidth(80);
        filePatternLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        filePatternLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        QWidget *patternWidget = createPatternWidget();
        filePatternLabel->setBuddy(patternWidget);
        gridLayout->addWidget(filePatternLabel, 2, 0);
        gridLayout->addWidget(patternWidget, 2, 1, 1, 2);
        m_configWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    }
    return m_configWidget;
}

void FindInFiles::openFileBrowser()
{
    if (!m_directory)
        return;
    QString dir = QFileDialog::getExistingDirectory(m_configWidget,
        tr("Directory to search"));
    if (!dir.isEmpty())
        m_directory->setEditText(dir);
}

void FindInFiles::writeSettings(QSettings *settings)
{
    settings->beginGroup("FindInFiles");
    writeCommonSettings(settings);
    settings->setValue("directories", m_directoryStrings.stringList());
    if (m_directory)
        settings->setValue("currentDirectory", m_directory->currentText());
    settings->endGroup();
}

void FindInFiles::readSettings(QSettings *settings)
{
    settings->beginGroup("FindInFiles");
    readCommonSettings(settings, "*.cpp,*.h");
    m_directoryStrings.setStringList(settings->value("directories").toStringList());
    m_directorySetting = settings->value("currentDirectory").toString();
    settings->endGroup();
    syncComboWithSettings(m_directory, m_directorySetting);
}
