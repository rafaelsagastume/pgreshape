cmake_minimum_required(VERSION 2.8.11)

project(pgreshape C)
add_executable(pgreshape
	src/pgreshape.c
	src/pgreshape.h
	src/config.c
	src/config.h
	src/table.c
	src/table.h
	src/common.c
	src/common.h
	src/view.c
	src/view.h
	src/update.c
	src/update.h
	src/trigger.c
	src/trigger.h
	src/acl.c
	src/acl.h
	src/schema.c
	src/schema.h
	src/sequence.c
	src/sequence.h
)

# ignore Windows warnings
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	add_definitions(/wd4005 /D_CRT_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_DEPRECATE /D_CRT_NONSTDC_NO_DEPRECATE)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID MATCHES "Clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -Wall -Wmissing-prototypes -Wpointer-arith -Wdeclaration-after-statement -Wendif-labels -fno-strict-aliasing -fwrapv -g")
endif()

# cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_PREFIX_PATH=/home/euler/pg101 .
find_package(PostgreSQL QUIET)

# check pg_config
find_program(PGCONFIG_PATH NAMES pg_config)
if(PGCONFIG_PATH)
	message("pg_config: ${PGCONFIG_PATH}")
else()
	message(FATAL_ERROR "could not find pg_config")
endif()

if(PostgreSQL_FOUND)
	include_directories(${PostgreSQL_INCLUDE_DIRS})
	# some postgres headers (those included by optionals) aren't on the include
	# path. Hence, we need to include them.
	if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
		include_directories(/usr/local/include)
	endif()
	# some Windows include paths
	if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
		include_directories(${PostgreSQL_INCLUDE_DIR}/server/port/win32 ${PostgreSQL_INCLUDE_DIR}/server/port/win32_msvc)
	endif()
	# include static library (for prompt password)
	# unfortunately CMake does not include it
	execute_process(COMMAND ${PGCONFIG_PATH} --libdir RESULT_VARIABLE result OUTPUT_VARIABLE pgpath OUTPUT_STRIP_TRAILING_WHITESPACE)
	if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
		set(LIBS ${pgpath}/libpgport.lib)
		# available in 9.3 or later
		if(EXISTS "${pgpath}/libpgcommon.lib")
			set(LIBS ${LIBS} "${pgpath}/libpgcommon.lib")
		endif()
	else()
		set(LIBS ${pgpath}/libpgport.a)
		# available in 9.3 or later
		if(EXISTS "${pgpath}/libpgcommon.a")
			set(LIBS ${LIBS} "${pgpath}/libpgcommon.a")
		endif()
	endif()
else()
	# setup library directory
	execute_process(COMMAND ${PGCONFIG_PATH} --libdir RESULT_VARIABLE result OUTPUT_VARIABLE pgpath OUTPUT_STRIP_TRAILING_WHITESPACE)
	set(PostgreSQL_LIBRARY_DIRS "${pgpath}")

	# figure out library directory and try to find libpgport
	if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
#		execute_process(COMMAND ${PGCONFIG_PATH} --libdir RESULT_VARIABLE result OUTPUT_VARIABLE pgpath OUTPUT_STRIP_TRAILING_WHITESPACE)
		if(EXISTS "${pgpath}/libpgport.lib")
			set(LIBS "${pgpath}/libpgport.lib")
		else()
			message(FATAL_ERROR "could not find libpgport.lib")
		endif()
		if(EXISTS "${pgpath}/libpgcommon.lib")
			set(LIBS ${LIBS} "${pgpath}/libpgcommon.lib")
		endif()
	else()
#		execute_process(COMMAND ${PGCONFIG_PATH} --libdir RESULT_VARIABLE result OUTPUT_VARIABLE pgpath OUTPUT_STRIP_TRAILING_WHITESPACE)
		if(EXISTS "${pgpath}/libpgport.a")
			set(LIBS "${pgpath}/libpgport.a")
		else()
			# Debian-like systems move static libraries to PKGLIBDIR. Let's probe it too.
			execute_process(COMMAND ${PGCONFIG_PATH} --pkglibdir OUTPUT_VARIABLE pgpath OUTPUT_STRIP_TRAILING_WHITESPACE)
			if(EXISTS "${pgpath}/libpgport.a")
				set(LIBS "${pgpath}/libpgport.a")
			else()
				message(FATAL_ERROR "could not find libpgport.a")
			endif()
		endif()
		if(EXISTS "${pgpath}/libpgcommon.a")
			set(LIBS ${LIBS} "${pgpath}/libpgcommon.a")
		else()
			# Debian-like systems move static libraries to PKGLIBDIR. Let's probe it too.
			execute_process(COMMAND ${PGCONFIG_PATH} --pkglibdir OUTPUT_VARIABLE pgpath OUTPUT_STRIP_TRAILING_WHITESPACE)
			if(EXISTS "${pgpath}/libpgcommon.a")
				set(LIBS ${LIBS} "${pgpath}/libpgcommon.a")
			endif()
		endif()
	endif()

	# setup include directories
	execute_process(COMMAND ${PGCONFIG_PATH} --includedir OUTPUT_VARIABLE pgpath OUTPUT_STRIP_TRAILING_WHITESPACE)
	set(PostgreSQL_INCLUDE_DIRS ${pgpath})
	execute_process(COMMAND ${PGCONFIG_PATH} --includedir-server OUTPUT_VARIABLE pgpath OUTPUT_STRIP_TRAILING_WHITESPACE)
	set(PostgreSQL_INCLUDE_DIRS ${PostgreSQL_INCLUDE_DIRS} ${pgpath})

	# setup libpq for linking into pgreshape
	set(PostgreSQL_LIBRARIES "pq")
	include_directories(${PostgreSQL_INCLUDE_DIRS})
endif(PostgreSQL_FOUND)

message("PostgreSQL FOUND: ${PostgreSQL_FOUND}")
message("LIBS: ${LIBS}")
message("PostgreSQL LIBRARIES: ${PostgreSQL_LIBRARIES}")
message("PostgreSQL LIBRARY DIRS: ${PostgreSQL_LIBRARY_DIRS}")
message("PostgreSQL INCLUDE DIRS: ${PostgreSQL_INCLUDE_DIRS}")

set(LIBS ${LIBS} ${PostgreSQL_LIBRARIES})

# postgres libraries are not installed in one of the standard system directories
# or
# postgres library directory is not in the dynamic linker
#set_target_properties(pgreshape PROPERTIES LINK_FLAGS "-L${PostgreSQL_LIBRARY_DIRS}")

target_link_libraries(pgreshape ${LIBS})

install(TARGETS pgreshape RUNTIME DESTINATION bin)
# copy DLLs into bin directory for Windows
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	install(FILES ${PostgreSQL_LIBRARY_DIRS}/libpq.dll DESTINATION bin)
endif()

find_program(astyle_EXECUTABLE astyle DOC "source code indenter, formatter, and beautifier")

add_custom_target("style" COMMAND
	"${astyle_EXECUTABLE}"
	--style=bsd
#	--attach-closing-while
	--indent=force-tab=4
	--indent-switches
	--pad-oper
	--align-pointer=name
	--align-reference=name
	--remove-brackets
#	--break-return-type
#	--break-return-type-decl
	--max-code-length=80
	--break-after-logical
	--suffix=none
	--lineend=linux
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
	VERBATIM
)