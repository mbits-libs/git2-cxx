set(SOURCES
    src/gnu.cc
    src/llvm.cc
    src/msvc.cc
    src/queue.hh
    src/report.cc
    src/report.hh
    src/toolkit.cc
    src/toolkit.hh
    src/task_watcher.cc
    src/task_watcher.hh
    src/thread_pool.cc
    src/thread_pool.hh
    src/walk.cc
    src/walk.hh
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

add_cov_library(collect-api ${SOURCES})
target_link_libraries(collect-api PUBLIC ext_native)

set_parent_scope()
