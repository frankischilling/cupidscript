// Simple benchmark for CupidScript (CPU-bound, single-threaded).
// Run: bin/cupidscript examples/benchmark.cs

fn bench_arith(n) {
  let i = 0;
  let x = 1;
  let y = 2;
  let acc = 0;
  while (i < n) {
    x = x * 1664525 + 1013904223;
    y = y * 1103515245 + 12345;
    acc = acc + (x % 97) + (y % 89);
    i = i + 1;
  }
  return acc;
}

fn bench_calls(n) {
  fn f(a, b, c) { return (a + b) * c + (a % 7); }
  let i = 0;
  let acc = 0;
  while (i < n) {
    acc = acc + f(i, i + 1, 3);
    i = i + 1;
  }
  return acc;
}

fn bench_lists(n) {
  let xs = list();
  let i = 0;
  while (i < n) {
    push(xs, i);
    i = i + 1;
  }
  // mutate + sum
  i = 0;
  let sum = 0;
  while (i < n) {
    xs[i] = xs[i] * 2;
    sum = sum + xs[i];
    i = i + 1;
  }
  return sum;
}

fn bench_maps(n) {
  let m = map();
  let i = 0;
  while (i < n) {
    m[fmt("k%d", i)] = i;
    i = i + 1;
  }
  let ks = keys(m);
  i = 0;
  let sum = 0;
  while (i < len(ks)) {
    sum = sum + m[ks[i]];
    i = i + 1;
  }
  return sum;
}

fn bench_strings_append(n) {
  let i = 0;
  let s = "";
  while (i < n) {
    // common pattern in plugins: incremental append
    s = s + "x";
    i = i + 1;
  }
  return len(s);
}

fn bench_strings_replace(n) {
  let i = 0;
  let s = "";
  while (i < n) {
    // worst-case: forces full-copy style string processing
    if (len(s) > 256) { s = str_replace(s, "x", "x"); }
    s = s + "x";
    i = i + 1;
  }
  return len(s);
}

fn bench_strbuf(n) {
  let b = strbuf();
  let i = 0;
  while (i < n) {
    b.append(i);
    b.append(",");
    i = i + 1;
  }
  let s = b.str();
  return len(s);
}

let N = 200000;
let Nsmall = 5000;

print("=== Cupidscript benchmark ===");
print("N =", N, "Nsmall =", Nsmall);

let t0 = now_ms();
let a = bench_arith(N);
let t1 = now_ms();

let b = bench_calls(N);
let t2 = now_ms();

let c = bench_lists(Nsmall);
let t3 = now_ms();

let d = bench_maps(Nsmall);
let t4 = now_ms();

let e = bench_strings_append(200000);
let t5 = now_ms();

let f = bench_strings_replace(20000);
let t6 = now_ms();

let g = bench_strbuf(200000);
let t7 = now_ms();

print("arith ms =", t1 - t0, "result =", a);
print("calls ms =", t2 - t1, "result =", b);
print("lists ms =", t3 - t2, "result =", c);
print("maps  ms =", t4 - t3, "result =", d);
print("str+  ms =", t5 - t4, "result =", e);
print("str~  ms =", t6 - t5, "result =", f);
print("strb  ms =", t7 - t6, "result =", g);
print("total ms =", t7 - t0);
