/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/


#include "clangformatconfigwidget.h"
#include "ui_clangformatconfigwidget.h"

#include <clang/Format/Format.h>

#include <coreplugin/icore.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

#include <QFile>

#include <sstream>

using namespace ProjectExplorer;

namespace ClangFormat {
namespace Internal {

static void createGlobalClangFormatFileIfNeeded(const QString &settingsDir)
{
    const QString fileName = settingsDir + "/.clang-format";
    if (QFile::exists(fileName))
        return;

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly))
        return;

    const clang::format::FormatStyle defaultStyle = clang::format::getLLVMStyle();
    const std::string configuration = clang::format::configurationAsText(defaultStyle);
    file.write(configuration.c_str());
    file.close();
}

static void readTable(QTableWidget *table, std::istringstream &stream)
{
    table->horizontalHeader()->hide();
    table->verticalHeader()->hide();

    table->setColumnCount(2);
    table->setRowCount(0);

    std::string line;
    while (std::getline(stream, line)) {
        if (line == "---" || line == "...")
            continue;

        const size_t firstLetter = line.find_first_not_of(' ');
        if (firstLetter == std::string::npos || line.at(firstLetter) == '#')
            continue;

        // Increase indent where it already exists.
        if (firstLetter > 0 && firstLetter < 5)
            line = "    " + line;

        table->insertRow(table->rowCount());
        const size_t colonPos = line.find_first_of(':');
        auto *keyItem = new QTableWidgetItem;
        auto *valueItem = new QTableWidgetItem;

        keyItem->setFlags(keyItem->flags() & ~Qt::ItemFlags(Qt::ItemIsEditable));
        table->setItem(table->rowCount() - 1, 0, keyItem);
        table->setItem(table->rowCount() - 1, 1, valueItem);

        if (colonPos == std::string::npos) {
            keyItem->setText(QString::fromStdString(line));
            valueItem->setFlags(valueItem->flags() & ~Qt::ItemFlags(Qt::ItemIsEditable));
            continue;
        }

        keyItem->setText(QString::fromStdString(line.substr(0, colonPos)));

        const size_t optionValueStart = line.find_first_not_of(' ', colonPos + 1);
        if (optionValueStart == std::string::npos)
            valueItem->setFlags(valueItem->flags() & ~Qt::ItemFlags(Qt::ItemIsEditable));
        else
            valueItem->setText(QString::fromStdString(line.substr(optionValueStart)));
    }

    table->resizeColumnToContents(0);
    table->resizeColumnToContents(1);
}

static QByteArray tableToYAML(QTableWidget *table)
{
    QByteArray text;
    text += "---\n";
    for (int i = 0; i < table->rowCount(); ++i) {
        auto *keyItem = table->item(i, 0);
        auto *valueItem = table->item(i, 1);

        QByteArray itemText = keyItem->text().toUtf8();
        // Change the indent back to 2 spaces
        itemText.replace("      ", "  ");
        if (!valueItem->text().isEmpty() || !itemText.trimmed().startsWith('-'))
            itemText += ": ";
        itemText += valueItem->text().toUtf8() + '\n';

        text += itemText;
    }
    text += "...\n";

    return text;
}

ClangFormatConfigWidget::ClangFormatConfigWidget(ProjectExplorer::Project *project,
                                                 QWidget *parent)
    : QWidget(parent)
    , m_project(project)
    , m_ui(std::make_unique<Ui::ClangFormatConfigWidget>())
{
    m_ui->setupUi(this);

    std::string testFilePath;
    if (m_project && !m_project->projectDirectory().appendPath(".clang-format").exists()) {
        m_ui->projectHasClangFormat->setText("No .clang-format file for the project");
        m_ui->clangFormatOptionsTable->hide();
        m_ui->applyButton->hide();
        return;
    }

    if (m_project) {
        testFilePath = m_project->projectDirectory().appendPath("t.cpp").toString().toStdString();
        connect(m_ui->applyButton, &QPushButton::clicked, this, &ClangFormatConfigWidget::apply);
    } else {
        const QString settingsDir = Core::ICore::userResourcePath();
        createGlobalClangFormatFileIfNeeded(settingsDir);
        testFilePath = settingsDir.toStdString() + "/t.cpp";
        m_ui->applyButton->hide();
    }

    llvm::Expected<clang::format::FormatStyle> formatStyle =
        clang::format::getStyle("file", testFilePath, "LLVM", "");
    if (!formatStyle)
        return;

    const std::string configText = clang::format::configurationAsText(*formatStyle);
    std::istringstream stream(configText);

    readTable(m_ui->clangFormatOptionsTable, stream);

    if (m_project) {
        m_ui->projectHasClangFormat->hide();
        return;
    }

    const Project *currentProject = SessionManager::startupProject();
    if (!currentProject || !currentProject->projectDirectory().appendPath(".clang-format").exists())
        m_ui->projectHasClangFormat->hide();

    connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
            this, [this](ProjectExplorer::Project *project) {
        if (project && project->projectDirectory().appendPath(".clang-format").exists())
            m_ui->projectHasClangFormat->show();
        else
            m_ui->projectHasClangFormat->hide();
    });
}

ClangFormatConfigWidget::~ClangFormatConfigWidget() = default;

void ClangFormatConfigWidget::apply()
{
    const QByteArray text = tableToYAML(m_ui->clangFormatOptionsTable);
    QString filePath;
    if (m_project)
        filePath = m_project->projectDirectory().appendPath(".clang-format").toString();
    else
        filePath = Core::ICore::userResourcePath() + "/.clang-format";
    QFile file(filePath);
    if (!file.open(QFile::WriteOnly))
        return;

    file.write(text);
    file.close();
}

} // namespace Internal
} // namespace ClangFormat
