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

#define EXPECT_M(a, b, s) do { if ((a) != (b)) throw std::logic_error(s); } while(false)
#define EXPECT(a, b) EXPECT_M(a, b, "Test failed.")

struct my_non_owning_traits : stpl::string_template_traits<char>
{
    using template_t = std::string_view;
    using subst_value_t = std::string_view;
};

struct my_pmr_non_owning_traits : stpl::pmr::pmr_string_template_traits<char>
{
    using template_t = std::string_view;
    using subst_value_t = std::string_view;
};

int main()
{
    using namespace stpl;

    try
    {
        // std::string version
        {
            string_template st("Hello {{name}}!");
            st.set_subst("name", "World");
            auto r = st.render();
            EXPECT(r, "Hello World!");
        }

        // std::wstring version
        {
            wstring_template st(L"Hello {{name}}!");
            st.set_subst(L"name", L"World");
            auto r = st.render();
            EXPECT(r, L"Hello World!");
        }

        // custom substitution template {name}->name
        {
            string_template st("Hello {name}!", R"(\{([^\}]+)\})");
            st.set_subst("name", "World");
            auto r = st.render();
            EXPECT(r, "Hello World!");
        }

        // custom substitution template {name}->{name}
        {
            string_template st("Hello {name}!", R"(\{[^\}]+\})");
            st.set_subst("{name}", "World");
            auto r = st.render();
            EXPECT(r, "Hello World!");
        }

        // std::pmr version
        {
            std::array<char, 512> buff;
            std::pmr::monotonic_buffer_resource mem(buff.data(), buff.size(), std::pmr::null_memory_resource());

            pmr::string_template st("Hello {{name}}!", &mem, &mem);
            st.set_subst("name", "World");
            auto r = st.render();
            EXPECT(r, "Hello World!");
        }

        // empty
        {
            string_template st("Hello World!");
            auto r = st.render();
            EXPECT(r, "Hello World!");
            EXPECT(st.substs().empty(), true);
        }

        // non-owning version
        {
            basic_string_template<my_non_owning_traits> st("Hello {{name}}!");
            st.set_subst("name", "World");
            auto r = st.render();
            EXPECT(r, "Hello World!");
        }

        // std::pmr non-owning version
        {
            std::array<char, 512> buff;
            std::pmr::monotonic_buffer_resource mem(buff.data(), buff.size(), std::pmr::null_memory_resource());

            basic_string_template<my_pmr_non_owning_traits> st("Hello {{name}}!", &mem, &mem);
            st.set_subst("name", "World");
            auto r = st.render();
            EXPECT(r, "Hello World!");
        }

        // multiple substitutions
        {
            string_template st("Hello {{name1}}!Hello {{name2}}!Hello {{name1}}!");
            st.set_subst("name1", "World");
            st.set_subst("name2", "Space");
            auto r = st.render();
            EXPECT(r, "Hello World!Hello Space!Hello World!");
        }

        // multiple substitutions using predicate
        {
            string_template st("Hello {{name1}}!Hello {{name2}}!Hello {{name1}}!");
            st.set_substs([](auto& k, auto&v) {
                if (k == "name1")
                    v = "World";
                else if (k == "name2")
                    v = "Space";
            });
            auto r = st.render();
            EXPECT(r, "Hello World!Hello Space!Hello World!");
        }

        // stream version
        {
            string_template st("Hello {{name}}!");
            st.set_subst("name", "World");

            std::stringstream str;
            st.render(str);
            EXPECT(str.str(), "Hello World!");
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
