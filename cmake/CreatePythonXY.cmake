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
    aifc.py              imghdr.py        sndhdr.py
    antigravity.py       imp.py           socket.py
    argparse.py          ipaddress.py     socketserver.py
    asynchat.py          locale.py        ssl.py
    asyncore.py          lzma.py          statistics.py
    bdb.py               mailbox.py       string.py
    binhex.py            mailcap.py       stringprep.py
    bisect.py            mimetypes.py     sunau.py
    bz2.py               modulefinder.py  symbol.py
    calendar.py          netrc.py         symtable.py
    cgi.py               nntplib.py       tabnanny.py
    cgitb.py             nturl2path.py    tarfile.py
    chunk.py             numbers.py       telnetlib.py
    cmd.py               optparse.py      tempfile.py
    code.py              pathlib.py       this.py
    codeop.py            pdb.py           timeit.py
    colorsys.py          pickle.py        trace.py
    compileall.py        pickletools.py   tracemalloc.py
    configparser.py      pipes.py         tty.py
    contextvars.py       plistlib.py      turtle.py
    cProfile.py          poplib.py        uu.py
    crypt.py             pprint.py        uuid.py
    csv.py               profile.py       wave.py
    dataclasses.py       pstats.py        webbrowser.py
    decimal.py           pty.py           xdrlib.py
    difflib.py           pyclbr.py        zipapp.py
    doctest.py           py_compile.py    zipfile.py
    dummy_threading.py   queue.py         zipimport.py
    filecmp.py           quopri.py        _compat_pickle.py
    fileinput.py         random.py        _compression.py
    formatter.py         rlcompleter.py   _dummy_thread.py
    fractions.py         runpy.py         _markupbase.py
    ftplib.py            sched.py         _osx_support.py
    getopt.py            secrets.py       _pydecimal.py
    getpass.py           selectors.py     _pyio.py
    gettext.py           shelve.py        _py_abc.py
    gzip.py              shlex.py         _strptime.py
    hashlib.py           shutil.py        _threading_local.py
    hmac.py              smtpd.py         __phello__.foo.py
    imaplib.py           smtplib.py
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
