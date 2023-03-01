cmake_minimum_required(VERSION 3.22)

include(ExternalProject)
ExternalProject_Add(apr
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
    URL https://dlcdn.apache.org//apr/apr-1.7.2.tar.gz
    BUILD_ALWAYS   OFF
    UPDATE_COMMAND ""
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<BINARY_DIR> --enable-static=yes
    BUILD_COMMAND make -j8
    INSTALL_COMMAND make install &&
                    ${LIBARY_PREFIX}/bin/apr-1-config --cflags --cppflags --includes > ${CMAKE_CURRENT_BINARY_DIR}/apr-build.flags &&
                    ${LIBARY_PREFIX}/bin/apr-1-config --ldflags --link-ld --libs > ${CMAKE_CURRENT_BINARY_DIR}/apr-link.flags
)


add_library(libapr SHARED IMPORTED GLOBAL)

set(LIBARY_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/apr-prefix/src/apr)
set(LIBAPR_INCLUDE ${LIBARY_PREFIX}/include)

########################### sadly only runs at configuration time, not build time
#file(READ ${CMAKE_CURRENT_BINARY_DIR}/apr-build.flags COMP_FLAGS)
#file(READ ${CMAKE_CURRENT_BINARY_DIR}/apr-link.flags LK_FLAGS)

########################### hence we must hardcode the output of the flags
set(COMP_FLAGS -g -O2 -pthread -DLINUX -D_REENTRANT -D_GNU_SOURCE -I/home/charlesq/Projects/nxlog/recruitment-test/build/apr-prefix/src/apr/include/apr-1)
set(LK_FLAGS -L/home/charlesq/Projects/nxlog/recruitment-test/build/apr-prefix/src/apr/lib -lapr-1 -luuid -lrt -lcrypt  -lpthread -ldl)


file(MAKE_DIRECTORY ${LIBAPR_INCLUDE})

set_target_properties(libapr PROPERTIES IMPORTED_LOCATION ${LIBARY_PREFIX}/lib/libapr-1.so)
set_target_properties(libapr PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${LIBAPR_INCLUDE})
set_target_properties(libapr PROPERTIES INTERFACE_COMPILE_OPTIONS ${COMP_FLAGS})
target_link_options(libapr INTERFACE ${LK_FLAGS})