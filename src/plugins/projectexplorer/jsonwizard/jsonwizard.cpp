/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "jsonwizard.h"

#include "jsonwizardexpander.h"
#include "jsonwizardgeneratorfactory.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QMessageBox>
#include <QVariant>

namespace ProjectExplorer {

JsonWizard::JsonWizard(QWidget *parent) :
    Utils::Wizard(parent),
    m_expander(new Internal::JsonWizardExpander(this))
{ }

JsonWizard::~JsonWizard()
{
    qDeleteAll(m_generators);
    delete m_expander;
}

void JsonWizard::addGenerator(JsonWizardGenerator *gen)
{
    QTC_ASSERT(gen, return);
    QTC_ASSERT(!m_generators.contains(gen), return);

    m_generators.append(gen);
}

Utils::AbstractMacroExpander *JsonWizard::expander() const
{
    return m_expander;
}

void JsonWizard::resetFileList()
{
    m_files.clear();
}

JsonWizard::GeneratorFiles JsonWizard::fileList()
{
    QString errorMessage;
    GeneratorFiles list;

    QString targetPath = value(QLatin1String("TargetPath")).toString();
    if (targetPath.isEmpty()) {
        errorMessage = tr("Could not determine target path. \"TargetPath\" was not set on any page.");
        return list;
    }

    if (m_files.isEmpty()) {
        emit preGenerateFiles();
        foreach (JsonWizardGenerator *gen, m_generators) {
            Core::GeneratedFiles tmp = gen->fileList(m_expander, value(QStringLiteral("WizardDir")).toString(),
                                                     targetPath, &errorMessage);
            if (!errorMessage.isEmpty())
                break;
            list.append(Utils::transform(tmp, [&gen](const Core::GeneratedFile &f)
                                              { return JsonWizard::GeneratorFile(f, gen); }));
        }

        if (errorMessage.isEmpty())
            m_files = list;
        emit postGenerateFiles(m_files);
    }

    if (!errorMessage.isEmpty()) {
        QMessageBox::critical(this, tr("File Generation Failed"),
                              tr("The wizard failed to generate files.<br>"
                                 "The error message was: \"%1\".").arg(errorMessage));
        reject();
        return GeneratorFiles();
    }

    return m_files;
}

QVariant JsonWizard::value(const QString &n) const
{
    QVariant v = property(n.toUtf8());
    if (v.isValid()) {
        if (v.type() == QVariant::String)
            return Utils::expandMacros(v.toString(), m_expander);
        else
            return v;
    }
    if (hasField(n))
        return field(n); // Can not contain macros!
    return QVariant();
}

void JsonWizard::setValue(const QString &key, const QVariant &value)
{
    setProperty(key.toUtf8(), value);
}

bool JsonWizard::boolFromVariant(const QVariant &v, Utils::AbstractMacroExpander *expander)
{
    if (v.type() == QVariant::String)
        return !Utils::expandMacros(v.toString(), expander).isEmpty();
    return v.toBool();
}

void JsonWizard::removeAttributeFromAllFiles(Core::GeneratedFile::Attribute a)
{
    for (int i = 0; i < m_files.count(); ++i)
        m_files[i].file.setAttributes(m_files.at(i).file.attributes() ^ a);
}

void JsonWizard::accept()
{
    Utils::Wizard::accept();

    QString errorMessage;
    GeneratorFiles list = fileList();

    if (list.isEmpty())
        return;

    emit prePromptForOverwrite(m_files);
    JsonWizardGenerator::OverwriteResult overwrite =
            JsonWizardGenerator::promptForOverwrite(&list, &errorMessage);
    if (overwrite == JsonWizardGenerator::OverwriteError) {
        if (!errorMessage.isEmpty())
            QMessageBox::warning(this, tr("Failed to Overwrite Files"), errorMessage);
        return;
    }

    emit preFormatFiles(m_files);
    if (!JsonWizardGenerator::formatFiles(this, &list, &errorMessage)) {
        if (!errorMessage.isEmpty())
            QMessageBox::warning(this, tr("Failed to Format Files"), errorMessage);
        return;
    }

    emit preWriteFiles(m_files);
    if (!JsonWizardGenerator::writeFiles(this, &list, &errorMessage)) {
        if (!errorMessage.isEmpty())
            QMessageBox::warning(this, tr("Failed to Write Files"), errorMessage);
        return;
    }

    emit postProcessFiles(m_files);
    if (!JsonWizardGenerator::postWrite(this, &list, &errorMessage)) {
        if (!errorMessage.isEmpty())
            QMessageBox::warning(this, tr("Failed to Post-Process Files"), errorMessage);
        return;
    }
    emit filesReady(m_files);
    if (!JsonWizardGenerator::allDone(this, &list, &errorMessage)) {
        if (!errorMessage.isEmpty())
            QMessageBox::warning(this, tr("Failed to Open Files"), errorMessage);
        return;
    }

    emit allDone(m_files);
}

} // namespace ProjectExplorer
