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
	namespace detail
	{
		template<typename CharT>
		constexpr std::basic_string_view<CharT> default_arg_template();

		template<>
		constexpr std::string_view default_arg_template<char>()
		{
			return R"(\{\{([^\}]+)\}\})";
		}

		template<>
		constexpr std::wstring_view default_arg_template<wchar_t>()
		{
			return LR"(\{\{([^\}]+)\}\})";
		}
	}

	template <typename CharT>
	struct string_template_traits
	{
		// char type
		using char_t = CharT;
		// type to store template string
		using template_t = std::basic_string<char_t>;
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

		constexpr static inline std::basic_string_view<char_t> default_arg_template = detail::default_arg_template<char_t>();
	};

	struct default_arg_template_t
	{};
	default_arg_template_t default_arg_template() { return {}; }
	default_arg_template_t dat() { return {}; }


	template<class Traits>
	class basic_string_template
	{
	public:
		using char_t = typename Traits::char_t;
		using string_view_t = std::basic_string_view<char_t>;
		using string_t = std::basic_string<char_t>;
		using basic_ostream_t = std::basic_ostream<char_t>;
		using template_t = typename Traits::template_t;
		using arg_value_t = typename Traits::arg_value_t;
		using arg_store_value_t = std::variant<string_view_t, arg_value_t>;
		using args_map_t = typename Traits::template args_map_t<string_view_t, arg_store_value_t>;
		using part_t = std::variant<string_view_t, const arg_store_value_t*>;
		using parts_vector_t = typename Traits::template parts_vector_t<part_t>;

		using regex_t = std::basic_regex<char_t>;
		using template_const_it_t = decltype(std::cbegin(template_t()));
		using match_allocator_t = typename Traits::template match_allocator_t<std::sub_match<template_const_it_t>>;
		using match_results_t = std::match_results<template_const_it_t, match_allocator_t>;

		constexpr static inline string_view_t default_arg_template = Traits::default_arg_template;

		basic_string_template(template_t template_, const regex_t& arg_template)
			: m_template(std::move(template_))
		{
			match_results_t mr;
			compile(arg_template, mr);
		}

		basic_string_template(template_t template_, const regex_t& arg_template, const typename args_map_t::allocator_type& a_alloc, const typename parts_vector_t::allocator_type& p_alloc, const typename match_results_t::allocator_type& m_alloc)
			: m_template(std::move(template_)),
			m_args(a_alloc),
			m_parts(p_alloc)
		{
			match_results_t mr(m_alloc);
			compile(arg_template, mr);
		}

		template <class Alloc>
		basic_string_template(template_t template_, const regex_t& arg_template, const Alloc& alloc)
			: m_template(std::move(template_)),
			m_args(alloc),
			m_parts(alloc)
		{
			match_results_t mr(alloc);
			compile(arg_template, mr);
		}

		explicit basic_string_template(template_t template_, string_view_t arg_template = default_arg_template)
			: m_template(std::move(template_))
		{
			match_results_t mr;
			compile(regex_t(arg_template.data(), arg_template.length()), mr);
		}

		basic_string_template(template_t template_, string_view_t arg_template, const typename args_map_t::allocator_type& a_alloc, const typename parts_vector_t::allocator_type& p_alloc, const typename match_results_t::allocator_type& m_alloc)
			: m_template(std::move(template_)),
			m_args(a_alloc),
			m_parts(p_alloc)
		{
			match_results_t mr(m_alloc);
			compile(regex_t(arg_template.data(), arg_template.length()), mr);
		}

		template <class Alloc>
		basic_string_template(template_t template_, string_view_t arg_template, const Alloc& alloc)
			: m_template(std::move(template_)),
			m_args(alloc),
			m_parts(alloc)
		{
			match_results_t mr(alloc);
			compile(regex_t(arg_template.data(), arg_template.length()), mr);
		}


		basic_string_template(template_t template_, const typename args_map_t::allocator_type& a_alloc, const typename parts_vector_t::allocator_type& p_alloc, const typename match_results_t::allocator_type& m_alloc)
			: m_template(std::move(template_)),
			m_args(a_alloc),
			m_parts(p_alloc)
		{
			match_results_t mr(m_alloc);
			compile(regex_t(default_arg_template.data(), default_arg_template.length()), mr);
		}

		template <class Alloc>
		basic_string_template(template_t template_, default_arg_template_t, const Alloc& alloc)
			: m_template(std::move(template_)),
			m_args(alloc),
			m_parts(alloc)
		{
			match_results_t mr(alloc);
			compile(regex_t(default_arg_template.data(), default_arg_template.length()), mr);
		}

		arg_value_t* get_arg(string_view_t key) noexcept
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
		void set_args(const Visitor vis)
		{
			for (auto& [k, v] : m_args)
			{
				if (v.index() == 0)
					v.emplace<1>();
				vis(k, std::get<1>(v));
			}
		}

		template<typename Visitor>
		void set_args_if(const Visitor vis)
		{
			for (auto& [k, v] : m_args)
			{
				arg_value_t value;
				if (vis(k, value))
					v.emplace<1>(std::move(value));
			}
		}

		const auto& args() const noexcept { return m_args; }

		void render(string_t& result) const
		{
			for (const auto& p : m_parts)
			{
				if (p.index() == 0)
					result += std::get<0>(p);
				else
				{
					const auto& arg_value = *std::get<1>(p);
					if (arg_value.index() == 0)
						result += std::get<0>(arg_value);
					else
					{
						if constexpr (std::is_invocable_v<arg_value_t>)
							result += std::get<1>(arg_value)();
						else
							result += std::get<1>(arg_value);
					}
				}
			}
		}

		string_t render() const
		{
			string_t result;
			render(result);
			return result;
		}

		void render(basic_ostream_t& out) const
		{
			for (const auto& p : m_parts)
			{
				if (p.index() == 0)
					out << std::get<0>(p);
				else
				{
					const auto& arg_value = *std::get<1>(p);
					if (arg_value.index() == 0)
						out << std::get<0>(arg_value);
					else
					{
						if constexpr (std::is_invocable_v<arg_value_t>)
							out << std::get<1>(arg_value)();
						else
							out << std::get<1>(arg_value);
					}
				}
			}
		}

	private:
		void compile(const regex_t& arg_template, match_results_t& match)
		{
			auto b = std::cbegin(m_template);
			auto e = std::cend(m_template);

			// search arguments using arg_template
			while (std::regex_search(b, e, match, arg_template))
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

		template_t m_template;
		args_map_t m_args;
		parts_vector_t m_parts;
	};

	using string_template = basic_string_template<string_template_traits<char>>;
	using wstring_template = basic_string_template<string_template_traits<wchar_t>>;

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
	}
}

#endif
