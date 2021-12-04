\echo Use "CREATE EXTENSION pg_rand_ext" to load this file. \quit

CREATE SCHEMA rand_ext;
GRANT USAGE on SCHEMA rand_ext TO PUBLIC;

CREATE FUNCTION rand_ext.random_exponential(bigint, bigint, double precision)
RETURNS bigint
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION rand_ext.random_gaussian(bigint, bigint, double precision)
RETURNS bigint
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION rand_ext.random_zipfian(bigint, bigint, double precision)
RETURNS bigint
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT VOLATILE;
