fn hello(name) {
  return "hello " + name;
}

fn make_sample_data() {
  let user = map();
  user["name"] = "cupid";
  user["age"] = 5;

  let tags = list();
  push(tags, "plugin");
  push(tags, "demo");
  user["tags"] = tags;

  return user;
}

