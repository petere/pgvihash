CREATE OR REPLACE FUNCTION vihashtext(text) RETURNS int
AS '$libdir/vihash'
STRICT IMMUTABLE
LANGUAGE C;