create or replace function bigarray() returns integer as $$
# container: plc_python
import math
import os
a=[]
for i in range(1,10):
    a.append(bytearray(os.urandom(20000000)))
return 0
$$ language plcontainer;

