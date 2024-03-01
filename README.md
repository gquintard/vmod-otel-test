``` shell
# clone, get in
git clone git@github.com:gquintard/vmod-otel-test.git --recurse-submodules
cd vmod-otel-test

# build opentelemetry
cd opentelemetry-cpp/
cmake -B build  -DWITH_ABSEIL=ON -DBUILD_SHARED_LIBS=ON -DWITH_OTLP_HTTP=ON
cmake --build build/ -- -j32
cd ..

# set some useful env variables
OPENTELEMETRY_ROOT=`pwd`/opentelemetry-cpp
export LD_LIBRARY_PATH=${OPENTELEMETRY_ROOT}/build/sdk/src/common/:${OPENTELEMETRY_ROOT}/build/sdk/src/trace/:${OPENTELEMETRY_ROOT}/build/exporters/ostream/:${OPENTELEMETRY_ROOT}/build/sdk/src/resource/:${OPENTELEMETRY_ROOT}/build/exporters/otlp
./autogen.sh 
./configure 
make -j 32

# varnish should start, send a request or two
varnishd -f `realpath example.vcl` -n /tmp/beepboop -a :1234 -F -p vmod_path=/usr/lib/varnish/vmods/:`pwd`/src/.libs
curl -v localhost:1234

# start jaeger
docker run   -e COLLECTOR_ZIPKIN_HOST_PORT=:9411   -e COLLECTOR_OTLP_ENABLED=true   -p 6831:6831/udp   -p 6832:6832/udp   -p 5778:5778   -p 16686:16686   -p 4317:4317   -p 4318:4318   -p 14250:14250   -p 14268:14268   -p 14269:14269   -p 9411:9411   jaegertracing/all-in-one:latest

# visit localhost:16686 on your browser
```
