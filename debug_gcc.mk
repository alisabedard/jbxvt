include debug.mk
CFLAGS+=-ggdb
CFLAGS+=-pg
CFLAGS+=-fsanitize=shift
CFLAGS+=-fsanitize=integer-divide-by-zero
CFLAGS+=-fsanitize=unreachable
CFLAGS+=-fsanitize=null
CFLAGS+=-fsanitize=alignment
CFLAGS+=-fsanitize=object-size
CFLAGS+=-fsanitize=nonnull-attribute
CFLAGS+=-fsanitize=bool
CFLAGS+=-fsanitize=enum
CFLAGS+=-fsanitize=undefined
CFLAGS+=-ftrapv
CFLAGS+=-Wimplicit-fallthrough=0
CFLAGS+=-Wsuggest-attribute=pure
CFLAGS+=-Wsuggest-attribute=const
CFLAGS+=-Wsuggest-attribute=noreturn
CFLAGS+=-Wsuggest-attribute=format
CFLAGS+=-Wsuggest-final-types
CFLAGS+=-Wmissing-format-attribute
CFLAGS+=-Wrestrict
CFLAGS+=-Wredundant-decls
CFLAGS+=-Wdisabled-optimization
CFLAGS+=-Woverlength-strings
CFLAGS+=-fstack-protector-all
CFLAGS+=-Wshadow
CFLAGS+=-Wsign-conversion
CFLAGS+=-Wpacked
CFLAGS+=-Wpointer-sign
