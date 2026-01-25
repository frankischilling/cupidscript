// switch basic matching + default + non-fallthrough

let hit = "";

switch (2) {
  case 1 { hit = "one"; }
  case 2 { hit = "two"; }
  default { hit = "default"; }
}
assert(hit == "two", "int match");

hit = "";
switch ("b") {
  case "a" { hit = "a"; }
  case "b" { hit = "b"; }
  default { hit = "d"; }
}
assert(hit == "b", "string match");

hit = "";
switch (true) {
  case false { hit = "f"; }
  case true { hit = "t"; }
}
assert(hit == "t", "bool match");

hit = "";
switch (nil) {
  case 0 { hit = "zero"; }
  case nil { hit = "nil"; }
  default { hit = "d"; }
}
assert(hit == "nil", "nil match");

hit = "";
switch (123) {
  case 1 { hit = "no"; }
  default { hit = "yes"; }
}
assert(hit == "yes", "default hit");

// Ensure non-fallthrough: only one case executes.
let c = 0;
switch (1) {
  case 1 { c += 1; }
  case 1 { c += 100; }
  default { c += 1000; }
}
assert(c == 1, "non-fallthrough");

print("switch_basic ok");

