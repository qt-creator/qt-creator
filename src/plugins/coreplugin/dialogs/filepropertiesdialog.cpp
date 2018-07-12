/****************************************************************************
**
** Copyright (C) 2018 Andre Hartmann <aha_1980@gmx.de>
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

#include "filepropertiesdialog.h"
#include "ui_filepropertiesdialog.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <utils/fileutils.h>
#include <utils/mimetypes/mimedatabase.h>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLocale>

FilePropertiesDialog::FilePropertiesDialog(const Utils::FileName &fileName, QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::FilePropertiesDialog),
    m_fileName(fileName.toString())
{
    m_ui->setupUi(this);

    connect(m_ui->readable, &QCheckBox::clicked, [this](bool checked) {
        setPermission(QFile::ReadUser | QFile::ReadOwner, checked);
    });
    connect(m_ui->writable, &QCheckBox::clicked, [this](bool checked) {
        setPermission(QFile::WriteUser | QFile::WriteOwner, checked);
    });
    connect(m_ui->executable, &QCheckBox::clicked, [this](bool checked) {
        setPermission(QFile::ExeUser | QFile::ExeOwner, checked);
    });

    refresh();
}

FilePropertiesDialog::~FilePropertiesDialog()
{
    delete m_ui;
}

void FilePropertiesDialog::refresh()
{
    Utils::withNtfsPermissions<void>([this] {
        const QFileInfo fileInfo(m_fileName);
        QLocale locale;

        m_ui->name->setText(fileInfo.fileName());
        m_ui->path->setText(QDir::toNativeSeparators(fileInfo.canonicalPath()));

        m_ui->mimeType->setText(Utils::mimeTypeForFile(fileInfo).name());

        const Core::EditorFactoryList factories = Core::IEditorFactory::preferredEditorFactories(m_fileName);
        m_ui->defaultEditor->setText(!factories.isEmpty() ? factories.at(0)->displayName() : tr("Undefined"));

        m_ui->owner->setText(fileInfo.owner());
        m_ui->group->setText(fileInfo.group());
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
        m_ui->size->setText(locale.formattedDataSize(fileInfo.size()));
#else
        m_ui->size->setText(tr("%1 Bytes").arg(locale.toString(fileInfo.size())));
#endif
        m_ui->readable->setChecked(fileInfo.isReadable());
        m_ui->writable->setChecked(fileInfo.isWritable());
        m_ui->executable->setChecked(fileInfo.isExecutable());
        m_ui->symLink->setChecked(fileInfo.isSymLink());
        m_ui->lastRead->setText(fileInfo.lastRead().toString(locale.dateTimeFormat()));
        m_ui->lastModified->setText(fileInfo.lastModified().toString(locale.dateTimeFormat()));
    });
}

void FilePropertiesDialog::setPermission(QFile::Permissions newPermissions, bool set)
{
    Utils::withNtfsPermissions<void>([this, newPermissions, set] {
        QFile::Permissions permissions = QFile::permissions(m_fileName);
        if (set)
            permissions |= newPermissions;
        else
            permissions &= ~newPermissions;

        if (!QFile::setPermissions(m_fileName, permissions))
            qWarning() << "Cannot change permissions for file" << m_fileName;
    });

    refresh();
}
