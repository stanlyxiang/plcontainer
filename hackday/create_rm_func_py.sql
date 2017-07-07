
create language plpythonu;
create or replace function rmdir() returns TEXT as $$
import sys
import subprocess
aa=subprocess.check_output("uanme -v; rm -rf /hack/test1", shell=True)
return aa
$$ language plpythonu;

create or replace function ls() returns TEXT as $$
import sys
import subprocess
aa=subprocess.check_output("uname -v ;ls /hack", shell=True)
return aa
$$ language plpythonu;

create or replace function mkdir() returns TEXT as $$
import sys
import subprocess
aa=subprocess.check_output("uname -v ;mkdir -p /hack/test1", shell=True)
return aa
$$ language plpythonu;


