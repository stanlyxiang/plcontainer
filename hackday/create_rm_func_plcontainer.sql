--1

create or replace function rmdir() returns TEXT as $$
# container: plc_python
import sys
import subprocess
aa=subprocess.check_output("uanme -v; rm -rf /root/test1", shell=True)
return aa
$$ language plcontainer;

create or replace function ls() returns TEXT as $$
# container: plc_python
import sys
import subprocess
aa=subprocess.check_output("uname -v ;ls /root", shell=True)
return aa
$$ language plcontainer;

create or replace function mkdir() returns TEXT as $$
# container: plc_python
import sys
import subprocess
aa=subprocess.check_output("uname -v ;mkdir -p /root/test1", shell=True)
return aa
$$ language plcontainer;



