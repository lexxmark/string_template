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

#pragma once

#ifndef STRING_TEMPLATE_H
#define STRING_TEMPLATE_H

#include <string>
#include <string_view>
#include <regex>
#include <map>
#include <vector>
#include <variant>

namespace stpl
{
	template <typename CharT>
	struct string_template_traits
	{
		// char type
		using char_t = CharT;
		// type to store template string
		using template_t = std::basic_string<char_t>;
		// type to store substitution value
		using subst_value_t = std::basic_string<char_t>;

		// container type for substition map
		template <typename K, typename V>
		using subst_map_t = std::map<K, V>;

		// container type for parts vector
		template <typename V>
		using parts_vector_t = std::vector<V>;
	};

	template<class Traits>
	class basic_string_template
	{
	public:
		using char_t = typename Traits::char_t;
		using string_view_t = std::basic_string_view<char_t>;
		using template_t = typename Traits::template_t;
		using subst_value_t = typename Traits::subst_value_t;
		using subst_map_t = typename Traits::template subst_map_t<string_view_t, subst_value_t>;
		using part_t = std::variant<string_view_t, std::reference_wrapper<const subst_value_t>>;
		using parts_vector_t = typename Traits::template parts_vector_t<part_t>;

		basic_string_template(template_t template_, const std::basic_regex<char_t>& subst_template)
			: m_template(std::move(template_))
		{
			compile(subst_template);
		}

		basic_string_template(template_t template_, const std::basic_regex<char_t>& subst_template, const typename subst_map_t::allocator_type& m_alloc, const typename parts_vector_t::allocator_type& p_alloc)
			: m_template(std::move(template_)),
			m_substs(m_alloc),
			m_parts(p_alloc)
		{
			compile(std::basic_regex<char_t>(subst_template));
		}

		explicit basic_string_template(template_t template_, string_view_t subst_template = default_subst_template<char_t>())
			: m_template(std::move(template_))
		{
			compile(std::basic_regex<char_t>(subst_template.data(), subst_template.length()));
		}

		basic_string_template(template_t template_, string_view_t subst_template, const typename subst_map_t::allocator_type& m_alloc, const typename parts_vector_t::allocator_type& p_alloc)
			: m_template(std::move(template_)),
			m_substs(m_alloc),
			m_parts(p_alloc)
		{
			compile(std::basic_regex<char_t>(subst_template.data(), subst_template.length()));
		}

		basic_string_template(template_t template_, const typename subst_map_t::allocator_type& m_alloc, const typename parts_vector_t::allocator_type& p_alloc)
			: m_template(std::move(template_)),
			m_substs(m_alloc),
			m_parts(p_alloc)
		{
			auto subst_template = default_subst_template<char_t>();
			compile(std::basic_regex<char_t>(subst_template.data(), subst_template.length()));
		}

		subst_value_t* get_subst(string_view_t key) noexcept
		{
			if (auto it = m_substs.find(key); it != m_substs.end())
				return &it->second;
			else
				return nullptr;
		}

		bool set_subst(string_view_t key, subst_value_t value)
		{
			if (auto subst = get_subst(key))
			{
				*subst = std::move(value);
				return true;
			}

			return false;
		}

		template<typename Pred>
		void set_substs(const Pred pred)
		{
			for (auto& [k, v] : m_substs)
				pred(k, v);
		}

		const auto& substs() const noexcept { return m_substs; }

		void render(std::basic_string<char_t>& result) const
		{
			for (const auto& p : m_parts)
			{
				if (p.index() == 0)
					result += std::get<0>(p);
				else
					result += std::get<1>(p).get();
			}
		}

		std::basic_string<char_t> render() const
		{
			std::basic_string<char_t> result;
			render(result);
			return result;
		}

		void render(std::basic_ostream<char_t>& out) const
		{
			for (const auto& p : m_parts)
			{
				if (p.index() == 0)
					out << std::get<0>(p);
				else
					out << std::get<1>(p).get();
			}
		}

	private:
		void compile(const std::basic_regex<char_t>& subst_template)
		{
			std::match_results<typename template_t::const_iterator> match;
			auto b = std::cbegin(m_template);
			auto e = std::cend(m_template);

			// search substitutions using subst_template
			while (std::regex_search(b, e, match, subst_template))
			{
				if (match.empty())
					break;

				// save unmatched prefix to m_parts
				string_view_t prefix(&*match.prefix().first, match.prefix().length());
				m_parts.push_back(prefix);

				size_t i = 0;
				if (match.size() == 2)
					i = 1;

				// save matched substitution name to m_substs
				// and save const ref to substitution value to m_parts
				string_view_t s(&*match[i].first, match[i].length());
				m_parts.push_back(std::cref(m_substs[s]));

				// move to first un-searched symbol
				b = match[0].second;
			}

			// save remaining symbols to m_parts
			if (b != e)
			{
				string_view_t last_suffix(&*b, std::distance(b, e));
				m_parts.push_back(last_suffix);
			}
		}

		template<typename CharT>
		static std::basic_string_view<CharT> default_subst_template();

		template<>
		static std::string_view default_subst_template<char>()
		{
			return R"(\{\{([^\}]+)\}\})";
		}

		template<>
		static std::wstring_view default_subst_template<wchar_t>()
		{
			return LR"(\{\{([^\}]+)\}\})";
		}

		template_t m_template;
		subst_map_t m_substs;
		parts_vector_t m_parts;
	};

	using string_template = basic_string_template<string_template_traits<char>>;
	using wstring_template = basic_string_template<string_template_traits<wchar_t>>;

	namespace pmr
	{
		template <typename CharT>
		struct pmr_string_template_traits : public string_template_traits<CharT>
		{
			// container type for substition map
			template <typename K, typename V>
			using subst_map_t = std::pmr::map<K, V>;

			// container type for parts vector
			template <typename V>
			using parts_vector_t = std::pmr::vector<V>;
		};

		using string_template = basic_string_template<pmr_string_template_traits<char>>;
		using wstring_template = basic_string_template<pmr_string_template_traits<wchar_t>>;
	}
}

#endif
