##Process this file with automake to create Makefile.in

bindir = ${libdir}/nagios/plugins

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/lib $(LIBCURL_CPPFLAGS)

bin_PROGRAMS = 

if HAVE_CURL_TFTP
bin_PROGRAMS += check_tftp
endif

check_tftp_SOURCE = check_tftp.c

LDADD = $(LIBCURL) ../lib/libmonitoringplug.a