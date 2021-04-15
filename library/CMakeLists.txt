message(STATUS "PROJECT_VERSION=${PROJECT_VERSION}")
if(NOT stiffer_VERSION_MAJOR)
	set(stiffer_VERSION_MAJOR 0)
endif()
if(NOT stiffer_VERSION_MINOR)
	set(stiffer_VERSION_MINOR 0)
endif()
if(NOT stiffer_VERSION_PATCH)
	set(stiffer_VERSION_PATCH 0)
endif()
message(STATUS "stiffer_VERSION=${stiffer_VERSION}")
message(STATUS "stiffer_VERSION_MAJOR=${stiffer_VERSION_MAJOR}")
message(STATUS "stiffer_VERSION_MINOR=${stiffer_VERSION_MINOR}")
message(STATUS "stiffer_VERSION_PATCH=${stiffer_VERSION_PATCH}")
add_compile_definitions(STIFFER_VERSION_MAJOR=${stiffer_VERSION_MAJOR})
add_compile_definitions(STIFFER_VERSION_MINOR=${stiffer_VERSION_MINOR})
add_compile_definitions(STIFFER_VERSION_PATCH=${stiffer_VERSION_PATCH})

file(GLOB STIFFER_SRCS
	"*.cpp"
)
file(GLOB STIFFER_HDRS
	"*.hpp"
)
include_directories( ../ )

if(STIFFER_BUILD_SHARED)
	add_library(stiffer_shared SHARED
		${STIFFER_HDRS}
		${STIFFER_SRCS}
	)
	target_compile_features(stiffer PUBLIC cxx_std_17)
	set_target_properties(stiffer_shared PROPERTIES
		OUTPUT_NAME "stiffer"
		CLEAN_DIRECT_OUTPUT 1
		VERSION ${STIFFER_VERSION}
	)
endif()

if(STIFFER_BUILD_STATIC)
	add_library(stiffer STATIC
		${STIFFER_HDRS}
		${STIFFER_SRCS}
	)
	target_compile_features(stiffer PUBLIC cxx_std_17)
	set_target_properties(stiffer PROPERTIES
		CLEAN_DIRECT_OUTPUT 1
		VERSION ${STIFFER_VERSION}
	)
endif()

# These are used to create visual studio folders.
source_group(Collision FILES ${STIFFER_Collision_SRCS} ${STIFFER_Collision_HDRS})
source_group(Collision\\Shapes FILES ${STIFFER_Shapes_SRCS} ${STIFFER_Shapes_HDRS})
source_group(Common FILES ${STIFFER_Common_SRCS} ${STIFFER_Common_HDRS})
source_group(Dynamics FILES ${STIFFER_Dynamics_SRCS} ${STIFFER_Dynamics_HDRS})
source_group(Dynamics\\Contacts FILES ${STIFFER_Contacts_SRCS} ${STIFFER_Contacts_HDRS})
source_group(Dynamics\\Joints FILES ${STIFFER_Joints_SRCS} ${STIFFER_Joints_HDRS})
source_group(Include FILES ${STIFFER_General_HDRS})

if(STIFFER_INSTALL)
	# install headers
	install(FILES ${STIFFER_HDRS} DESTINATION include/stiffer)

	# install libraries
	if(STIFFER_BUILD_SHARED)
		install(TARGETS stiffer_shared EXPORT stiffer-targets
                  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
                  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
                  RUNTIME DESTINATION bin)
	endif()
	if(STIFFER_BUILD_STATIC)
		install(TARGETS stiffer EXPORT stiffer-targets DESTINATION ${LIB_INSTALL_DIR})
	endif()

	# install build system hooks for third-party apps
	install(EXPORT stiffer-targets DESTINATION ${LIB_INSTALL_DIR}/stiffer)

	set(STIFFER_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include)
	set(STIFFER_INCLUDE_DIRS ${STIFFER_INCLUDE_DIR} )
	set(STIFFER_LIBRARY_DIRS ${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR})
	set(STIFFER_LIBRARY stiffer)
	set(STIFFER_LIBRARIES ${STIFFER_LIBRARY})
	set(STIFFER_USE_FILE ${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}/cmake/stiffer/Usestiffer.cmake)
	configure_file(stifferConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/stifferConfig.cmake @ONLY ESCAPE_QUOTES)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/stifferConfig.cmake Usestiffer.cmake DESTINATION ${LIB_INSTALL_DIR}/cmake/stiffer)
endif(STIFFER_INSTALL)
