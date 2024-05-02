add_cov_library(ext_native INTERFACE)
target_link_libraries(ext_native INTERFACE ctre::ctre expat::expat)

set_parent_scope()
