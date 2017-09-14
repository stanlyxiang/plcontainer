#!/bin/bash
# ------------------------------------------------------------------------------
#
# Copyright (c) 2016-Present Pivotal Software, Inc
#
# ------------------------------------------------------------------------------
if [ -d /usr/local/greenplum-db ] ; then
    source /usr/local/greenplum-db/greenplum_path.sh
fi

cd /clientdir

./pyclient
