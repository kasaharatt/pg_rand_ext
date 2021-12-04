

MODULE_big = pg_rand_ext
OBJS = pg_rand_ext.o

EXTENSION = pg_rand_ext
DATA = pg_rand_ext--1.0.sql
PGFILEDESC = "pg_rand_ext - generate random number extension"

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/pg_rand_ext
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
