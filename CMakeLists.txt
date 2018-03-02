cmake_minimum_required (VERSION 2.8)

# general ---------------------------------------------------------------------

project( openarc C)
set(PROJECT_DESCRIPTION "Alarm Receiving Center")
set(APPLICATION "${PROJECT_NAME}-agent")

# settings --------------------------------------------------------------------


set(BUILD_DIRECTORY                "build")

set(VERSION_FILE                   "version")

set(CONFIG_FILE                    "${PROJECT_NAME}.conf")
set(SERVICE_FILE                   "${APPLICATION}.service")
set(LOG_FILE                       "${APPLICATION}.log")
set(PID_FILE                       "${APPLICATION}.pid")

set(COMPILER_CFLAGS                "-std=c99 -pedantic")
set(DEBUG_CFLAGS                   "-Wall -Werror")
set(RELEASE_CFLAGS                 "-g0 -O2")

# sources ---------------------------------------------------------------------
set ( SOURCE_FILES
  src/main.c

  src/daemon.c
  src/configuration.c
  src/loop.c
  src/module.c
  src/channel.c
  src/sql-queries.c
  src/commands-pool.c
  src/common-utils.c
#  src/store-heartbeat.c
#  src/store-contact-id.c
#  src/store-location.c
#  src/store-client.c
#  src/store-media.c
  src/store.c


  cStuff/dbx.c
  cStuff/log.c
  cStuff/config.c
  cStuff/datetime.c
  cStuff/fs-utils.c
  cStuff/sock-utils.c
  cStuff/str-utils.c
  cStuff/list.c
  cStuff/uri.c
  cStuff/query-stream.c
  cStuff/base64.c
)

# definitions -----------------------------------------------------------------

add_definitions(

  -D_POSIX_C_SOURCE=200809L
  -D_XOPEN_SOURCE
  -DPROJECT_NAME="${PROJECT_NAME}"
  -DAPPLICATION="${APPLICATION}"

  -DCSTUFF_STR_UTILS_WITH_NCOPY
  -DCSTUFF_STR_UTILS_WITH_COPY
  -DCSTUFF_STR_UTILS_WITH_CAT 
  -DCSTUFF_STR_UTILS_WITH_NCAT
  -DCSTUFF_STR_UTILS_WITH_CMPI
  -DCSTUFF_STR_UTILS_WITH_PRINTF

  -DCSTUFF_LIST_WITH_APPEND
  -DCSTUFF_LIST_WITH_INSERT
  -DCSTUFF_LIST_WITH_REMOVE_INDEX
  -DCSTUFF_LIST_WITH_REMOVE

  -DCSTUFF_LOG_WITH_STDLOG
  -DCSTUFF_FS_WITH_MAKE_FILE_PATH
  -DCSTUFF_URI_WITH_PARSE
  -DCSTUFF_QUERY_STREAM_WITH_CONSTRUCTOR

)

# linking ---------------------------------------------------------------------

find_package(PostgreSQL)
find_package(OpenSSL)

include_directories(
  "./"
  "./include/"
  ${PostgreSQL_INCLUDE_DIRS}
  ${OPENSSL_INCLUDE_DIRS}
)

link_directories(
  ${PostgreSQL_LIBRARY_DIRS}
  ${OPENSSL_LIBRARY_DIRS}
)

# executable ------------------------------------------------------------------

add_executable( ${APPLICATION} ${SOURCE_FILES} )

target_link_libraries (
  ${APPLICATION}
  dl
  ${PostgreSQL_LIBRARIES}
  ${OPENSSL_LIBRARIES}
)


# versioning ------------------------------------------------------------------

set (
  VERSION_REGEX
  "^([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)(-([a-zA-Z0-9\ ]+))"
)

# Read file
file (READ ${VERSION_FILE} VERSION_STRING)

# Match file content
string(REGEX MATCH ${VERSION_REGEX} VERSION_STRING ${VERSION_STRING} )

# Set Version constants
set (VERSION_MAJOR ${CMAKE_MATCH_1})
set (VERSION_MINOR ${CMAKE_MATCH_2})
set (VERSION_TWEAK ${CMAKE_MATCH_3})
set (VERSION_BUILD ${CMAKE_MATCH_4})
set (VERSION_LABEL "${CMAKE_MATCH_6}")

MATH(EXPR VERSION_NEXT "${VERSION_BUILD}+1")
set (VERSION_NEXT "${VERSION_TWEAK}.${VERSION_NEXT}")
set (VERSION_NEXT "${VERSION_MINOR}.${VERSION_NEXT}")
set (VERSION_NEXT "${VERSION_MAJOR}.${VERSION_NEXT}")
set (VERSION_NEXT "${VERSION_NEXT}-${VERSION_LABEL}")

add_definitions(
  -DVERSION_MAJOR=${VERSION_MAJOR}
  -DVERSION_MINOR=${VERSION_MINOR}
  -DVERSION_TWEAK=${VERSION_TWEAK}
  -DVERSION_BUILD=${VERSION_BUILD}
  -DVERSION_LABEL="${VERSION_LABEL}"
)

file(WRITE ${VERSION_FILE} "${VERSION_NEXT}")

# building --------------------------------------------------------------------

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMPILER_CFLAGS}")

if(NOT DEFINED CMAKE_DEBUG)
  set(CMAKE_DEBUG 0)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(SYSTEM_BITNESS    64)
  set(SYSTEM_LIB_SUFFIX "64")
else()
  set(SYSTEM_BITNESS    32)
  set(SYSTEM_LIB_SUFFIX "")
endif()

if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr")
endif()

if(NOT DEFINED SYSCONF_INSTALL_DIR)
  set(SYSCONF_INSTALL_DIR "/etc")
endif()

if(NOT DEFINED SYSVAR_INSTALL_DIR)
  set(SYSVAR_INSTALL_DIR "/var")
endif()

if(NOT DEFINED INCLUDE_INSTALL_DIR)
  set(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include")
endif()

if(NOT DEFINED SHARE_INSTALL_PREFIX)
  set(SHARE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/share")
endif()

if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
  set(CMAKE_INSTALL_LIBDIR "lib${SYSTEM_LIB_SUFFIX}")
endif()

if(NOT DEFINED LIB_INSTALL_DIR)
  set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
endif()

set(CONFIG_DESTINATION   "${SYSCONF_INSTALL_DIR}/${PROJECT_NAME}" )
set(SYSTEMD_DESTINATION  "${CMAKE_INSTALL_PREFIX}/lib/systemd/system" )
set(BINARIES_DESTINATION "${CMAKE_INSTALL_PREFIX}/sbin" )
set(INCLUDES_DESTINATION "${CMAKE_INSTALL_PREFIX}/include" )
set(SHARES_DESTINATION   "${SHARE_INSTALL_PREFIX}/${PROJECT_NAME}" )
set(LOCALE_DESTINATION   "${SHARE_INSTALL_PREFIX}/locale" )
set(MODULES_DESTINATION  "${LIB_INSTALL_DIR}/${PROJECT_NAME}/modules" )

if( CMAKE_DEBUG )

  add_definitions( -DDEBUG )

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DEBUG_CFLAGS}")

  set(PID_PATH    "${CMAKE_CURRENT_SOURCE_DIR}/${BUILD_DIRECTORY}/${PID_FILE}")
  set(LOG_PATH    "${CMAKE_CURRENT_SOURCE_DIR}/${BUILD_DIRECTORY}/${LOG_FILE}")
  set(CONFIG_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_FILE}")
  set(BINARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${BUILD_DIRECTORY}/${APPLICATION}")
  set(LOCALE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/locale")
  set(MODULES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${BUILD_DERECTORY}")

  set(APPLICATION_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/")

else()

  add_definitions( -DRELEASE )

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${RELEASE_CFLAGS}")

  set(PID_PATH     "${SYSVAR_INSTALL_DIR}/run/${PROJECT_NAME}/${PID_FILE}")
  set(LOG_PATH     "${SYSVAR_INSTALL_DIR}/log/${PROJECT_NAME}/${LOG_FILE}")
  set(CONFIG_PATH  "${CONFIG_DESTINATION}/${CONFIG_FILE}")
  set(BINARY_PATH  "${BINARIES_DESTINATION}/${APPLICATION}")
  set(LOCALE_PATH  "${LOCALE_DESTINATION}/locale")
  set(MODULES_PATH "${MODULES_DESTINATION}" )

  set(APPLICATION_ROOT "${CMAKE_INSTALL_PREFIX}/")
endif()

add_definitions(
  -DLOG_PATH="${LOG_PATH}"
  -DPID_PATH="${PID_PATH}"
  -DCONFIG_PATH="${CONFIG_PATH}"
  -DBINARY_PATH="${BINARY_PATH}"
  -DLOCALE_PATH="${LOCALE_PATH}"
  -DMODULES_PATH="${MODULES_PATH}"
  -DAPPLICATION_ROOT="${CMAKE_INSTALL_PREFIX}"
)

# service file ----------------------------------------------------------------

set( SERVICE_FILE_TEXT "${SERVICE_FILE_TEXT}[Unit]\n" )
set( SERVICE_FILE_TEXT "${SERVICE_FILE_TEXT}Description=${PROJECT_DESCRIPTION}\n")
set( SERVICE_FILE_TEXT "${SERVICE_FILE_TEXT}After=network.target remote-fs.target nss-lookup.target\n")
set( SERVICE_FILE_TEXT "${SERVICE_FILE_TEXT}\n")
set( SERVICE_FILE_TEXT "${SERVICE_FILE_TEXT}[Service]\n")
set( SERVICE_FILE_TEXT "${SERVICE_FILE_TEXT}Type=forking\n")
set( SERVICE_FILE_TEXT "${SERVICE_FILE_TEXT}PIDFile=${PID_PATH}\n")
set( SERVICE_FILE_TEXT "${SERVICE_FILE_TEXT}ExecStart=${BINARY_PATH} --daemon-mode start\n")
set( SERVICE_FILE_TEXT "${SERVICE_FILE_TEXT}ExecReload=/bin/kill -s HUP $MAINPID\n")
set( SERVICE_FILE_TEXT "${SERVICE_FILE_TEXT}KillSignal=SIGQUIT\n")
set( SERVICE_FILE_TEXT "${SERVICE_FILE_TEXT}TimeoutStopSec=5\n")
set( SERVICE_FILE_TEXT "${SERVICE_FILE_TEXT}KillMode=process\n")
set( SERVICE_FILE_TEXT "${SERVICE_FILE_TEXT}PrivateTmp=true\n")
set( SERVICE_FILE_TEXT "${SERVICE_FILE_TEXT}Restart=Always\n")
set( SERVICE_FILE_TEXT "${SERVICE_FILE_TEXT}\n")
set( SERVICE_FILE_TEXT "${SERVICE_FILE_TEXT}[Install]\n")
set( SERVICE_FILE_TEXT "${SERVICE_FILE_TEXT}WantedBy=multi-user.target\n")

file( WRITE "${APPLICATION}.service" "${SERVICE_FILE_TEXT}")

# installation ----------------------------------------------------------------

install(
  FILES ${CONFIG_FILE}
  DESTINATION ${CONFIG_DESTINATION}
  RENAME ${CONFIG_FILE}.dist
)

install(
  FILES ${SERVICE_FILE}
  DESTINATION ${SYSTEMD_DESTINATION}
)

install(
  TARGETS ${APPLICATION}
  DESTINATION ${BINARIES_DESTINATION}
)

install(
  DIRECTORY include/${PROJECT_NAME}
  DESTINATION ${INCLUDES_DESTINATION}
)

# -----------------------------------------------------------------------------
