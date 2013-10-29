MODULES = vihash
DATA_built = vihash.sql
DATA = uninstall_vihash.sql
REGRESS = vihash

PG_CONFIG = pg_config

pg_version := $(word 2,$(shell $(PG_CONFIG) --version))
extensions_supported = $(filter-out 6.% 7.% 8.% 9.0%,$(pg_version))

extension_version = 1

DATA = $(if $(extensions_supported),vihash--unpackaged--1.sql,vihash.sql uninstall_vihash.sql)
DATA_built = $(if $(extensions_supported),vihash--$(extension_version).sql)
EXTENSION = vihash

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

vihash--$(extension_version).sql: vihash.sql
	cp $< $@
