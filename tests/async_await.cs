async fn add(a, b) { return a + b; }

let r = await add(2, 3);
assert(r == 5, "await async fn");

let twice = async fn(x) { return x * 2; };
assert(await twice(4) == 8, "await async literal");

print("async await ok");
