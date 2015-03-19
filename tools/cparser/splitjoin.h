#ifndef KATAHIROMZ_SPLITJOIN
#define KATAHIROMZ_SPLITJOIN 0x001  // Verison 0.0.1

////////////////////////////////////////////////////////////////////////////
// splitjoin.h --- katahiromz's C++ split and join
// by Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>.
// This file is public domain software (PDS).
////////////////////////////////////////////////////////////////////////////
// [HISTORY]
// 2015.03.18 v0.0.1 Created.
////////////////////////////////////////////////////////////////////////////
// string join(const string_container&, char);
// string join(const string_container&, const string&);
// void split(string_container&, const string&, char);
// void split(string_container&, const string&, const string&);
////////////////////////////////////////////////////////////////////////////

#ifndef __cplusplus
	#error Use C++ compiler. You lose.
#endif

#ifdef KATAHIROMZ_NAMESPACE
namespace KATAHIROMZ_NAMESPACE
{
#endif

    // string join(const string_container&, char);
	template <typename t_string_container>
	typename t_string_container::value_type
	join(const t_string_container& container,
	     typename t_string_container::value_type::value_type sep)
	{
		typename t_string_container::value_type result;
		typename t_string_container::const_iterator it = container.begin();
		typename t_string_container::const_iterator end = container.end();
		if (it != end) {
			result = *it;
			for (++it; it != end; ++it) {
				result += sep;
				result += *it;
			}
		}
		return result;
	}

    // string join(const string_container&, const string&);
	template <typename t_string_container>
	typename t_string_container::value_type
	join(const t_string_container& container,
	     const typename t_string_container::value_type& sep)
	{
		typename t_string_container::value_type result;
		typename t_string_container::const_iterator it = container.begin();
		typename t_string_container::const_iterator end = container.end();
		if (it != end) {
			result = *it;
			for (++it; it != end; ++it) {
				result += sep;
				result += *it;
			}
		}
		return result;
	}

	// void split(string_container&, const string&, char);
	template <typename t_string_container>
	void split(t_string_container& container,
	    const typename t_string_container::value_type& str,
	    typename t_string_container::value_type::value_type sep)
	{
		container.clear();
		std::size_t i = 0, j = str.find(sep);
#if (__cplusplus >= 201103L) // modern C++
		while (j != t_string_container::value_type::npos) {
			container.push_back(std::move(str.substr(i, j - i)));
			i = j + 1;
			j = str.find(sep, i);
		}
		container.push_back(std::move(str.substr(i, -1)));
#else // not modern C++
		while (j != t_string_container::value_type::npos) {
			container.push_back(str.substr(i, j - i));
			i = j + 1;
			j = str.find(sep, i);
		}
		container.push_back(str.substr(i, -1));
#endif // not modern C++
	}

    // void split(string_container&, const string&, const string&);
	template <typename t_string_container>
	void split(t_string_container& container,
	    const typename t_string_container::value_type& str,
	    const typename t_string_container::value_type& sep)
	{
		container.clear();
		std::size_t i = 0, j = str.find(sep);
#if __cplusplus >= 201103L // modern C++
		while (j != t_string_container::value_type::npos) {
			container.push_back(std::move(str.substr(i, j - i)));
			i = j + 1;
			j = str.find(sep, i);
		}
		container.push_back(std::move(str.substr(i, -1)));
#else // not modern C++
		while (j != t_string_container::value_type::npos) {
			container.push_back(str.substr(i, j - i));
			i = j + 1;
			j = str.find(sep, i);
		}
		container.push_back(str.substr(i, -1));
#endif // not modern C++
	}

#ifdef KATAHIROMZ_NAMESPACE
} // namespace KATAHIROMZ_NAMESPACE
#endif

#endif  // ndef KATAHIROMZ_SPLITJOIN
