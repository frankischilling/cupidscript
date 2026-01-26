# Async / Await

CupidScript supports `async` functions, promises, and the `await` operator. Async calls return a promise and run on a cooperative, single-threaded scheduler. `await` blocks the current execution until the promise resolves (or throws if it rejects).

## Async Functions

```c
async fn add(a, b) {
  return a + b;
}

let v = await add(2, 3); // 5
```

Async function literals are also supported:

```c
let twice = async fn(x) { return x * 2; };
print(await twice(4)); // 8
```

## Await Operator

`await expr` evaluates `expr`. If the value is a promise, it runs the scheduler until the promise resolves and returns its value. If the promise rejects, `await` throws the rejection value.

```c
let v = await (1 + 2); // 3

// Awaiting a promise
let p = promise();
resolve(p, 42);
print(await p); // 42
```

## Notes

* Async functions return a promise and are scheduled for execution.
* `await` blocks until a promise resolves and throws on rejection.
* The scheduler runs cooperatively in the VM; there is no parallelism.

## Promise Helpers

### `promise() -> promise`

Creates a new pending promise.

### `resolve(promise, value = nil) -> bool`

Resolves a promise. Returns `true` if it was pending.

### `reject(promise, value = nil) -> bool`

Rejects a promise. Returns `true` if it was pending.

### `is_promise(value) -> bool`

Checks if a value is a promise.

## Promise Combinators

### `await_all(list[promise]) -> list`

Waits for all promises to resolve. If any reject, the first rejection is thrown.

### `await_any(list[promise]) -> value`

Waits until any promise resolves and returns its value. If all reject, throws the last rejection.

### `await_all_settled(list[promise]) -> list`

Waits for all promises to settle and returns a list of result maps:

```c
// Each result: {status: "fulfilled"|"rejected", value: any}
```

### `timeout(promise, ms) -> promise`

Returns a new promise that resolves/rejects like the input, but rejects if `ms` elapses first.

## Sleep

`sleep(ms)` returns a promise that resolves after `ms` milliseconds.

```c
async fn delayed(msg) {
  await sleep(10);
  return "done: " + msg;
}

print(await delayed("work"));
```
