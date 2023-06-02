function(my_add_executable targetName)
  add_executable(${targetName}
    ${ARGN}
  )
endfunction()
