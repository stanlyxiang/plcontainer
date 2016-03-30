select rlog100();
select rbool('t');
select rbool('f');
select rint(NULL::int2);
select rint(123::int2);
select rint(234::int4);
select rint(345::int8);
select rfloat(3.1415926535897932384626433832::float4);
select rfloat(3.1415926535897932384626433832::float8);
select rtest_mia();
select rlog100_shared();
select rpg_spi_exec('select 1');
select rtest_spi_tup('select fname, lname,username from users order by 1,2,3');
-- This function is of "return setof record" type which is not supported yet
-- select rtest_spi_ta('select oid, typname from pg_type where typname = ''oid'' or typname = ''text''');