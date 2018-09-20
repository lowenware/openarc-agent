#!/usr/bin/env bash

# -----------------------------------------------------------------------------
#
# File:      sited.project.sh
# Version:   2.1
# Copyright: 2017, Löwenware Ltd.
# Author:    Elias Löwe <elias@lowenware.com>
#
# -----------------------------------------------------------------------------

CURRENT_PATH="$(pwd)"
ABSOLUTE_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ "$CURRENT_PATH" != "$ABSOLUTE_PATH" ]; then
  echo " > script has to be run from its parent folder"
  exit 1
fi

PROJECT=$(cat ./CMakeLists.txt | \
            grep 'project('    | \
            cut -d '(' -f2     | \
            cut -d ' ' -f2     | \
            tr -d '[:space:]')

VERSION=$(cat ./version)

# -----------------------------------------------------------------------------

function do_clean {
  echo "--> clean"

  if [ -d build/ ]; then
    rm -R build/*
  else
    mkdir build/
  fi
}

# -----------------------------------------------------------------------------

function do_cmake {
  if hash cmake3 2>/dev/null; then
    cmake_cmd=cmake3
  else
    cmake_cmd=cmake
  fi
  
  echo "--> $cmake_cmd"

  $cmake_cmd -Bbuild/ -H./ $1 -DCMAKE_INSTALL_PREFIX=/usr
}

# -----------------------------------------------------------------------------

function do_build {
  do_clean

  case $1 in
    release)
      debug_flag=0
      ;;
    *)
      debug_flag=1
  esac
  
  do_cmake -DCMAKE_DEBUG=$debug_flag

  do_compile
}

# -----------------------------------------------------------------------------

function do_compile {
  echo "--> compile"
  if [ ! -f build/Makefile ]; then
    do_cmake -DCMAKE_DEBUG=1
  fi

  cd build/
  make # VERBOSE=1
  cd ..
}

# -----------------------------------------------------------------------------

function do_install {
  echo "--> install"

  do_clean
  do_build release
  cd build/
  sudo make install $1
  cd ..
  sudo rm -Rf build/*

}

# -----------------------------------------------------------------------------

function do_install_db {
  DB_NAME=$PROJECT
  DB_USER=$PROJECT
  
  echo "--> install DB ${DB_NAME}"

  DB_PASS=$(tr -dc _A-Z-a-z-0-9 < /dev/urandom | fold -w32 | head -n1)

  echo "  +-> dropdb ${DB_NAME} --if-exists"
  dropdb ${DB_NAME} --if-exists
  echo "  +-> dropuser ${DB_USER} --if-exists"
  dropuser ${DB_USER} --if-exists

  echo "  +-> createuser ${DB_USER}"
  createuser -l ${DB_USER}
  psql -c "ALTER USER ${DB_USER} WITH PASSWORD '${DB_PASS}';"

  echo "Create project database"
  createdb -O ${DB_USER} ${DB_NAME}

  SQLS=$(find ./sql/ -iname  '*.sql' | sort)
  for f in $SQLS; do
    echo "  +-> execute ${f}"
    psql -d ${DB_NAME} < $f
  done

  sed -i "s/  pass  = .*/  pass  = ${DB_PASS}/g" ./${PROJECT}-sited.conf
}

# ---------:--------------------------------------------------------------------

case $1 in
  clean)
    do_clean
    ;;

  compile)
    do_compile
    ;;

  build)
    do_build $2
    ;;

  install-db)
    do_install_db
    ;;

  install)
    do_install
    ;;

  *)
    echo "USAGE:"
    echo "  ./sited.project.sh (COMMAND)"
    echo ""
    echo "COMMANDS:"
    echo "  clean                  - remove temporary and build files"
    echo "  compile                - compile project"
    echo "  build [debug|release]  - make build (default: debug)"
    echo "  install [DESTDIR=]     - install project"
    echo "  install-db             - install project database and sql files"
    echo ""
  ;;
esac

# -----------------------------------------------------------------------------
