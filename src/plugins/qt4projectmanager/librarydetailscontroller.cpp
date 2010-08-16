#include "librarydetailscontroller.h"
#include "ui_librarydetailswidget.h"
#include "findqt4profiles.h"
#include "qt4nodes.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QTextStream>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

LibraryDetailsController::LibraryDetailsController(
        Ui::LibraryDetailsWidget *libraryDetails, QObject *parent) :
    QObject(parent),
    m_platforms(AddLibraryWizard::LinuxPlatform
                | AddLibraryWizard::MacPlatform
                | AddLibraryWizard::WindowsPlatform
                | AddLibraryWizard::SymbianPlatform),
    m_linkageType(AddLibraryWizard::NoLinkage),
    m_macLibraryType(AddLibraryWizard::NoLibraryType),
    m_ignoreGuiSignals(false),
    m_includePathChanged(false),
    m_linkageRadiosVisible(true),
    m_macLibraryRadiosVisible(true),
    m_includePathVisible(true),
    m_windowsGroupVisible(true),
    m_libraryDetailsWidget(libraryDetails)
{
#ifdef Q_OS_MAC
    setMacLibraryRadiosVisible(false);
#endif

#ifndef Q_OS_WIN
    setLinkageRadiosVisible(false);
#endif

    connect(m_libraryDetailsWidget->includePathChooser, SIGNAL(changed(QString)),
            this, SLOT(slotIncludePathChanged()));
    connect(m_libraryDetailsWidget->frameworkRadio, SIGNAL(clicked(bool)),
            this, SLOT(slotMacLibraryTypeChanged()));
    connect(m_libraryDetailsWidget->libraryRadio, SIGNAL(clicked(bool)),
            this, SLOT(slotMacLibraryTypeChanged()));
    connect(m_libraryDetailsWidget->useSubfoldersCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotUseSubfoldersChanged(bool)));
    connect(m_libraryDetailsWidget->addSuffixCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotAddSuffixChanged(bool)));
    connect(m_libraryDetailsWidget->linCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(slotPlatformChanged()));
    connect(m_libraryDetailsWidget->macCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(slotPlatformChanged()));
    connect(m_libraryDetailsWidget->winCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(slotPlatformChanged()));
    connect(m_libraryDetailsWidget->symCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(slotPlatformChanged()));
}

void LibraryDetailsController::setProFile(const QString &proFile)
{
    m_proFile = proFile;
    proFileChanged();
    updateGui();

    emit completeChanged();
}

Ui::LibraryDetailsWidget *LibraryDetailsController::libraryDetailsWidget() const
{
    return m_libraryDetailsWidget;
}

AddLibraryWizard::Platforms LibraryDetailsController::platforms() const
{
    return m_platforms;
}

AddLibraryWizard::LinkageType LibraryDetailsController::linkageType() const
{
    return m_linkageType;
}

AddLibraryWizard::MacLibraryType LibraryDetailsController::macLibraryType() const
{
    return m_macLibraryType;
}

void LibraryDetailsController::updateGui()
{
    // read values from gui
    m_platforms = 0;
    if (libraryDetailsWidget()->linCheckBox->isChecked())
        m_platforms |= AddLibraryWizard::LinuxPlatform;
    if (libraryDetailsWidget()->macCheckBox->isChecked())
        m_platforms |= AddLibraryWizard::MacPlatform;
    if (libraryDetailsWidget()->winCheckBox->isChecked())
        m_platforms |= AddLibraryWizard::WindowsPlatform;
    if (libraryDetailsWidget()->symCheckBox->isChecked())
        m_platforms |= AddLibraryWizard::SymbianPlatform;

    bool macLibraryTypeUpdated = false;
    if (!m_linkageRadiosVisible) {
        m_linkageType = suggestedLinkageType();
        if (m_linkageType == AddLibraryWizard::StaticLinkage) {
            m_macLibraryType = AddLibraryWizard::LibraryType;
            macLibraryTypeUpdated = true;
        }
    } else {
        m_linkageType = AddLibraryWizard::DynamicLinkage; // the default
        if (libraryDetailsWidget()->staticRadio->isChecked())
            m_linkageType = AddLibraryWizard::StaticLinkage;
    }

    if (!macLibraryTypeUpdated) {
        if (!m_macLibraryRadiosVisible) {
            m_macLibraryType = suggestedMacLibraryType();
        } else {
            m_macLibraryType = AddLibraryWizard::LibraryType; // the default
            if (libraryDetailsWidget()->frameworkRadio->isChecked())
                m_macLibraryType = AddLibraryWizard::FrameworkType;
        }
    }

    // enable or disable some parts of gui
    libraryDetailsWidget()->macGroupBox->setEnabled(platforms()
                                & AddLibraryWizard::MacPlatform);
    updateWindowsOptionsEnablement();
    const bool macRadiosEnabled = m_linkageRadiosVisible ||
            linkageType() != AddLibraryWizard::StaticLinkage;
    libraryDetailsWidget()->libraryRadio->setEnabled(macRadiosEnabled);
    libraryDetailsWidget()->frameworkRadio->setEnabled(macRadiosEnabled);

    // update values in gui
    setIgnoreGuiSignals(true);

    showLinkageType(linkageType());
    showMacLibraryType(macLibraryType());
    if (!m_includePathChanged)
        libraryDetailsWidget()->includePathChooser->setPath(suggestedIncludePath());

    setIgnoreGuiSignals(false);
}

QString LibraryDetailsController::proFile() const
{
    return m_proFile;
}

bool LibraryDetailsController::isIncludePathChanged() const
{
    return m_includePathChanged;
}

void LibraryDetailsController::setIgnoreGuiSignals(bool ignore)
{
    m_ignoreGuiSignals = ignore;
}

bool LibraryDetailsController::guiSignalsIgnored() const
{
    return m_ignoreGuiSignals;
}

void LibraryDetailsController::showLinkageType(
        AddLibraryWizard::LinkageType linkageType)
{
    const QString linkage(tr("Linkage:"));
    QString linkageTitle;
    switch (linkageType) {
    case AddLibraryWizard::DynamicLinkage:
        libraryDetailsWidget()->dynamicRadio->setChecked(true);
        linkageTitle = tr("%1 Dynamic").arg(linkage);
        break;
    case AddLibraryWizard::StaticLinkage:
        libraryDetailsWidget()->staticRadio->setChecked(true);
        linkageTitle = tr("%1 Static").arg(linkage);
        break;
    default:
        libraryDetailsWidget()->dynamicRadio->setChecked(false);
        libraryDetailsWidget()->staticRadio->setChecked(false);
        linkageTitle = linkage;
        break;
        }
    libraryDetailsWidget()->linkageGroupBox->setTitle(linkageTitle);
}

void LibraryDetailsController::showMacLibraryType(
        AddLibraryWizard::MacLibraryType libType)
{
    const QString libraryType(tr("Mac:"));
    QString libraryTypeTitle;
    switch (libType) {
    case AddLibraryWizard::FrameworkType:
        libraryDetailsWidget()->frameworkRadio->setChecked(true);
        libraryTypeTitle = tr("%1 Framework").arg(libraryType);
        break;
    case AddLibraryWizard::LibraryType:
        libraryDetailsWidget()->libraryRadio->setChecked(true);
        libraryTypeTitle = tr("%1 Library").arg(libraryType);
        break;
    default:
        libraryDetailsWidget()->frameworkRadio->setChecked(false);
        libraryDetailsWidget()->libraryRadio->setChecked(false);
        libraryTypeTitle = libraryType;
        break;
    }
    libraryDetailsWidget()->macGroupBox->setTitle(libraryTypeTitle);
}

void LibraryDetailsController::setLinkageRadiosVisible(bool ena)
{
    m_linkageRadiosVisible = ena;
    libraryDetailsWidget()->staticRadio->setVisible(ena);
    libraryDetailsWidget()->dynamicRadio->setVisible(ena);
}

void LibraryDetailsController::setMacLibraryRadiosVisible(bool ena)
{
    m_macLibraryRadiosVisible = ena;
    libraryDetailsWidget()->frameworkRadio->setVisible(ena);
    libraryDetailsWidget()->libraryRadio->setVisible(ena);
}

void LibraryDetailsController::setLibraryPathChooserVisible(bool ena)
{
    libraryDetailsWidget()->libraryPathChooser->setVisible(ena);
    libraryDetailsWidget()->libraryFileLabel->setVisible(ena);
}

void LibraryDetailsController::setLibraryComboBoxVisible(bool ena)
{
    libraryDetailsWidget()->libraryComboBox->setVisible(ena);
    libraryDetailsWidget()->libraryLabel->setVisible(ena);
}

void LibraryDetailsController::setIncludePathVisible(bool ena)
{
    m_includePathVisible = ena;
    libraryDetailsWidget()->includeLabel->setVisible(ena);
    libraryDetailsWidget()->includePathChooser->setVisible(ena);
}

void LibraryDetailsController::setWindowsGroupVisible(bool ena)
{
    m_windowsGroupVisible = ena;
    libraryDetailsWidget()->winGroupBox->setVisible(ena);
}

void LibraryDetailsController::setRemoveSuffixVisible(bool ena)
{
    libraryDetailsWidget()->removeSuffixCheckBox->setVisible(ena);
}

bool LibraryDetailsController::isMacLibraryRadiosVisible() const
{
    return m_macLibraryRadiosVisible;
}

bool LibraryDetailsController::isIncludePathVisible() const
{
    return m_includePathVisible;
}

bool LibraryDetailsController::isWindowsGroupVisible() const
{
    return m_windowsGroupVisible;
}

void LibraryDetailsController::slotIncludePathChanged()
{
    if (m_ignoreGuiSignals)
        return;
    m_includePathChanged = true;
}

void LibraryDetailsController::slotPlatformChanged()
{
    updateGui();
    emit completeChanged();
}

void LibraryDetailsController::slotMacLibraryTypeChanged()
{
    if (guiSignalsIgnored())
        return;

    if (m_linkageRadiosVisible
            && libraryDetailsWidget()->frameworkRadio->isChecked()) {
        setIgnoreGuiSignals(true);
        libraryDetailsWidget()->dynamicRadio->setChecked(true);
        setIgnoreGuiSignals(true);
    }

    updateGui();
}

void LibraryDetailsController::slotUseSubfoldersChanged(bool ena)
{
    if (ena) {
        libraryDetailsWidget()->addSuffixCheckBox->setChecked(false);
        libraryDetailsWidget()->removeSuffixCheckBox->setChecked(false);
    }
}

void LibraryDetailsController::slotAddSuffixChanged(bool ena)
{
    if (ena) {
        libraryDetailsWidget()->useSubfoldersCheckBox->setChecked(false);
        libraryDetailsWidget()->removeSuffixCheckBox->setChecked(false);
    }
}

static QString appendSpaceIfNotEmpty(const QString &aString)
{
    if (aString.isEmpty())
        return aString;
    return aString + QLatin1Char(' ');
}

static QString appendSeparator(const QString &aString)
{
    if (aString.isEmpty())
        return aString;
    if (aString.at(aString.size() - 1) == QLatin1Char('/'))
        return aString;
    return aString + QLatin1Char('/');
}

static QString commonScopes(AddLibraryWizard::Platforms scopes,
                            AddLibraryWizard::Platforms excludedScopes)
{
    QString scopesString;
    QTextStream str(&scopesString);
    AddLibraryWizard::Platforms common = scopes | excludedScopes;
    bool unixLikeScopes = false;
    if (scopes & ~QFlags<AddLibraryWizard::Platform>(AddLibraryWizard::WindowsPlatform)) {
        unixLikeScopes = true;
        if (common & AddLibraryWizard::LinuxPlatform) {
            str << "unix";
            if (!(common & AddLibraryWizard::MacPlatform))
                str << ":!macx";
            if (!(common & AddLibraryWizard::SymbianPlatform))
                str << ":!symbian";
        } else {
            if (scopes & AddLibraryWizard::MacPlatform)
                str << "macx";
            if (scopes & AddLibraryWizard::MacPlatform &&
                    scopes & AddLibraryWizard::SymbianPlatform)
                str << "|";
            if (scopes & AddLibraryWizard::SymbianPlatform)
                str << "symbian";
        }
    }
    if (scopes & AddLibraryWizard::WindowsPlatform) {
        if (unixLikeScopes)
            str << "|";
        str << "win32";
    }
    return scopesString;
}

static QString generateLibsSnippet(AddLibraryWizard::Platforms platforms,
                     AddLibraryWizard::MacLibraryType macLibraryType,
                     const QString &libName,
                     const QString &targetRelativePath, const QString &pwd,
                     bool useSubfolders, bool addSuffix, bool generateLibPath)
{
    // if needed it contains: $$[pwd]/_PATH_
    const QString libraryPathSnippet = generateLibPath ?
                QLatin1String("$$") + pwd + QLatin1Char('/') +
                targetRelativePath : QString();

    // if needed it contains: -L$$[pwd]/_PATH_
    const QString simpleLibraryPathSnippet = generateLibPath ?
                QLatin1String("-L") + libraryPathSnippet : QString();
    // if needed it contains: -F$$[pwd]/_PATH_
    const QString macLibraryPathSnippet = generateLibPath ?
                QLatin1String("-F") + libraryPathSnippet : QString();

    AddLibraryWizard::Platforms commonPlatforms = platforms;
    if (macLibraryType == AddLibraryWizard::FrameworkType) // we will generate a separate -F -framework line
        commonPlatforms &= ~QFlags<AddLibraryWizard::Platform>(AddLibraryWizard::MacPlatform);
    if (useSubfolders || addSuffix) // we will generate a separate debug/release conditions
        commonPlatforms &= ~QFlags<AddLibraryWizard::Platform>(AddLibraryWizard::WindowsPlatform);
    if (generateLibPath) // we will generate a separate line without -L
        commonPlatforms &= ~QFlags<AddLibraryWizard::Platform>(AddLibraryWizard::SymbianPlatform);

    AddLibraryWizard::Platforms diffPlatforms = platforms ^ commonPlatforms;
    AddLibraryWizard::Platforms generatedPlatforms = 0;

    QString snippetMessage;
    QTextStream str(&snippetMessage);

    if (diffPlatforms & AddLibraryWizard::WindowsPlatform) {
        str << "win32:CONFIG(release, debug|release): LIBS += ";
        if (useSubfolders)
            str << simpleLibraryPathSnippet << "release/ " << "-l" << libName << "\n";
        else if (addSuffix)
            str << appendSpaceIfNotEmpty(simpleLibraryPathSnippet) << "-l" << libName << "\n";

        str << "else:win32:CONFIG(debug, debug|release): LIBS += ";
        if (useSubfolders)
            str << simpleLibraryPathSnippet << "debug/ " << "-l" << libName << "\n";
        else if (addSuffix)
            str << appendSpaceIfNotEmpty(simpleLibraryPathSnippet) << "-l" << libName << "d\n";
        generatedPlatforms |= AddLibraryWizard::WindowsPlatform;
    }
    if (diffPlatforms & AddLibraryWizard::MacPlatform) {
        if (generatedPlatforms)
            str << "else:";
        str << "mac: LIBS += " << appendSpaceIfNotEmpty(macLibraryPathSnippet)
                    << "-framework " << libName << "\n";
        generatedPlatforms |= AddLibraryWizard::MacPlatform;
    }
    if (diffPlatforms & AddLibraryWizard::SymbianPlatform) {
        if (generatedPlatforms)
            str << "else:";
        str << "symbian: LIBS += -l" << libName << "\n";
        generatedPlatforms |= AddLibraryWizard::SymbianPlatform;
    }

    if (commonPlatforms) {
        if (generatedPlatforms)
            str << "else:";
        str << commonScopes(commonPlatforms, generatedPlatforms) << ": LIBS += "
                << appendSpaceIfNotEmpty(simpleLibraryPathSnippet) << "-l" << libName << "\n";
    }
    return snippetMessage;
}

static QString generateIncludePathSnippet(const QString &includeRelativePath)
{
    return QLatin1String("\nINCLUDEPATH += $$PWD/")
            + includeRelativePath + QLatin1Char('\n')
            + QLatin1String("DEPENDPATH += $$PWD/")
            + includeRelativePath + QLatin1Char('\n');
}

static QString generatePreTargetDepsSnippet(AddLibraryWizard::Platforms platforms,
                                            AddLibraryWizard::LinkageType linkageType,
                                            const QString &libName,
                                            const QString &targetRelativePath, const QString &pwd,
                                            bool useSubfolders, bool addSuffix)
{
    if (linkageType != AddLibraryWizard::StaticLinkage)
        return QString();

    // if needed it contains: PRE_TARGETDEPS += $$[pwd]/_PATH_TO_LIB_WITHOUT_LIB_NAME_
    const QString preTargetDepsSnippet = QLatin1String("PRE_TARGETDEPS += $$") +
            pwd + QLatin1Char('/') + targetRelativePath;

    QString snippetMessage;
    QTextStream str(&snippetMessage);
    str << "\n";
    AddLibraryWizard::Platforms generatedPlatforms = 0;
    if (platforms & AddLibraryWizard::WindowsPlatform) {
        if (useSubfolders || addSuffix) {
            str << "win32:CONFIG(release, debug|release): "
                << preTargetDepsSnippet;
            if (useSubfolders)
                str << "release/" << libName << ".lib\n";
            else if (addSuffix)
                str << libName << ".lib\n";

            str << "else:win32:CONFIG(debug, debug|release): "
                << preTargetDepsSnippet;
            if (useSubfolders)
                str << "debug/" << libName << ".lib\n";
            else if (addSuffix)
                str << libName << "d.lib\n";
        } else {
            str << "win32: " << preTargetDepsSnippet << libName << ".lib\n";
        }
        generatedPlatforms |= AddLibraryWizard::WindowsPlatform;
    }
    AddLibraryWizard::Platforms commonPlatforms = platforms;
    commonPlatforms &= ~QFlags<AddLibraryWizard::Platform>(AddLibraryWizard::WindowsPlatform);
    // don't generate PRE_TARGETDEPS for symbian - relinking static lib apparently works without that
    commonPlatforms &= ~QFlags<AddLibraryWizard::Platform>(AddLibraryWizard::SymbianPlatform);
    if (commonPlatforms) {
        if (generatedPlatforms)
            str << "else:";
        str << commonScopes(commonPlatforms, generatedPlatforms) << ": "
            << preTargetDepsSnippet << "lib" << libName << ".a\n";
    }
    return snippetMessage;
}

NonInternalLibraryDetailsController::NonInternalLibraryDetailsController(
        Ui::LibraryDetailsWidget *libraryDetails, QObject *parent) :
    LibraryDetailsController(libraryDetails, parent)
{
    setLibraryComboBoxVisible(false);
    setLibraryPathChooserVisible(true);

#ifdef Q_OS_WIN
    libraryDetailsWidget()->libraryPathChooser->setPromptDialogFilter(
            QLatin1String("Library file (*.lib)"));
    setLinkageRadiosVisible(true);
    setRemoveSuffixVisible(true);
#else
    setLinkageRadiosVisible(false);
    setRemoveSuffixVisible(false);
#endif

#ifdef Q_OS_LINUX
    libraryDetailsWidget()->libraryPathChooser->setPromptDialogFilter(
            QLatin1String("Library file (lib*.so lib*.a)"));
#endif

#ifdef Q_OS_MAC
    libraryDetailsWidget()->libraryPathChooser->setPromptDialogFilter(
            QLatin1String("Library file (*.dylib *.a *.framework)"));
           // QLatin1String("Library file (lib*.dylib lib*.a *.framework)"));
    libraryDetailsWidget()->libraryPathChooser->setExpectedKind(Utils::PathChooser::Any);
#else
    libraryDetailsWidget()->libraryPathChooser->setExpectedKind(Utils::PathChooser::File);
#endif

    connect(libraryDetailsWidget()->libraryPathChooser, SIGNAL(validChanged()),
            this, SIGNAL(completeChanged()));
    connect(libraryDetailsWidget()->libraryPathChooser, SIGNAL(changed(QString)),
            this, SLOT(slotLibraryPathChanged()));
    connect(libraryDetailsWidget()->removeSuffixCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotRemoveSuffixChanged(bool)));
    connect(libraryDetailsWidget()->dynamicRadio, SIGNAL(clicked(bool)),
            this, SLOT(slotLinkageTypeChanged()));
    connect(libraryDetailsWidget()->staticRadio, SIGNAL(clicked(bool)),
            this, SLOT(slotLinkageTypeChanged()));
}

AddLibraryWizard::LinkageType NonInternalLibraryDetailsController::suggestedLinkageType() const
{
    AddLibraryWizard::LinkageType type = AddLibraryWizard::NoLinkage;
#ifndef Q_OS_WIN
    if (libraryDetailsWidget()->libraryPathChooser->isValid()) {
        QFileInfo fi(libraryDetailsWidget()->libraryPathChooser->path());
        if (fi.suffix() == QLatin1String("a"))
            type = AddLibraryWizard::StaticLinkage;
        else
            type = AddLibraryWizard::DynamicLinkage;
    }
#endif
    return type;
}

AddLibraryWizard::MacLibraryType NonInternalLibraryDetailsController::suggestedMacLibraryType() const
{
    AddLibraryWizard::MacLibraryType type = AddLibraryWizard::NoLibraryType;
#ifdef Q_OS_MAC
    if (libraryDetailsWidget()->libraryPathChooser->isValid()) {
        QFileInfo fi(libraryDetailsWidget()->libraryPathChooser->path());
        if (fi.suffix() == QLatin1String("framework"))
            type = AddLibraryWizard::FrameworkType;
        else
            type = AddLibraryWizard::LibraryType;
    }
#endif
    return type;
}

QString NonInternalLibraryDetailsController::suggestedIncludePath() const
{
    QString includePath;
    if (libraryDetailsWidget()->libraryPathChooser->isValid()) {
        QFileInfo fi(libraryDetailsWidget()->libraryPathChooser->path());
        includePath = fi.absolutePath();
        QFileInfo dfi(includePath);
        // TODO: Win: remove debug or release folder first if appropriate
        if (dfi.fileName() == QLatin1String("lib")) {
            QDir dir = dfi.absoluteDir();
            includePath = dir.absolutePath();
            QDir includeDir(dir.absoluteFilePath(QLatin1String("include")));
            if (includeDir.exists())
                includePath = includeDir.absolutePath();
        }
    }
    return includePath;
}

void NonInternalLibraryDetailsController::updateWindowsOptionsEnablement()
{
    bool ena = platforms() & AddLibraryWizard::WindowsPlatform;
#ifdef Q_OS_WIN
    libraryDetailsWidget()->addSuffixCheckBox->setEnabled(ena);
    ena = true;
#endif
    libraryDetailsWidget()->winGroupBox->setEnabled(ena);
}

void NonInternalLibraryDetailsController::slotLinkageTypeChanged()
{
    if (guiSignalsIgnored())
        return;

    if (isMacLibraryRadiosVisible()
            && libraryDetailsWidget()->staticRadio->isChecked()) {
        setIgnoreGuiSignals(true);
        libraryDetailsWidget()->libraryRadio->setChecked(true);
        setIgnoreGuiSignals(true);
    }

    updateGui();
}

void NonInternalLibraryDetailsController::slotRemoveSuffixChanged(bool ena)
{
    if (ena) {
        libraryDetailsWidget()->useSubfoldersCheckBox->setChecked(false);
        libraryDetailsWidget()->addSuffixCheckBox->setChecked(false);
    }
}

void NonInternalLibraryDetailsController::slotLibraryPathChanged()
{
#ifdef Q_OS_WIN
    bool subfoldersEnabled = true;
    bool removeSuffixEnabled = true;
    if (libraryDetailsWidget()->libraryPathChooser->isValid()) {
        QFileInfo fi(libraryDetailsWidget()->libraryPathChooser->path());
        QFileInfo dfi(fi.absolutePath());
        const QString parentFolderName = dfi.fileName().toLower();
        if (parentFolderName != QLatin1String("debug") &&
                parentFolderName != QLatin1String("release"))
            subfoldersEnabled = false;
        const QString baseName = fi.baseName();

        if (baseName.isEmpty() || baseName.at(baseName.size() - 1).toLower() != QLatin1Char('d'))
            removeSuffixEnabled = false;

        if (subfoldersEnabled)
            libraryDetailsWidget()->useSubfoldersCheckBox->setChecked(true);
        else if (removeSuffixEnabled)
            libraryDetailsWidget()->removeSuffixCheckBox->setChecked(true);
        else
            libraryDetailsWidget()->addSuffixCheckBox->setChecked(true);
    }
#endif

    updateGui();

    emit completeChanged();
}

bool NonInternalLibraryDetailsController::isComplete() const
{
    return libraryDetailsWidget()->libraryPathChooser->isValid() &&
           platforms();
}

QString NonInternalLibraryDetailsController::snippet() const
{
    QString libPath = libraryDetailsWidget()->libraryPathChooser->path();
    QFileInfo fi(libPath);
    QString libName;
    const bool removeSuffix = isWindowsGroupVisible()
            && libraryDetailsWidget()->removeSuffixCheckBox->isChecked();
#if defined (Q_OS_WIN)
    libName = fi.baseName();
    if (removeSuffix && !libName.isEmpty()) // remove last letter which needs to be "d"
        libName = libName.left(libName.size() - 1);
#elif defined (Q_OS_MAC)
    if (macLibraryType() == AddLibraryWizard::FrameworkType)
        libName = fi.baseName();
    else
        libName = fi.baseName().mid(3); // cut the "lib" prefix
#else
    libName = fi.baseName().mid(3); // cut the "lib" prefix
#endif
    QString targetRelativePath;
    QString includeRelativePath;
    bool useSubfolders = false;
    bool addSuffix = false;
    if (isWindowsGroupVisible()) {
        const bool useSubfoldersCondition =
#ifdef Q_OS_WIN
            true; // we are on Win but we in case don't generate the code for Win we still need to remove "debug" or "release" subfolder
#else
            platforms() & AddLibraryWizard::WindowsPlatform;
#endif
        if (useSubfoldersCondition)
            useSubfolders = libraryDetailsWidget()->useSubfoldersCheckBox->isChecked();
        if (platforms() & AddLibraryWizard::WindowsPlatform)
            addSuffix = libraryDetailsWidget()->addSuffixCheckBox->isChecked() || removeSuffix;
    }
    if (isIncludePathVisible()) { // generate also the path to lib
        QFileInfo pfi(proFile());
        QDir pdir = pfi.absoluteDir();
        QString absoluteLibraryPath = fi.absolutePath();
#if defined (Q_OS_WIN)
        if (useSubfolders) { // drop last subfolder which needs to be "debug" or "release"
            QFileInfo libfi(absoluteLibraryPath);
            absoluteLibraryPath = libfi.absolutePath();
        }
#endif // Q_OS_WIN
        targetRelativePath = appendSeparator(pdir.relativeFilePath(absoluteLibraryPath));

        const QString includePath = libraryDetailsWidget()->includePathChooser->path();
        if (!includePath.isEmpty())
            includeRelativePath = pdir.relativeFilePath(includePath);
    }

    QString snippetMessage;
    QTextStream str(&snippetMessage);
    str << "\n";
    str << generateLibsSnippet(platforms(), macLibraryType(), libName,
                               targetRelativePath, QLatin1String("PWD"),
                               useSubfolders, addSuffix, isIncludePathVisible());
    if (isIncludePathVisible()) {
        str << generateIncludePathSnippet(includeRelativePath);
        str << generatePreTargetDepsSnippet(platforms(), linkageType(), libName,
                               targetRelativePath, QLatin1String("PWD"),
                               useSubfolders, addSuffix);
    }
    return snippetMessage;
}

/////////////

SystemLibraryDetailsController::SystemLibraryDetailsController(
    Ui::LibraryDetailsWidget *libraryDetails, QObject *parent)
    : NonInternalLibraryDetailsController(libraryDetails, parent)
{
    setIncludePathVisible(false);
    setWindowsGroupVisible(false);
}

/////////////

ExternalLibraryDetailsController::ExternalLibraryDetailsController(
    Ui::LibraryDetailsWidget *libraryDetails, QObject *parent)
    : NonInternalLibraryDetailsController(libraryDetails, parent)
{
    setIncludePathVisible(true);
    setWindowsGroupVisible(true);
}

void ExternalLibraryDetailsController::updateWindowsOptionsEnablement()
{
    NonInternalLibraryDetailsController::updateWindowsOptionsEnablement();
#ifdef Q_OS_WIN
    bool subfoldersEnabled = true;
    bool removeSuffixEnabled = true;
    if (libraryDetailsWidget()->libraryPathChooser->isValid()) {
        QFileInfo fi(libraryDetailsWidget()->libraryPathChooser->path());
        QFileInfo dfi(fi.absolutePath());
        const QString parentFolderName = dfi.fileName().toLower();
        if (parentFolderName != QLatin1String("debug") &&
                parentFolderName != QLatin1String("release"))
            subfoldersEnabled = false;
        const QString baseName = fi.baseName();

        if (baseName.isEmpty() || baseName.at(baseName.size() - 1).toLower() != QLatin1Char('d'))
            removeSuffixEnabled = false;

    }
    libraryDetailsWidget()->useSubfoldersCheckBox->setEnabled(subfoldersEnabled);
    libraryDetailsWidget()->removeSuffixCheckBox->setEnabled(removeSuffixEnabled);
#endif
}

/////////////

InternalLibraryDetailsController::InternalLibraryDetailsController(
        Ui::LibraryDetailsWidget *libraryDetails, QObject *parent)
    : LibraryDetailsController(libraryDetails, parent),
      m_proFileNode(0)
{
    setLinkageRadiosVisible(false);
    setLibraryPathChooserVisible(false);
    setLibraryComboBoxVisible(true);
    setIncludePathVisible(true);
    setWindowsGroupVisible(true);
    setRemoveSuffixVisible(false);

#ifdef Q_OS_WIN
    libraryDetailsWidget()->useSubfoldersCheckBox->setEnabled(true);
#endif

    connect(libraryDetailsWidget()->libraryComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotCurrentLibraryChanged()));
}

AddLibraryWizard::LinkageType InternalLibraryDetailsController::suggestedLinkageType() const
{
    const int currentIndex = libraryDetailsWidget()->libraryComboBox->currentIndex();
    AddLibraryWizard::LinkageType type = AddLibraryWizard::NoLinkage;
    if (currentIndex >= 0) {
        Qt4ProFileNode *proFileNode = m_proFileNodes.at(currentIndex);
        const QStringList configVar = proFileNode->variableValue(ConfigVar);
        if (configVar.contains(QLatin1String("staticlib"))
                || configVar.contains(QLatin1String("static")))
            type = AddLibraryWizard::StaticLinkage;
        else
            type = AddLibraryWizard::DynamicLinkage;
    }
    return type;
}

AddLibraryWizard::MacLibraryType InternalLibraryDetailsController::suggestedMacLibraryType() const
{
    const int currentIndex = libraryDetailsWidget()->libraryComboBox->currentIndex();
    AddLibraryWizard::MacLibraryType type = AddLibraryWizard::NoLibraryType;
    if (currentIndex >= 0) {
        Qt4ProFileNode *proFileNode = m_proFileNodes.at(currentIndex);
        const QStringList configVar = proFileNode->variableValue(ConfigVar);
        if (configVar.contains(QLatin1String("lib_bundle")))
            type = AddLibraryWizard::FrameworkType;
        else
            type = AddLibraryWizard::LibraryType;
    }
    return type;
}

QString InternalLibraryDetailsController::suggestedIncludePath() const
{
    const int currentIndex = libraryDetailsWidget()->libraryComboBox->currentIndex();
    QString includePath;
    if (currentIndex >= 0) {
        Qt4ProFileNode *proFileNode = m_proFileNodes.at(currentIndex);
        QFileInfo fi(proFileNode->path());
        includePath = fi.absolutePath();
    }
    return includePath;
}

void InternalLibraryDetailsController::updateWindowsOptionsEnablement()
{
#ifdef Q_OS_WIN
    libraryDetailsWidget()->addSuffixCheckBox->setEnabled(true);
#endif
    libraryDetailsWidget()->winGroupBox->setEnabled(platforms()
                                & AddLibraryWizard::WindowsPlatform);
}

void InternalLibraryDetailsController::proFileChanged()
{
    m_proFileNodes.clear();
    libraryDetailsWidget()->libraryComboBox->clear();
    m_proFileNode = 0;

    const ProjectExplorer::Project *project =
            ProjectExplorer::ProjectExplorerPlugin::instance()->session()->projectForFile(proFile());
    if (!project)
        return;

    setIgnoreGuiSignals(true);

    ProjectExplorer::ProjectNode *rootProject = project->rootProjectNode();
    QFileInfo fi(rootProject->path());
    QDir rootDir(fi.absolutePath());
    FindQt4ProFiles findQt4ProFiles;
    QList<Qt4ProFileNode *> proFiles = findQt4ProFiles(rootProject);
    foreach (Qt4ProFileNode *proFileNode, proFiles) {
        const QString proFilePath = proFileNode->path();
        if (proFilePath == proFile()) {
            m_proFileNode = proFileNode;
        } else if (proFileNode->projectType() == LibraryTemplate) {
            const QStringList configVar = proFileNode->variableValue(ConfigVar);
            if (!configVar.contains(QLatin1String("plugin"))) {
                const QString relProFilePath = rootDir.relativeFilePath(proFilePath);
                TargetInformation targetInfo = proFileNode->targetInformation();
                const QString itemToolTip = tr("%1 (%2)").arg(targetInfo.target).arg(relProFilePath);
                m_proFileNodes.append(proFileNode);
                libraryDetailsWidget()->libraryComboBox->addItem(targetInfo.target);
                libraryDetailsWidget()->libraryComboBox->setItemData(
                            libraryDetailsWidget()->libraryComboBox->count() - 1,
                            itemToolTip, Qt::ToolTipRole);
            }
        }
    }

    setIgnoreGuiSignals(false);
}

void InternalLibraryDetailsController::slotCurrentLibraryChanged()
{
    const int currentIndex = libraryDetailsWidget()->libraryComboBox->currentIndex();
    if (currentIndex >= 0) {
        libraryDetailsWidget()->libraryComboBox->setToolTip(
                    libraryDetailsWidget()->libraryComboBox->itemData(
                        currentIndex, Qt::ToolTipRole).toString());
        Qt4ProFileNode *proFileNode = m_proFileNodes.at(currentIndex);
        const QStringList configVar = proFileNode->variableValue(ConfigVar);
#ifdef Q_OS_WIN
        bool useSubfolders = false;
        if (configVar.contains(QLatin1String("debug_and_release"))
                && configVar.contains(QLatin1String("debug_and_release_target")))
            useSubfolders = true;
        libraryDetailsWidget()->useSubfoldersCheckBox->setChecked(useSubfolders);
        libraryDetailsWidget()->addSuffixCheckBox->setChecked(!useSubfolders);
#endif // Q_OS_WIN
    }

    if (guiSignalsIgnored())
        return;

    updateGui();

    emit completeChanged();
}

bool InternalLibraryDetailsController::isComplete() const
{
    return libraryDetailsWidget()->libraryComboBox->count() && platforms();
}

QString InternalLibraryDetailsController::snippet() const
{
    const int currentIndex = libraryDetailsWidget()->libraryComboBox->currentIndex();
    if (currentIndex < 0)
        return QString();

    if (!m_proFileNode)
        return QString();

    Qt4ProFileNode *proFileNode = m_proFileNodes.at(currentIndex);

    QFileInfo fi(proFile());
    QDir projectBuildDir(m_proFileNode->buildDir());
    QDir projectSrcDir(fi.absolutePath());
    TargetInformation targetInfo = proFileNode->targetInformation();

    const QString targetRelativePath = appendSeparator(projectBuildDir.relativeFilePath(targetInfo.buildDir));
    const QString includeRelativePath = projectSrcDir.relativeFilePath(libraryDetailsWidget()->includePathChooser->path());

    const bool useSubfolders = libraryDetailsWidget()->useSubfoldersCheckBox->isChecked();
    const bool addSuffix = libraryDetailsWidget()->addSuffixCheckBox->isChecked();

    QString snippetMessage;
    QTextStream str(&snippetMessage);
    str << "\n";
    str << generateLibsSnippet(platforms(), macLibraryType(), targetInfo.target,
                               targetRelativePath, QLatin1String("OUT_PWD"),
                               useSubfolders, addSuffix, true);
    str << generateIncludePathSnippet(includeRelativePath);
    str << generatePreTargetDepsSnippet(platforms(), linkageType(), targetInfo.target,
                               targetRelativePath, QLatin1String("OUT_PWD"),
                               useSubfolders, addSuffix);
    return snippetMessage;
}
