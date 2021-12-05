# pg_rand_ext
This module makes the function for generating random values in pgbench available as a user-defined function.

It implements random number generation functions according to each of the exponential, gaussian, and zipfian distributions available in pgbench.

## How to build
The common way to build extension modules in PostgreSQL.
It has been tested with PostgreSQL v14 and above.
```
<Go to pg_rand_ext dir>
$ make USE_PGXS=1 install
```

## How to use
When you execute CREATE EXTENSION, the rand_ext schema is created and the functions are registered in it.
```
psql -c "CREATE EXTENSION pg_rand_ext"
$ psql -c "SELECT rand_ext.random_exponential(1,1000,0.3)"
 random_exponential 
--------------------
                 54
(1 row)
```

## Functions

| Function | Args | Return | Desc |
| ------------- | ------------- | ------------- | ------------- |
| random_exponential | lb bigint, ub bigint, param dp  | bigint | Return a random value between lb and ub with param of the exponential distribution |
| random_gaussian | lb bigint, ub bigint, param dp  | bigint | Return a random value between lb and ub with param of the gaussian distribution |
| random_zipfian | lb bigint, ub bigint, param dp  | bigint | Return a random value between lb and ub with param of the zipfian distribution |

It works the same as the built-in function of pgbench.
Please see more detail: https://www.postgresql.org/docs/current/pgbench.html#PGBENCH-FUNCTIONS

