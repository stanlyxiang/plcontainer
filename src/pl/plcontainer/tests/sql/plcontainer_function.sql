/* really stupid function just to get the module loaded
*/
CREATE OR REPLACE FUNCTION rlog100() RETURNS text AS $$
# container: plc_r
return(log10(100))
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION writeFile() RETURNS text AS $$
# container: plc_python
f = open("/tmp/foo", "w")
f.write("foobar")
f.close
return 'wrote foobar to /tmp/foo'
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pylog100() RETURNS double precision AS $$
# container: plc_python
import math
return math.log10(100)
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION pylog(a integer, b integer) RETURNS double precision AS $$
# container: plc_python
import math
return math.log(a, b)
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION concat(a text, b text) RETURNS text AS $$
# container: plc_python
import math
return a + b
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION concatall() RETURNS text AS $$
# container: plc_python
res = plpy.execute('select fname from users order by 1')
names = map(lambda x: x['fname'], res)
return reduce(lambda x,y: x + ',' + y, names)
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION FUNCTION invalid_function() RETURNS double precision AS $$
# container: plc_python
import math
return math.foobar(a, b)
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION invalid_syntax() RETURNS double precision AS $$
# container: plc_python
import math
return math.foobar(a,
$$ LANGUAGE plcontainer;

CREATE OR REPLACE FUNCTION nested_call_one(a text) RETURNS text AS $$
# container: plc_python
q = "SELECT nested_call_two('%s')" % a
r = plpy.execute(q)
return r[0]
$$ LANGUAGE plcontainer ;

CREATE OR REPLACE FUNCTION nested_call_two(a text) RETURNS text AS $$
# container: plc_python
q = "SELECT nested_call_three('%s')" % a
r = plpy.execute(q)
return r[0]
$$ LANGUAGE plcontainer ;

CREATE OR REPLACE FUNCTION nested_call_three(a text) RETURNS text AS $$
# container: plc_python
return a
$$ LANGUAGE plcontainer ;

