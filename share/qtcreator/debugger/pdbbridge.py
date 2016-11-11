############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

import os
import re
import sys
import dis
import code
import glob
import base64
import signal
import string
import inspect
import traceback
import linecache
import fnmatch

class QuitException(Exception):
    pass

class Breakpoint:
    """Breakpoint class.
    Breakpoints are indexed by number through bpbynumber and by
    the file,line tuple using bplist. The former points to a
    single instance of class Breakpoint. The latter points to a
    list of such instances since there may be more than one
    breakpoint per line.
    """

    next = 1        # Next bp to be assigned
    bplist = {}     # indexed by (file, lineno) tuple
    bpbynumber = [None] # Each entry is None or an instance of Bpt
                # index 0 is unused, except for marking an
                # effective break .... see effective()

    def __init__(self, file, line, temporary=False, cond=None, funcname=None):
        self.funcname = funcname
        # Needed if funcname is not None.
        self.func_first_executable_line = None
        self.file = file    # This better be in canonical form!
        self.line = line
        self.temporary = temporary
        self.cond = cond
        self.enabled = True
        self.ignore = 0
        self.hits = 0
        self.number = Breakpoint.next
        Breakpoint.next += 1
        # Build the two lists
        self.bpbynumber.append(self)
        if (file, line) in self.bplist:
            self.bplist[file, line].append(self)
        else:
            self.bplist[file, line] = [self]

    def deleteMe(self):
        index = (self.file, self.line)
        self.bpbynumber[self.number] = None   # No longer in list
        self.bplist[index].remove(self)
        if not self.bplist[index]:
            # No more bp for this f:l combo
            del self.bplist[index]

    def enable(self):
        self.enabled = True

    def disable(self):
        self.enabled = False

    def __str__(self):
        return 'breakpoint %s at %s:%s' % (self.number, self.file, self.line)


def checkfuncname(b, frame):
    """Check whether we should break here because of `b.funcname`."""
    if not b.funcname:
        # Breakpoint was set via line number.
        if b.line != frame.f_lineno:
            # Breakpoint was set at a line with a def statement and the function
            # defined is called: don't break.
            return False
        return True

    # Breakpoint set via function name.

    if frame.f_code.co_name != b.funcname:
        # It's not a function call, but rather execution of def statement.
        return False

    # We are in the right frame.
    if not b.func_first_executable_line:
        # The function is entered for the 1st time.
        b.func_first_executable_line = frame.f_lineno

    if  b.func_first_executable_line != frame.f_lineno:
        # But we are not at the first line number: don't break.
        return False
    return True

# Determines if there is an effective (active) breakpoint at this
# line of code. Returns breakpoint number or 0 if none
def effective(file, line, frame):
    """Determine which breakpoint for this file:line is to be acted upon.

    Called only if we know there is a bpt at this
    location. Returns breakpoint that was triggered and a flag
    that indicates if it is ok to delete a temporary bp.

    """
    possibles = Breakpoint.bplist[file, line]
    for b in possibles:
        if not b.enabled:
            continue
        if not checkfuncname(b, frame):
            continue
        # Count every hit when bp is enabled
        b.hits += 1
        if not b.cond:
            # If unconditional, and ignoring go on to next, else break
            if b.ignore > 0:
                b.ignore -= 1
                continue
            else:
                # breakpoint and marker that it's ok to delete if temporary
                return (b, True)
        else:
            # Conditional bp.
            # Ignore count applies only to those bpt hits where the
            # condition evaluates to true.
            try:
                val = eval(b.cond, frame.f_globals, frame.f_locals)
                if val:
                    if b.ignore > 0:
                        b.ignore -= 1
                        # continue
                    else:
                        return (b, True)
                # else:
                #   continue
            except:
                # if eval fails, most conservative thing is to stop on
                # breakpoint regardless of ignore count. Don't delete
                # temporary, as another hint to user.
                return (b, False)
    return (None, None)



#__all__ = ['Dumper']

def find_function(funcname, filename):
    cre = re.compile(r'def\s+%s\s*[(]' % re.escape(funcname))
    try:
        fp = open(filename)
    except OSError:
        return None
    # consumer of this info expects the first line to be 1
    with fp:
        for lineno, line in enumerate(fp, start=1):
            if cre.match(line):
                return funcname, filename, lineno
    return None

class _rstr(str):
    """String that doesn't quote its repr."""
    def __repr__(self):
        return self

class Dumper:
    identchars = string.ascii_letters + string.digits + '_'
    lastcmd = ''
    use_rawinput = 1

    def __init__(self, stdin=None, stdout=None):
        self.skip = None
        self.breaks = {}
        self.fncache = {}
        self.frame_returning = None

        nosigint = False

        if stdin is not None:
            self.stdin = stdin
        else:
            self.stdin = sys.stdin
        if stdout is not None:
            self.stdout = stdout
        else:
            self.stdout = sys.stdout
        self.cmdqueue = []

        if stdout:
            self.use_rawinput = 0
        self.aliases = {}
        self.mainpyfile = ''
        self._wait_for_mainpyfile = False
        # Try to load readline if it exists
        try:
            import readline
            # remove some common file name delimiters
            readline.set_completer_delims(' \t\n`@#$%^&*()=+[{]}\\|;:\'",<>?')
        except ImportError:
            pass
        self.allow_kbdint = False
        self.nosigint = nosigint

        self.commands = {} # associates a command list to breakpoint numbers

    # Hex decoding operating on str, return str.
    def hexdecode(self, s):
        if sys.version_info[0] == 2:
            return s.decode('hex')
        return bytes.fromhex(s).decode('utf8')

    # Hex encoding operating on str or bytes, return str.
    def hexencode(self, s):
        if sys.version_info[0] == 2:
            return s.encode('hex')
        if isinstance(s, str):
            s = s.encode('utf8')
        return base64.b16encode(s).decode('utf8')
    def canonic(self, filename):
        if filename == '<' + filename[1:-1] + '>':
            return filename
        canonic = self.fncache.get(filename)
        if not canonic:
            canonic = os.path.abspath(filename)
            canonic = os.path.normcase(canonic)
            self.fncache[filename] = canonic
        return canonic

    def reset(self):
        import linecache
        linecache.checkcache()
        self.botframe = None
        self._set_stopinfo(None, None)
        self.forget()

    def trace_dispatch(self, frame, event, arg):
        if self.quitting:
            return # None
        if event == 'line':
            return self.dispatch_line(frame)
        if event == 'call':
            return self.dispatch_call(frame)
        if event == 'return':
            return self.dispatch_return(frame, arg)
        if event == 'exception':
            return self.dispatch_exception(frame, arg)
        if event == 'c_call':
            return self.trace_dispatch
        if event == 'c_exception':
            return self.trace_dispatch
        if event == 'c_return':
            return self.trace_dispatch
        print('Bdb.dispatch: unknown debugging event:', repr(event))
        return self.trace_dispatch

    def dispatch_line(self, frame):
        if self.stop_here(frame) or self.break_here(frame):
            self.user_line(frame)
            if self.quitting: raise QuitException
        return self.trace_dispatch

    def dispatch_call(self, frame):
        if self.botframe is None:
            # First call of dispatch since reset()
            self.botframe = frame.f_back # (CT) Note that this may also be None!
            return self.trace_dispatch
        if not (self.stop_here(frame) or self.break_anywhere(frame)):
            # No need to trace this function
            return # None
        # Ignore call events in generator except when stepping.
        if self.stopframe and frame.f_code.co_flags & inspect.CO_GENERATOR:
            return self.trace_dispatch
        self.user_call(frame)
        if self.quitting: raise QuitException
        return self.trace_dispatch

    def dispatch_return(self, frame, arg):
        if self.stop_here(frame) or frame == self.returnframe:
            # Ignore return events in generator except when stepping.
            if self.stopframe and frame.f_code.co_flags & inspect.CO_GENERATOR:
                return self.trace_dispatch
            try:
                self.frame_returning = frame
                self.user_return(frame, arg)
            finally:
                self.frame_returning = None
            if self.quitting: raise QuitException
            # The user issued a 'next' or 'until' command.
            if self.stopframe is frame and self.stoplineno != -1:
                self._set_stopinfo(None, None)
        return self.trace_dispatch

    def dispatch_exception(self, frame, arg):
        if self.stop_here(frame):
            # When stepping with next/until/return in a generator frame, skip
            # the internal StopIteration exception (with no traceback)
            # triggered by a subiterator run with the 'yield from' statement.
            if not (frame.f_code.co_flags & inspect.CO_GENERATOR
                    and arg[0] is StopIteration and arg[2] is None):
                self.user_exception(frame, arg)
                if self.quitting: raise QuitException
        # Stop at the StopIteration or GeneratorExit exception when the user
        # has set stopframe in a generator by issuing a return command, or a
        # next/until command at the last statement in the generator before the
        # exception.
        elif (self.stopframe and frame is not self.stopframe
                and self.stopframe.f_code.co_flags & inspect.CO_GENERATOR
                and arg[0] in (StopIteration, GeneratorExit)):
            self.user_exception(frame, arg)
            if self.quitting: raise QuitException

        return self.trace_dispatch

    # Normally derived classes don't override the following
    # methods, but they may if they want to redefine the
    # definition of stopping and breakpoints.

    def is_skipped_module(self, module_name):
        for pattern in self.skip:
            if fnmatch.fnmatch(module_name, pattern):
                return True
        return False

    def stop_here(self, frame):
        # (CT) stopframe may now also be None, see dispatch_call.
        # (CT) the former test for None is therefore removed from here.
        if self.skip and \
               self.is_skipped_module(frame.f_globals.get('__name__')):
            return False
        if frame is self.stopframe:
            if self.stoplineno == -1:
                return False
            return frame.f_lineno >= self.stoplineno
        if not self.stopframe:
            return True
        return False

    def break_here(self, frame):
        filename = self.canonic(frame.f_code.co_filename)
        if filename not in self.breaks:
            return False
        lineno = frame.f_lineno
        if lineno not in self.breaks[filename]:
            # The line itself has no breakpoint, but maybe the line is the
            # first line of a function with breakpoint set by function name.
            lineno = frame.f_code.co_firstlineno
            if lineno not in self.breaks[filename]:
                return False

        # flag says ok to delete temp. bp
        (bp, flag) = effective(filename, lineno, frame)
        if bp:
            self.currentbp = bp.number
            if (flag and bp.temporary):
                self.do_clear(str(bp.number))
            return True
        else:
            return False

    def break_anywhere(self, frame):
        return self.canonic(frame.f_code.co_filename) in self.breaks

    def _set_stopinfo(self, stopframe, returnframe, stoplineno=0):
        self.stopframe = stopframe
        self.returnframe = returnframe
        self.quitting = False
        # stoplineno >= 0 means: stop at line >= the stoplineno
        # stoplineno -1 means: don't stop at all
        self.stoplineno = stoplineno

    # Derived classes and clients can call the following methods
    # to affect the stepping state.

    def set_step(self):
        """Stop after one line of code."""
        # Issue #13183: pdb skips frames after hitting a breakpoint and running
        # step commands.
        # Restore the trace function in the caller (that may not have been set
        # for performance reasons) when returning from the current frame.
        if self.frame_returning:
            caller_frame = self.frame_returning.f_back
            if caller_frame and not caller_frame.f_trace:
                caller_frame.f_trace = self.trace_dispatch
        self._set_stopinfo(None, None)

    def set_trace(self, frame=None):
        """Start debugging from `frame`.

        If frame is not specified, debugging starts from caller's frame.
        """
        if frame is None:
            frame = sys._getframe().f_back
        self.reset()
        while frame:
            frame.f_trace = self.trace_dispatch
            self.botframe = frame
            frame = frame.f_back
        self.set_step()
        sys.settrace(self.trace_dispatch)

    def set_continue(self):
        # Don't stop except at breakpoints or when finished
        self._set_stopinfo(self.botframe, None, -1)
        if not self.breaks:
            # no breakpoints; run without debugger overhead
            sys.settrace(None)
            frame = sys._getframe().f_back
            while frame and frame is not self.botframe:
                del frame.f_trace
                frame = frame.f_back

    def set_quit(self):
        self.stopframe = self.botframe
        self.returnframe = None
        self.quitting = True
        sys.settrace(None)

    # Derived classes and clients can call the following methods
    # to manipulate breakpoints. These methods return an
    # error message if something went wrong, None if all is well.
    # Set_break prints out the breakpoint line and file:lineno.
    # Call self.get_*break*() to see the breakpoints or better
    # for bp in Breakpoint.bpbynumber: if bp: bp.bpprint().

    def set_break(self, filename, lineno, temporary=False, cond=None,
                  funcname=None):
        filename = self.canonic(filename)
        import linecache # Import as late as possible
        line = linecache.getline(filename, lineno)
        if not line:
            return 'Line %s:%d does not exist' % (filename, lineno)
        list = self.breaks.setdefault(filename, [])
        if lineno not in list:
            list.append(lineno)
        bp = Breakpoint(filename, lineno, temporary, cond, funcname)

    def _prune_breaks(self, filename, lineno):
        if (filename, lineno) not in Breakpoint.bplist:
            self.breaks[filename].remove(lineno)
        if not self.breaks[filename]:
            del self.breaks[filename]

    def clear_break(self, filename, lineno):
        filename = self.canonic(filename)
        if filename not in self.breaks:
            return 'There are no breakpoints in %s' % filename
        if lineno not in self.breaks[filename]:
            return 'There is no breakpoint at %s:%d' % (filename, lineno)
        # If there's only one bp in the list for that file,line
        # pair, then remove the breaks entry
        for bp in Breakpoint.bplist[filename, lineno][:]:
            bp.deleteMe()
        self._prune_breaks(filename, lineno)

    def clear_bpbynumber(self, arg):
        try:
            bp = self.get_bpbynumber(arg)
        except ValueError as err:
            return str(err)
        bp.deleteMe()
        self._prune_breaks(bp.file, bp.line)

    def clear_all_file_breaks(self, filename):
        filename = self.canonic(filename)
        if filename not in self.breaks:
            return 'There are no breakpoints in %s' % filename
        for line in self.breaks[filename]:
            blist = Breakpoint.bplist[filename, line]
            for bp in blist:
                bp.deleteMe()
        del self.breaks[filename]

    def clear_all_breaks(self):
        if not self.breaks:
            return 'There are no breakpoints'
        for bp in Breakpoint.bpbynumber:
            if bp:
                bp.deleteMe()
        self.breaks = {}

    def get_bpbynumber(self, arg):
        if not arg:
            raise ValueError('Breakpoint number expected')
        try:
            number = int(arg)
        except ValueError:
            raise ValueError('Non-numeric breakpoint number %s' % arg)
        try:
            bp = Breakpoint.bpbynumber[number]
        except IndexError:
            raise ValueError('Breakpoint number %d out of range' % number)
        if bp is None:
            raise ValueError('Breakpoint %d already deleted' % number)
        return bp

    def get_break(self, filename, lineno):
        filename = self.canonic(filename)
        return filename in self.breaks and \
            lineno in self.breaks[filename]

    def get_breaks(self, filename, lineno):
        filename = self.canonic(filename)
        return filename in self.breaks and \
            lineno in self.breaks[filename] and \
            Breakpoint.bplist[filename, lineno] or []

    def get_file_breaks(self, filename):
        filename = self.canonic(filename)
        if filename in self.breaks:
            return self.breaks[filename]
        else:
            return []

    def get_all_breaks(self):
        return self.breaks

    # Derived classes and clients can call the following method
    # to get a data structure representing a stack trace.

    def get_stack(self, f, t):
        stack = []
        if t and t.tb_frame is f:
            t = t.tb_next
        while f is not None:
            stack.append((f, f.f_lineno))
            if f is self.botframe:
                break
            f = f.f_back
        stack.reverse()
        i = max(0, len(stack) - 1)
        while t is not None:
            stack.append((t.tb_frame, t.tb_lineno))
            t = t.tb_next
        if f is None:
            i = max(0, len(stack) - 1)
        return stack, i

    # The following methods can be called by clients to use
    # a debugger to debug a statement or an expression.
    # Both can be given as a string, or a code object.

    def run(self, cmd, globals=None, locals=None):
        if globals is None:
            import __main__
            globals = __main__.__dict__
        if locals is None:
            locals = globals
        self.reset()
        if isinstance(cmd, str):
            cmd = compile(cmd, '<string>', 'exec')
        sys.settrace(self.trace_dispatch)
        try:
            exec(cmd, globals, locals)
        except QuitException:
            pass
        finally:
            self.quitting = True
            sys.settrace(None)

    def runeval(self, expr, globals=None, locals=None):
        if globals is None:
            import __main__
            globals = __main__.__dict__
        if locals is None:
            locals = globals
        self.reset()
        sys.settrace(self.trace_dispatch)
        try:
            return eval(expr, globals, locals)
        except QuitException:
            pass
        finally:
            self.quitting = True
            sys.settrace(None)


    # This method is more useful to debug a single function call.
    def runcall(self, func, *args, **kwds):
        self.reset()
        sys.settrace(self.trace_dispatch)
        res = None
        try:
            res = func(*args, **kwds)
        except QuitException:
            pass
        finally:
            self.quitting = True
            sys.settrace(None)
        return res


    def cmdloop(self):
        """Repeatedly accept input, parse an initial prefix
        off the received input, and dispatch to action methods, passing them
        the remainder of the line as argument.
        """
        try:
            stop = None
            while not stop:
                if self.cmdqueue:
                    line = self.cmdqueue.pop(0)
                else:
                    if self.use_rawinput:
                        try:
                            if sys.version_info[0] == 2:
                                line = raw_input('')
                            else:
                                line = input('')
                        except EOFError:
                            line = 'EOF'
                    else:
                        self.stdout.write('')
                        self.stdout.flush()
                        line = self.stdin.readline()
                        if not len(line):
                            line = 'EOF'
                        else:
                            line = line.rstrip('\r\n')
                print('LINE: %s' % line)
                stop = self.onecmd(line)
        finally:
            pass

    def parseline(self, line):
        """Parse the line into a command name and a string containing
        the arguments. Returns a tuple containing (command, args, line).
        'command' and 'args' may be None if the line couldn't be parsed.
        """
        line = line.strip()
        if not line:
            return None, None, line
        elif line[0] == '?':
            line = 'help ' + line[1:]
        elif line[0] == '!':
            if hasattr(self, 'do_shell'):
                line = 'shell ' + line[1:]
            else:
                return None, None, line
        i, n = 0, len(line)
        while i < n and line[i] in self.identchars: i = i+1
        cmd, arg = line[:i], line[i:].strip()
        return cmd, arg, line

    def onecmd(self, line):
        """Interpret the argument as though it had been typed in response
        to the prompt.

        The return value is a flag indicating whether interpretation of
        commands by the interpreter should stop.
        """
        line = str(line)
        print('LINE 0: %s' % line)
        cmd, arg, line = self.parseline(line)
        print('LINE 1: %s' % line)
        if cmd is None:
            return self.default(line)
        self.lastcmd = line
        if line == 'EOF' :
            self.lastcmd = ''
        if cmd == '':
            return self.default(line)
        else:
            func = getattr(self, 'do_' + cmd, None)
            if func:
                return func(arg)
            else:
                return self.default(line)

    def runit(self):

        print('DIR: %s' % dir())
        if sys.argv[0] == '-c':
            sys.argv = sys.argv[2:]
        else:
            sys.argv = sys.argv[1:]
        print('ARGV: %s' % sys.argv)
        mainpyfile = sys.argv[0]     # Get script filename
        sys.path.append(os.path.dirname(mainpyfile))
        print('MAIN: %s' % mainpyfile)

        while True:
            try:
                # The script has to run in __main__ namespace (or imports from
                # __main__ will break).
                #
                # So we clear up the __main__ and set several special variables
                # (this gets rid of pdb's globals and cleans old variables on restarts).

                #import __main__
                #__main__.__dict__.clear()
                #__main__.__dict__.update({'__name__'    : '__main__',
                #                          '__file__'    : mainpyfile,
                #                          '__builtins__': __builtins__,
                #                         })

                # When bdb sets tracing, a number of call and line events happens
                # BEFORE debugger even reaches user's code (and the exact sequence of
                # events depends on python version). So we take special measures to
                # avoid stopping before we reach the main script (see user_line and
                # user_call for details).
                self._wait_for_mainpyfile = True
                self.mainpyfile = self.canonic(mainpyfile)
                self._user_requested_quit = False
                with open(mainpyfile, 'rb') as fp:
                    statement = "exec(compile(%r, %r, 'exec'))" % \
                                (fp.read(), self.mainpyfile)
                self.run(statement)

                if self._user_requested_quit:
                    break
                print('The program finished')
            except SystemExit:
                # In most cases SystemExit does not warrant a post-mortem session.
                print('The program exited via sys.exit(). Exit status:')
                print(sys.exc_info()[1])
            except:
                traceback.print_exc()
                print('Uncaught exception. Entering post mortem debugging')
                print('Running "cont" or "step" will restart the program')
                t = sys.exc_info()[2]
                self.interaction(None, t)
                print('Post mortem debugger finished. The ' + mainpyfile +
                      ' will be restarted')

    def sigint_handler(self, signum, frame):
        if self.allow_kbdint:
            raise KeyboardInterrupt
        self.report('state="stopped"')
        self.set_step()
        self.set_trace(frame)
        # restore previous signal handler
        signal.signal(signal.SIGINT, self._previous_sigint_handler)

    def forget(self):
        self.lineno = None
        self.stack = []
        self.curindex = 0
        self.curframe = None

    def setup(self, f, tb):
        self.forget()
        self.stack, self.curindex = self.get_stack(f, tb)
        self.curframe = self.stack[self.curindex][0]
        # The f_locals dictionary is updated from the actual frame
        # locals whenever the .f_locals accessor is called, so we
        # cache it here to ensure that modifications are not overwritten.
        self.curframe_locals = self.curframe.f_locals

    def user_call(self, frame):
        """This method is called when there is the remote possibility
        that we ever need to stop in this function."""
        if self._wait_for_mainpyfile:
            return
        if self.stop_here(frame):
            self.message('--Call--')
            self.interaction(frame, None)

    def user_line(self, frame):
        """This function is called when we stop or break at this line."""
        if self._wait_for_mainpyfile:
            if (self.mainpyfile != self.canonic(frame.f_code.co_filename)
                or frame.f_lineno <= 0):
                return
            self._wait_for_mainpyfile = False
        if self.bp_commands(frame):
            self.interaction(frame, None)

    def bp_commands(self, frame):
        """Call every command that was set for the current active breakpoint
        (if there is one).

        Returns True if the normal interaction function must be called,
        False otherwise."""
        # self.currentbp is set in break_here if a breakpoint was hit
        if getattr(self, 'currentbp', False) and self.currentbp in self.commands:
            currentbp = self.currentbp
            self.currentbp = 0
            lastcmd_back = self.lastcmd
            self.setup(frame, None)
            for line in self.commands[currentbp]:
                self.onecmd(line)
            self.lastcmd = lastcmd_back
            self.forget()
            return
        return 1

    def user_return(self, frame, return_value):
        """This function is called when a return trap is set here."""
        if self._wait_for_mainpyfile:
            return
        frame.f_locals['__return__'] = return_value
        self.message('--Return--')
        self.interaction(frame, None)

    def user_exception(self, frame, exc_info):
        """This function is called if an exception occurs,
        but only if we are to stop at or just below this level."""
        if self._wait_for_mainpyfile:
            return
        exc_type, exc_value, exc_traceback = exc_info
        frame.f_locals['__exception__'] = exc_type, exc_value

        # An 'Internal StopIteration' exception is an exception debug event
        # issued by the interpreter when handling a subgenerator run with
        # 'yield from' or a generator controled by a for loop. No exception has
        # actually occurred in this case. The debugger uses this debug event to
        # stop when the debuggee is returning from such generators.
        prefix = 'Internal ' if (not exc_traceback
                                    and exc_type is StopIteration) else ''
        self.message('%s%s' % (prefix,
            traceback.format_exception_only(exc_type, exc_value)[-1].strip()))
        self.interaction(frame, exc_traceback)

    def interaction(self, frame, traceback):
        if self.setup(frame, traceback):
            # no interaction desired at this time (happens if .pdbrc contains
            # a command like 'continue')
            self.forget()
            return

        frame, lineNumber = self.stack[self.curindex]
        fileName = self.canonic(frame.f_code.co_filename)
        self.report('location={file="%s",line="%s"}' % (fileName, lineNumber))

        while True:
            try:
                # keyboard interrupts allow for an easy way to cancel
                # the current command, so allow them during interactive input
                self.allow_kbdint = True
                self.cmdloop()
                self.allow_kbdint = False
                break
            except KeyboardInterrupt:
                self.message('--KeyboardInterrupt--')
        self.forget()

    def displayhook(self, obj):
        """Custom displayhook for the exec in default(), which prevents
        assignment of the _ variable in the builtins.
        """
        # reproduce the behavior of the standard displayhook, not printing None
        if obj is not None:
            self.message(repr(obj))

    def default(self, line):
        if line[:1] == '!': line = line[1:]
        locals = self.curframe_locals
        globals = self.curframe.f_globals
        try:
            code = compile(line + '\n', '<stdin>', 'single')
            save_stdout = sys.stdout
            save_stdin = sys.stdin
            save_displayhook = sys.displayhook
            try:
                sys.stdin = self.stdin
                sys.stdout = self.stdout
                sys.displayhook = self.displayhook
                exec(code, globals, locals)
            finally:
                sys.stdout = save_stdout
                sys.stdin = save_stdin
                sys.displayhook = save_displayhook
        except:
            exc_info = sys.exc_info()[:2]
            self.error(traceback.format_exception_only(*exc_info)[-1].strip())

    def message(self, msg):
        print(msg)
        pass

    def error(self, msg):
        #print('***'+ msg)
        pass

    def do_break(self, arg, temporary = 0):
        """b(reak) [ ([filename:]lineno | function) [, condition] ]
        Without argument, list all breaks.

        With a line number argument, set a break at this line in the
        current file. With a function name, set a break at the first
        executable line of that function. If a second argument is
        present, it is a string specifying an expression which must
        evaluate to true before the breakpoint is honored.

        The line number may be prefixed with a filename and a colon,
        to specify a breakpoint in another file (probably one that
        hasn't been loaded yet). The file is searched for on
        sys.path; the .py suffix may be omitted.
        """
        if not arg:
            if self.breaks:  # There's at least one
                self.message('Num Type         Disp Enb   Where')
                for bp in Breakpoint.bpbynumber:
                    if bp:
                        self.message(bp.bpformat())
            return
        # parse arguments; comma has lowest precedence
        # and cannot occur in filename
        filename = None
        lineno = None
        cond = None
        comma = arg.find(',')
        if comma > 0:
            # parse stuff after comma: 'condition'
            cond = arg[comma+1:].lstrip()
            arg = arg[:comma].rstrip()
        # parse stuff before comma: [filename:]lineno | function
        colon = arg.rfind(':')
        funcname = None
        if colon >= 0:
            filename = arg[:colon].rstrip()
            f = self.lookupmodule(filename)
            if not f:
                self.error('%r not found from sys.path' % filename)
                return
            else:
                filename = f
            arg = arg[colon+1:].lstrip()
            try:
                lineno = int(arg)
            except ValueError:
                self.error('Bad lineno: %s' % arg)
                return
        else:
            # no colon; can be lineno or function
            try:
                lineno = int(arg)
            except ValueError:
                try:
                    func = eval(arg,
                                self.curframe.f_globals,
                                self.curframe_locals)
                except:
                    func = arg
                try:
                    if hasattr(func, '__func__'):
                        func = func.__func__
                    code = func.__code__
                    #use co_name to identify the bkpt (function names
                    #could be aliased, but co_name is invariant)
                    funcname = code.co_name
                    lineno = code.co_firstlineno
                    filename = code.co_filename
                except:
                    # last thing to try
                    (ok, filename, ln) = self.lineinfo(arg)
                    if not ok:
                        self.error('The specified object %r is not a function '
                                   'or was not found along sys.path.' % arg)
                        return
                    funcname = ok # ok contains a function name
                    lineno = int(ln)
        if not filename:
            filename = self.defaultFile()
        # Check for reasonable breakpoint
        line = self.checkline(filename, lineno)
        if line:
            # now set the break point
            err = self.set_break(filename, line, temporary, cond, funcname)
            if err:
                self.error(err)
            else:
                bp = self.get_breaks(filename, line)[-1]
                self.message('Breakpoint %d at %s:%d' %
                             (bp.number, bp.file, bp.line))

    # To be overridden in derived debuggers
    def defaultFile(self):
        """Produce a reasonable default."""
        filename = self.curframe.f_code.co_filename
        if filename == '<string>' and self.mainpyfile:
            filename = self.mainpyfile
        return filename

    def do_tbreak(self, arg):
        """tbreak [ ([filename:]lineno | function) [, condition] ]
        Same arguments as break, but sets a temporary breakpoint: it
        is automatically deleted when first hit.
        """
        self.do_break(arg, 1)

    def lineinfo(self, identifier):
        failed = (None, None, None)
        # Input is identifier, may be in single quotes
        idstring = identifier.split("'")
        if len(idstring) == 1:
            # not in single quotes
            id = idstring[0].strip()
        elif len(idstring) == 3:
            # quoted
            id = idstring[1].strip()
        else:
            return failed
        if id == '': return failed
        parts = id.split('.')
        # Protection for derived debuggers
        if parts[0] == 'self':
            del parts[0]
            if len(parts) == 0:
                return failed
        # Best first guess at file to look at
        fname = self.defaultFile()
        if len(parts) == 1:
            item = parts[0]
        else:
            # More than one part.
            # First is module, second is method/class
            f = self.lookupmodule(parts[0])
            if f:
                fname = f
            item = parts[1]
        answer = find_function(item, fname)
        return answer or failed

    def checkline(self, filename, lineno):
        """Check whether specified line seems to be executable.

        Return `lineno` if it is, 0 if not (e.g. a docstring, comment, blank
        line or EOF). Warning: testing is not comprehensive.
        """
        # this method should be callable before starting debugging, so default
        # to "no globals" if there is no current frame
        globs = self.curframe.f_globals if hasattr(self, 'curframe') else None
        line = linecache.getline(filename, lineno, globs)
        if not line:
            self.message('End of file')
            return 0
        line = line.strip()
        # Don't allow setting breakpoint at a blank line
        if (not line or (line[0] == '#') or
             (line[:3] == '"""') or line[:3] == "'''"):
            self.error('Blank or comment')
            return 0
        return lineno

    def do_enable(self, arg):
        """enable bpnumber [bpnumber ...]
        Enables the breakpoints given as a space separated list of
        breakpoint numbers.
        """
        args = arg.split()
        for i in args:
            try:
                bp = self.get_bpbynumber(i)
            except ValueError as err:
                self.error(err)
            else:
                bp.enable()
                self.message('Enabled %s' % bp)

    def do_disable(self, arg):
        """disable bpnumber [bpnumber ...]
        Disables the breakpoints given as a space separated list of
        breakpoint numbers. Disabling a breakpoint means it cannot
        cause the program to stop execution, but unlike clearing a
        breakpoint, it remains in the list of breakpoints and can be
        (re-)enabled.
        """
        args = arg.split()
        for i in args:
            try:
                bp = self.get_bpbynumber(i)
            except ValueError as err:
                self.error(err)
            else:
                bp.disable()
                self.message('Disabled %s' % bp)

    def do_condition(self, arg):
        """condition bpnumber [condition]
        Set a new condition for the breakpoint, an expression which
        must evaluate to true before the breakpoint is honored. If
        condition is absent, any existing condition is removed; i.e.,
        the breakpoint is made unconditional.
        """
        args = arg.split(' ', 1)
        try:
            cond = args[1]
        except IndexError:
            cond = None
        try:
            bp = self.get_bpbynumber(args[0].strip())
        except IndexError:
            self.error('Breakpoint number expected')
        except ValueError as err:
            self.error(err)
        else:
            bp.cond = cond
            if not cond:
                self.message('Breakpoint %d is now unconditional.' % bp.number)
            else:
                self.message('New condition set for breakpoint %d.' % bp.number)

    def do_ignore(self, arg):
        """ignore bpnumber [count]
        Set the ignore count for the given breakpoint number. If
        count is omitted, the ignore count is set to 0. A breakpoint
        becomes active when the ignore count is zero. When non-zero,
        the count is decremented each time the breakpoint is reached
        and the breakpoint is not disabled and any associated
        condition evaluates to true.
        """
        args = arg.split()
        try:
            count = int(args[1].strip())
        except:
            count = 0
        try:
            bp = self.get_bpbynumber(args[0].strip())
        except IndexError:
            self.error('Breakpoint number expected')
        except ValueError as err:
            self.error(err)
        else:
            bp.ignore = count
            if count > 0:
                if count > 1:
                    countstr = '%d crossings' % count
                else:
                    countstr = '1 crossing'
                self.message('Will ignore next %s of breakpoint %d.' %
                             (countstr, bp.number))
            else:
                self.message('Will stop next time breakpoint %d is reached.'
                             % bp.number)

    def do_clear(self, arg):
        """cl(ear) filename:lineno\ncl(ear) [bpnumber [bpnumber...]]
        With a space separated list of breakpoint numbers, clear
        those breakpoints. Without argument, clear all breaks (but
        first ask confirmation). With a filename:lineno argument,
        clear all breaks at that line in that file.
        """
        if not arg:
            try:
                reply = input('Clear all breaks? ')
            except EOFError:
                reply = 'no'
            reply = reply.strip().lower()
            if reply in ('y', 'yes'):
                bplist = [bp for bp in Breakpoint.bpbynumber if bp]
                self.clear_all_breaks()
                for bp in bplist:
                    self.message('Deleted %s' % bp)
            return
        if ':' in arg:
            # Make sure it works for 'clear C:\foo\bar.py:12'
            i = arg.rfind(':')
            filename = arg[:i]
            arg = arg[i+1:]
            try:
                lineno = int(arg)
            except ValueError:
                err = 'Invalid line number (%s)' % arg
            else:
                bplist = self.get_breaks(filename, lineno)
                err = self.clear_break(filename, lineno)
            if err:
                self.error(err)
            else:
                for bp in bplist:
                    self.message('Deleted %s' % bp)
            return
        numberlist = arg.split()
        for i in numberlist:
            try:
                bp = self.get_bpbynumber(i)
            except ValueError as err:
                self.error(err)
            else:
                self.clear_bpbynumber(i)
                self.message('Deleted %s' % bp)

    def do_until(self, arg):
        """until [lineno]
        Without argument, continue execution until the line with a
        number greater than the current one is reached. With a line
        number, continue execution until a line with a number greater
        or equal to that is reached. In both cases, also stop when
        the current frame returns.
        """
        if arg:
            try:
                lineno = int(arg)
            except ValueError:
                self.error('Error in argument: %r' % arg)
                return
            if lineno <= self.curframe.f_lineno:
                self.error('"until" line number is smaller than current '
                           'line number')
                return
        else:
            lineno = None
            lineno = self.curframe.f_lineno + 1
        self._set_stopinfo(self.curframe, self.curframe, lineno)

        return 1

    def do_step(self, arg):
        self.set_step()
        return 1

    def do_next(self, arg):
        self._set_stopinfo(self.curframe, None)
        return 1

    def do_return(self, arg):
        if self.curframe.f_code.co_flags & inspect.CO_GENERATOR:
            self._set_stopinfo(self.curframe, None, -1)
        else:
            self._set_stopinfo(self.curframe.f_back, self.curframe)
        return 1

    def do_continue(self, arg):
        """continue
        Continue execution, only stop when a breakpoint is encountered.
        """
        if not self.nosigint:
            try:
                self._previous_sigint_handler = \
                    signal.signal(signal.SIGINT, self.sigint_handler)
            except ValueError:
                # ValueError happens when do_continue() is invoked from
                # a non-main thread in which case we just continue without
                # SIGINT set. Would printing a message here (once) make
                # sense?
                pass
        self.set_continue()
        return 1

    def do_jump(self, arg):
        """jump lineno
        Set the next line that will be executed. Only available in
        the bottom-most frame. This lets you jump back and execute
        code again, or jump forward to skip code that you don't want
        to run.

        It should be noted that not all jumps are allowed -- for
        instance it is not possible to jump into the middle of a
        for loop or out of a finally clause.
        """
        if self.curindex + 1 != len(self.stack):
            self.error('You can only jump within the bottom frame')
            return
        try:
            arg = int(arg)
        except ValueError:
            self.error("The 'jump' command requires a line number")
        else:
            try:
                # Do the jump, fix up our copy of the stack, and display the
                # new position
                self.curframe.f_lineno = arg
                self.stack[self.curindex] = self.stack[self.curindex][0], arg
            except ValueError as e:
                self.error('Jump failed: %s' % e)

    def do_debug(self, arg):
        """debug code
        Enter a recursive debugger that steps through the code
        argument (which is an arbitrary expression or statement to be
        executed in the current environment).
        """
        sys.settrace(None)
        globals = self.curframe.f_globals
        locals = self.curframe_locals
        p = Dumper(self.stdin, self.stdout)
        self.message('ENTERING RECURSIVE DEBUGGER')
        sys.call_tracing(p.run, (arg, globals, locals))
        self.message('LEAVING RECURSIVE DEBUGGER')
        sys.settrace(self.trace_dispatch)
        self.lastcmd = p.lastcmd

    def do_quit(self, arg):
        """q(uit)\nexit
        Quit from the debugger. The program being executed is aborted.
        """
        self._user_requested_quit = True
        self.set_quit()
        return 1

    def do_EOF(self, arg):
        """EOF
        Handles the receipt of EOF as a command.
        """
        self.message('')
        self._user_requested_quit = True
        self.set_quit()
        return 1

    def do_args(self, arg):
        """a(rgs)
        Print the argument list of the current function.
        """
        co = self.curframe.f_code
        dict = self.curframe_locals
        n = co.co_argcount
        if co.co_flags & 4: n = n+1
        if co.co_flags & 8: n = n+1
        for i in range(n):
            name = co.co_varnames[i]
            if name in dict:
                self.message('%s = %r' % (name, dict[name]))
            else:
                self.message('%s = *** undefined ***' % (name,))

    def do_retval(self, arg):
        """retval
        Print the return value for the last return of a function.
        """
        if '__return__' in self.curframe_locals:
            self.message(repr(self.curframe_locals['__return__']))
        else:
            self.error('Not yet returned!')

    def _getval(self, arg):
        try:
            return eval(arg, self.curframe.f_globals, self.curframe_locals)
        except:
            exc_info = sys.exc_info()[:2]
            self.error(traceback.format_exception_only(*exc_info)[-1].strip())
            raise

    def _getval_except(self, arg, frame=None):
        try:
            if frame is None:
                return eval(arg, self.curframe.f_globals, self.curframe_locals)
            else:
                return eval(arg, frame.f_globals, frame.f_locals)
        except:
            exc_info = sys.exc_info()[:2]
            err = traceback.format_exception_only(*exc_info)[-1].strip()
            return _rstr('** raised %s **' % err)

    def do_whatis(self, arg):
        """whatis arg
        Print the type of the argument.
        """
        try:
            value = self._getval(arg)
        except:
            # _getval() already printed the error
            return
        code = None
        # Is it a function?
        try:
            code = value.__code__
        except Exception:
            pass
        if code:
            self.message('Function %s' % code.co_name)
            return
        # Is it an instance method?
        try:
            code = value.__func__.__code__
        except Exception:
            pass
        if code:
            self.message('Method %s' % code.co_name)
            return
        # Is it a class?
        if value.__class__ is type:
            self.message('Class %s.%s' % (value.__module__, value.__name__))
            return
        # None of the above...
        self.message(type(value))

    def do_interact(self, arg):
        """interact

        Start an interactive interpreter whose global namespace
        contains all the (global and local) names found in the current scope.
        """
        ns = self.curframe.f_globals.copy()
        ns.update(self.curframe_locals)
        code.interact('*interactive*', local=ns)

    def lookupmodule(self, filename):
        """Helper function for break/clear parsing -- may be overridden.

        lookupmodule() translates (possibly incomplete) file or module name
        into an absolute file name.
        """
        if os.path.isabs(filename) and os.path.exists(filename):
            return filename
        f = os.path.join(sys.path[0], filename)
        if  os.path.exists(f) and self.canonic(f) == self.mainpyfile:
            return f
        root, ext = os.path.splitext(filename)
        if ext == '':
            filename = filename + '.py'
        if os.path.isabs(filename):
            return filename
        for dirname in sys.path:
            while os.path.islink(dirname):
                dirname = os.readlink(dirname)
            fullname = os.path.join(dirname, filename)
            if os.path.exists(fullname):
                return fullname
        return None

    def evaluateTooltip(self, args):
        self.updateData(args)

    def updateData(self, args):
        self.expandedINames = set(args.get('expanded', []))
        self.typeformats = args.get('typeformats', {})
        self.formats = args.get('formats', {})
        self.output = ''

        frameNr = args.get('frame', 0)
        if frameNr == -1:
            frameNr = 0

        frame_lineno = self.stack[-1-frameNr]
        frame, lineno = frame_lineno
        filename = self.canonic(frame.f_code.co_filename)

        self.output += 'data={'
        for var in frame.f_locals.keys():
            if var in ('__file__', '__name__', '__package__', '__spec__',
                       '__doc__', '__loader__', '__cached__', '__the_dumper__'):
                continue
            value = frame.f_locals[var]
            # this applies only for anonymous arguments
            # e.g. def dummy(var, (width, height), var2) would create an anonymous local var
            # named '.1' for (width, height) as this is the second argument
            if var.startswith('.'):
                var = '@arg' + var[1:]
            self.dumpValue(value, var, 'local.%s' % var)

        for watcher in args.get('watchers', []):
            iname = watcher['iname']
            exp = self.hexdecode(watcher['exp'])
            exp = str(exp).strip()
            escapedExp = self.hexencode(exp)
            self.put('{')
            self.putField('iname', iname)
            self.putField('wname', escapedExp)
            try:
                res = eval(exp, {}, frame.f_locals)
                self.putValue(res)
            except:
                self.putValue('<unavailable>')
            self.put('}')
            #self.dumpValue(eval(value), escapedExp, iname)

        self.output += '}'
        self.output += '{frame="%s"}' % frameNr
        self.flushOutput()

    def flushOutput(self):
        sys.stdout.write('@\n' + self.output + '@\n')
        sys.stdout.flush()
        self.output = ''

    def put(self, value):
        #sys.stdout.write(value)
        self.output += value

    def putField(self, name, value):
        self.put('%s="%s",' % (name, value))

    def putItemCount(self, count):
        self.put('value="<%s items>",numchild="%s",' % (count, count))

    def cleanType(self, type):
        t = str(type)
        if t.startswith("<type '") and t.endswith("'>"):
            t = t[7:-2]
        if t.startswith("<class '") and t.endswith("'>"):
            t = t[8:-2]
        return t

    def putType(self, type, priority = 0):
        self.putField('type', self.cleanType(type))

    def putNumChild(self, numchild):
        self.put('numchild="%s",' % numchild)

    def putValue(self, value, encoding = None, priority = 0):
        self.putField('value', value)

    def putName(self, name):
        self.put('name="%s",' % name)

    def isExpanded(self, iname):
        #self.warn('IS EXPANDED: %s in %s' % (iname, self.expandedINames))
        if iname.startswith('None'):
            raise "Illegal iname '%s'" % iname
        #self.warn('   --> %s' % (iname in self.expandedINames))
        return iname in self.expandedINames

    def isExpandedIName(self, iname):
        return iname in self.expandedINames

    def itemFormat(self, item):
        format = self.formats.get(str(cleanAddress(item.value.address)))
        if format is None:
            format = self.typeformats.get(str(item.value.type))
        return format

    # Hex encoding operating on str or bytes, return str.
    def hexencode(self, s):
        if sys.version_info[0] == 2:
            return s.encode('hex')
        if isinstance(s, str):
            s = s.encode('utf8')
        return base64.b16encode(s).decode('utf8')

    def dumpValue(self, value, name, iname):
        t = type(value)
        tt = self.cleanType(t)
        if tt == 'module' or tt == 'function':
            return
        if str(value).startswith("<class '"):
            return
        # FIXME: Should we?
        if str(value).startswith('<enum-item '):
            return
        self.put('{')
        self.putField('iname', iname)
        self.putName(name)
        if tt == 'NoneType':
            self.putType(tt)
            self.putValue('None')
            self.putNumChild(0)
        elif tt == 'list' or tt == 'tuple':
            self.putType(tt)
            self.putItemCount(len(value))
            #self.putValue(value)
            self.put('children=[')
            for i in range(len(value)):
                self.dumpValue(value[i], str(i), '%s.%d' % (iname, i))
            self.put(']')
        elif tt == 'str':
            v = value
            self.putType(tt)
            self.putValue(self.hexencode(v))
            self.putField('valueencoded', 'utf8')
            self.putNumChild(0)
        elif tt == 'unicode':
            v = value
            self.putType(tt)
            self.putValue(self.hexencode(v))
            self.putField('valueencoded', 'utf8')
            self.putNumChild(0)
        elif tt == 'buffer':
            v = str(value)
            self.putType(tt)
            self.putValue(self.hexencode(v))
            self.putField('valueencoded', 'latin1')
            self.putNumChild(0)
        elif tt == 'xrange':
            b = iter(value).next()
            e = b + len(value)
            self.putType(tt)
            self.putValue('(%d, %d)' % (b, e))
            self.putNumChild(0)
        elif tt == 'dict':
            self.putType(tt)
            self.putItemCount(len(value))
            self.putField('childnumchild', 2)
            self.put('children=[')
            i = 0
            if sys.version_info[0] >= 3:
                vals = value.items()
            else:
                vals = value.iteritems()
            for (k, v) in vals:
                self.put('{')
                self.putType(' ')
                self.putValue('%s: %s' % (k, v))
                if self.isExpanded(iname):
                    self.put('children=[')
                    self.dumpValue(k, 'key', '%s.%d.k' % (iname, i))
                    self.dumpValue(v, 'value', '%s.%d.v' % (iname, i))
                    self.put(']')
                self.put('},')
                i += 1
            self.put(']')
        elif tt == 'class':
            pass
        elif tt == 'module':
            pass
        elif tt == 'function':
            pass
        elif str(value).startswith('<enum-item '):
            # FIXME: Having enums always shown like this is not nice.
            self.putType(tt)
            self.putValue(str(value)[11:-1])
            self.putNumChild(0)
        else:
            v = str(value)
            p = v.find(' object at ')
            if p > 1:
                self.putValue('@' + v[p + 11:-1])
                self.putType(v[1:p])
            else:
                p = v.find(' instance at ')
                if p > 1:
                    self.putValue('@' + v[p + 13:-1])
                    self.putType(v[1:p])
                else:
                    self.putType(tt)
                    self.putValue(v)
            if self.isExpanded(iname):
                self.put('children=[')
                for child in dir(value):
                    if child in ('__dict__', '__doc__', '__module__'):
                        continue
                    attr = getattr(value, child)
                    if callable(attr):
                        continue
                    try:
                        self.dumpValue(attr, child, '%s.%s' % (iname, child))
                    except:
                        pass
                self.put('],')
        self.put('},')


    def warn(self, msg):
        self.putField('warning', msg)

    def listModules(self, args):
        self.put('modules=[');
        for name in sys.modules:
            self.put('{')
            self.putName(name)
            self.putValue(sys.modules[name])
            self.put('},')
        self.put(']')
        self.flushOutput()

    def listSymbols(self, args):
        moduleName = args['module']
        module = sys.modules.get(moduleName, None)
        self.put("symbols={module='%s',symbols='%s'}"
                % (module, dir(module) if module else []))
        self.flushOutput()

    def assignValue(self, args):
        exp = args['expression']
        value = args['value']
        cmd = '%s=%s' % (exp, exp, value)
        eval(cmd, {})
        self.put('CMD: "%s"' % cmd)
        self.flushOutput()

    def stackListFrames(self, args):
        #result = 'stack={current-thread="%s"' % thread.GetThreadID()
        result = 'stack={current-thread="%s"' % 1

        result += ',frames=['
        try:
            level = 0
            frames = list(reversed(self.stack))
            frames = frames[:-2]  # Drop "pdbbridge" and "<string>" levels
            for frame_lineno in frames:
                frame, lineno = frame_lineno
                filename = self.canonic(frame.f_code.co_filename)
                level += 1
                result += '{'
                result += 'file="%s",' % filename
                result += 'line="%s",' % lineno
                result += 'level="%s",' % level
                result += '}'
        except KeyboardInterrupt:
            pass
        result += ']'

        #result += ',hasmore="%d"' % isLimited
        #result += ',limit="%d"' % limit
        result += '}'
        self.report(result)

    def report(self, stuff):
        sys.stdout.write('@\n' + stuff + '@\n')
        sys.stdout.flush()


def qdebug(cmd, args):
    global __the_dumper__
    method = getattr(__the_dumper__, cmd)
    method(args)

__the_dumper__=Dumper()
__the_dumper__.runit()
