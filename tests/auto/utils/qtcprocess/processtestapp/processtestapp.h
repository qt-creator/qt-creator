/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <functional>

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>

namespace Utils { class QtcProcess; }

#define SUB_PROCESS(SubProcessClass)\
class SubProcessClass\
{\
public:\
    static const char *envVar() { return m_envVar; }\
private:\
    SubProcessClass() { registerSubProcess(envVar(), &SubProcessClass::main); }\
    ~SubProcessClass() { unregisterSubProcess(envVar()); }\
    static int main();\
    static constexpr char m_envVar[] = "TST_QTC_PROCESS_" QTC_ASSERT_STRINGIFY(SubProcessClass);\
    friend class ProcessTestApp;\
};\
\
SubProcessClass m_ ## SubProcessClass

class ProcessTestApp
{
public:
    using SubProcessMain = std::function<int ()>;

    static void invokeSubProcess();

    // Many tests inside tst_qtcprocess need to start a new subprocess with custom code.
    // In order to simplify things we produce just one separate executable - processtestapp.
    // We embed all our custom subprocesses in processtestapp and enclose them in separate
    // classes. We select desired process to run by setting the relevant environment variable.
    // Test classes are defined by the SUB_PROCESS macro. The macro defines a class
    // alongside of the corresponding environment variable which is set prior to the execution
    // of the subprocess. The following subprocess classes are defined:

    SUB_PROCESS(SimpleTest);
    SUB_PROCESS(ExitCode);
    SUB_PROCESS(RunBlockingStdOut);
    SUB_PROCESS(LineCallback);
    SUB_PROCESS(StandardOutputAndErrorWriter);
    SUB_PROCESS(ChannelForwarding);
    SUB_PROCESS(KillBlockingProcess);
    SUB_PROCESS(EmitOneErrorOnCrash);
    SUB_PROCESS(CrashAfterOneSecond);
    SUB_PROCESS(RecursiveCrashingProcess);
    SUB_PROCESS(RecursiveBlockingProcess);

    // In order to get a value associated with the certain subprocess use SubProcessClass::envVar().
    // The classes above define different custom executables. Inside invokeSubProcess(), called
    // by processtestapp, we are detecting if one of these variables is set and invoke a respective
    // custom executable code directly. The exit code of the process is reported to the caller
    // by the return value of SubProcessClass::main().

private:
    ProcessTestApp();

    static void registerSubProcess(const char *envVar, const SubProcessMain &main);
    static void unregisterSubProcess(const char *envVar);
};

class SubProcessConfig
{
public:
    SubProcessConfig(const char *envVar, const QString &envVal);
    void setupSubProcess(Utils::QtcProcess *subProcess);

    static void setPathToProcessTestApp(const QString &path);

private:
    const Utils::Environment m_environment;
};

static const char s_simpleTestData[] = "Test process successfully executed.";
static const char s_runBlockingStdOutSubProcessMagicWord[] = "42";

// Expect ending lines detected at '|':
static const char s_lineCallbackData[] =
       "This is the first line\r\n|"
       "Here comes the second one\r\n|"
       "And a line without LF\n|"
       "Rebasing (1/10)\r| <delay> Rebasing (2/10)\r| <delay> ...\r\n|"
       "And no end";

static const char s_outputData[] = "This is the output message.";
static const char s_errorData[] = "This is the error message.";

enum class BlockType {
    EndlessLoop,
    InfiniteSleep,
    MutexDeadlock,
    EventLoop
};

static const int s_crashCode = 123;
static const char s_leafProcessStarted[] = "Leaf process started";
static const char s_leafProcessTerminated[] = "Leaf process terminated";

Q_DECLARE_METATYPE(BlockType)
