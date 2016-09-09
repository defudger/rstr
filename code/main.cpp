/*--!>
This file is part of 'rstr', a simple random string generator written in C++.

Copyright 2016 outshined (outshined@riseup.net)
    (PGP: 0x8A80C12396A4836F82A93FA79CA3D0F7E8FBCED6)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
--------------------------------------------------------------------------<!--*/
#include <nebula/foundation/exception.h>
#include <nebula/foundation/format.h>
#include <nebula/foundation/cstring.h>
#include <nebula/foundation/filesystem.h>
#include <nebula/foundation/scope_exit.h>
#include <nebula/foundation/opts.h>
#include <nebula/foundation/qlog.h>
#include <nebula/foundation/random.h>
#include <nebula/foundation/utf.h>
#include <nebula/sex/sex.h>
#include <nebula/crypt/crypt.h>

namespace fnd = nebula::foundation;
namespace fmt = fnd::fmt;
namespace io = fnd::io;
namespace sex = nebula::sex;
namespace chrono = fnd::chrono;
namespace ncrypt = nebula::crypt;

//------------------------------------------------------------------------------
struct runtime_error : public virtual fnd::runtime_error {};
struct logic_error : public virtual fnd::logic_error {};

//------------------------------------------------------------------------------
const fnd::const_cstring program_author = "outshined (outshined@riseup.net)";
const fnd::const_cstring program_bugreport = PACKAGE_BUGREPORT;
const fnd::const_cstring program_url = PACKAGE_URL;
const fnd::const_cstring program_name = PACKAGE_NAME;
const fnd::const_cstring program_version = PACKAGE_VERSION;
const fnd::const_cstring program_description =
    "Non-Deterministic Random String Generator";
const fnd::const_cstring program_license = 
    "Copyright 2016 outshined (outshined@riseup.net)\n"
    "(PGP: 0x8A80C12396A4836F82A93FA79CA3D0F7E8FBCED6)\n\n"

    "This program is free software: you can redistribute it and/or modify\n"
    "it under the terms of the GNU Affero General Public License as\n"
    "published by the Free Software Foundation, either version 3 of the\n"
    "License, or (at your option) any later version.\n\n"

    "This program is distributed in the hope that it will be useful,\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    "GNU Affero General Public License for more details.\n\n"

    "You should have received a copy of the GNU Affero General Public License\n"
    "along with this program.  If not, see <http://www.gnu.org/licenses/>.";

//------------------------------------------------------------------------------
inline void print_version()
{
    fmt::fwrite(io::cout, program_name, ' ', program_version, fmt::endl);
}
//------------------------------------------------------------------------------
inline void print_license()
{
    fmt::fwrite(io::cout, program_license, fmt::endl);
}
//------------------------------------------------------------------------------
inline void print_help(const fnd::const_cstring argv0)
{
    fmt::fwrite(io::cout,
        program_description, fmt::endl,
        "Please send bugreports to ", program_bugreport, fmt::endl, fmt::endl,
        
"Usage: ", argv0, " [OPTION]...", fmt::endl, fmt::endl,

"Below is a list of available OPTIONs. Each OPTION can be indicated with both", fmt::endl,
"a single dash '-' or a double dash '--'. So '-help' is equivalent to '--help'.", fmt::endl, fmt::endl,
                
"-h --help      Show this message.", fmt::endl,
"--license      Show the license.", fmt::endl,
"--version      Show the version.", fmt::endl,

"-v --verbose   Verbose output.", fmt::endl,
"-l --length    The number of characters to generate. [32]", fmt::endl,
"-w --raw       Output a random byte stream of --length bytes", fmt::endl,
"               of raw data coming directly from the random number generator.", fmt::endl,
"-r --random    Quality of the random number generator. [crypt-strong]", fmt::endl,
"                   strong ... Generate strong random numbers quickly.", fmt::endl,
"                   very-strong ... Slow(!) but very strong.", fmt::endl,
"                       Drains the system's entropy pool!", fmt::endl,
"                   crypt-strong ... Like strong but using Nebula.Crypt.", fmt::endl,
"                   crypt-very-strong ... You get the idea.", fmt::endl,
"-n --numeric-range     Parse one or more 'ranges' of the form '(N M)'", fmt::endl,
"                       where N and M are decimal numbers such that N <= M.", fmt::endl,
"                       (N) is the same as (N N).", fmt::endl,
"                       Random values will be picked from the given ranges.", fmt::endl,
"-u --unicode-range     Same as --numeric-range but accepts '(A B)' where ", fmt::endl,
"                       A and B are unicode characters instead.", fmt::endl,
"                       For example (A Z) would be the range (65 90).", fmt::endl,
"                       (A) is the same as (A A).", fmt::endl,
"-A --AZ        Add the range (A Z): ABCDEFGHIJKLMNOPQRSTUVWXYZ", fmt::endl,
"-a --az        Add the range (a z): abcdefghijklmnopqrstuvwxyz", fmt::endl,
"-0 --09        Add the range (0 9): 0123456789", fmt::endl,
"-x             Add the range: !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~", fmt::endl,
"--show-ascii   Print a simple ASCII table and exit.", fmt::endl, fmt::endl,

"NOTE: This program works with UTF-8 strings only.", fmt::endl,
"NOTE: Adding the same range N times increases the likelihood of a character", fmt::endl,
"being picked from it by N times.", fmt::endl,
"NOTE: Characters consisting of more than one UTF-8 codeunit must be", fmt::endl,
"placed inside double-quotes like: (\"あ\" \"わ\").", fmt::endl, fmt::endl,

"The following examples are equivalent.", fmt::endl,
"    # ", argv0, " -n='(65 90)(97 122)(48 57)'", fmt::endl,
"    # ", argv0, " -u='(A Z)(a z)(0 9)'", fmt::endl,
"    # ", argv0, " -AZ -az -09", fmt::endl);
}
//------------------------------------------------------------------------------
inline void print_ascii_table()
{
    auto f = [&] (const size_t beg, const size_t end) {
        fmt::fwrite(io::cout, '(', beg, ' ', end, "): ");
        for(size_t i = beg; i <= end; ++i)
            io::put(io::cout, static_cast<char>(i));
        fmt::fwrite(io::cout, fmt::endl);
    };
    f(33, 47);
    f(48, 57);
    f(58, 64);
    f(65, 90);
    f(91, 96);
    f(97, 122);
    f(123, 126);
}

//------------------------------------------------------------------------------
fnd::intrusive_ptr<fnd::qlog::logger> gl;
//------------------------------------------------------------------------------
inline static auto init_log() noexcept
{
    namespace qlog = fnd::qlog;
    
    try
    {
        gl = new qlog::logger();
        gl->threshold(qlog::level::warning);
        gl->formatter(qlog::formatter::capture(
            [] (qlog::level lvl, fnd::string &s){
                io::msink<fnd::string> ss;
                fmt::fwrite(ss, 
                    "[ ", qlog::to_cstr(lvl), " ] ", s);
                s = fnd::move(ss.container());
            }));
        qlog::sink::init_console();
        gl->sink(qlog::sink::console());
        gl->error_handler(
            [&] (fnd::exception_ptr x) {
                try {
                    fmt::fwrite(io::cerr, 
                        "*** Log Error ***", fmt::endl,
                        fnd::diagnostic_information(fnd::current_exception()),
                        fmt::endl);
                } catch(...) {} // eat exception
            });
    }
    catch(...)
    {
        fmt::fwrite(io::cerr, 
            "*** Log Error ***", fmt::endl,
            fnd::diagnostic_information(fnd::current_exception()),
            fmt::endl);
        throw; // terminate
    }
}

//------------------------------------------------------------------------------
enum class errc
{
    unknown = -1,
    success = 0,
    expected_lbracket,
    expected_rbracket,
    expected_string,
    invalid_number,
    invalid_range,
    inverted_range,
    expected_single_character,
    invalid_utf32
};

//------------------------------------------------------------------------------
class app_error_category_impl : public fnd::system::error_category
{
public:
    app_error_category_impl() = default;

    inline const char *name() const noexcept {
        return "app";
    }
    inline fnd::string message(fnd::system::errval_t e) const
    {
        using fnd::system::errval_t;
        
        switch(e)
        {
        case static_cast<errval_t>(errc::success):
            return "Everything went fine.";
        case static_cast<errval_t>(errc::expected_lbracket):
            return "Missing left bracket.";
        case static_cast<errval_t>(errc::expected_rbracket):
            return "Missing right bracket.";
        case static_cast<errval_t>(errc::expected_string):
            return "Expected a String.";
        case static_cast<errval_t>(errc::invalid_number):
            return "Expected a number in the range [0 2097151].";
        case static_cast<errval_t>(errc::invalid_range):
            return "The range is not valid.";
        case static_cast<errval_t>(errc::inverted_range):
            return "The range is inverted.";
        case static_cast<errval_t>(errc::expected_single_character):
            return "Expected a single character.";
        case static_cast<errval_t>(errc::invalid_utf32):
            return "Invalid unicode value. Allowed range: (0 2097151)";
        default: return "unknown";
        }
    }

    inline fnd::system::error_condition default_error_condition(
        fnd::system::errval_t e) const noexcept
    {
        return fnd::system::error_condition(e, *this);
    }
};

//------------------------------------------------------------------------------
const fnd::system::error_category &app_error_category() noexcept {
    static app_error_category_impl r;
    return r;
}

//------------------------------------------------------------------------------
n_register_error_code_enum(::errc, ::app_error_category());
n_register_error_condition_enum(::errc, ::app_error_category());


//------------------------------------------------------------------------------
enum class random_mode
{
    strong,
    very_strong,
    crypt_strong,
    crypt_very_strong
};

//------------------------------------------------------------------------------
inline errc parse_numeric_range(
    const fnd::const_cstring s,
    fnd::vector<fnd::array<char32_t, 2>> &v)
{
    sex::iterative_parser<fnd::const_cstring> sexp(s);
    
    while(true)
    {
        fnd::optional<sex::token> tok_ = sexp();
        if(!tok_.valid())
        {
            return errc::unknown;
        }
        sex::token tok = tok_.get();
        if(sex::token_id::eof == tok.id())
            break;
        else if(sex::token_id::lbracket != tok.id())
        {
            return errc::expected_lbracket;
        }
        
        auto r = sexp.parse_string();
        if(!r.valid())
        {
            return errc::expected_string;
        }
        
        auto ir = fmt::to_integer<char32_t>(r.get(), 10, fnd::nothrow_tag());
        if(!ir.valid())
        {
            return errc::invalid_number;
        }
        const char32_t beg = ir.get();
        char32_t end = beg;
        
        tok_ = sexp();
        if(!tok_.valid())
        {
            return errc::unknown;
        }
        tok = tok_.get();
        if(sex::token_id::string == tok.id())
        {
            ir = fmt::to_integer<char32_t>(tok.value(), 10, fnd::nothrow_tag());
            if(!ir.valid())
            {
                return errc::invalid_number;
            }
            end = ir.get();
            
            tok_ = sexp();
            if(!tok_.valid())
            {
                return errc::unknown;
            }
            tok = tok_.get();
        }
        
        if(beg > end)
        {
            return errc::inverted_range;
        }
        if(end > 2097151) // unicode range
        {
            return errc::invalid_number;
        }
        
        if(sex::token_id::rbracket != tok.id())
        {
            return errc::expected_rbracket;
        }
        
        v.emplace_back(fnd::array<char32_t, 2>{beg, end + 1});
    }
    
    return errc::success;
}
//------------------------------------------------------------------------------
inline errc parse_unicode_range(
    const fnd::const_cstring s,
    fnd::vector<fnd::array<char32_t, 2>> &v)
{
    sex::iterative_parser<fnd::const_cstring> sexp(s);
    
    while(true)
    {
        fnd::optional<sex::token> tok_ = sexp();
        if(!tok_.valid())
        {
            return errc::unknown;
        }
        sex::token tok = tok_.get();
        if(sex::token_id::eof == tok.id())
            break;
        else if(sex::token_id::lbracket != tok.id())
        {
            return errc::expected_lbracket;
        }
        
        auto r = sexp.parse_any_string();
        if(!r.valid())
        {
            return errc::expected_string;
        }
        
        fnd::const_cstring c_ = r.get();
        if(1 != fnd::utf::count(c_))
        {
            return errc::expected_single_character;
        }
        const char32_t beg = fnd::utf::widen(c_);
        char32_t end = beg;
        
        tok_ = sexp();
        if(!tok_.valid())
        {
            return errc::unknown;
        }
        tok = tok_.get();
        if(sex::token_id::string == tok.id()
            || sex::token_id::quoted_string == tok.id()
            || sex::token_id::data == tok.id())
        {
            c_ = tok.value();
            if(1 != fnd::utf::count(c_))
            {
                return errc::expected_single_character;
            }
            end = fnd::utf::widen(c_);
            
            tok_ = sexp();
            if(!tok_.valid())
            {
                return errc::unknown;
            }
            tok = tok_.get();
        }
        
        if(beg > end)
        {
            return errc::inverted_range;
        }
        if(char32_t(-1) == end)
        {
            return errc::invalid_range;
        }
        
        if(sex::token_id::rbracket != tok.id())
        {
            return errc::expected_rbracket;
        }
        
        v.emplace_back(fnd::array<char32_t, 2>{beg, end + 1});
    }
    
    return errc::success;
}

//------------------------------------------------------------------------------
template <class Range, class Rnd, class Out>
inline void gen_from_ranges(
    const Range &ranges,
    const size_t length,
    Rnd &rnd,
    Out &&out)
{
    if(ranges.empty())
        n_throw(logic_error);
    
    fnd::vector<uint64_t> weights;
    weights.resize(ranges.size(), 0);
    
    for(size_t i = 0; i < ranges.size(); ++i)
    {
        auto &r = ranges[i];
        weights[i] = r[1] - r[0];
    }
    for(size_t i = 1; i < ranges.size(); ++i)
    {
        weights[i] = weights[i-1] + weights[i];
    }
    
    for(size_t i = 0; i < length; ++i)
    {
        uint64_t n = 0;
        io::read(rnd, reinterpret_cast<char *>(&n), sizeof(n));
        
        size_t indx = 0;
        {
            const uint64_t rnd_w = n % (weights.back()+1);
            auto fi = fnd::range::find_if(weights,
                [rnd_w] (const uint64_t x) {
                    return rnd_w <= x;
                });
            if(weights.end() == fi) // paranoid
                n_throw(logic_error);
            indx = fi - weights.begin();
        }
        const auto r = ranges[indx];
        
        out(fnd::utf::narrow(r[0] + (char32_t(n) % (r[1]-r[0]))));
    }
}

//------------------------------------------------------------------------------
template <class Rnd>
inline void dump_raw(size_t length, Rnd &rnd)
{
    fnd::array<char, 4096> buf;
    for(size_t i = 0; i < length; )
    {
        const size_t delta = 4096 <= length - i ? 4096 : length - i;
        io::read(rnd, buf.data(), delta);
        io::write(io::cout, buf.data(), delta);
        i += delta;
    }
}
//------------------------------------------------------------------------------
inline void init_crypt()
{
    ncrypt::config cfg;
    cfg.secure_random_pool = true;
    ncrypt::init(cfg);
}

//------------------------------------------------------------------------------
int main(int argc, char **argv)
{
    init_log();
    
    try {
        fnd::vector<fnd::array<char32_t, 2>> ranges;
        int quit = -1;
        size_t length = 32;
        random_mode rndmode = random_mode::crypt_strong;
        bool raw = false;
        
        for(int i = 1; i < argc; ++i) {
            const fnd::const_cstring s(argv[i]);
            if(s.end() != fnd::get<0>(fnd::utf::validate(s))) {
                gl->error("Command line must be UTF-8 encoded.");
                return EXIT_FAILURE;
            }
        }
        
        auto opts_ctx = fnd::opts::context(
            [&] (fnd::const_cstring id, fnd::const_cstring val, const size_t i)
            {
                quit = EXIT_FAILURE;
                gl->error("Unknown parameter: '-", id, "'.");
            },
            [&] (fnd::const_cstring val, const size_t i) {
                if(i > 0) {
                    quit = EXIT_FAILURE;
                    gl->error("Arguments without - or -- are not allowed.");
                    return false;
                }
                return true;
            },
            
            fnd::opts::argument(
                [&] (fnd::const_cstring id, fnd::const_cstring val, size_t i) {
                    if(!val.empty())
                        gl->warning("Value ignored. '-", id, "' is a flag.");
                    print_help(argv[0]);
                    quit = EXIT_SUCCESS;
                    return false;
                },
                "help", "h"),
            fnd::opts::argument(
                [&] (fnd::const_cstring id, fnd::const_cstring val, size_t i) {
                    if(!val.empty())
                        gl->warning("Value ignored. '-", id, "' is a flag.");
                    print_license();
                    quit = EXIT_SUCCESS;
                    return false;
                },
                "license"),
            fnd::opts::argument(
                [&] (fnd::const_cstring id, fnd::const_cstring val, size_t i) {
                    if(!val.empty())
                        gl->warning("Value ignored. '-", id, "' is a flag.");
                    print_version();
                    quit = EXIT_SUCCESS;
                    return false;
                },
                "version"),
            
            fnd::opts::argument(
                [&] (fnd::const_cstring id, fnd::const_cstring val, size_t i) {
                    if(val.empty())
                    {
                        quit = EXIT_FAILURE;
                        gl->error("Missing value. '-", id, "=???'.");
                        return false;
                    }
                    if(val == "strong") {
                        rndmode = random_mode::strong;
                    }
                    else if(val == "very-strong") {
                        rndmode = random_mode::very_strong;
                    }
                    else if(val == "crypt-strong") {
                        rndmode = random_mode::crypt_strong;
                    }
                    else if(val == "crypt-very-strong") {
                        rndmode = random_mode::crypt_very_strong;
                    }
                    else
                    {
                        quit = EXIT_FAILURE;
                        gl->error("Invalid random mode. '-", id, "=#ERROR'.");
                        return false;
                    }
                    return true;
                },
                "random", "r"),
            
            fnd::opts::argument(
                [&] (fnd::const_cstring id, fnd::const_cstring val, size_t i) {
                    if(val.empty())
                    {
                        quit = EXIT_FAILURE;
                        gl->error("Missing value. '-", id, "=???'.");
                        return false;
                    }
                    fnd::optional<size_t> x = fmt::to_integer<size_t>(
                        val, 10, fnd::nothrow_tag());
                    if(!x.valid())
                    {
                        quit = EXIT_FAILURE;
                        gl->error("Expected positive number. '-",
                            id, "=#ERROR'");
                        return false;
                    }
                    length = x.get();
                    return true;
                },
                "length", "l"),
            fnd::opts::argument(
                [&] (fnd::const_cstring id, fnd::const_cstring val, size_t i) {
                    if(!val.empty())
                        gl->warning("Value ignored. '-", id, "' is a flag.");
                    raw = true;
                    return true;
                },
                "raw", "w"),
            fnd::opts::argument(
                [&] (fnd::const_cstring id, fnd::const_cstring val, size_t i) {
                    if(val.empty())
                    {
                        quit = EXIT_FAILURE;
                        gl->error("Missing value. '-", id, "=???'.");
                        return false;
                    }
                    const errc err = parse_numeric_range(val, ranges);
                    if(errc::success != err) {
                        quit = EXIT_FAILURE;
                        gl->error("'-", id, "=#ERROR'; ",
                            fnd::system::error_code(err).message());
                        return false;
                    }
                    return true;
                },
                "numeric-range", "n"),
            fnd::opts::argument(
                [&] (fnd::const_cstring id, fnd::const_cstring val, size_t i) {
                    if(val.empty())
                    {
                        quit = EXIT_FAILURE;
                        gl->error("Missing value. '-", id, "=???'.");
                        return false;
                    }
                    const errc err = parse_unicode_range(val, ranges);
                    if(errc::success != err) {
                        quit = EXIT_FAILURE;
                        gl->error("'-", id, "=#ERROR'; ",
                            fnd::system::error_code(err).message());
                        return false;
                    }
                    return true;
                },
                "unicode-range", "u"),
            fnd::opts::argument(
                [&] (fnd::const_cstring id, fnd::const_cstring val, size_t i) {
                    if(!val.empty())
                        gl->warning("Value ignored. '-", id, "' is a flag.");
                    ranges.emplace_back(fnd::array<char32_t, 2>{'A', 'Z'+1});
                    return true;
                },
                "AZ", "A"),
            fnd::opts::argument(
                [&] (fnd::const_cstring id, fnd::const_cstring val, size_t i) {
                    if(!val.empty())
                        gl->warning("Value ignored. '-", id, "' is a flag.");
                    ranges.emplace_back(fnd::array<char32_t, 2>{'a', 'z'+1});
                    return true;
                },
                "az", "a"),
            fnd::opts::argument(
                [&] (fnd::const_cstring id, fnd::const_cstring val, size_t i) {
                    if(!val.empty())
                        gl->warning("Value ignored. '-", id, "' is a flag.");
                    ranges.emplace_back(fnd::array<char32_t, 2>{'0', '9'+1});
                    return true;
                },
                "09", "0"),
            fnd::opts::argument(
                [&] (fnd::const_cstring id, fnd::const_cstring val, size_t i) {
                    if(!val.empty())
                        gl->warning("Value ignored. '-", id, "' is a flag.");
                    ranges.emplace_back(fnd::array<char32_t, 2>{33, 48});
                    ranges.emplace_back(fnd::array<char32_t, 2>{58, 65});
                    ranges.emplace_back(fnd::array<char32_t, 2>{91, 97});
                    ranges.emplace_back(fnd::array<char32_t, 2>{123, 126});
                    return true;
                },
                "special", "x"),
            
            fnd::opts::argument(
                [&] (fnd::const_cstring id, fnd::const_cstring val, size_t i) {
                    if(!val.empty())
                        gl->warning("Value ignored. '-", id, "' is a flag.");
                    if(1 != i)
                        gl->warning("'-", id,
                            "' should be the first argument!");
                    gl->threshold(fnd::qlog::level::debug);
                    return true;
                },
                "verbose", "v"),
            
             fnd::opts::argument(
                [&] (fnd::const_cstring id, fnd::const_cstring val, size_t i) {
                    if(!val.empty())
                        gl->warning("Value ignored. '-", id, "' is a flag.");
                    print_ascii_table();
                    quit = EXIT_SUCCESS;
                    return false;
                },
                "show-ascii"));
        
        fnd::opts::parse_command_line(opts_ctx, argc, argv);
        
        if(quit != -1)
            return quit;
        
        if(raw)
        {
            switch(rndmode)
            {
            case random_mode::strong:
                {
                    gl->info("Using 'strong' RNG.");
                    fnd::random::pseudo_random_device rnd;
                    dump_raw(length, rnd);
                }
                break;
            case random_mode::very_strong:
                {
                    gl->info("Using 'very-strong' RNG.");
                    fnd::random::random_device rnd;
                    dump_raw(length, rnd);
                }
                break;
            case random_mode::crypt_strong:
                {
                    gl->info("Using 'crypt-strong' RNG.");
                    
                    init_crypt();
                    
                    ncrypt::pseudo_random_device rnd;
                    dump_raw(length, rnd);
                    
                    n_scope_exit() {
                        ncrypt::shutdown();
                    };
                }
                break;
            case random_mode::crypt_very_strong:
                {
                    gl->info("Using 'crypt-very-strong' RNG.");
                    
                    init_crypt();
                    
                    ncrypt::random_device rnd;
                    dump_raw(length, rnd);
                    
                    n_scope_exit() {
                        ncrypt::shutdown();
                    };
                }
                break;
            default:
                n_throw(logic_error);
            }
        }
        else
        {
            auto writer = [&] (const fnd::const_cstring s) {
                io::write(io::cout, s.data(), s.size());
            };
            
            if(ranges.empty())
            {
                gl->error("No input set specified, try -AZ -az -09.");
                return EXIT_FAILURE;
            }
            
            switch(rndmode)
            {
            case random_mode::strong:
                {
                    gl->info("Using 'strong' RNG.");
                    fnd::random::pseudo_random_device rnd;
                    gen_from_ranges(ranges, length, rnd, writer);
                }
                break;
            case random_mode::very_strong:
                {
                    gl->info("Using 'very-strong' RNG.");
                    fnd::random::random_device rnd;
                    gen_from_ranges(ranges, length, rnd, writer);
                }
                break;
            case random_mode::crypt_strong:
                {
                    gl->info("Using 'crypt-strong' RNG.");
                    
                    init_crypt();
                    
                    ncrypt::pseudo_random_device rnd;
                    gen_from_ranges(ranges, length, rnd, writer);
                    
                    n_scope_exit() {
                        ncrypt::shutdown();
                    };
                }
                break;
            case random_mode::crypt_very_strong:
                {
                    gl->info("Using 'crypt-very-strong' RNG.");
                    
                    init_crypt();
                    
                    ncrypt::random_device rnd;
                    gen_from_ranges(ranges, length, rnd, writer);
                    
                    n_scope_exit() {
                        ncrypt::shutdown();
                    };
                }
                break;
            default:
                n_throw(logic_error);
            }
            
            fmt::fwrite(io::cout, fmt::endl);
        }
    }
    catch(const fnd::exception &e)
    {
        const fnd::ei_msg *msg = fnd::get_error_info<fnd::ei_msg>(e);
        const fnd::ei_msg_c *msg_c = fnd::get_error_info<fnd::ei_msg_c>(e);
        
        gl->fatal("Internal Error");
        if(msg)
            gl->fatal(msg->value());
        if(msg_c)
            gl->fatal(msg_c->value());
        gl->debug(fnd::diagnostic_information(e));
        return EXIT_FAILURE;
    }
    catch(...)
    {
        gl->fatal("Internal Error");
        gl->debug(fnd::diagnostic_information(fnd::current_exception()));
        
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
