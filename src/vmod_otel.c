#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "cache/cache.h"

#include "vtim.h"
#include "vcc_otel_if.h"
#include "otel_glue.h"

const size_t infosz = 64;
char	     *info;

/*
 * handle vmod internal state, vmod init/fini and/or varnish callback
 * (un)registration here.
 *
 * malloc'ing the info buffer is only indended as a demonstration, for any
 * real-world vmod, a fixed-sized buffer should be a global variable
 */

int v_matchproto_(vmod_event_f)
vmod_event_function(VRT_CTX, struct vmod_priv *priv, enum vcl_event_e e)
{
	char	   ts[VTIM_FORMAT_SIZE];
	const char *event = NULL;

	(void) ctx;
	(void) priv;
	InitTracer();

	switch (e) {
	case VCL_EVENT_LOAD:
		info = malloc(infosz);
		if (! info)
			return (-1);
		event = "loaded";
		break;
	case VCL_EVENT_WARM:
		event = "warmed";
		break;
	case VCL_EVENT_COLD:
		event = "cooled";
		break;
	case VCL_EVENT_DISCARD:
		free(info);
		return (0);
		break;
	default:
		return (0);
	}
	AN(event);
	VTIM_format(VTIM_real(), ts);
	snprintf(info, infosz, "vmod_otel %s at %s", event, ts);

	return (0);
}

VCL_STRING
vmod_info(VRT_CTX)
{
	(void) ctx;

	return (info);
}

VCL_STRING
vmod_hello(VRT_CTX, VCL_STRING name)
{
	char *p;
	unsigned u, v;

	u = WS_ReserveAll(ctx->ws); /* Reserve some work space */
	p = ctx->ws->f;		/* Front of workspace area */
	v = snprintf(p, u, "Hello, %s", name);
	v++;
	if (v > u) {
		/* No space, reset and leave */
		WS_Release(ctx->ws, 0);
		return (NULL);
	}
	/* Update work space with what we've used */
	WS_Release(ctx->ws, v);
	return (p);
}

VCL_INT vmod_add_two(VRT_CTX, VCL_INT n) {
	return (n + 2);
}

struct vmod_otel_tracer {
	unsigned                magic;
#define OTLPTRACER_MAGIC	0x3256f9f4
};

VCL_VOID v_matchproto_(td_otel_tracer__fini)
vmod_tracer__fini(struct vmod_otel_tracer **tp)
{
        ASSERT_CLI();
	*tp = NULL;
}

VCL_VOID v_matchproto_(td_rewrite_ruleset__init)
vmod_tracer__init(VRT_CTX, struct vmod_otel_tracer **objp,
    const char *vcl_name)
{
        struct vmod_otel_tracer *t;

        ASSERT_CLI();
        CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);

        ALLOC_OBJ(t, OTLPTRACER_MAGIC);
	*objp = t;
}

struct span {
	unsigned                magic;
#define OTLPSPAN_MAGIC	0x56f9f432
	void			*ptr;
};

void span_fini(VRT_CTX, void *p) {
	EndSpan(p);
}

const struct vmod_priv_methods span_methods = {
	.magic = VMOD_PRIV_METHODS_MAGIC,
	.type = "otlp_span",
	.fini = span_fini,
};

VCL_STRING v_matchproto_()
vmod_tracer_trace(VRT_CTX,
    struct vmod_otel_tracer *obj, struct vmod_priv *priv, VCL_STRING traceparent)
{
	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);

	// TODO can't run in vcl_init

	AN(priv);
	// TODO: fail if already tracing
	if (priv->priv) {
		// on the client side, keep the current span
		// on the backend side, start a new one
		if (ctx->req) {
			return traceparent;
		} else {
			EndSpan(priv->priv);
			priv->priv = NULL;
		}
	}
	priv->methods = &span_methods;
	char *s1 = (StartSpan(&priv->priv, ctx->req != NULL, traceparent));
	char *s2 = WS_Copy(ctx->ws, s1, -1);
	free(s1);
	return s2;
}

VCL_VOID v_matchproto_()
vmod_tracer_set_attribute(VRT_CTX,
    struct vmod_otel_tracer *obj, struct vmod_priv *priv,
    VCL_STRING key, VCL_STRING value)
{
	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);

	AN(priv);
	// TODO: fail if not already tracing
	AN(priv->priv);
	SetAttribute(priv->priv, key, value);
}

VCL_VOID v_matchproto_()
vmod_tracer_update_name(VRT_CTX,
    struct vmod_otel_tracer *obj, struct vmod_priv *priv,
    VCL_STRING name)
{
	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);

	AN(priv);
	// TODO: fail if not already tracing
	AN(priv->priv);
	UpdateName(priv->priv, name);
}
