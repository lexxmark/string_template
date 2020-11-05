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

#ifndef STPL_STRING_TEMPLATE_H
#define STPL_STRING_TEMPLATE_H

#include <string>
#include <string_view>
#include <regex>
#include <map>
#include <vector>
#include <variant>

namespace stpl
{
	namespace detail
	{
		template<typename CharT>
		constexpr std::basic_string_view<CharT> default_arg_regex();

		template<>
		constexpr std::string_view default_arg_regex<char>()
		{
			return R"(\{\{([^\}]+)\}\})";
		}

		template<>
		constexpr std::wstring_view default_arg_regex<wchar_t>()
		{
			return LR"(\{\{([^\}]+)\}\})";
		}
	}

	template <typename CharT>
	struct string_template_traits
	{
		// char type
		using char_t = CharT;

		// type to store argument value
		using arg_value_t = std::basic_string<char_t>;

		// container type for argument map
		template <typename K, typename V>
		using args_map_t = std::map<K, V>;

		// container type for parts vector
		template <typename V>
		using parts_vector_t = std::vector<V>;

		// std::match allocator type
		template <typename V>
		using match_allocator_t = std::allocator<V>;

		constexpr static inline std::basic_string_view<char_t> default_arg_regex = detail::default_arg_regex<char_t>();
		constexpr static inline bool clear_args_on_parse_template = true;
	};

	struct default_arg_regex_t {};
	inline default_arg_regex_t default_arg_regex() { return {}; }
	inline default_arg_regex_t dar() { return {}; }


	template<class Traits>
	class basic_string_template
	{
	public:
		using char_t = typename Traits::char_t;
		using string_view_t = std::basic_string_view<char_t>;
		using string_t = std::basic_string<char_t>;
		using basic_ostream_t = std::basic_ostream<char_t>;
		using arg_value_t = typename Traits::arg_value_t;
		using arg_store_value_t = std::variant<string_view_t, arg_value_t>;
		using args_map_t = typename Traits::template args_map_t<string_view_t, arg_store_value_t>;
		using part_t = std::variant<string_view_t, const arg_store_value_t*>;
		using parts_vector_t = typename Traits::template parts_vector_t<part_t>;

		using regex_t = std::basic_regex<char_t>;
		using match_const_it_t = typename string_view_t::const_iterator;
		using match_allocator_t = typename Traits::template match_allocator_t<std::sub_match<match_const_it_t>>;
		using match_results_t = std::match_results<match_const_it_t, match_allocator_t>;

		constexpr static inline string_view_t default_arg_regex = Traits::default_arg_regex;
		constexpr static inline bool clear_args_on_parse_template = Traits::clear_args_on_parse_template;

		basic_string_template() = default;

		basic_string_template(basic_string_template&& other) noexcept
			: m_args(std::move(other.m_args)),
			m_parts(std::move(other.m_parts))
		{}
		basic_string_template& operator=(basic_string_template&& other) noexcept
		{
			m_args = std::move(other.m_args);
			m_parts = std::move(other.m_parts);
			return *this;
		}

		basic_string_template(const basic_string_template&) = delete;
		basic_string_template& operator=(const basic_string_template&) = delete;

		basic_string_template(const typename args_map_t::allocator_type& a_alloc, const typename parts_vector_t::allocator_type& p_alloc)
			: m_args(a_alloc),
			m_parts(p_alloc)
		{
		}

		template <class Alloc>
		explicit basic_string_template(const Alloc& alloc)
			: m_args(alloc),
			m_parts(alloc)
		{
		}

		void parse_template(string_view_t str_template)
		{
			match_results_t match_results;
			parse_template(str_template, regex_t(default_arg_regex.data(), default_arg_regex.length()), match_results);
		}

		void parse_template(string_view_t str_template, const typename match_results_t::allocator_type& alloc)
		{
			match_results_t match_results(alloc);
			parse_template(str_template, regex_t(default_arg_regex.data(), default_arg_regex.length()), match_results);
		}

		void parse_template(string_view_t str_template, match_results_t& match_results)
		{
			parse_template(str_template, regex_t(default_arg_regex.data(), default_arg_regex.length()), match_results);
		}

		void parse_template(string_view_t str_template, string_view_t arg_regex)
		{
			match_results_t match_results;
			parse_template(str_template, regex_t(arg_regex.data(), arg_regex.length()), match_results);
		}

		void parse_template(string_view_t str_template, string_view_t arg_regex, const typename match_results_t::allocator_type& alloc)
		{
			match_results_t mr(alloc);
			parse_template(str_template, regex_t(arg_regex.data(), arg_regex.length()), mr);
		}

		void parse_template(string_view_t str_template, string_view_t arg_regex, match_results_t& match_results)
		{
			parse_template(str_template, regex_t(arg_regex.data(), arg_regex.length()), match_results);
		}

		void parse_template(string_view_t str_template, const regex_t& arg_regex)
		{
			match_results_t match_results;
			parse_template(str_template, arg_regex, match_results);
		}

		void parse_template(string_view_t str_template, const regex_t& arg_regex, const typename match_results_t::allocator_type& alloc)
		{
			match_results_t mr(alloc);
			parse_template(str_template, arg_regex, mr);
		}

		void parse_template(string_view_t str_template, const regex_t& arg_regex, match_results_t& match)
		{
			m_parts.clear();

			if constexpr (clear_args_on_parse_template)
				m_args.clear();

			auto b = std::cbegin(str_template);
			auto e = std::cend(str_template);

			// search arguments using arg_template
			while (std::regex_search(b, e, match, arg_regex))
			{
				// save unmatched prefix to m_parts
				string_view_t prefix(&*match.prefix().first, match.prefix().length());
				m_parts.push_back(prefix);

				size_t i = 0;
				if (match.size() == 2)
					i = 1;

				// save matched argument name to m_args
				// by default argument value is a full argument name
				string_view_t arg_name(&*match[i].first, match[i].length());
				auto res = m_args.try_emplace(arg_name, std::in_place_index<0>, &*match[0].first, match[0].length());

				// and save const ptr to argument value to m_parts
				m_parts.push_back(&res.first->second);

				// move to next un-searched symbol
				b = match[0].second;
			}

			// save remaining symbols to m_parts
			if (b != e)
			{
				string_view_t last_suffix(&*b, std::distance(b, e));
				m_parts.push_back(last_suffix);
			}
		}

		arg_value_t* get_arg(string_view_t key)
		{
			if (auto it = m_args.find(key); it != m_args.end())
			{
				auto& v = it->second;
				if (v.index() == 0)
					v.emplace<1>();
				return &std::get<1>(v);
			}
			else
				return nullptr;
		}

		bool set_arg(string_view_t key, arg_value_t value)
		{
			if (auto it = m_args.find(key); it != m_args.end())
			{
				it->second.emplace<1>(std::move(value));
				return true;
			}

			return false;
		}

		template<class... Args>
		bool emplace_arg(string_view_t key, Args&&... args)
		{
			if (auto it = m_args.find(key); it != m_args.end())
			{
				it->second.emplace<1>(std::forward<Args>(args)...);
				return true;
			}

			return false;
		}

		template<typename Visitor>
		void set_args(Visitor&& vis)
		{
			for (auto& [k, v] : m_args)
			{
				if (v.index() == 0)
					v.emplace<1>();
				std::invoke(std::forward<Visitor>(vis), k, std::get<1>(v));
			}
		}

		template<typename Visitor>
		void set_args_if(Visitor&& vis)
		{
			for (auto& [k, v] : m_args)
			{
				arg_value_t value;
				if (std::invoke(std::forward<Visitor>(vis), k, value))
					v.emplace<1>(std::move(value));
			}
		}

		template<typename Visitor>
		void set_args_uninitialized_if(Visitor&& vis)
		{
			for (auto& [k, v] : m_args)
			{
				if (v.index() == 0)
				{
					arg_value_t value;
					if (std::invoke(std::forward<Visitor>(vis), k, value))
						v.emplace<1>(std::move(value));
				}
			}
		}

		const auto& args() const noexcept { return m_args; }

		bool is_args_complete() const noexcept
		{
			for (auto& [k, v] : m_args)
			{
				// argument value uninitialized (a reference to argument name)
				if (v.index() == 0)
					return false;
			}
			return true;
		}

		void clear() noexcept
		{
			m_parts.clear();
			m_args.clear();
		}

		void swap(basic_string_template& other) noexcept
		{
			std::swap(m_args, other.m_args);
			std::swap(m_parts, other.m_parts);
		}

		template <class Visitor>
		void render_to(Visitor&& vis) const
		{
			for (const auto& p : m_parts)
			{
				// if part is a piece of template
				if (p.index() == 0)
					std::invoke(std::forward<Visitor>(vis), std::get<0>(p));
				else
				{
					// part is argument value or piece of template
					const auto& arg_value = *std::get<1>(p);
					// if argument value uninitialized -> get piece of template
					if (arg_value.index() == 0)
						std::invoke(std::forward<Visitor>(vis), std::get<0>(arg_value));
					else
					{
						// argument value is initialized
						// if argument value is callable -> invoke with no arguments
						if constexpr (std::is_invocable_v<arg_value_t>)
							std::invoke(std::forward<Visitor>(vis), std::get<1>(arg_value)());
						else
							// argument value is orginary value
							std::invoke(std::forward<Visitor>(vis), std::get<1>(arg_value));
					}
				}
			}
		}

		void render(string_t& result) const
		{
			render_to([&result](const auto& part) { result += part; });
		}

		string_t render() const
		{
			string_t result;
			render(result);
			return result;
		}

		void render(basic_ostream_t& out) const
		{
			render_to([&out](const auto& part) { out << part; });
		}

	private:
		args_map_t m_args;
		parts_vector_t m_parts;
	};

	using string_template = basic_string_template<string_template_traits<char>>;
	using wstring_template = basic_string_template<string_template_traits<wchar_t>>;

	template <class StringTemplate = string_template>
	StringTemplate make_template(typename StringTemplate::string_view_t str_template)
	{
		StringTemplate st;
		st.parse_template(str_template);
		return st;
	}

	template <class StringTemplate = string_template>
	StringTemplate make_template(typename StringTemplate::string_view_t str_template, typename StringTemplate::string_view_t arg_regex)
	{
		StringTemplate st;
		st.parse_template(str_template, arg_regex);
		return st;
	}

	template <class StringTemplate, class Alloc>
	StringTemplate make_template(typename StringTemplate::string_view_t str_template, typename StringTemplate::string_view_t arg_regex, const Alloc& alloc)
	{
		StringTemplate st(alloc);
		st.parse_template(str_template, arg_regex, alloc);
		return st;
	}

	template <class StringTemplate, class Alloc>
	StringTemplate make_template(typename StringTemplate::string_view_t str_template, default_arg_regex_t , const Alloc& alloc)
	{
		StringTemplate st(alloc);
		st.parse_template(str_template, alloc);
		return st;
	}

	namespace pmr
	{
		template <typename CharT>
		struct pmr_string_template_traits : public string_template_traits<CharT>
		{
			// container type for arguments map
			template <typename K, typename V>
			using args_map_t = std::pmr::map<K, V>;

			// container type for parts vector
			template <typename V>
			using parts_vector_t = std::pmr::vector<V>;

			// std::match allocator type
			template <typename V>
			using match_allocator_t = std::pmr::polymorphic_allocator<V>;
		};

		using string_template = basic_string_template<pmr_string_template_traits<char>>;
		using wstring_template = basic_string_template<pmr_string_template_traits<wchar_t>>;

	} // end namespace pmr
} // end namespace stpl

#endif //STPL_STRING_TEMPLATE_H
