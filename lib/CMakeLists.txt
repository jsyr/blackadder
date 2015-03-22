# cmake configuration for library

set(LIBSRC blackadder.cpp nb_blackadder.cpp bitvector.cpp) 

add_library (core OBJECT ${LIBSRC})
add_library (blackadder STATIC $<TARGET_OBJECTS:core>)

install(TARGETS blackadder DESTINATION lib)

install(FILES blackadder.h nb_blackadder.h bitvector.h blackadder_defs.h DESTINATION include)
