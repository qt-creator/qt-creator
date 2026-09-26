// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "java_global.h"

#include <projectexplorer/kitaspect.h>

#include <utils/filepath.h>
#include <utils/id.h>
#include <utils/store.h>

#include <QFuture>
#include <QMetaType>
#include <QObject>

#include <functional>

namespace Java {

// The java and the javac of a JAVA_HOME directory, whether or not the
// directory has them.
JAVA_EXPORT Utils::FilePath javaExecutableIn(const Utils::FilePath &javaHome);
JAVA_EXPORT Utils::FilePath javaCompilerIn(const Utils::FilePath &javaHome);

// The version the JDK in that directory reports for itself, empty where the
// directory holds no JDK or one that says nothing about its version.
JAVA_EXPORT QString javaVersionIn(const Utils::FilePath &javaHome);

// What two directories holding one JDK both come down to: a distribution and
// a version manager each name it from one of their own, and which of them is
// seen first is not the same from one detection to the next. Comparing these
// rather than the directories themselves is what keeps one JDK from being
// entered into the list a second time under another of its names.
JAVA_EXPORT Utils::FilePath javaHomeIdentity(const Utils::FilePath &javaHome);

// What a JDK entered into the list because a kit or a plugin named it is
// marked with, as against one the user made on the JDKs page. The two are
// told apart where the detection finds such a JDK: one the machine has is
// one of the machine's, whoever named it first.
inline constexpr char JDK_NAMED_BY_A_KIT[] = "Kit";

// Plugins that know of a JDK the detection below does not look for - one
// inside an application rather than where a package manager puts one - say so
// here, so that the machine has it in its list like any other.
using JdkProvider = std::function<Utils::FilePaths()>;
JAVA_EXPORT QList<JdkProvider> &jdkProviders();

// A Java development kit, the way JAVA_HOME names one: the directory that
// holds bin/java, and bin/javac where it is a development kit rather than a
// runtime environment.
class JAVA_EXPORT Jdk
{
public:
    Utils::Id id;
    QString name;
    Utils::FilePath javaHome;
    ProjectExplorer::DetectionSource detectionSource;

    Utils::FilePath javaExecutable() const;
    Utils::FilePath compilerExecutable() const;
    // Whether the directory holds them. Asked of the file system, and for
    // a JDK on another device a round trip to it.
    bool hasJava() const;
    bool hasCompiler() const;

    void toMap(Utils::Store &store) const;
    void fromMap(const Utils::Store &store);

    friend bool operator==(const Jdk &first, const Jdk &second)
    {
        return first.id == second.id && first.name == second.name
               && first.javaHome == second.javaHome
               && first.detectionSource == second.detectionSource;
    }
};

class JAVA_EXPORT JdkManager : public QObject
{
    Q_OBJECT

public:
    JdkManager();
    ~JdkManager() override;

    static JdkManager *instance();

    static const QList<Jdk> &jdks();
    static Jdk jdk(Utils::Id id);
    static Jdk jdkForJavaHome(const Utils::FilePath &javaHome);
    // The JDK a kit that names none is to be built with: the one the user
    // picked out of the list, or the first the machine was found to have.
    static Jdk defaultJdk();
    static Utils::Id defaultJdkId();

    // What the user picked out of the list, invalid where they picked
    // nothing and the machine answers. Showing the answer is not picking it,
    // so this is what a page that has not been touched hands back.
    static Utils::Id chosenDefaultJdkId();

    // Whether the machine has been looked at. Until it has, the list holds
    // only what the kits named, so there is no default of the machine to
    // speak of and nothing is to be set up with one.
    static bool jdksWereDetected();

    static void setJdks(const QList<Jdk> &jdks, Utils::Id defaultId);

    // The JDK that is known for that directory, entered into the list where
    // there is none yet. Invalid for a directory that holds no java.
    static Jdk registerJavaHome(
        const Utils::FilePath &javaHome, const ProjectExplorer::DetectionSource &detectionSource);

    // The JDKs this machine has, in the order they deserve to be preferred
    // in. Reads directories and, on macOS, runs a process, so it is not for
    // a hot path - and not for the thread the GUI is on, which the second of
    // these keeps off. What the plugins know is asked of them here either
    // way, since it is this thread they live on.
    static QList<Jdk> detectJdks();
    static QFuture<QList<Jdk>> detectJdksAsync();

    // What the list comes to hold when \a detected is entered into
    // \a current: the JDKs of the machine in the order it has them, each the
    // entry the kits name it by where there was one, and after them the
    // entries the machine does not have and which are not its to drop.
    static QList<Jdk> mergedJdks(const QList<Jdk> &current, const QList<Jdk> &detected);

    // Enters \a detected into the list, keeping what is there already, and
    // drops the entries of JDKs that have been uninstalled.
    static void enterDetectedJdks(const QList<Jdk> &detected);

    // The two of those, with the looking done on another thread. The kits do
    // not wait for it: they are restored against the list as it stands, and
    // the ones that have no JDK are given one once it has been filled.
    static void restoreJdks();

signals:
    void jdksChanged();
};

} // namespace Java

Q_DECLARE_METATYPE(Java::Jdk)
