# create_python_xy function will precompile the Python/lib/*.py files
# and create a zip file containing all the pyc files
function(create_python_xy PythonExe PythonZipFilePath)
  get_filename_component(python_lib_dir "${PythonExe}" DIRECTORY)
  get_filename_component(python_lib_dir "${python_lib_dir}/Lib" ABSOLUTE)
  foreach(dir collections encodings importlib json urllib re)
      file(COPY ${python_lib_dir}/${dir}
          DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/python-lib
          FILES_MATCHING PATTERN "*.py"
      )
  endforeach()
  file(GLOB python_lib_files "${python_lib_dir}/*.py")
  foreach(not_needed
    aifc.py              imghdr.py        socket.py
    antigravity.py       imp.py           socketserver.py
    argparse.py          ipaddress.py     ssl.py
    asynchat.py          locale.py        statistics.py
    asyncore.py          lzma.py          string.py
    bdb.py               mailbox.py       stringprep.py
    binhex.py            mailcap.py       sunau.py
    bisect.py            mimetypes.py     symbol.py
    bz2.py               modulefinder.py  symtable.py
    calendar.py          netrc.py         tabnanny.py
    cgi.py               nntplib.py       tarfile.py
    cgitb.py             nturl2path.py    telnetlib.py
    chunk.py             numbers.py       tempfile.py
    cmd.py               optparse.py      textwrap.py
    code.py              pathlib.py       this.py
    codeop.py            pdb.py           timeit.py
    colorsys.py          pickle.py        trace.py
    compileall.py        pickletools.py   tracemalloc.py
    configparser.py      pipes.py         tty.py
    contextvars.py       plistlib.py      turtle.py
    cProfile.py          poplib.py        typing.py
    crypt.py             pprint.py        uu.py
    csv.py               profile.py       uuid.py
    dataclasses.py       pstats.py        wave.py
    datetime.py          pty.py           webbrowser.py
    decimal.py           pyclbr.py        xdrlib.py
    difflib.py           py_compile.py    zipapp.py
    doctest.py           queue.py         zipfile.py
    dummy_threading.py   quopri.py        zipimport.py
    filecmp.py           random.py        _compat_pickle.py
    fileinput.py         rlcompleter.py   _compression.py
    formatter.py         runpy.py         _dummy_thread.py
    fractions.py         sched.py         _markupbase.py
    ftplib.py            secrets.py       _osx_support.py
    getopt.py            selectors.py     _pydecimal.py
    getpass.py           shelve.py        _pyio.py
    gettext.py           shlex.py         _py_abc.py
    gzip.py              shutil.py        _strptime.py
    hashlib.py           smtpd.py         _threading_local.py
    hmac.py              smtplib.py       __future__.py
    imaplib.py           sndhdr.py        __phello__.foo.py
    )
    list(FIND python_lib_files "${python_lib_dir}/${not_needed}" found_not_needed)
    if (NOT found_not_needed STREQUAL "-1")
      list(REMOVE_AT python_lib_files ${found_not_needed})
    endif()
  endforeach()

  file(COPY ${python_lib_files} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/python-lib")

  set(ENV{PYTHONOPTIMIZE} "2")
  execute_process(
    COMMAND "${PythonExe}" -OO -m compileall "${CMAKE_CURRENT_BINARY_DIR}/python-lib" -b
    ${QTC_COMMAND_ERROR_IS_FATAL}
  )

  file(GLOB_RECURSE python_lib_files "${CMAKE_CURRENT_BINARY_DIR}/python-lib/*.py")
  file(REMOVE ${python_lib_files})

  file(GLOB_RECURSE python_lib_files LIST_DIRECTORIES ON "${CMAKE_CURRENT_BINARY_DIR}/python-lib/*/__pycache__$")
  file(REMOVE_RECURSE ${python_lib_files})

  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar cf "${PythonZipFilePath}" . --format=zip
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/python-lib/"
    ${QTC_COMMAND_ERROR_IS_FATAL}
  )
endfunction()
