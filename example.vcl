vcl 4.1;

import vtc;
import otel from "/home/guillaume/work/libvmod-otel/src/.libs/libvmod_otel.so";

backend be none;

sub vcl_init {
	new tracer = otel.tracer();
}

sub vcl_recv {
	if (req.restarts) {
		set req.http.traceparent = tracer.trace(req.http.traceparent);
		tracer.update_name("/users/{id}");
		tracer.set_attribute("foo", "bar");
		tracer.set_attribute("service.name", "varnish:client_side");
	}
}

sub vcl_backend_fetch {
	if (bereq.retries == 0) {
		set bereq.http.traceparent-top = bereq.http.traceparent;
	}
	set bereq.http.traceparent-top = tracer.trace(bereq.http.traceparent-top);
	tracer.set_attribute("service.name", "varnish:backend_side");
	vtc.sleep(0.4s);
}

sub vcl_deliver {
        set resp.http.traceparent = req.http.traceparent;
	vtc.sleep(0.3s);
	if (req.restarts < 4) {
		return (restart);
	}
}

sub vcl_synth {
        set resp.http.traceparent = req.http.traceparent;
}
