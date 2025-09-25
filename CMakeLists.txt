cmake_minimum_required(VERSION 3.15)
project(AoC)

function(add_task year task_nr)
  cmake_path(APPEND task_dir "${CMAKE_SOURCE_DIR}" "${year}" "Task-${task_nr}")
  if(IS_DIRECTORY "${task_dir}")
    file(GLOB cpp_files CONFIGURE_DEPENDS "${task_dir}/*.cpp")
    list(LENGTH cpp_files file_count)
    if(file_count GREATER 0)
      set(target_name "aoc_${year}_${task_nr}")
      cmake_path(GET cpp_files FILENAME cpp_fn)
      cmake_path(RELATIVE_PATH task_dir OUTPUT_VARIABLE rel_task_dir)
      message(STATUS "cpp files [${cpp_fn}] found in ./${rel_task_dir}/ - adding target ${target_name}")
      ## create target
      add_executable(${target_name} "${cpp_files}")
      target_include_directories(${target_name} PRIVATE "${task_dir}")
      target_compile_definitions(${target_name} PRIVATE STANDALONE)
      set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${task_dir}"
      )
      target_compile_options(${target_name} PRIVATE -fdiagnostics-color=always)
      set_target_properties(${target_name} PROPERTIES
        OUTPUT_NAME "Task-${task_nr}"
      )
      if(EXISTS "${task_dir}/CMakeLists.txt")
        # need to export module name for usage in sub module
        set(MODULE_TARGET_NAME ${target_name} CACHE INTERNAL "Target name for subproject")
        message(STATUS "Found CMakeLists.txt in ./${rel_task_dir}/ - including it")
        add_subdirectory("${task_dir}")
      endif()
    endif()
  endif()
endfunction()

set(YEAR_FOLDER
    2015
    2016
    2017
    2021
    2024
)



foreach(year ${YEAR_FOLDER})
  foreach(task_nr RANGE 1 25)
    add_task(${year} ${task_nr})  
  endforeach()
endforeach()
    




