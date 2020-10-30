# string_template - simple and small string template engine in C++17

This is one header library defines *basic_string_template* template class with *string_template* and *wstring_template* specializations.

Simple use:
```
	stpl::string_template st("Hello {{name}}!");
	st.set_arg("name", "World");
	auto r = st.render();
	EXPECT(r, "Hello World!");
```

User can define custom argument regexp to use different argument wrapping. In the example below arguments will be captured by single *{...}* braces:
```
	stpl::string_template st("Hello {name}!", R"(\{([^\}]+)\})");
	st.set_arg("name", "World");
	auto r = st.render();
	EXPECT(r, "Hello World!");
```

There is a polymorphic version available. User can supply different allocators for different internal containers:
```
	std::array<char, 1024> buff;
	std::pmr::monotonic_buffer_resource mem(buff.data(), buff.size(), std::pmr::null_memory_resource());

	stpl::pmr::string_template st("Hello {{name}}!", &mem, &mem, &mem);
	st.set_arg("name", "World");
	auto r = st.render();
	EXPECT(r, "Hello World!");
```

Or single allocator can be used:
```
	std::array<char, 1024> buff;
	std::pmr::monotonic_buffer_resource mem(buff.data(), buff.size(), std::pmr::null_memory_resource());

	stpl::pmr::string_template st("Hello {{name}}!", stpl::default_arg_template(), &mem);
	st.set_arg("name", "World");
	auto r = st.render();
	EXPECT(r, "Hello World!");
```

It's possible to supply visitor to process all arguments:
```
	string_template st("Hello {{name1}}!Hello {{name2}}!Hello {{name1}}!");
	st.set_args([](auto& name, auto& value) {
		if (name == "name1")
			value = "World";
		else if (name == "name2")
			value = "Space";
	});
	auto r = st.render();
	EXPECT(r, "Hello World!Hello Space!Hello World!");
```

There is a stream version of *render* function:
```
	string_template st("Hello {{name}}!");
	st.set_arg("name", "World");

	std::stringstream str;
	st.render(str);
	EXPECT(str.str(), "Hello World!");
```

More usage examples see in tests.cpp file.

Tested in VS C++ 2019.
