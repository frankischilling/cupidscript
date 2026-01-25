// Demonstrates: require/load, lists, maps, indexing, fmt, string/path helpers

require("lib.cs");

print(hello("world"));

let xs = list();
push(xs, 10);
push(xs, 20);
xs[1] = 99;
print("xs len =", len(xs), "xs[0] =", xs[0], "xs[1] =", xs[1]);

let m = make_sample_data();
print("name =", m["name"], "age =", m["age"]);
print("tags len =", len(m["tags"]), "tag0 =", m["tags"][0]);

print(fmt("fmt: x=%d s=%s b=%b v=%v", 42, "ok", true, m));

let parts = str_split("a,b,c", ",");
print("split len =", len(parts), "parts[2] =", parts[2]);

print("basename =", path_basename("/tmp/a/b.txt"), "ext =", path_ext("/tmp/a/b.txt"));

