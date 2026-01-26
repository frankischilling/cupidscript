// Test tcp_connect - connects to a local echo server or times out gracefully

// Test 1: Basic connection (will fail if no server, that's expected)
let connected = false;
try {
    // Try to connect to localhost port 12345 (likely nothing there)
    let sock = await tcp_connect("127.0.0.1", 12345);
    connected = true;
    socket_close(sock);
} catch (e) {
    // Expected: connection refused or timeout
    print("connect failed (expected):", e);
}

// Test 2: Invalid host should error
let bad_host_error = false;
try {
    let sock = await tcp_connect("this-host-does-not-exist-12345.invalid", 80);
    socket_close(sock);
} catch (e) {
    bad_host_error = true;
}
assert(bad_host_error, "should error on invalid host");

print("tcp_connect tests passed");
