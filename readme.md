# curl-thread

Wraps the `curl_easy_..()` interface for multithreading applications.

Once the `curl::thread()` is started, HTTP requests can be queued from various threads.

## Example
[`blue::fn()`](./test/src/main.cpp#L134) and `cyan::fn()` are good starting points.
