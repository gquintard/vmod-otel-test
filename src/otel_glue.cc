#include "opentelemetry/exporters/ostream/span_exporter_factory.h"
#include "opentelemetry/sdk/trace/exporter.h"
#include "opentelemetry/sdk/trace/processor.h"
#include "opentelemetry/sdk/trace/simple_processor_factory.h"
#include "opentelemetry/sdk/trace/tracer_provider_factory.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/exporters/otlp/otlp_http_exporter_factory.h"
#include "opentelemetry/exporters/otlp/otlp_http_exporter_options.h"
#include "opentelemetry/trace/context.h"
#include "opentelemetry/trace/propagation/http_trace_context.h"


#include <cstdlib>
#include <ctime>
#include <string>

using namespace std;
namespace trace_api = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace trace_exporter = opentelemetry::exporter::trace;
namespace otlp      = opentelemetry::exporter::otlp;
namespace sdktrace   = opentelemetry::sdk::trace;

extern "C"
{
struct container {
	opentelemetry::v1::nostd::shared_ptr<opentelemetry::v1::trace::Span> span;
};

otlp::OtlpHttpExporterOptions opts;
void InitTracer() {
	// Create OTLP exporter instance
	opts.url = "http://localhost:4318/v1/traces";
	auto exporter  = otlp::OtlpHttpExporterFactory::Create(opts);
	auto processor = trace_sdk::SimpleSpanProcessorFactory::Create(std::move(exporter));
	std::shared_ptr<opentelemetry::trace::TracerProvider> provider =
		trace_sdk::TracerProviderFactory::Create(std::move(processor));
	// Set the global trace provider
	trace_api::Provider::SetTracerProvider(provider);

}
void CleanupTracer() {
	std::shared_ptr<opentelemetry::trace::TracerProvider> none;
	trace_api::Provider::SetTracerProvider(none);
}
class TextMapCarrierTest : public opentelemetry::context::propagation::TextMapCarrier
{
	public:
		virtual opentelemetry::v1::nostd::string_view Get(opentelemetry::v1::nostd::string_view key) const noexcept override
		{
			auto it = headers_.find(std::string(key));
			if (it != headers_.end())
			{
				return opentelemetry::v1::nostd::string_view(it->second);
			}
			return "";
		}
		virtual void Set(opentelemetry::v1::nostd::string_view key, opentelemetry::v1::nostd::string_view value) noexcept override
		{
			headers_[std::string(key)] = std::string(value);
		}

		std::map<std::string, std::string> headers_;
};

char * StartSpan(container **cp, int is_client_side, const char *traceparent) {
	auto tracer = opentelemetry::trace::Provider::GetTracerProvider()->GetTracer("vmod-otel");
	auto cont = new container;
	opentelemetry::trace::StartSpanOptions opts;
	// it's no a mistake, when we are on the VCL's client side, we act as a backend
	if (is_client_side) {
		opts.kind = opentelemetry::trace::SpanKind::kServer;
	} else {
		opts.kind = opentelemetry::trace::SpanKind::kClient;
	}
	TextMapCarrierTest carrier;
	carrier.headers_ = {{"traceparent", traceparent ? traceparent : ""}, {"tracestate", "congo=t61rcWkgMzE" }};

	using MapHttpTraceContext = trace_api::propagation::HttpTraceContext;
	MapHttpTraceContext format = MapHttpTraceContext();
	// yeah, I know...
	try {

		opentelemetry::v1::context::Context ctx1 = opentelemetry::v1::context::Context{};
		opentelemetry::v1::context::Context ctx2 = format.Extract(carrier, ctx1);
		auto ctx2_span = ctx2.GetValue(trace_api::kSpanKey);
		auto span = opentelemetry::v1::nostd::get<opentelemetry::v1::nostd::shared_ptr<trace_api::Span>>(ctx2_span);
		opts.parent = span->GetContext();
	} catch (...) {}
	cont->span = tracer->StartSpan("Varnish", opts);

	opentelemetry::v1::context::Context ctx{
		trace_api::kSpanKey,
		cont->span};

	format.Inject(carrier, ctx);

	*cp = cont;
	char *s = carrier.headers_["traceparent"].data();
	return (strdup(s));
}

void SetAttribute(container *cp, const char *key, const char *value) {
	cp->span->SetAttribute(key, value);
}

void UpdateName(container *cp, const char *name) {
	cp->span->UpdateName(name);
}

void EndSpan(container *cp) {
	cp->span->End();
}
}
