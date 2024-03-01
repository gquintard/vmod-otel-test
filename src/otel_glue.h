void InitTracer();
void CleanupTracer(); 
char *StartSpan(void *cp, int is_client_side, const char *traceparent);
void EndSpan(void *);
void SetAttribute(void *cp, const char *key, const char *value);
void UpdateName(void *cp, const char *name);
