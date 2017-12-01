project(performance CXX)

include_directories(../db)

find_package(Boost 1.40 COMPONENTS regex filesystem system program_options chrono REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

include_directories(../db)

add_definitions(-fexceptions)

set(DB_BACKEND_WRAPPER_SOURCES_CPP "")
foreach(filename ${DB_BACKEND_WRAPPER_SOURCES})
    set(DB_BACKEND_WRAPPER_SOURCES_CPP ${DB_BACKEND_WRAPPER_SOURCES_CPP} "${CMAKE_SOURCE_DIR}/${filename}")
endforeach ()

add_library(perfutils STATIC dbhelpers.cpp)

add_executable(${PROJECT_NAME} testsuite.cpp ${DB_BACKEND_WRAPPER_SOURCES_CPP})
target_link_libraries(${PROJECT_NAME}
	mojocore
	mojodb
	${Boost_LIBRARIES}
	${DB_BACKEND_LIB}
	perfutils
	atomic
)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
        DESTINATION ${WEBOS_INSTALL_LIBDIR}/${CMAKE_PROJECT_NAME}/tests
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ)

add_executable(generator generate.cpp ${DB_BACKEND_WRAPPER_SOURCES_CPP})
target_link_libraries(generator
	mojocore
	mojodb
	${Boost_LIBRARIES}
	${DB_BACKEND_LIB}
	perfutils
	atomic
)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/generator
        DESTINATION ${WEBOS_INSTALL_LIBDIR}/${CMAKE_PROJECT_NAME}/tests
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ)
