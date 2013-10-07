AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = -I.. -I. -I${top_srcdir} -I${top_srcdir}/include 

bin_PROGRAMS = basic_checks

basic_checks_SOURCES = basic_checks.cc ${top_srcdir}/src/flight.cc

TESTS = ${bin_PROGRAMS}

EXTRA_DIST=sample_flight.asc

