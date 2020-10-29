# string_template - simple and small string template engine in C++17

This is one header library defines *basic_string_template* template class with *string_template* and *wstring_template* specializations.

Simple use:
```
	stpl::string_template st("Hello {{name}}!");
	st.set_subst("name", "World");
	auto r = st.render();
	EXPECT(r, "Hello World!");
```

User can define custom argument regexp to use different argument wrapping. In the example below arguments will be captured by single *{...}* braces:
```
	stpl::string_template st("Hello {name}!", R"(\{([^\}]+)\})");
	st.set_subst("name", "World");
	auto r = st.render();
	EXPECT(r, "Hello World!");
```

There is a polymorphic version available:
```
	std::array<char, 512> buff;
	std::pmr::monotonic_buffer_resource mem(buff.data(), buff.size(), std::pmr::null_memory_resource());

	stpl::pmr::string_template st("Hello {{name}}!", &mem, &mem);
	st.set_subst("name", "World");
	auto r = st.render();
	EXPECT(r, "Hello World!");
```

More usage examples see in tests.cpp file.

Tested in VS C++ 2019.
