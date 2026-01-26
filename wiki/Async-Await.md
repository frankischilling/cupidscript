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

## Sleep

`sleep(ms)` returns a promise that resolves after `ms` milliseconds.

```c
async fn delayed(msg) {
  await sleep(10);
  return "done: " + msg;
}

print(await delayed("work"));
```
