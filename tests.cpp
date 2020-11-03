/*
   Copyright (c) 2020 Alex Zhondin <lexxmark.dev@gmail.com>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "string_template.h"
#include <iostream>
#include <memory_resource>
#include <array>
#include <sstream>
#include <functional>

#define EXPECT_M(a, b, s, l) do { if ((a) != (b)) throw std::logic_error(std::string(s) + " At line " + std::to_string(l)); } while(false)
#define EXPECT(a, b) EXPECT_M(a, b, "Test failed.", __LINE__)

struct my_non_owning_traits : stpl::string_template_traits<char>
{
    using arg_value_t = std::string_view;
};

struct my_pmr_non_owning_traits : stpl::pmr::pmr_string_template_traits<char>
{
    using arg_value_t = std::string_view;
};

struct my_callback_traits : stpl::string_template_traits<char>
{
    using arg_value_t = std::function<std::string_view()>;
};

struct my_reusable_traits : stpl::string_template_traits<char>
{
    constexpr static inline bool clear_args_on_parse_template = false;
};

int main()
{
    using namespace stpl;

    try
    {
        // std::string version
        {
            auto st = make_template("Hello {{name}}!");
            st.set_arg("name", "World");
            auto r = st.render();
            EXPECT(r, "Hello World!");
        }

        // std::wstring version
        {
            auto st = make_template<wstring_template>(L"Hello {{name}}!");
            st.set_arg(L"name", L"World");
            auto r = st.render();
            EXPECT(r, L"Hello World!");
        }

        // get_arg
        {
            auto st = make_template("Hello {{name}}!");
            if (auto arg = st.get_arg("name"))
                *arg = "World";
            auto r = st.render();
            EXPECT(r, "Hello World!");
        }

        // emplace_arg
        {
            auto st = make_template("Hello {{name}}!");
            st.emplace_arg("name", "World");
            auto r = st.render();
            EXPECT(r, "Hello World!");
        }

        // custom argument template {name}->name
        {
            auto st = make_template("Hello {name}!", R"(\{([^\}]+)\})");
            st.set_arg("name", "World");
            auto r = st.render();
            EXPECT(r, "Hello World!");
        }

        // custom argument template {name}->{name}
        {
            auto st = make_template("Hello {name}!", R"(\{[^\}]+\})");
            st.set_arg("{name}", "World");
            auto r = st.render();
            EXPECT(r, "Hello World!");
        }

        // std::pmr version
        {
            std::array<char, 1024> buff;
            std::pmr::monotonic_buffer_resource mem(buff.data(), buff.size(), std::pmr::null_memory_resource());

            pmr::string_template st(&mem, &mem);
            st.parse_template("Hello {{name}}!", &mem);
            st.set_arg("name", "World");
            auto r = st.render();
            EXPECT(r, "Hello World!");
        }

        // std::pmr version with single allocator
        {
            std::array<char, 1024> buff;
            std::pmr::monotonic_buffer_resource mem(buff.data(), buff.size(), std::pmr::null_memory_resource());

            auto st = make_template<pmr::string_template>("Hello {{name}}!", dar(), &mem);
            st.set_arg("name", "World");
            auto r = st.render();
            EXPECT(r, "Hello World!");
        }

        // empty
        {
            auto st = make_template("Hello World!");
            auto r = st.render();
            EXPECT(r, "Hello World!");
            EXPECT(st.args().empty(), true);
        }

        // non-owning version
        {
            auto st = make_template<basic_string_template<my_non_owning_traits>>("Hello {{name}}!");
            st.set_arg("name", "World");
            auto r = st.render();
            EXPECT(r, "Hello World!");
        }

        // std::pmr non-owning version
        {
            std::array<char, 1024> buff;
            std::pmr::monotonic_buffer_resource mem(buff.data(), buff.size(), std::pmr::null_memory_resource());

            basic_string_template<my_pmr_non_owning_traits> st(&mem, &mem);
            st.parse_template("Hello {{name}}!", &mem);
            st.set_arg("name", "World");
            auto r = st.render();
            EXPECT(r, "Hello World!");
        }

        // std::pmr non-owning version with single allocator
        {
            std::array<char, 1024> buff;
            std::pmr::monotonic_buffer_resource mem(buff.data(), buff.size(), std::pmr::null_memory_resource());

            auto st = make_template<basic_string_template<my_pmr_non_owning_traits>>("Hello {{name}}!", dar(), &mem);
            st.set_arg("name", "World");
            auto r = st.render();
            EXPECT(r, "Hello World!");
        }

        // multiple arguments
        {
            auto st = make_template("Hello {{name1}}! Hello {{name2}}! Hello {{name1}}!");
            st.set_arg("name1", "World");
            st.set_arg("name2", "Space");
            auto r = st.render();
            EXPECT(r, "Hello World! Hello Space! Hello World!");
        }

        // multiple arguments using visitor
        {
            auto st = make_template("Hello {{name1}}! Hello {{name2}}! Hello {{name1}}!");
            st.set_args([](auto& name, auto& value) {
                if (name == "name1")
                    value = "World";
                else if (name == "name2")
                    value = "Space";
            });
            auto r = st.render();
            EXPECT(r, "Hello World! Hello Space! Hello World!");
        }

        // stream version
        {
            auto st = make_template("Hello {{name}}!");
            st.set_arg("name", "World");

            std::stringstream str;
            st.render(str);
            EXPECT(str.str(), "Hello World!");
        }

        // argument values as callbacks
        {
            using namespace std::literals;
            auto st = make_template<basic_string_template<my_callback_traits>>("Hello {{name}}!");
            st.parse_template("Hello {{name}}!");
            st.set_arg("name", [] { return "World"sv; });
            auto r = st.render();
            EXPECT(r, "Hello World!");
        }

        // partial arguments
        {
            auto st = make_template("Hello {{name1}}! Hello {{name2}}! Hello {{name1}}!");
            st.set_arg("name2", "Space");
            auto r = st.render();
            EXPECT(r, "Hello {{name1}}! Hello Space! Hello {{name1}}!");
        }

        // partial arguments using visitor 
        {
            auto st = make_template("Hello {{name1}}! Hello {{name2}}! Hello {{name1}}!");
            st.set_args_if([](auto& name, auto& value) {
                if (name == "name2")
                {
                    value = "Space";
                    return true;
                }
                return false;
            });
            auto r = st.render();
            EXPECT(r, "Hello {{name1}}! Hello Space! Hello {{name1}}!");
        }

        // multiple templates for the same args
        {
            auto st = make_template<basic_string_template<my_reusable_traits>>("Hello {{name}}!");
            st.set_arg("name", "World");
            auto r = st.render();
            EXPECT(r, "Hello World!");

            st.parse_template("Bye {{name}}!");
            r = st.render();
            EXPECT(r, "Bye World!");
        }
    }
    catch (const std::logic_error& e)
    {
        std::cout << e.what() << std::endl;
        return 1;
    }

    std::cout << "SUCCESS!" << std::endl;
    return 0;
}
