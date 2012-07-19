/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "rvcttoolchain.h"
#include "rvctparser.h"
#include "ui_rvcttoolchainconfigwidget.h"
#include "qt4projectmanager/qt4projectmanagerconstants.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/toolchainmanager.h>
#include <utils/environment.h>
#include <utils/environmentmodel.h>
#include <utils/synchronousprocess.h>

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QGridLayout>
#include <QLabel>

namespace Qt4ProjectManager {
namespace Internal {

#if defined Q_OS_WIN
static const char *const RVCT_BINARY = "armcc.exe";
#else
static const char *const RVCT_BINARY = "armcc";
#endif

static const char *const RVCT_LICENSE_KEY = "ARMLMD_LICENSE_FILE";

static const char rvctPathKeyC[] = "Qt4ProjectManager.RvctToolChain.CompilerPath";
static const char rvctEnvironmentKeyC[] = "Qt4ProjectManager.RvctToolChain.Environment";
static const char rvctArmVersionKeyC[] = "Qt4ProjectManager.RvctToolChain.ArmVersion";

static QString valueOf(const QList<Utils::EnvironmentItem> &items, const QString &suffix)
{
    foreach (const Utils::EnvironmentItem &i, items) {
        if (i.name.mid(6) == suffix && !i.unset)
            return i.value;
    }
    return QString();
}

static QString armVersionString(RvctToolChain::ArmVersion av)
{
    switch (av) {
    case RvctToolChain::ARMv5:
        return RvctToolChainFactory::tr("ARMv5");
    case RvctToolChain::ARMv6:
        return RvctToolChainFactory::tr("ARMv6");
    };
    return QString();
}

static Utils::Environment baseEnvironment(RvctToolChain *tc)
{
    Utils::Environment result;
    result.modify(tc->environmentChanges());
    return result;
}

// ==========================================================================
// RvctToolChain
// ==========================================================================

RvctToolChain::RvctToolChain(bool autodetected) :
    ToolChain(QLatin1String(Constants::RVCT_TOOLCHAIN_ID), autodetected),
    m_armVersion(ARMv5)
{ }

RvctToolChain::RvctToolChain(const RvctToolChain &tc) :
    ToolChain(tc),
    m_compilerCommand(tc.m_compilerCommand),
    m_environmentChanges(tc.m_environmentChanges),
    m_armVersion(tc.m_armVersion)
{ }

RvctToolChain::RvctVersion RvctToolChain::version(const Utils::FileName &rvctPath)
{
    RvctToolChain::RvctVersion v;

    QProcess armcc;
    armcc.start(rvctPath.toString(), QStringList(QLatin1String("--version_number")));
    if (!armcc.waitForStarted()) {
        qWarning("Unable to run rvct binary '%s' when trying to determine version.", qPrintable(rvctPath.toUserOutput()));
        return v;
    }
    armcc.closeWriteChannel();
    if (!armcc.waitForFinished()) {
        Utils::SynchronousProcess::stopProcess(armcc);
        qWarning("Timeout running rvct binary '%s' trying to determine version.", qPrintable(rvctPath.toUserOutput()));
        return v;
    }
    if (armcc.exitStatus() != QProcess::NormalExit) {
        qWarning("A crash occurred when running rvct binary '%s' trying to determine version.", qPrintable(rvctPath.toUserOutput()));
        return v;
    }
    QString versionLine = QString::fromLocal8Bit(armcc.readAllStandardOutput());
    versionLine += QString::fromLocal8Bit(armcc.readAllStandardError());
    QRegExp versionRegExp(QLatin1String("^(\\d)(\\d)0*([1-9]\\d*)"), Qt::CaseInsensitive);
    Q_ASSERT(versionRegExp.isValid());

    if (versionRegExp.indexIn(versionLine) != -1) {
        v.majorVersion = versionRegExp.cap(1).toInt();
        v.minorVersion = versionRegExp.cap(2).toInt();
        v.build = versionRegExp.cap(3).toInt();
    }
    return v;
}

QString RvctToolChain::type() const
{
    return QLatin1String("rvct");
}

QString RvctToolChain::typeDisplayName() const
{
    return RvctToolChainFactory::tr("RVCT");
}

ProjectExplorer::Abi RvctToolChain::targetAbi() const
{
    return ProjectExplorer::Abi(ProjectExplorer::Abi::ArmArchitecture, ProjectExplorer::Abi::SymbianOS,
                                ProjectExplorer::Abi::SymbianDeviceFlavor, ProjectExplorer::Abi::ElfFormat,
                                32);
}

bool RvctToolChain::isValid() const
{
    return !m_compilerCommand.isEmpty();
}

QByteArray RvctToolChain::predefinedMacros(const QStringList &cxxflags) const
{
    Q_UNUSED(cxxflags);
    // see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0205f/Babbacdb.html (version 2.2)
    // and http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0491b/BABJFEFG.html (version 4.0)
    QByteArray ba("#define __ARRAY_OPERATORS\n"
                  "#define _BOOL\n"
                  "#define __cplusplus\n"
                  "#define __CC_ARM 1\n"
                  "#define __EDG__\n"
                  "#define __STDC__\n"
                  "#define __STDC_VERSION__\n"
                  "#define __sizeof_int 4"
                  "#define __sizeof_long 4"
                  "#define __sizeof_ptr 4"
                  "#define __TARGET_FEATURE_DOUBLEWORD\n"
                  "#define __TARGET_FEATURE_DSPMUL\n"
                  "#define __TARGET_FEATURE_HALFWORD\n"
                  "#define __TARGET_FEATURE_THUMB\n"
                  "#define _WCHAR_T\n"
                  "#define __SYMBIAN32__\n");
    return ba;
}

ProjectExplorer::ToolChain::CompilerFlags RvctToolChain::compilerFlags(const QStringList &cxxflags) const
{
    Q_UNUSED(cxxflags);
    return NO_FLAGS;
}

QList<ProjectExplorer::HeaderPath> RvctToolChain::systemHeaderPaths() const
{
    return QList<ProjectExplorer::HeaderPath>()
            << ProjectExplorer::HeaderPath(valueOf(m_environmentChanges, QLatin1String("INC")),
                                           ProjectExplorer::HeaderPath::GlobalHeaderPath);
}

void RvctToolChain::addToEnvironment(Utils::Environment &env) const
{
    if (m_compilerCommand.isEmpty())
        return;

    if (m_version.isNull())
        setVersion(version(m_compilerCommand));
    if (m_version.isNull())
        return;

    env.modify(m_environmentChanges);

    env.set(QLatin1String("QT_RVCT_VERSION"), QString::fromLatin1("%1.%2")
            .arg(m_version.majorVersion).arg(m_version.minorVersion));
    const QString cxxPath = compilerCommand().toFileInfo().absolutePath();
    env.set(varName(QLatin1String("BIN")), QDir::toNativeSeparators(cxxPath));

    // Add rvct to path and set locale to 'C'
    if (!m_compilerCommand.isEmpty())
        env.prependOrSetPath(cxxPath);
    env.set(QLatin1String("LANG"), QString(QLatin1Char('C')));
}

QString RvctToolChain::makeCommand() const
{
#if defined(Q_OS_WIN)
    return QLatin1String("make.exe");
#else
    return QLatin1String("make");
#endif
}

QString RvctToolChain::defaultMakeTarget() const
{
    if (!isValid())
        return QString();
    if (m_armVersion == ARMv6)
        return QLatin1String("armv6");
    return QLatin1String("armv5");
}

ProjectExplorer::IOutputParser *RvctToolChain::outputParser() const
{
    return new RvctParser;
}

bool RvctToolChain::operator ==(const ToolChain &other) const
{
    if (!ToolChain::operator ==(other))
        return false;
    const RvctToolChain *otherPtr = dynamic_cast<const RvctToolChain *>(&other);
    return m_compilerCommand == otherPtr->m_compilerCommand
            && m_environmentChanges == otherPtr->m_environmentChanges
            && m_armVersion == otherPtr->m_armVersion;
}

void RvctToolChain::setEnvironmentChanges(const QList<Utils::EnvironmentItem> &changes)
{
    if (m_environmentChanges == changes)
        return;
    m_environmentChanges = changes;
    toolChainUpdated();
}

QList<Utils::EnvironmentItem> RvctToolChain::environmentChanges() const
{
    return m_environmentChanges;
}

void RvctToolChain::setCompilerCommand(const Utils::FileName &path)
{
    if (m_compilerCommand == path)
        return;

    m_compilerCommand = path;
    m_version.reset();
    toolChainUpdated();
}

Utils::FileName RvctToolChain::compilerCommand() const
{
    return m_compilerCommand;
}

void RvctToolChain::setArmVersion(RvctToolChain::ArmVersion av)
{
    if (m_armVersion == av)
        return;
    m_armVersion = av;
    toolChainUpdated();
}

RvctToolChain::ArmVersion RvctToolChain::armVersion() const
{
    return m_armVersion;
}

void RvctToolChain::setVersion(const RvctVersion &v) const
{
    if (m_version == v)
        return;
    m_version = v;
    // Internal use only! No need to call toolChainUpdated()!
}

ProjectExplorer::ToolChainConfigWidget *RvctToolChain::configurationWidget()
{
    return new RvctToolChainConfigWidget(this);
}

ProjectExplorer::ToolChain *RvctToolChain::clone() const
{
    return new RvctToolChain(*this);
}


QVariantMap RvctToolChain::toMap() const
{
    QVariantMap result = ToolChain::toMap();
    result.insert(QLatin1String(rvctPathKeyC), m_compilerCommand.toString());
    QVariantMap tmp;
    foreach (const Utils::EnvironmentItem &i, m_environmentChanges)
        tmp.insert(i.name, i.value);
    result.insert(QLatin1String(rvctEnvironmentKeyC), tmp);
    result.insert(QLatin1String(rvctArmVersionKeyC), static_cast<int>(m_armVersion));
    return result;
}

bool RvctToolChain::fromMap(const QVariantMap &data)
{
    if (!ToolChain::fromMap(data))
        return false;
    m_compilerCommand = Utils::FileName::fromString(data.value(QLatin1String(rvctPathKeyC)).toString());

    m_environmentChanges.clear();
    QVariantMap tmp = data.value(QLatin1String(rvctEnvironmentKeyC)).toMap();
    for (QVariantMap::const_iterator i = tmp.constBegin(); i != tmp.constEnd(); ++i)
        m_environmentChanges.append(Utils::EnvironmentItem(i.key(), i.value().toString()));
    m_armVersion = static_cast<ArmVersion>(data.value(QLatin1String(rvctArmVersionKeyC), 0).toInt());
    return isValid();
}

QString RvctToolChain::varName(const QString &postFix) const
{
    return QString::fromLatin1("RVCT%1%2%3")
            .arg(m_version.majorVersion).arg(m_version.minorVersion).arg(postFix);
}

// ==========================================================================
// RvctToolChainConfigWidget
// ==========================================================================

RvctToolChainConfigWidget::RvctToolChainConfigWidget(RvctToolChain *tc) :
    ProjectExplorer::ToolChainConfigWidget(tc),
    m_ui(new Ui::RvctToolChainConfigWidget()),
    m_model(new Utils::EnvironmentModel(this))
{
    m_ui->setupUi(this);

    m_ui->environmentView->setModel(m_model);
    m_ui->environmentView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    m_ui->environmentView->horizontalHeader()->setStretchLastSection(true);
    m_ui->environmentView->setGridStyle(Qt::NoPen);
    m_ui->environmentView->horizontalHeader()->setHighlightSections(false);
    m_ui->environmentView->verticalHeader()->hide();
    QFontMetrics fm(font());
    m_ui->environmentView->verticalHeader()->setDefaultSectionSize(qMax(static_cast<int>(fm.height() * 1.2), fm.height() + 4));

    connect(m_model, SIGNAL(userChangesChanged()), this, SLOT(emitDirty()));

    m_ui->compilerPath->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_ui->compilerPath->setFileName(tc->compilerCommand());
    connect(m_ui->compilerPath, SIGNAL(changed(QString)), this, SLOT(emitDirty()));
    m_ui->versionComboBox->setCurrentIndex(static_cast<int>(tc->armVersion()));
    connect(m_ui->versionComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(emitDirty()));

    setFromToolChain();
}

RvctToolChainConfigWidget::~RvctToolChainConfigWidget()
{
    delete m_ui;
}

void RvctToolChainConfigWidget::apply()
{
    RvctToolChain *tc = static_cast<RvctToolChain *>(toolChain());
    Q_ASSERT(tc);

    QList<Utils::EnvironmentItem> changes = environmentChanges();
    tc->setCompilerCommand(m_ui->compilerPath->fileName());
    tc->setArmVersion(static_cast<RvctToolChain::ArmVersion>(m_ui->versionComboBox->currentIndex()));
    tc->setEnvironmentChanges(changes);

    m_model->setUserChanges(changes);
}

void RvctToolChainConfigWidget::setFromToolChain()
{
    RvctToolChain *tc = static_cast<RvctToolChain *>(toolChain());
    Q_ASSERT(tc);

    m_model->setBaseEnvironment(baseEnvironment(tc));

    m_ui->compilerPath->setFileName(tc->compilerCommand());
    m_ui->versionComboBox->setCurrentIndex(static_cast<int>(tc->armVersion()));
}

bool RvctToolChainConfigWidget::isDirty() const
{
    RvctToolChain *tc = static_cast<RvctToolChain *>(toolChain());
    Q_ASSERT(tc);

    return tc->compilerCommand() != m_ui->compilerPath->fileName()
            || tc->armVersion() != static_cast<RvctToolChain::ArmVersion>(m_ui->versionComboBox->currentIndex())
            || tc->environmentChanges() != environmentChanges();
}

void RvctToolChainConfigWidget::makeReadOnly()
{
    m_ui->versionComboBox->setEnabled(false);
    m_ui->compilerPath->setEnabled(false);
    m_ui->environmentView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ProjectExplorer::ToolChainConfigWidget::makeReadOnly();
}

QList<Utils::EnvironmentItem> RvctToolChainConfigWidget::environmentChanges() const
{
    Utils::Environment baseEnv;
    Utils::Environment resultEnv = baseEnvironment(static_cast<RvctToolChain *>(toolChain()));
    resultEnv.modify(m_model->userChanges());
    return baseEnv.diff(resultEnv);
}

void RvctToolChainConfigWidget::changeEvent(QEvent *ev)
{
    if (ev->type() == QEvent::EnabledChange) {
        if (isEnabled()) {
            m_ui->environmentView->horizontalHeader()->setVisible(true);
            m_ui->environmentView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        } else {
            m_ui->environmentView->horizontalHeader()->setVisible(false);
            m_ui->environmentView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }
    ToolChainConfigWidget::changeEvent(ev);
}

// ==========================================================================
// RvctToolChainFactory
// ==========================================================================

QString RvctToolChainFactory::displayName() const
{
    return tr("RVCT");
}

QString RvctToolChainFactory::id() const
{
    return QLatin1String(Constants::RVCT_TOOLCHAIN_ID);
}

QList<ProjectExplorer::ToolChain *> RvctToolChainFactory::autoDetect()
{
    Utils::Environment env = Utils::Environment::systemEnvironment();

    QMap<QString, QList<Utils::EnvironmentItem> > rvcts;
    QList<Utils::EnvironmentItem> globalItems;

    // Find all RVCT..x variables
    for (Utils::Environment::const_iterator i = env.constBegin(); i != env.constEnd(); ++i) {
        if (i.key() == QLatin1String(RVCT_LICENSE_KEY))
            globalItems.append(Utils::EnvironmentItem(i.key(), i.value()));
        if (!i.key().startsWith(QLatin1String("RVCT")))
            continue;

        const QString key = i.key().left(6);
        QList<Utils::EnvironmentItem> values = rvcts.value(key);

        values.append(Utils::EnvironmentItem(i.key(), i.value()));

        rvcts.insert(key, values);
    }

    // Set up tool chains for each RVCT.. set
    QList<ProjectExplorer::ToolChain *> result;
    for (QMap<QString, QList<Utils::EnvironmentItem> >::const_iterator i = rvcts.constBegin();
         i != rvcts.constEnd(); ++i) {
        QList<Utils::EnvironmentItem> changes = i.value();
        changes.append(globalItems);

        Utils::FileName binary = Utils::FileName::fromUserInput(valueOf(changes, QLatin1String("BIN")));
        if (binary.isEmpty())
            continue;
        binary.appendPath(QLatin1String(RVCT_BINARY));
        QFileInfo fi(binary.toFileInfo());
        if (!fi.exists() || !fi.isExecutable())
            continue;

        RvctToolChain::RvctVersion v = RvctToolChain::version(binary);
        if (v.majorVersion == 0 && v.minorVersion == 0 && v.build == 0)
            continue; // Failed to start.

        //: %1 arm version, %2 major version, %3 minor version, %4 build number
        const QString name = tr("RVCT (%1 %2.%3 Build %4)");

        RvctToolChain *tc = new RvctToolChain(true);
        tc->setCompilerCommand(binary);
        tc->setEnvironmentChanges(changes);
        tc->setDisplayName(name.arg(armVersionString(tc->armVersion()))
                           .arg(v.majorVersion).arg(v.minorVersion).arg(v.build));
        tc->setVersion(v);
        result.append(tc);

        tc = new RvctToolChain(true);
        tc->setCompilerCommand(binary);
        tc->setEnvironmentChanges(changes);
        tc->setArmVersion(RvctToolChain::ARMv6);
        tc->setDisplayName(name.arg(armVersionString(tc->armVersion()))
                           .arg(v.majorVersion).arg(v.minorVersion).arg(v.build));
        tc->setVersion(v);
        result.append(tc);
    }

    return result;
}

bool RvctToolChainFactory::canCreate()
{
    return true;
}

ProjectExplorer::ToolChain *RvctToolChainFactory::create()
{
    RvctToolChain *tc = new RvctToolChain(false);
    Utils::Environment env = Utils::Environment::systemEnvironment();
    if (env.hasKey(QLatin1String(RVCT_LICENSE_KEY))) {
        tc->setEnvironmentChanges(QList<Utils::EnvironmentItem>()
                                  << Utils::EnvironmentItem(QLatin1String(RVCT_LICENSE_KEY),
                                                            env.value(QLatin1String(RVCT_LICENSE_KEY))));
    }
    tc->setDisplayName(tr("RVCT"));
    return tc;
}

bool RvctToolChainFactory::canRestore(const QVariantMap &data)
{
    return idFromMap(data).startsWith(QLatin1String(Constants::RVCT_TOOLCHAIN_ID));
}

ProjectExplorer::ToolChain *RvctToolChainFactory::restore(const QVariantMap &data)
{
    RvctToolChain *tc = new RvctToolChain(false);
    if (tc->fromMap(data))
        return tc;

    delete tc;
    return 0;

}

} // Internal
} // Qt4ProjectManager
