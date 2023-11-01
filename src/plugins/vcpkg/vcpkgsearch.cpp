// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcpkgsearch.h"

#include "qpushbutton.h"
#include "vcpkgsettings.h"
#include "vcpkgtr.h"

#include <utils/algorithm.h>
#include <utils/fancylineedit.h>
#include <utils/fileutils.h>
#include <utils/infolabel.h>
#include <utils/itemviews.h>
#include <utils/layoutbuilder.h>

#include <coreplugin/icore.h>

#include <QDialogButtonBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextBrowser>

using namespace Utils;

namespace Vcpkg::Internal::Search {

class VcpkgPackageSearchDialog : public QDialog
{
public:
    explicit VcpkgPackageSearchDialog(const VcpkgManifest &preexistingPackages, QWidget *parent);

    VcpkgManifest selectedPackage() const;

private:
    void listPackages(const QString &filter);
    void showPackageDetails(const QString &packageName);
    void updateStatus();

    VcpkgManifests m_allPackages;
    VcpkgManifest m_selectedPackage;

    const VcpkgManifest m_projectManifest;

    FancyLineEdit *m_packagesFilter;
    ListWidget *m_packagesList;
    QLineEdit *m_vcpkgName;
    QLabel *m_vcpkgVersion;
    QLabel *m_vcpkgLicense;
    QTextBrowser *m_vcpkgDescription;
    QLabel *m_vcpkgHomepage;
    InfoLabel *m_infoLabel;
    QDialogButtonBox *m_buttonBox;
};

VcpkgPackageSearchDialog::VcpkgPackageSearchDialog(const VcpkgManifest &preexistingPackages,
                                                   QWidget *parent)
    : QDialog(parent)
    , m_projectManifest(preexistingPackages)
{
    resize(920, 400);
    setWindowTitle(Tr::tr("Add vcpkg Package"));

    m_packagesFilter = new FancyLineEdit;
    m_packagesFilter->setFiltering(true);
    m_packagesFilter->setFocus();
    m_packagesFilter->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

    m_packagesList = new ListWidget;
    m_packagesList->setMaximumWidth(300);

    m_vcpkgName = new QLineEdit;
    m_vcpkgName->setReadOnly(true);

    m_vcpkgVersion = new QLabel;
    m_vcpkgLicense = new QLabel;
    m_vcpkgDescription = new QTextBrowser;

    m_vcpkgHomepage = new QLabel;
    m_vcpkgHomepage->setOpenExternalLinks(true);
    m_vcpkgHomepage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_vcpkgHomepage->setTextInteractionFlags(Qt::TextBrowserInteraction);

    m_infoLabel = new InfoLabel(Tr::tr("This package is already a project dependency."),
                                InfoLabel::Information);
    m_infoLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    // clang-format off
    using namespace Layouting;
    Column {
        Row {
            Column {
                m_packagesFilter,
                Tr::tr("Packages:"),
                m_packagesList,
            },
            Group {
                title(Tr::tr("Package Details")),
                Form {
                    Tr::tr("Name:"), m_vcpkgName, br,
                    Tr::tr("Version:"), m_vcpkgVersion, br,
                    Tr::tr("License:"), m_vcpkgLicense, br,
                    Tr::tr("Description:"), m_vcpkgDescription, br,
                    Tr::tr("Homepage:"), m_vcpkgHomepage, br,
                },
            }
        },
        Row { m_infoLabel, m_buttonBox },
    }.attachTo(this);
    // clang-format on

    m_allPackages = vcpkgManifests(settings().vcpkgRoot());

    listPackages({});
    updateStatus();

    connect(m_packagesFilter, &FancyLineEdit::filterChanged,
            this, &VcpkgPackageSearchDialog::listPackages);
    connect(m_packagesList, &ListWidget::currentTextChanged,
            this, &VcpkgPackageSearchDialog::showPackageDetails);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

VcpkgManifest VcpkgPackageSearchDialog::selectedPackage() const
{
    return m_selectedPackage;
}

void VcpkgPackageSearchDialog::listPackages(const QString &filter)
{
    const VcpkgManifests filteredPackages = filtered(m_allPackages,
                                                     [&filter] (const VcpkgManifest &package) {
        return filter.isEmpty()
               || package.name.contains(filter, Qt::CaseInsensitive)
               || package.shortDescription.contains(filter, Qt::CaseInsensitive)
               || package.description.contains(filter, Qt::CaseInsensitive);
    });
    QStringList names = transform(filteredPackages, [] (const VcpkgManifest &package) {
        return package.name;
    });
    names.sort();
    m_packagesList->clear();
    m_packagesList->addItems(names);
}

void VcpkgPackageSearchDialog::showPackageDetails(const QString &packageName)
{
    const VcpkgManifest manifest = findOrDefault(m_allPackages,
                                                 [&packageName] (const VcpkgManifest &m) {
        return m.name == packageName;
    });

    m_vcpkgName->setText(manifest.name);
    m_vcpkgVersion->setText(manifest.version);
    m_vcpkgLicense->setText(manifest.license);
    QString description = manifest.shortDescription;
    if (!manifest.description.isEmpty())
        description.append("<p>" + manifest.description.join("</p><p>") + "</p>");
    m_vcpkgDescription->setText(description);
    m_vcpkgHomepage->setText(QString::fromLatin1("<a href=\"%1\">%1</a>")
                                 .arg(manifest.homepage.toDisplayString()));

    m_selectedPackage = manifest;
    updateStatus();
}

void VcpkgPackageSearchDialog::updateStatus()
{
    const QString package = selectedPackage().name;
    const bool isProjectDependency = m_projectManifest.dependencies.contains(package);
    m_infoLabel->setVisible(isProjectDependency);
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!(package.isEmpty()
                                                            || isProjectDependency));
}

VcpkgManifest parseVcpkgManifest(const QByteArray &vcpkgManifestJsonData, bool *ok)
{
    // https://learn.microsoft.com/en-us/vcpkg/reference/vcpkg-json
    VcpkgManifest result;
    const QJsonObject jsonObject = QJsonDocument::fromJson(vcpkgManifestJsonData).object();
    if (const QJsonValue name = jsonObject.value("name"); !name.isUndefined())
        result.name = name.toString();
    for (const char *key : {"version", "version-semver", "version-date", "version-string"} ) {
        if (const QJsonValue ver = jsonObject.value(QLatin1String(key)); !ver.isUndefined()) {
            result.version = ver.toString();
            break;
        }
    }
    if (const QJsonValue license = jsonObject.value("license"); !license.isUndefined())
        result.license = license.toString();
    if (const QJsonValue deps = jsonObject.value("dependencies"); !deps.isUndefined()) {
        const QJsonArray dependencies = deps.toArray();
        for (const QJsonValue &dependency : dependencies) {
            if (dependency.isString())
                result.dependencies.append(dependency.toString());
            else if (const QJsonValue name = dependency.toObject().value("name"); name.isString())
                result.dependencies.append(name.toString());
        }
    }
    if (const QJsonValue description = jsonObject.value("description"); !description.isUndefined()) {
        if (description.isArray()) {
            const QJsonArray descriptionLines = description.toArray();
            for (const QJsonValue &val : descriptionLines) {
                const QString line = val.toString();
                if (result.shortDescription.isEmpty()) {
                    result.shortDescription = line;
                    continue;
                }
                result.description.append(line);
            }
        } else {
            result.shortDescription = description.toString();
        }
    }
    if (const QJsonValue homepage = jsonObject.value("homepage"); !homepage.isUndefined())
        result.homepage = QUrl::fromUserInput(homepage.toString());

    if (ok)
        *ok = !(result.name.isEmpty() || result.version.isEmpty());

    return result;
}

VcpkgManifests vcpkgManifests(const FilePath &vcpkgRoot)
{
    const FilePath portsDir = vcpkgRoot / "ports";
    VcpkgManifests result;
    const FilePaths manifestFiles =
            portsDir.dirEntries({{"vcpkg.json"}, QDir::Files, QDirIterator::Subdirectories});
    for (const FilePath &manifestFile : manifestFiles) {
        FileReader reader;
        if (reader.fetch(manifestFile)) {
            const QByteArray &manifestData = reader.data();
            const VcpkgManifest manifest = parseVcpkgManifest(manifestData);
            result.append(manifest);
        }
    }
    return result;
}

VcpkgManifest showVcpkgPackageSearchDialog(const VcpkgManifest &projectManifest, QWidget *parent)
{
    QWidget *dlgParent = parent ? parent : Core::ICore::dialogParent();
    VcpkgPackageSearchDialog dlg(projectManifest, dlgParent);
    const VcpkgManifest result = (dlg.exec() == QDialog::Accepted) ? dlg.selectedPackage()
                                                                   : VcpkgManifest();
    return result;
}

} // namespace Vcpkg::Internal::Search
