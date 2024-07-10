# Copyright(C) 2006-2008, David Allouche, Jp Calderone, Itamar Shtull-Trauring
# Copyright(C) 2006-2017, Johan Dahlin
# Copyright(C) 2008, Olivier Grisel <olivier.grisel@ensta.org>
# Copyright(C) 2008, David Glick
# Copyright(C) 2013, Steven Maude
# Copyright(C) 2013-2018, Peter Waller <p@pwaller.net>
# Copyright(C) 2013, Lukas Graf <lukas.graf@4teamwork.ch>
# Copyright(C) 2013, Jamie Wong <http://jamie-wong.com>
# Copyright(C) 2013, Yury V. Zaytsev <yury@shurup.com>
# Copyright(C) 2014, Michael Droettboom <mdroe@stsci.edu>
# Copyright(C) 2015, Zev Benjamin <zev@mit.edu>
# Copyright(C) 2018, Jon Dufresne <jon.dufresne@gmail.com>
#
# All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

""" This is a stripped down and modified version of pyprof2calltree """

def profile_visualize(entries):
    import cProfile
    import io
    import subprocess
    import tempfile
    from collections import defaultdict

    def add_code_by_position(code):
        filename, _, name = cProfile.label(code)
        code_by_position[(filename, name)].add(code)

    def munged_function_name(code):
        filename, firstlineno, name = cProfile.label(code)
        if len(code_by_position[(filename, name)]) == 1:
            return name
        return "%s:%d" % (name, firstlineno)

    code_by_position = defaultdict(set)
    for entry in entries:
        add_code_by_position(entry.code)
        if entry.calls:
            for subentry in entry.calls:
                add_code_by_position(subentry.code)

    fd, outfile = tempfile.mkstemp("-qtc-debugger-profile.txt")
    with io.open(fd, "w") as out:
        scale = 1e9

        out.write('event: ns : Nanoseconds\n')
        out.write('events: ns\n')

        for entry in entries:
            code = entry.code

            filename, firstlineno, name = cProfile.label(code)
            munged_name = munged_function_name(code)

            out.write('fl=%s\nfn=%s\n' % (filename, munged_name))
            out.write('%d %d\n' % (firstlineno, int(entry.inlinetime * scale)))

            if entry.calls:
                for subentry in entry.calls:
                    filename, firstlineno, name = cProfile.label(subentry.code)
                    munged_name = munged_function_name(subentry.code)
                    out.write('cfl=%s\ncfn=%s\n' % (filename, munged_name))
                    out.write('calls=%d %d\n' % (subentry.callcount, firstlineno))
                    out.write('%d %d\n' % (firstlineno, int(subentry.totaltime * scale)))
            out.write('\n')

    subprocess.Popen(["kcachegrind", outfile])

