/*
 * Copyright (C) 2015 Christopher Gilbert.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define CATCH_CONFIG_MAIN
#include "../test/catch.hpp"

#include "aho_corasick/aho_corasick.hpp"
#include <string>

namespace ac = aho_corasick;

TEST_CASE("trie works as required", "[trie]") {
	auto check_emit = [](ac::trie::emit_ptr_type next, size_t expect_start, size_t expect_end, const std::string& expect_keyword) -> void {
		REQUIRE(expect_start == next->get_start());
		REQUIRE(expect_end == next->get_end());
		REQUIRE(expect_keyword == next->get_keyword());
	};
	auto check_wemit = [](ac::wtrie::emit_ptr_type next, size_t expect_start, size_t expect_end, const std::wstring& expect_keyword) -> void {
		REQUIRE(expect_start == next->get_start());
		REQUIRE(expect_end == next->get_end());
		REQUIRE(expect_keyword == next->get_keyword());
	};
	auto check_token = [](ac::trie::token_ptr_type next, const std::string& expect_fragment) -> void {
		REQUIRE(expect_fragment == next->get_fragment());
	};
	SECTION("keyword and text are the same") {
		ac::trie t;
		ac::trie::string_type str("abc");
		t.add_keyword(str);
		auto emits = t.parse_text(str);
		auto it = emits.begin();
		check_emit(*it, 0, 2, str);
	}
	SECTION("text is longer than the keyword") {
		ac::trie t;

		ac::trie::string_type keyword("abc");
		t.add_keyword(keyword);

		ac::trie::string_type text(" abc");
		auto emits = t.parse_text(text);

		auto it = emits.begin();
		check_emit(*it, 1, 3, keyword);
	}
	SECTION("various keywords one match") {
		ac::trie t;

		ac::trie::string_type keyword1("abc");
		ac::trie::string_type keyword2("bcd");
		ac::trie::string_type keyword3("cde");
		t.add_keyword(keyword1);
		t.add_keyword(keyword2);
		t.add_keyword(keyword3);

		ac::trie::string_type text("bcd");
		auto emits = t.parse_text(text);

		auto it = emits.begin();
		check_emit(*it, 0, 2, keyword2);
	}
	SECTION("ushers test") {
		ac::trie t;

		ac::trie::string_type keyword1("hers");
		ac::trie::string_type keyword2("his");
		ac::trie::string_type keyword3("she");
		ac::trie::string_type keyword4("he");
		t.add_keyword(keyword1);
		t.add_keyword(keyword2);
		t.add_keyword(keyword3);
		t.add_keyword(keyword4);

		ac::trie::string_type text("ushers");
		auto emits = t.parse_text(text);
		REQUIRE(3 == emits.size());

		auto it = emits.begin();
		check_emit(*it++, 2, 3, keyword4);
		check_emit(*it++, 1, 3, keyword3);
		check_emit(*it++, 2, 5, keyword1);
	}
	SECTION("misleading test") {
		ac::trie t;

		ac::trie::string_type keyword("hers");
		t.add_keyword(keyword);

		ac::trie::string_type text("h he her hers");
		auto emits = t.parse_text(text);

		auto it = emits.begin();
		check_emit(*it++, 9, 12, keyword);
	}
	SECTION("recipes") {
		ac::trie t;

		ac::trie::string_type keyword1("veal");
		ac::trie::string_type keyword2("cauliflower");
		ac::trie::string_type keyword3("broccoli");
		ac::trie::string_type keyword4("tomatoes");
		t.add_keyword(keyword1);
		t.add_keyword(keyword2);
		t.add_keyword(keyword3);
		t.add_keyword(keyword4);

		ac::trie::string_type text("2 cauliflowers, 3 tomatoes, 4 slices of veal, 100g broccoli");
		auto emits = t.parse_text(text);
		REQUIRE(4 == emits.size());

		auto it = emits.begin();
		check_emit(*it++, 2, 12, keyword2);
		check_emit(*it++, 18, 25, keyword4);
		check_emit(*it++, 40, 43, keyword1);
		check_emit(*it++, 51, 58, keyword3);
	}
	SECTION("long and short overlapping match") {
		ac::trie t;

		ac::trie::string_type keyword1("he");
		ac::trie::string_type keyword2("hehehehe");
		t.add_keyword(keyword1);
		t.add_keyword(keyword2);

		ac::trie::string_type text("hehehehehe");
		auto emits = t.parse_text(text);
		REQUIRE(7 == emits.size());

		auto it = emits.begin();
		check_emit(*it++, 0, 1, keyword1);
		check_emit(*it++, 2, 3, keyword1);
		check_emit(*it++, 4, 5, keyword1);
		check_emit(*it++, 6, 7, keyword1);
		check_emit(*it++, 0, 7, keyword2);
		check_emit(*it++, 8, 9, keyword1);
		check_emit(*it++, 2, 9, keyword2);
	}
	//SECTION("non overlapping") {
	//	ac::trie t;
	//	t.remove_overlaps();

	//	ac::trie::string_type keyword1("ab");
	//	ac::trie::string_type keyword2("cba");
	//	ac::trie::string_type keyword3("ababc");
	//	t.add_keyword(keyword1);
	//	t.add_keyword(keyword2);
	//	t.add_keyword(keyword3);

	//	ac::trie::string_type text("ababcbab");
	//	auto emits = t.parse_text(text);
	//	REQUIRE(2 == emits.size());

	//	auto it = emits.begin();
	//	check_emit(*it++, 0, 4, keyword3);
	//	check_emit(*it++, 6, 7, keyword1);
	//}
	SECTION("partial match") {
		ac::trie t;
		t.only_whole_words();

		ac::trie::string_type keyword("sugar");
		t.add_keyword(keyword);

		ac::trie::string_type text("sugarcane sugarcane sugar canesugar");
		auto emits = t.parse_text(text);
		REQUIRE(1 == emits.size());

		auto it = emits.begin();
		check_emit(*it, 20, 24, keyword);
	}
	SECTION("tokenise tokens in sequence") {
		ac::trie t;

		ac::trie::string_type keyword1("Alpha");
		ac::trie::string_type keyword2("Beta");
		ac::trie::string_type keyword3("Gamma");
		t.add_keyword(keyword1);
		t.add_keyword(keyword2);
		t.add_keyword(keyword3);

		ac::trie::string_type text("Alpha Beta Gamma");
		auto tokens = t.tokenise(text);
		REQUIRE(5 == tokens.size());
	}
	SECTION("tokenise full sentence") {
		ac::trie t;
		t.only_whole_words();

		ac::trie::string_type keyword1("Alpha");
		ac::trie::string_type keyword2("Beta");
		ac::trie::string_type keyword3("Gamma");
		t.add_keyword(keyword1);
		t.add_keyword(keyword2);
		t.add_keyword(keyword3);

		ac::trie::string_type text("Hear: Alpha team first, Beta from the rear, Gamma in reserve");
		auto tokens = t.tokenise(text);
		REQUIRE(7 == tokens.size());

		auto it = tokens.begin();
		check_token(*it++, "Hear: ");
		//check_token(*it++, "Alpha");
		//check_token(*it++, " team first, ");
		//check_token(*it++, "Beta");
		//check_token(*it++, " from the rear, ");
		//check_token(*it++, "Gamma");
		//check_token(*it++, " in reserve");
	}
	SECTION("wtrie case insensitive") {
		ac::wtrie t;
		t.case_insensitive().only_whole_words();

		ac::wtrie::string_type keyword1(L"turning");
		ac::wtrie::string_type keyword2(L"once");
		ac::wtrie::string_type keyword3(L"again");
		t.add_keyword(keyword1);
		t.add_keyword(keyword2);
		t.add_keyword(keyword3);

		ac::wtrie::string_type text(L"TurninG OnCe AgAiN");
		auto emits = t.parse_text(text);
		REQUIRE(3 == emits.size());

		auto it = emits.begin();
		check_wemit(*it++, 0, 6, keyword1);
		check_wemit(*it++, 8, 11, keyword2);
		check_wemit(*it++, 13, 17, keyword3);
	}
	SECTION("trie case insensitive") {
		ac::trie t;
		t.case_insensitive();

		ac::trie::string_type keyword1("turning");
		ac::trie::string_type keyword2("once");
		ac::trie::string_type keyword3("again");
		t.add_keyword(keyword1);
		t.add_keyword(keyword2);
		t.add_keyword(keyword3);

		ac::trie::string_type text("TurninG OnCe AgAiN");
		auto emits = t.parse_text(text);
		REQUIRE(3 == emits.size());

		auto it = emits.begin();
		check_emit(*it++, 0, 6, keyword1);
		check_emit(*it++, 8, 11, keyword2);
		check_emit(*it++, 13, 17, keyword3);
	}
}