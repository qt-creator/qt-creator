# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import gdb
import sys
import time

# for ProcessName capture
try:
    import psutil
except:
    psutil = None

# Caps types
Address, \
Caller, \
Callstack, \
FilePos, \
Function, \
Pid, \
ProcessName, \
Tick, \
Tid, \
ThreadName, \
Expression, \
    = range(0, 11)

class GDBTracepoint(gdb.Breakpoint):
    """
    Python Breakpoint extension for "tracepoints", breakpoints that do not stop the inferior
    """

    @staticmethod
    def create(args, onModified, onHit, onExpression):
        """
        Static creator function
        """
        tp_kwargs = {}
        if 'temporary' in args.keys():
            tp_kwargs['temporary'] = args['temporary']
        spec = args['spec']
        tp = GDBTracepoint(spec, **tp_kwargs)
        tp.onModified = onModified
        tp.onHit = onHit
        tp.onExpression = onExpression
        if 'ignore_count' in args.keys():
            tp.ignore_count = args['ignore_count']
        if 'enabled' in args.keys():
            tp.enabled = args['enabled']
        if 'thread' in args.keys():
            tp.thread = args['thread']
        if 'condition' in args.keys():
            tp.condition = args['condition']
        if 'caps' in args.keys():
            for ce in args['caps']:
                tp.addCaps(ce[0], ce[1])
        return tp

    def __init__(self, spec, **kwargs):
        """
        Constructor
        """
        kwargs['internal'] = True
        super(GDBTracepoint, self).__init__(spec, **kwargs)
        self.caps = []

    _hexSize = 8 if sys.maxsize > 2**32 else 4
    _hasMonotonicTime = False if sys.version_info[0] <= 2 or (sys.version_info[0] == 3 and sys.version_info[1] < 3) else True

    def dicts(self):
        """
        Returns dictionareis for mi representation
        """
        results = []
        result = {}
        result['number'] = str(self.number)
        result['enabled'] = 'y' if self.enabled else 'n'
        result['type'] = 'pseudo_tracepoint'
        result['disp'] = 'del' if self.temporary else 'keep'
        result['times'] = str(self.hit_count)
        result['original-location'] = self.location
        try:
            d = gdb.decode_line(self.location)
            if d[1] is None:
                result['addr'] = '<PENDING>'
                result['pending'] = self.location
                results.append(result)
            else:
                if len(d[1]) > 1:
                    result['addr'] = '<MULTIPLE>'
                    results.append(result)
                    for i, sl in enumerate(d[1]):
                        result_ = {}
                        result_['number'] = result['number'] + "." + str(i + 1)
                        result_['enabled'] = 'y' if self.enabled else 'n'
                        if sl.pc is None:
                            result_['addr'] = '<no address>'
                        else:
                            result_['addr'] = '{0:#0{1}x}'.format(sl.pc, self._hexSize + 2)
                        if sl.symtab and sl.symtab.is_valid():
                            func = self._getFunctionFromAddr(sl.pc)
                            if func:
                                result_['func'] = func.print_name
                            result_['file'] = sl.symtab.filename
                            result_['fullname'] = sl.symtab.fullname()
                            result_['line'] = sl.line
                        results.append(result_)
                else:
                    sl = d[1][0]
                    if sl.pc is None:
                        result['addr'] = '<no address>'
                    else:
                        result['addr'] = '{0:#0{1}x}'.format(sl.pc, self._hexSize + 2)
                    if sl.symtab and sl.symtab.is_valid():
                        func = self._getFunctionFromAddr(sl.pc)
                        if func:
                            result['func'] = func.print_name
                        result['file'] = sl.symtab.filename
                        result['fullname'] = sl.symtab.fullname()
                        result['line'] = sl.line
                    results.append(result)
        except Exception as e:
            import traceback
            traceback.print_exc()
            result['addr'] = '<PENDING>'
            result['pending'] = self.location
            results.append(result)
        return results

    def addCaps(self, capsType, expression=None):
        """
        Adds capture expressions for a tracepoint

        :param caps_type:   Type of capture
        :param expression:  Expression for Expression caps type
        """
        if capsType != Expression:
            expression = None
        else:
            if expression is None:
                expression = ''
        self.caps.append((self.capsMap[capsType], expression))

    def stop(self):
        """
        Overridden stop function, this evaluates conditions and captures data from the inferior

        :return: Always False
        """
        try:
            self.onModified(self)
            result = {}
            result['number'] = self.number
            try:
                if self.condition:
                    try:
                        result = gdb.parse_and_eval(self.condition)
                        if result.type.code == gdb.TYPE_CODE_BOOL and str(result) == 'false':
                            return False
                    except:
                        pass
                if self.ignore_count > 0:
                    return False
                if self.thread and gdb.selected_thread().global_num != self.thread:
                    return False
            except Exception as e:
                result['warning'] = str(e)
                self.onHit(self, result)
                return False
            if len(self.caps) > 0:
                caps = []
                try:
                    for func, expr in self.caps:
                        if expr is None:
                            caps.append(func(self))
                        else:
                            caps.append(func(self, expr))
                except Exception as e:
                    result['warning'] = str(e)
                    self.onHit(self, result)
                    return False
                result['caps'] = caps
            self.onHit(self, result)
            return False
        except:
            # Always return false, regardless...
            return False

    def _getFunctionFromAddr(self, addr):
        try:
            block = gdb.block_for_pc(addr)
            while block and not block.function:
                block = block.superblock
            if block is None:
                return None
            return block.function
        except:
            return None

    def _getAddress(self):
        """
        Capture function for Address
        """
        try:
            frame = gdb.selected_frame()
            if not (frame is None) and (frame.is_valid()):
                return '{0:#0{1}x}'.format(frame.pc(), self._hexSize + 2)
        except Exception as e:
            return str(e)
        return '<null address>'

    def _getCaller(self):
        """
        Capture function for Caller
        """
        try:
            frame = gdb.selected_frame()
            if not (frame is None) and (frame.is_valid()):
                frame = frame.older()
                if not (frame is None) and (frame.is_valid()):
                    name = frame.name()
                    if name is None:
                        return '<unknown caller>'
                    return name
        except Exception as e:
            return str(e)
        return '<unknown caller>'

    def _getCallstack(self):
        """
        Capture function for Callstack
        """
        try:
            frames = []
            frame = gdb.selected_frame()
            if (frame is None) or (not frame.is_valid()):
                frames.append('<unknown frame>')
                return str(frames)
            while not (frame is None):
                func = frame.function()
                if func is None:
                    frames.append('{0:#0{1}x}'.format(frame.pc(), self._hexSize + 2))
                else:
                    sl = frame.find_sal()
                    if sl is None:
                        frames.append(func.symtab.filename)
                    else:
                        frames.append(func.symtab.filename + ':' + str(sl.line))
                frame = frame.older()
            return frames
        except Exception as e:
            return str(e)

    def _getFilePos(self):
        """
        Capture function for FilePos
        """
        try:
            frame = gdb.selected_frame()
            if (frame is None) or (not frame.is_valid()):
                return '<unknown file pos>'
            sl = frame.find_sal()
            if sl is None:
                return '<unknown file pos>'
            return sl.symtab.filename + ':' + str(sl.line)
        except Exception as e:
            return str(e)

    def _getFunction(self):
        """
        Capture function for Function
        """
        try:
            frame = gdb.selected_frame()
            if not (frame is None):
                return str(frame.name())
        except Exception as e:
            return str(e)
        return '<unknown function>'

    def _getPid(self):
        """
        Capture function for Pid
        """
        try:
            thread = gdb.selected_thread()
            if not (thread is None):
                (pid, lwpid, tid) = thread.ptid
                return str(pid)
        except Exception as e:
            return str(e)
        return '<unknown pid>'

    def _getProcessName(slef):
        """
        Capture for ProcessName
        """
        # gdb does not expose process name, neither does (standard) python
        # You can use for example psutil, but it might not be present.
        # Default to name of thread with ID 1
        inf = gdb.selected_inferior()
        if psutil is None:
            try:
                if inf is None:
                    return '<unknown process name>'
                threads = filter(lambda t: t.num == 1, list(inf.threads()))
                if len(threads) < 1:
                    return '<unknown process name>'
                thread = threads[0]
                # use thread name
                return thread.name
            except Exception as e:
                return str(e)
        else:
            return psutil.Process(inf.pid).name()

    def _getTick(self):
        """
        Capture function for Tick
        """
        if self._hasMonotonicTime:
            return str(int(time.monotonic() * 1000))
        else:
            return '<monotonic time not available>'

    def _getTid(self):
        """
        Capture function for Tid
        """
        try:
            thread = gdb.selected_thread()
            if not (thread is None):
                (pid, lwpid, tid) = thread.ptid
                if tid == 0:
                    return str(lwpid)
                else:
                    return str(tid)
        except Exception as e:
            return str(e)
        return '<unknown tid>'

    def _getThreadName(self):
        """
        Capture function for ThreadName
        """
        try:
            thread = gdb.selected_thread()
            if not (thread is None):
                return str(thread.name)
        except Exception as e:
            return str(e)
        return '<unknown thread name>'

    def _getExpression(self, expression):
        """
        Capture function for Expression

        :param expr: The expression to evaluate
        """
        try:
            value = gdb.parse_and_eval(expression)
            if value:
                return self.onExpression(self, expression, value)
        except Exception as e:
            return self.onExpression(self, expression, e)

    capsMap = {Address: _getAddress,
               Caller: _getCaller,
               Callstack: _getCallstack,
               FilePos: _getFilePos,
               Function: _getFunction,
               Pid: _getPid,
               ProcessName: _getProcessName,
               Tid: _getTid,
               Tick: _getTick,
               ThreadName: _getThreadName,
               Expression: _getExpression}
