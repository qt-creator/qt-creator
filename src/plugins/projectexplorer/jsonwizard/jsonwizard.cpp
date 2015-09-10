/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "jsonwizard.h"

#include "jsonwizardgeneratorfactory.h"

#include "../project.h"
#include "../projectexplorer.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/messagemanager.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/wizardpage.h>

#include <QFileInfo>
#include <QMessageBox>
#include <QVariant>

namespace ProjectExplorer {

JsonWizard::JsonWizard(QWidget *parent) :
    Utils::Wizard(parent)
{
    setMinimumSize(800, 500);
    m_expander.registerExtraResolver([this](const QString &name, QString *ret) -> bool {
        *ret = stringValue(name);
        return !ret->isNull();
    });
    m_expander.registerPrefix("Exists", tr("Check whether a variable exists. Returns \"true\" if it does and an empty string if not."),
                   [this](const QString &value) -> QString
    {
        const QString key = QString::fromLatin1("%{") + value + QLatin1Char('}');
        return m_expander.expand(key) == key ? QString() : QLatin1String("true");
    });

}

JsonWizard::~JsonWizard()
{
    qDeleteAll(m_generators);
}

void JsonWizard::addGenerator(JsonWizardGenerator *gen)
{
    QTC_ASSERT(gen, return);
    QTC_ASSERT(!m_generators.contains(gen), return);

    m_generators.append(gen);
}

Utils::MacroExpander *JsonWizard::expander()
{
    return &m_expander;
}

JsonWizard::GeneratorFiles JsonWizard::generateFileList()
{
    QString errorMessage;
    GeneratorFiles list;

    QString targetPath = stringValue(QLatin1String("TargetPath"));
    if (targetPath.isEmpty())
        errorMessage = tr("Could not determine target path. \"TargetPath\" was not set on any page.");

    if (m_files.isEmpty() && errorMessage.isEmpty()) {
        emit preGenerateFiles();
        foreach (JsonWizardGenerator *gen, m_generators) {
            Core::GeneratedFiles tmp = gen->fileList(&m_expander, stringValue(QStringLiteral("WizardDir")),
                                                     targetPath, &errorMessage);
            if (!errorMessage.isEmpty())
                break;
            list.append(Utils::transform(tmp, [&gen](const Core::GeneratedFile &f)
                                              { return JsonWizard::GeneratorFile(f, gen); }));
        }
    }

    if (!errorMessage.isEmpty()) {
        QMessageBox::critical(this, tr("File Generation Failed"),
                              tr("The wizard failed to generate files.<br>"
                                 "The error message was: \"%1\".").arg(errorMessage));
        reject();
        return GeneratorFiles();
    }

    return list;
}

void JsonWizard::commitToFileList(const JsonWizard::GeneratorFiles &list)
{
    m_files = list;
    emit postGenerateFiles(m_files);
}

QString JsonWizard::stringValue(const QString &n) const
{
    QVariant v = value(n);
    if (!v.isValid())
        return QString();

    if (v.type() == QVariant::String) {
        QString tmp = m_expander.expand(v.toString());
        if (tmp.isEmpty())
            tmp = QString::fromLatin1(""); // Make sure isNull() is *not* true.
        return tmp;
    }

    if (v.type() == QVariant::StringList)
        return stringListToArrayString(v.toStringList(), &m_expander);

    return v.toString();
}

void JsonWizard::setValue(const QString &key, const QVariant &value)
{
    setProperty(key.toUtf8(), value);
}

QVariant JsonWizard::value(const QString &n) const
{
    QVariant v = property(n.toUtf8());
    if (v.isValid())
        return v;
    if (hasField(n))
        return field(n); // Can not contain macros!
    return QVariant();
}

bool JsonWizard::boolFromVariant(const QVariant &v, Utils::MacroExpander *expander)
{
    if (v.type() == QVariant::String) {
        const QString tmp = expander->expand(v.toString());
        return !(tmp.isEmpty() || tmp == QLatin1String("false"));
    }
    return v.toBool();
}

QString JsonWizard::stringListToArrayString(const QStringList &list, const Utils::MacroExpander *expander)
{
    // Todo: Handle ' embedded in the strings better.
    if (list.isEmpty())
        return QString();

    QStringList tmp = Utils::transform(list, [expander](const QString &i) {
        return expander->expand(i).replace(QLatin1Char('\''), QLatin1String("\\'"));
    });

    QString result;
    result.append(QLatin1Char('\''));
    result.append(tmp.join(QLatin1String("', '")));
    result.append(QLatin1Char('\''));

    return result;
}

void JsonWizard::removeAttributeFromAllFiles(Core::GeneratedFile::Attribute a)
{
    for (int i = 0; i < m_files.count(); ++i) {
        if (m_files.at(i).file.attributes() & a)
            m_files[i].file.setAttributes(m_files.at(i).file.attributes() ^ a);
    }
}

QHash<QString, QVariant> JsonWizard::variables() const
{
    QHash<QString, QVariant> result = Wizard::variables();
    foreach (const QByteArray &p, dynamicPropertyNames()) {
        QString key = QString::fromUtf8(p);
        result.insert(key, value(key));
    }
    return result;
}

void JsonWizard::accept()
{
    auto page = qobject_cast<Utils::WizardPage *>(currentPage());
    if (page && page->handleAccept())
        return;

    Utils::Wizard::accept();

    QString errorMessage;
    if (m_files.isEmpty()) {
        commitToFileList(generateFileList()); // The Summary page does this for us, but a wizard
                                              // does not need to have one.
    }
    QTC_ASSERT(!m_files.isEmpty(), return);

    emit prePromptForOverwrite(m_files);
    JsonWizardGenerator::OverwriteResult overwrite =
            JsonWizardGenerator::promptForOverwrite(&m_files, &errorMessage);
    if (overwrite != JsonWizardGenerator::OverwriteOk) {
        if (!errorMessage.isEmpty())
            QMessageBox::warning(this, tr("Failed to Overwrite Files"), errorMessage);
        return;
    }

    emit preFormatFiles(m_files);
    if (!JsonWizardGenerator::formatFiles(this, &m_files, &errorMessage)) {
        if (!errorMessage.isEmpty())
            QMessageBox::warning(this, tr("Failed to Format Files"), errorMessage);
        return;
    }

    emit preWriteFiles(m_files);
    if (!JsonWizardGenerator::writeFiles(this, &m_files, &errorMessage)) {
        if (!errorMessage.isEmpty())
            QMessageBox::warning(this, tr("Failed to Write Files"), errorMessage);
        return;
    }

    emit postProcessFiles(m_files);
    if (!JsonWizardGenerator::postWrite(this, &m_files, &errorMessage)) {
        if (!errorMessage.isEmpty())
            QMessageBox::warning(this, tr("Failed to Post-Process Files"), errorMessage);
        return;
    }
    emit filesReady(m_files);
    if (!JsonWizardGenerator::polish(this, &m_files, &errorMessage)) {
        if (!errorMessage.isEmpty())
            QMessageBox::warning(this, tr("Failed to Polish Files"), errorMessage);
        return;
    }
    emit filesPolished(m_files);
    if (!JsonWizardGenerator::allDone(this, &m_files, &errorMessage)) {
        if (!errorMessage.isEmpty())
            QMessageBox::warning(this, tr("Failed to Open Files"), errorMessage);
        return;
    }
    emit allDone(m_files);

    openFiles(m_files);
}

void JsonWizard::reject()
{
    auto page = qobject_cast<Utils::WizardPage *>(currentPage());
    if (page && page->handleReject())
        return;

    Utils::Wizard::reject();
}

void JsonWizard::handleNewPages(int pageId)
{
    Utils::WizardPage *wp = qobject_cast<Utils::WizardPage *>(page(pageId));
    if (!wp)
        return;

    connect(wp, &Utils::WizardPage::reportError, this, &JsonWizard::handleError);
}

void JsonWizard::handleError(const QString &message)
{
    Core::MessageManager::write(message, Core::MessageManager::ModeSwitch);
}

QString JsonWizard::stringify(const QVariant &v) const
{
    if (v.type() == QVariant::StringList)
        return stringListToArrayString(v.toStringList(), &m_expander);
    return Wizard::stringify(v);
}

QString JsonWizard::evaluate(const QVariant &v) const
{
    return m_expander.expand(stringify(v));
}

void JsonWizard::openFiles(const JsonWizard::GeneratorFiles &files)
{
    QString errorMessage;
    bool openedSomething = false;
    foreach (const JsonWizard::GeneratorFile &f, files) {
        const Core::GeneratedFile &file = f.file;
        if (!QFileInfo(file.path()).exists()) {
            errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizard",
                                                       "\"%1\" does not exist in the file system.")
                    .arg(QDir::toNativeSeparators(file.path()));
            break;
        }
        if (file.attributes() & Core::GeneratedFile::OpenProjectAttribute) {
            Project *project = ProjectExplorerPlugin::instance()->openProject(file.path(), &errorMessage);
            if (!project) {
                if (errorMessage.isEmpty()) {
                    errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizard",
                                                               "Failed to open \"%1\" as a project.")
                            .arg(QDir::toNativeSeparators(file.path()));
                }
                break;
            }
            openedSomething = true;
        }
        if (file.attributes() & Core::GeneratedFile::OpenEditorAttribute) {
            if (!Core::EditorManager::openEditor(file.path(), file.editorId())) {
                errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizard",
                                                           "Failed to open an editor for \"%1\".")
                        .arg(QDir::toNativeSeparators(file.path()));
                break;
            }
            openedSomething = true;
        }
    }

    const QString path
            = QDir::toNativeSeparators(m_expander.expand(value(QLatin1String("TargetPath")).toString()));

    // Now try to find the project file and open
    if (!openedSomething) {
        errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizard",
                                                   "No file to open found in \"%1\".")
                .arg(path);
    }

    if (!errorMessage.isEmpty()) {
        const QString text = path.isEmpty() ? tr("Failed to open project.")
                                            : tr("Failed to open project in \"%1\".").arg(path);
        QMessageBox msgBox(QMessageBox::Warning, tr("Cannot Open Project"), text);
        msgBox.setDetailedText(errorMessage);
        msgBox.addButton(QMessageBox::Ok);
        msgBox.exec();
    }
}

} // namespace ProjectExplorer
