#!/usr/bin/env python

import sys
sys.path.append("/data/dev/llvm-git/build/Debug+Asserts/lib/python2.7/site-packages")

import cmd
import inspect
import os

import lldb

cdbLoaded = False
lldbLoaded = False
gdbLoaded = False

#######################################################################
#
# Helpers
#
#######################################################################

currentDir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
#print "DIR: %s " % currentDir

def warn(message):
    print 'XXX={"%s"}@\n' % message.encode("latin1").replace('"', "'")

def showException(msg, exType, exValue, exTraceback):
    warn("**** CAUGHT EXCEPTION: %s ****" % msg)
    try:
        import traceback
        for line in traceback.format_exception(exType, exValue, exTraceback):
            warn("%s" % line)
    except:
        pass

def registerCommand(name, func):
    pass

class Type:
    def __init__(self, var):
        self.raw = var
        if var.num_children == 0:
            self.code = SimpleValueCode
        else:
            self.code = StructCode
        self.value_type = var.value_type

    def __str__(self):
        #try:
            return self.raw.type.name
        #except:
        #    return "<illegal type>"

    def fieldCount(self):
        return self.raw.num_children

    def unqualified(self):
        return self

    def strip_typedefs(self):
        return self

class Value:
    def __init__(self, var):
        self.raw = var
        self.is_optimized_out = False
        self.address = var.addr
        self.type = Type(var)
        self.name = var.name

    def __str__(self):
        return str(self.raw.value)

    def __getitem__(self, name):
        return None if self.raw is None else  self.raw.GetChildMemberWithName(name)

    def fields(self):
        return [Value(self.raw.GetChildAtIndex(i)) for i in xrange(self.raw.num_children)]

def currentFrame():
    currentThread = self.process.GetThreadAtIndex(0)
    return currentThread.GetFrameAtIndex(0)

def fieldCount(type):
    return type.fieldCount();

def fileName(file):
    return str(file) if file.IsValid() else ''

def breakpoint_function_wrapper(baton, process, frame, bp_loc):
    result = '*stopped'
    result += ',line="%s"' % frame.line_entry.line
    result += ',file="%s"' % frame.line_entry.file
    warn("WRAPPER: %s " %result)
    return result


def onBreak():
    db.debugger.HandleCommand("settings set frame-format ''")
    db.debugger.HandleCommand("settings set thread-format ''")
    result = "*stopped,frame={....}"
    print result


PointerCode = None
ArrayCode = None
StructCode = None
UnionCode = None
EnumCode = None
FlagsCode = None
FunctionCode = None
IntCode = None
FloatCode = None
VoidCode = None
SetCode = None
RangeCode = None
StringCode = None
BitStringCode = None
ErrorTypeCode = None
MethodCode = None
MethodPointerCode = None
MemberPointerCode = None
ReferenceCode = None
CharCode = None
BoolCode = None
ComplexCode = None
TypedefCode = None
NamespaceCode = None
SimpleValueCode = None # LLDB only


# Data members
SimpleValueCode = 100
StructCode = 101
PointerCode = 102

# Breakpoints. Keep synchronized with BreakpointType in breakpoint.h
UnknownType = 0
BreakpointByFileAndLine = 1
BreakpointByFunction = 2
BreakpointByAddress = 3
BreakpointAtThrow = 4
BreakpointAtCatch = 5
BreakpointAtMain = 6
BreakpointAtFork = 7
BreakpointAtExec = 8
BreakpointAtSysCall = 10
WatchpointAtAddress = 11
WatchpointAtExpression = 12
BreakpointOnQmlSignalEmit = 13
BreakpointAtJavaScriptThrow = 14

# See db.StateType
stateNames = ["invalid", "unloaded", "connected", "attaching", "launching", "stopped",
    "running", "stepping", "crashed", "detached", "exited", "suspended" ]

def loggingCallback(args):
    s = args.strip()
    s = s.replace('"', "'")
    sys.stdout.write('log="%s"@\n' % s)

class Children:
    def __init__(self, d, numChild = 1, childType = None, childNumChild = None,
            maxNumChild = None, addrBase = None, addrStep = None):
        self.d = d

    def __enter__(self):
        self.d.put('children=[')

    def __exit__(self, exType, exValue, exTraceBack):
        if not exType is None:
            if self.d.passExceptions:
                showException("CHILDREN", exType, exValue, exTraceBack)
        self.d.put(']')
        return True

class SubItem:
    def __init__(self, d, component):
        self.d = d
        self.iname = "%s.%s" % (d.currentIName, component)
        self.name = component

    def __enter__(self):
        self.d.put('{')
        #if not self.name is None:
        if isinstance(self.name, str):
            self.d.put('name="%s",' % self.name)
        self.savedIName = self.d.currentIName
        self.savedValue = self.d.currentValue
        self.savedValuePriority = self.d.currentValuePriority
        self.savedValueEncoding = self.d.currentValueEncoding
        self.savedType = self.d.currentType
        self.savedTypePriority = self.d.currentTypePriority
        self.d.currentIName = self.iname
        self.d.currentValuePriority = -100
        self.d.currentValueEncoding = None
        self.d.currentType = ""
        self.d.currentTypePriority = -100

    def __exit__(self, exType, exValue, exTraceBack):
        if not exType is None:
            if self.d.passExceptions:
                showException("SUBITEM", exType, exValue, exTraceBack)
            self.d.putNumChild(0)
            self.d.putValue("<not accessible>")
        try:
            typeName = self.d.currentType
            if len(typeName) > 0 and typeName != self.d.currentChildType:
                self.d.put('type="%s",' % typeName) # str(type.unqualified()) ?
            if  self.d.currentValue is None:
                self.d.put('value="<not accessible>",numchild="0",')
            else:
                if not self.d.currentValueEncoding is None:
                    self.d.put('valueencoded="%d",' % self.d.currentValueEncoding)
                self.d.put('value="%s",' % self.d.currentValue)
        except:
            pass
        self.d.put('},')
        self.d.currentIName = self.savedIName
        self.d.currentValue = self.savedValue
        self.d.currentValuePriority = self.savedValuePriority
        self.d.currentValueEncoding = self.savedValueEncoding
        self.d.currentType = self.savedType
        self.d.currentTypePriority = self.savedTypePriority
        return True

class Debugger(cmd.Cmd):
    def __init__(self):
        cmd.Cmd.__init__(self)
        self.prompt = ""

        self.debugger = lldb.SBDebugger.Create()
        #self.debugger.SetLoggingCallback(loggingCallback)
        #Same as: self.debugger.HandleCommand("log enable lldb dyld step")
        #self.debugger.EnableLog("lldb", ["dyld", "step", "process", "state", "thread", "events",
        #    "communication", "unwind", "commands"])
        #self.debugger.EnableLog("lldb", ["all"])
        self.debugger.Initialize()
        self.debugger.HandleCommand("settings set auto-confirm on")
        self.process = None
        self.target = None
        self.executable = None
        self.pid = None
        self.eventState = lldb.eStateInvalid
        self.listener = None
        self.options = {}
        self.expandedINames = {}
        self.passExceptions = True

        self.currentIName = None
        self.currentValuePriority = -100
        self.currentValueEncoding = None
        self.currentType = ""
        self.currentTypePriority = -100
        self.currentValue = -100

    def handleCommand(self, command):
        result = lldb.SBCommandReturnObject()
        self.debugger.GetCommandInterpreter().HandleCommand(command, result)
        success = result.Succeeded()
        if success:
            self.report('output="%s"' % result.GetOutput())
        else:
            self.report('error="%s"' % result.GetError())
        self.reportData()

    def put(self, stuff):
        sys.stdout.write(stuff)

    def putNumChild(self, numchild):
        #warn("NUM CHILD: '%s' '%s'" % (numchild, self.currentChildNumChild))
        if numchild != self.currentChildNumChild:
            self.put('numchild="%s",' % numchild)

    def putValue(self, value, encoding = None, priority = 0):
        # Higher priority values override lower ones.
        if priority >= self.currentValuePriority:
            self.currentValue = value
            self.currentValuePriority = priority
            self.currentValueEncoding = encoding

    def setupInferior(self, fileName):
        error = lldb.SBError()
        self.executable = fileName
        self.target = self.debugger.CreateTarget(self.executable, None, None, True, error)
        if self.target.IsValid():
            self.report('state="inferiorsetupok",msg="%s",exe="%s"' % (error, fileName))
        else:
            self.report('state="inferiorsetupfailed",msg="%s",exe="%s"' % (error, fileName))

    def runEngine(self):
        error = lldb.SBError()
        #launchInfo = lldb.SBLaunchInfo(["-s"])
        #self.process = self.target.Launch(self.listener, None, None,
        #                                    None, '/tmp/stdout.txt', None,
        #                                    None, 0, True, error)
        self.listener = lldb.SBListener("event_Listener")
        self.process = self.target.Launch(self.listener, None, None,
                                            None, None, None,
                                            os.getcwd(),
                  lldb.eLaunchFlagExec
                + lldb.eLaunchFlagDebug
                #+ lldb.eLaunchFlagDebug
                #+ lldb.eLaunchFlagStopAtEntry
                #+ lldb.eLaunchFlagDisableSTDIO
                #+ lldb.eLaunchFlagLaunchInSeparateProcessGroup
            , False, error)
        self.reportError(error)
        self.pid = self.process.GetProcessID()
        self.report('pid="%s"' % self.pid)
        self.report('state="enginerunok"')

    def reportError(self, error):
        desc = lldb.SBStream()
        error.GetDescription(desc)
        result = 'error={type="%s"' % error.GetType()
        result += ',code="%s"' % error.GetError()
        result += ',msg="%s"' % error.GetCString()
        result += ',desc="%s"}' % desc.GetData()
        self.report(result)

    def currentThread(self):
        #return self.process.GetSelectedThread()
        return self.process.GetThreadAtIndex(0)

    def currentFrame(self):
        return self.currentThread().GetSelectedFrame()

    def reportLocation(self):
        thread = self.currentThread()
        frame = thread.GetFrameAtIndex(0)
        file = fileName(frame.line_entry.file)
        line = frame.line_entry.line
        self.report('location={file="%s",line="%s",addr="%s"}' % (file, line, frame.pc))

    def reportThreads(self):
        result = 'threads={threads=['
        for thread in self.process.threads:
            result += '{id="%d"' % thread.id
            result += ',index="%s"' % thread.idx
            result += ',stop-reason="%s"' % thread.stop_reason

            if not thread.name is None:
                result += ',name="%s"' % thread.name

            result += ',frame={'
            frame = thread.GetFrameAtIndex(0)
            result += 'pc="0x%x"' % frame.pc
            result += ',addr="0x%x"' % frame.pc
            result += ',fp="0x%x"' % frame.fp
            result += ',func="%s"' % frame.function.name
            result += ',line="%s"' % frame.line_entry.line
            result += ',fullname="%s"' % fileName(frame.line_entry.file)
            result += ',file="%s"' % fileName(frame.line_entry.file)
            result += '}},'

        result += '],current-thread-id="%s"},' % self.currentThread().id
        self.report(result)

    def reportStack(self):
        if self.process is None:
            self.report('msg="No process"')
        else:
            thread = self.currentThread()
            result = 'stack={current-thread="%s",frames=[' % thread.GetThreadID()
            for frame in thread.frames:
                result += '{pc="0x%x"' % frame.pc
                result += ',level="%d"' % frame.idx
                result += ',addr="0x%x"' % frame.pc
                result += ',fp="0x%x"' % frame.fp
                result += ',func="%s"' % frame.function.name
                result += ',line="%d"' % frame.line_entry.line
                result += ',fullname="%s"' % fileName(frame.line_entry.file)
                result += ',usable="1"'
                result += ',file="%s"},' % fileName(frame.line_entry.file)

            hasmore = '0'
            result += '],hasmore="%s"},' % hasmore
            self.report(result)

    def putItem(self, value, tryDynamic=True):
        val = value.GetDynamicValue(lldb.eDynamicCanRunTarget)
        v = val.GetValue()
        numchild = 1 if val.MightHaveChildren() else 0
        self.put('iname="%s",' % self.currentIName)
        self.put('type="%s",' % val.GetTypeName())
        #self.put('vtype="%s",' % val.GetValueType())
        self.put('value="%s",' % ("" if v is None else v))
        self.put('numchild="%s",' % numchild)
        self.put('addr="0x%x",' % val.GetLoadAddress())
        if self.currentIName in self.expandedINames:
            with Children(self):
                self.putFields(value)

    def putFields(self, value):
        n = value.GetNumChildren()
        if n > 10000:
            n = 10000
        for i in xrange(n):
            field = value.GetChildAtIndex(i)
            with SubItem(self, field.GetName()):
                self.putItem(field)

    def reportVariables(self):
        frame = self.currentThread().GetSelectedFrame()
        self.currentIName = "local"
        self.put('data=[')
        for value in frame.GetVariables(True, True, False, False):
            with SubItem(self, value.GetName()):
                self.putItem(value)
        self.put(']')
        self.report('')

    def reportData(self):
        self.reportRegisters()
        if self.process is None:
            self.report('process="none"')
        else:
            state = self.process.GetState()
            if state == lldb.eStateStopped:
                self.reportStack()
                self.reportThreads()
                self.reportLocation()
                self.reportVariables()

    def reportRegisters(self):
        if self.process is None:
            self.report('process="none"')
        else:
            frame = self.currentFrame()
            result = 'registers=['
            for group in frame.GetRegisters():
                for reg in group:
                    result += '{name="%s"' % reg.GetName()
                    result += ',value="%s"' % reg.GetValue()
                    result += ',type="%s"},' % reg.GetType()
            result += ']'
            self.report(result)

    def report(self, stuff):
        sys.stdout.write(stuff)
        sys.stdout.write("@\n")

    def interruptInferior(self):
        if self.process is None:
            self.report('msg="No process"')
        else:
            #self.debugger.DispatchInputInterrupt()
            error = self.process.Stop()
            self.reportError(error)

    def detachInferior(self):
        if self.process is None:
            self.report('msg="No process"')
        else:
            error = self.process.Detach()
            self.reportError(error)
            self.reportData()

    def continueInferior(self):
        if self.process is None:
            self.report('msg="No process"')
        else:
            error = self.process.Continue()
            self.reportError(error)

    def handleEvent(self, event):
        out = lldb.SBStream()
        event.GetDescription(out)
        #warn("EVENT: %s" % event)
        type = event.GetType()
        msg = lldb.SBEvent.GetCStringFromEvent(event)
        flavor = event.GetDataFlavor()
        state = lldb.SBProcess.GetStateFromEvent(event)
        self.report('event={type="%s",data="%s",msg="%s",flavor="%s",state="%s"}'
            % (type, out.GetData(), msg, flavor, state))
        if state != self.eventState:
            self.report('state="%s"' % stateNames[state])
            self.eventState = state
            #if state == lldb.eStateExited:
            #    warn("PROCESS EXITED. %d: %s"
            #        % (self.process.GetExitStatus(), self.process.GetExitDescription()))
        if type == lldb.SBProcess.eBroadcastBitStateChanged:
            #if state == lldb.eStateStopped:
            self.reportData()
        elif type == lldb.SBProcess.eBroadcastBitInterrupt:
            pass
        elif type == lldb.SBProcess.eBroadcastBitSTDOUT:
            pass
        elif type == lldb.SBProcess.eBroadcastBitSTDERR:
            pass
        elif type == lldb.SBProcess.eBroadcastBitProfileData:
            pass

    def processEvents(self, delay = 0):
        if self.process is None:
            return

        state = self.process.GetState()
        if state == lldb.eStateInvalid or state == lldb.eStateExited:
            return

        event = lldb.SBEvent()
        while self.listener.PeekAtNextEvent(event):
            self.listener.GetNextEvent(event)
            self.handleEvent(event)


    def describeBreakpoint(self, bp, modelId):
        cond = bp.GetCondition()
        result  = '{lldbid="%s"' % bp.GetID()
        result += ',modelid="%s"' % modelId
        result += ',hitcount="%s"' % bp.GetHitCount()
        result += ',threadid="%s"' % bp.GetThreadID()
        try:
            result += ',oneshot="%s"' % (1 if bp.IsOneShot() else 0)
        except:
            pass
        result += ',enabled="%s"' % (1 if bp.IsEnabled() else 0)
        result += ',valid="%s"' % (1 if bp.IsValid() else 0)
        result += ',condition="%s"' % ("" if cond is None else cond)
        result += ',ignorecount="%s"' % bp.GetIgnoreCount()
        result += ',locations=['
        for i in xrange(bp.GetNumLocations()):
            loc = bp.GetLocationAtIndex(i)
            addr = loc.GetAddress()
            result += '{locid="%s"' % loc.GetID()
            result += ',func="%s"' % addr.GetFunction().GetName()
            result += ',enabled="%s"' % (1 if loc.IsEnabled() else 0)
            result += ',resolved="%s"' % (1 if loc.IsResolved() else 0)
            result += ',valid="%s"' % (1 if loc.IsValid() else 0)
            result += ',ignorecount="%s"' % loc.GetIgnoreCount()
            result += ',addr="%s"},' % loc.GetLoadAddress()
        result += ']},'
        return result

    def handleBreakpoints(self, toAdd, toChange, toRemove):
        result = "bkpts={added=["

        for bp in toAdd:
            bpType = bp["type"]
            if bpType == BreakpointByFileAndLine:
                bpNew = self.target.BreakpointCreateByLocation(str(bp["file"]), int(bp["line"]))
            elif bpType == BreakpointByFunction:
                bpNew = self.target.BreakpointCreateByName(bp["function"])
            elif bpType == BreakpointAtMain:
                bpNew = self.target.BreakpointCreateByName("main", self.target.GetExecutable().GetFilename())
            else:
                warn("UNKNOWN TYPE")
            bpNew.SetIgnoreCount(int(bp["ignorecount"]))
            bpNew.SetCondition(str(bp["condition"]))
            bpNew.SetEnabled(int(bp["enabled"]))
            try:
                bpNew.SetOneShot(int(bp["oneshot"]))
            except:
                pass
            #bpNew.SetCallback(breakpoint_function_wrapper, None)
            #bpNew.SetCallback(breakpoint_function_wrapper, None)
            #"breakpoint command add 1 -o \"import time; print time.asctime()\"
            #cmd = "script print(11111111)"
            #cmd = "continue"
            #self.debugger.HandleCommand(
            #    "breakpoint command add -o 'script onBreak()' %s" % bpNew.GetID())
            result += self.describeBreakpoint(bpNew, bp["modelid"])

        result += "],changed=["

        for bp in toChange:
            bpChange = self.target.FindBreakpointByID(int(bp["lldbid"]))
            bpChange.SetIgnoreCount(int(bp["ignorecount"]))
            bpChange.SetCondition(str(bp["condition"]))
            bpChange.SetEnabled(int(bp["enabled"]))
            try:
                bpChange.SetOneShot(int(bp["oneshot"]))
            except:
                pass
            result += self.describeBreakpoint(bpChange, bp["modelid"])

        result += "],removed=["

        for bp in toRemove:
            bpDead = self.target.BreakpointDelete(int(bp["lldbid"]))
            result += '{modelid="%s"}' % bp["modelid"]

        result += "]}"
        self.report(result)

    def listModules(self):
        result = 'modules=['
        for module in self.target.modules:
            result += '{file="%s"' % module.file.fullpath
            result += ',name="%s"' % module.file.basename
            #result += ',addrsize="%s"' % module.addr_size
            #result += ',triple="%s"' % module.triple
            #result += ',sections={"
            #for section in module.sections:
            #    result += '[name="%s"' % section.name
            #    result += ',addr="%s"' % section.addr
            #    result += ',size="%s"]," % section.size
            #result += '}'
            result += '},'
        result += ']'
        self.report(result)

    def executeNext(self):
        self.currentThread().StepOver()

    def executeNextI(self):
        self.currentThread().StepOver()

    def executeStep(self):
        self.currentThread().StepInto()

    def quit(self):
        self.debugger.Terminate()

    def executeStepI(self):
        self.currentThread().StepInstOver()

    def executeStepOut(self):
        self.debugger.HandleCommand("thread step-out")

    def executeRunToLine(self, file, line):
        self.thread.StepOverUntil(file, line)
        self.reportData()

    def executeJumpToLine(self):
        self.report('error={msg="Not implemented"},state="stopped"')

    def breakList(self):
        result = lldb.SBCommandReturnObject()
        self.debugger.GetCommandInterpreter().HandleCommand("break list", result)
        self.report('success="%d",output="%s",error="%s"'
            % (result.Succeeded(), result.GetOutput(), result.GetError()))

    def activateFrame(self, frame):
        self.handleCommand("frame select " + frame)

    def selectThread(self, thread):
        self.handleCommand("thread select " + thread)

    def requestModuleSymbols(self, frame):
        self.handleCommand("target module list " + frame)

    def executeDebuggerCommand(self, command):
        result = lldb.SBCommandReturnObject()
        self.debugger.GetCommandInterpreter().HandleCommand(command, result)
        success = result.Succeeded()
        if success:
            output = result.GetOutput()
            error = ''
        else:
            output = ''
            error = str(result.GetError())
        self.report('success="%d",output="%s",error="%s"' % (success, output, error))

    def setOptions(self, options):
        self.options = options

    def do_interrupt(self, args):
        self.interruptInferior()

    def do_detach(self, args):
        self.detachInferior()

    def do_continue(self, args):
        self.continueInferior()

    def do_c(self, args):
        self.continueInferior()

    def do_backtrace(self, args):
        self.reportStack()

    def do_next(self, args):
        self.executeNext()

    def do_nexti(self, args):
        self.executeNextI()

    def do_step(self, args):
        self.executeStep()

    def do_stepi(self, args):
        self.executeStepI()

    def do_finish(self, args):
        self.executeStepOut()

    def do_EOF(self, line):
        return True

    # Qt Creator internal
    def do_setupInferior(self, args):
        fileName = eval(args)
        self.setupInferior(fileName)

    def do_handleBreakpoints(self, args):
        toAdd, toChange, toRemove = eval(args)
        self.handleBreakpoints(toAdd, toChange, toRemove)

    def do_runEngine(self, args):
        self.runEngine()

    def do_interrupt(self, args):
        self.interruptInferior()

    def do_reloadRegisters(self, args):
        self.reportRegisters()

    def do_createReport(self, args):
        self.reportData()

    # Convenience
    def do_r(self, args):
        self.setupInferior(args)
        self.target.BreakpointCreateByName("main", self.db.target.GetExecutable().GetFilename())
        self.runEngine()

    def do_bb(self, args):
        options = eval(args)
        self.expandedINames = set(options['expanded'].split(','))
        self.reportData()

#execfile(os.path.join(currentDir, "dumper.py"))
#execfile(os.path.join(currentDir, "qttypes.py"))

#def importPlainDumpers(args):
#    pass

#def bbsetup(args = ''):
#    global qqDumpers, qqFormats, qqEditable
#    typeCache = {}
#
#    items = globals()
#    for key in items:
#        registerDumper(items[key])

#bbsetup()

import sys
import select

lldbLoaded = True

if __name__ == '__main__':
    db = Debugger()
    db.report('state="enginesetupok"')
    while True:
        rlist, _, _ = select.select([sys.stdin], [], [], 1)
        if rlist:
            line = sys.stdin.readline()
            if line.startswith("db "):
                (cmd, token, extra, cont) = eval(line[3:])
                db.onecmd("%s %s" % (cmd, extra))
                db.report('token="%s"' % token)
                if len(cont):
                    db.report('continuation="%s"' % cont)
            else:
                db.onecmd(line)
        else:
            db.processEvents()

