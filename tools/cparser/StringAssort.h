#ifndef KATAHIROMZ_STRINGASSORT
#define KATAHIROMZ_STRINGASSORT 0x000  // Verison 0.0.0

////////////////////////////////////////////////////////////////////////////
// StringAssort.h --- katahiromz's C++ string function assortment
// by Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>.
// This file is public domain software (PDS).
////////////////////////////////////////////////////////////////////////////

#ifndef __cplusplus
    #error Use C++ compiler. You lose.
#endif

#include <string>

namespace katahiromz
{
    // string join_by_char(const string_container&, char);
    template <typename t_string_container,
              typename t_string = typename t_string_container::value_type>
    typename t_string_container::value_type
    join_by_char(const t_string_container& container,
                 typename t_string::value_type sep)
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

    // void split_by_char(string_container&, const string&, char);
    template <typename t_string_container, 
              typename t_string = typename t_string_container::value_type>
    void split_by_char(t_string_container& container,
        const typename t_string_container::value_type& str,
        typename t_string::value_type sep)
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
            container.emplace_back(std::move(str.substr(i, j - i)));
            i = j + 1;
            j = str.find(sep, i);
        }
        container.emplace_back(std::move(str.substr(i, -1)));
#else // not modern C++
        while (j != t_string_container::value_type::npos) {
            container.push_back(str.substr(i, j - i));
            i = j + 1;
            j = str.find(sep, i);
        }
        container.push_back(str.substr(i, -1));
#endif // not modern C++
    }

    // void tokenize(string_container&, const string&, const string&);
    template <typename t_string_container>
    void tokenize(t_string_container& container,
        const typename t_string_container::value_type& str,
        const typename t_string_container::value_type& seps)
    {
        container.clear();
        std::size_t i, j;
        for (i = 0; ;) {
            j = str.find_first_not_of(seps, i);
            if (j == t_string_container::value_type::npos) {
                return;
            }
            i = str.find_first_of(seps, j);
#if __cplusplus >= 201103L // modern C++
            if (i == t_string_container::value_type::npos) {
                container.emplace_back(std::move(str.substr(j)));
                return;
            }
            container.emplace_back(std::move(str.substr(j, i - j)));
#else // not modern C++
            if (i == t_string_container::value_type::npos) {
                container.push_back(str.substr(j));
                return;
            }
            container.push_back(str.substr(j, i - j));
#endif // not modern C++
        }
    }

    template <typename t_string, typename t_char = typename t_string::value_type>
    inline bool replace_char(t_string& str, t_char from, t_char to) {
        bool ret = false;
#if __cplusplus >= 201103L // modern C++
        for (auto& ch : str) {
            if (ch == from) {
                ch = to;
                ret = true;
            }
        }
#else
        const size_t siz = str.size();
        for (size_t i = 0; i < siz; ++i) {
            if (str[i] == from) {
                str[i] = to;
                ret = true;
            }
        }
#endif
        return ret;
    }

    template <typename t_string>
    bool replace_string(t_string& str, const t_string& from, const t_string& to) {
        bool ret = false;
        size_t i = 0;
        for (;;) {
            i = str.find(from, i);
            if (i == t_string::npos)
                break;

            str.replace(i, from.size(), to);
            i += to.size();
            ret = true;
        }
        return ret;
    }

	template <typename t_string, typename t_char = typename t_string::value_type>
	bool replace_string(t_string& str, const t_char *from, const t_char *to) {
		bool ret = false;
		size_t i = 0, from_len = strlen(from), to_len = strlen(to);
		for (;;) {
			i = str.find(from, i);
			if (i == t_string::npos)
				break;

			str.replace(i, from_len, to);
			i += to_len;
			ret = true;
		}
		return ret;
	}

    template <typename t_string>
    inline void trim(t_string& str, const t_string& spaces) {
        size_t i = str.find_first_not_of(spaces);
        size_t j = str.find_last_not_of(spaces);
        if (i != t_string::npos) {
            if (j != t_string::npos) {
                str = str.substr(i, j - i + 1);
            } else {
                str = str.substr(i);
            }
        } else {
            if (j != t_string::npos) {
                str = str.substr(0, j + 1);
            }
        }
    }

    inline void trim(std::string& str)  { trim<std::string>(str, " \t\n\r\f\v"); }
    inline void trim(std::wstring& str) { trim<std::wstring>(str, L" \t\n\r\f\v"); }

    template <typename t_string>
    inline void chomp(t_string& str) {
        const typename t_string::value_type CR = '\r';
        const typename t_string::value_type LF = '\n';
        const size_t siz = str.size();
        if (siz && str[siz - 1] == LF) {
            if (siz >= 2 && str[siz - 2] == CR) {
                str.resize(siz - 2);
            } else {
                str.resize(siz - 1);
            }
        }
    }

} // namespace katahiromz

#endif  // ndef KATAHIROMZ_STRINGASSORT
