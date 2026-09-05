# Third-party notices

MathSDK statically links the following dependencies. Their public symbols are
hidden (`CXX_VISIBILITY_PRESET hidden`, `--exclude-libs,ALL` on ELF targets),
so no SymEngine or Boost API crosses the MathSDK ABI boundary — but the
compiled binary still contains their object code, so their license notices
are reproduced here.

## SymEngine `v0.14.0`

MIT License. Copyright (c) 2013-2017 SymEngine Development Team.

SymEngine additionally vendors a small number of files under compatible
permissive licenses (BSD-3-Clause and MIT), listed in full in its own
`LICENSE` file. The Bison-generated parser
(`symengine/parser/parser.tab.{hh,cc}`) is produced under Bison's special
exception, which places the generated parser output under SymEngine's own
license terms rather than the GPL.

Full text: https://github.com/symengine/symengine/blob/v0.14.0/LICENSE

## Boost `1.80.0`

Boost Software License - Version 1.0. This license does not require
attribution for object code distributed in compiled form; it is reproduced
here for completeness.

Full text: https://www.boost.org/LICENSE_1_0.txt
