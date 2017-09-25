#setup R_HOME
R_HOME=/usr/lib64/R/lib64/R
PATH=$R_HOME/bin:$PATH
LD_LIBRARY_PATH=$R_HOME/lib:$R_HOME/extlib:/opt/ds/extlib:$LD_LIBRARY_PATH
export PATH
export LD_LIBRARY_PATH
export R_HOME

