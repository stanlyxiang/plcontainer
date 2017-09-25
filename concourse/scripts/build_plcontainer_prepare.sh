#!/bin/bash

set -exo pipefail

GPDBBIN=$1
OUTPUT=$2
MODE=$3

CWDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
TOP_DIR=${CWDIR}/../../../
source "${TOP_DIR}/gpdb_src/concourse/scripts/common.bash"

function _main() {
  install_gpdb
  ${TOP_DIR}/gpdb_src/concourse/scripts/setup_gpadmin_user.bash "centos"
  make_cluster
 
  ln -s /usr/local/greenplum-db-devel /usr/local/greenplum-db
  chown -h gpadmin:gpadmin /usr/local/greenplum-db
  
  chown -R gpadmin:gpadmin ${TOP_DIR}
  chown gpadmin:gpadmin ${CWDIR}/build_plcontainer.sh
  su gpadmin -c "OUTPUT=${OUTPUT} \
                 MODE=${MODE} \
                 bash ${CWDIR}/build_plcontainer.sh $(pwd)"
}

_main "$@"
