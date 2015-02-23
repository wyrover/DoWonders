#ifndef CSCANNER_H_
#define CSCANNER_H_

namespace cparser
{
    //
    // Packing
    //
    class Packing {
    public:
        Packing() : m_pack(8) { }
        Packing(int packing) : m_pack(packing) { }

        void push(int pack) {
            assert(pack == 1 || pack == 2 || pack == 4 || pack == 8 || pack == 16);
            m_packs.push_back(m_pack);
            m_idents.emplace_back("");
            m_pack = pack;
        }

        void pop(int pack) {
            assert(pack == 1 || pack == 2 || pack == 4 || pack == 8 || pack == 16);
            pop();
            m_pack = pack;
        }

        void pop() {
            assert(m_packs.size());
            m_pack = m_packs.back();
            m_packs.pop_back();
            m_idents.pop_back();
        }

        void push(const std::string& ident) {
            m_packs.push_back(m_pack);
            m_idents.emplace_back(ident);
        }

        bool pop(const std::string& ident) {
            while (m_packs.size()) {
                assert(m_idents.size());
                if (m_idents.back() == ident) {
                    pop();
                    return true;
                }
                pop();
            }
            set(m_default_packing);
            return false;
        }

        int  get() const        { return m_pack; }
        void set(int pack)      { m_pack = pack; }
        operator int() const    { return get(); }

              int& default_packing()        { return m_default_packing; }
        const int& default_packing() const  { return m_default_packing; }

    protected:
        int                         m_pack;
        std::deque<int>             m_packs;
        std::deque<std::string>     m_idents;
        int                         m_default_packing;
    }; // Packing

    //
    // ScannerBase
    //
    class ScannerBase {
    public:
        ScannerBase(scanner_iterator begin, scanner_iterator end) :
            m_current(begin), m_end(end) { }

        char getch() {
            char c;
            if (is_eof()) {
                c = -1;
            } else {
                c = *m_current++;
            }
            return c;
        } // getch

        bool is_eof() const {
            return m_current == m_end || *m_current == -1;
        }

              Packing& packing()       { return m_packing; }
        const Packing& packing() const { return m_packing; }

              CR_Location& location()       { return m_location; }
        const CR_Location& location() const { return m_location; }

              int& default_packing()        { return packing().default_packing(); }
        const int& default_packing() const  { return packing().default_packing(); }

    protected:
        scanner_iterator    m_current;
        scanner_iterator    m_end;
        Packing             m_packing;
        CR_Location         m_location;
    };

    //
    // Scanner
    //
    class Scanner {
    public:
        Scanner(shared_ptr<CR_ErrorInfo> error_info, bool is_64bit = false) :
            m_error_info(error_info), m_is_64bit(is_64bit),
            m_in_c_comment(false), m_in_cxx_comment(false)
        {
            init_symbol_table();
        }

        bool is_in_block_comment() const { return m_in_c_comment; }
        bool is_in_line_comment() const { return m_in_cxx_comment; }

        void scan(token_container& infos,
                  scanner_iterator begin, scanner_iterator end);

        void show_tokens(token_iterator begin, token_iterator end) const {
            for (auto it = begin; it != end; ++it) {
                std::printf("%s ", token_to_string(*it).data());
            }
        }

        std::string token_to_string(const token_type& info) const;

        void newline(token_container& infos) {
            infos.emplace_back(T_NEWLINE, "\\n");
        }

        void skip_block_comment(ScannerBase& base, token_container& infos);
        void skip_line_comment(ScannerBase& base, token_container& infos);

        std::string guts_escape_sequence(str_iterator& it, str_iterator end) const;
        std::string guts_string(str_iterator& it, str_iterator end) const;
        std::string guts_char(str_iterator& it, str_iterator end) const;
        std::string guts_hex(str_iterator& it, str_iterator end) const;
        std::string guts_digits(str_iterator& it, str_iterator end) const;
        std::string guts_octal(str_iterator& it, str_iterator end) const;
        std::string guts_floating(str_iterator& it, str_iterator end) const;
        std::string guts_indentifier(str_iterator& it, str_iterator end) const;

        std::string guts_integer_suffix(
            str_iterator& it, str_iterator end, CR_TypeFlags& flags) const;
        std::string guts_floating_suffix(
            str_iterator& it, str_iterator end, CR_TypeFlags& flags) const;
        std::string guts_exponent(str_iterator& it, str_iterator end) const;

        bool lexeme(str_iterator& it, str_iterator end, const std::string& str);

        std::string get_line(ScannerBase& base);
        bool get_tokens(ScannerBase& base, token_container& infos);
        bool token_pattern_match(
            ScannerBase& base, token_iterator it, token_iterator end,
            const std::vector<Token>& tokens) const;

    protected:
        //
        // resynth
        //
        void resynth(ScannerBase& base, token_container& c);

        void resynth1(ScannerBase& base, token_container& c);
        void resynth2(token_container& c);
        void resynth3(token_iterator begin, token_iterator end);

        token_iterator
        resynth_typedef(token_iterator begin, token_iterator end);

        token_iterator resynth_parameter_list(
            token_iterator begin, token_iterator end);

        token_iterator skip_gnu_attribute(
            token_iterator begin, token_iterator end);

        token_iterator skip_asm_for_fn_decl(
            token_iterator begin, token_iterator end);

        void resynth4(token_container& c);
        void resynth5(token_iterator begin, token_iterator end);

        Token parse_identifier(const std::string& text) const;

        CR_ErrorInfo::Type
        parse_pragma(ScannerBase& base, token_iterator it, token_iterator end);

        CR_ErrorInfo::Type
        parse_pack(ScannerBase& base, token_iterator it, token_iterator end);

    protected:
        shared_ptr<CR_ErrorInfo>    m_error_info;
        bool                        m_is_64bit;
        bool                        m_in_c_comment;
        bool                        m_in_cxx_comment;
        std::unordered_map<std::string, Token> m_symbol_table;

        std::set<std::string>       m_type_names;

        //
        // pragma
        //
        void lib(const std::string str) { }

        void linker(const std::string str) { }

        void add_notice(const CR_Location& location, const std::string& text) {
            m_error_info.get()->add_notice(location, text);
        }
        void add_warning(const CR_Location& location, const std::string& text) {
            m_error_info.get()->add_warning(location, text);
        }
        void add_error(const CR_Location& location, const std::string& text) {
            m_error_info.get()->add_error(location, text);
        }
        void add_message(CR_ErrorInfo::Type type, const CR_Location& location,
                         const std::string& text)
        {
            m_error_info.get()->add_message(type, location, text);
        }

        void message(const CR_Location& location, const std::string& text) {
            add_notice(location, text);
        }

        void init_symbol_table();

    private:
        // NOTE: Scanner is not copyable.
        Scanner(const Scanner&) = delete;
        Scanner& operator=(const Scanner&) = delete;
    }; // class Scanner
} // namespace cparser

#endif  // ndef CSCANNER_H_
