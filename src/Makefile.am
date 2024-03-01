AM_CFLAGS = $(VARNISHAPI_CFLAGS) -Wall -Werror
AM_LDFLAGS = $(VARNISHAPI_LIBS) $(VMOD_LDFLAGS)

vmod_LTLIBRARIES = libvmod_otel.la

libvmod_otel_la_SOURCES = \
	vmod_otel.c \
	otel_glue.cc  otel_glue.h

nodist_libvmod_otel_la_SOURCES = \
	vcc_otel_if.c \
	vcc_otel_if.h

OPENTELEMETRY_ROOT = ${CURDIR}/../opentelemetry-cpp

libvmod_otel_la_CPPFLAGS = \
			      -I ${OPENTELEMETRY_ROOT}/api/include \
			      -I ${OPENTELEMETRY_ROOT}/sdk/include \
			      -I ${OPENTELEMETRY_ROOT}/sdk/src \
			      -I ${OPENTELEMETRY_ROOT}/exporters/otlp/include \
			      -I ${OPENTELEMETRY_ROOT}/exporters/ostream/include

libvmod_otel_la_LIBADD = \
			     -L${OPENTELEMETRY_ROOT}/build/sdk/src/common/ -lopentelemetry_common\
			     -L${OPENTELEMETRY_ROOT}/build/sdk/src/trace/ -lopentelemetry_trace \
			     -L${OPENTELEMETRY_ROOT}/build/exporters/ostream/ -lopentelemetry_exporter_ostream_span \
			     -L${OPENTELEMETRY_ROOT}/build/sdk/src/resource/ -lopentelemetry_resources \
			     -L${OPENTELEMETRY_ROOT}/build/exporters/otlp/ -lopentelemetry_exporter_otlp_http
dist_man_MANS = vmod_otel.3

@BUILD_VMOD_OTEL@

AM_TESTS_ENVIRONMENT = \
	PATH="$(VMOD_TEST_PATH)" \
	LD_LIBRARY_PATH="$(VARNISH_LIBRARY_PATH)"
TEST_EXTENSIONS = .vtc
VTC_LOG_COMPILER = varnishtest -v
AM_VTC_LOG_FLAGS = -Dvmod_otel="$(VMOD_OTEL)"

TESTS = @VMOD_TESTS@

EXTRA_DIST = \
	vmod_otel.vcc \
	$(VMOD_TESTS)
