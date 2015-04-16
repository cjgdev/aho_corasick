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
	auto check_emit = [](const ac::emit<char>& next, size_t expect_start, size_t expect_end, const std::string& expect_keyword) -> void {
		REQUIRE(expect_start == next.get_start());
		REQUIRE(expect_end == next.get_end());
		REQUIRE(expect_keyword == next.get_keyword());
	};
	auto check_wemit = [](const ac::emit<wchar_t>& next, size_t expect_start, size_t expect_end, const std::wstring& expect_keyword) -> void {
		REQUIRE(expect_start == next.get_start());
		REQUIRE(expect_end == next.get_end());
		REQUIRE(expect_keyword == next.get_keyword());
	};
	auto check_token = [](const ac::trie::token_type& next, const std::string& expect_fragment) -> void {
		REQUIRE(expect_fragment == next.get_fragment());
	};
	SECTION("keyword and text are the same") {
		ac::trie t;
		t.add_keyword(std::string("abc"));
		auto emits = t.parse_text(std::string("abc"));
		auto it = emits.begin();
		check_emit(*it, 0, 2, std::string("abc"));
	}
	SECTION("text is longer than the keyword") {
		ac::trie t;
		t.add_keyword(std::string("abc"));

		auto emits = t.parse_text(std::string(" abc"));

		auto it = emits.begin();
		check_emit(*it, 1, 3, std::string("abc"));
	}
	SECTION("various keywords one match") {
		ac::trie t;
		t.add_keyword(std::string("abc"));
		t.add_keyword(std::string("bcd"));
		t.add_keyword(std::string("cde"));

		auto emits = t.parse_text(std::string("bcd"));

		auto it = emits.begin();
		check_emit(*it, 0, 2, std::string("bcd"));
	}
	SECTION("ushers test") {
		ac::trie t;
		t.add_keyword(std::string("hers"));
		t.add_keyword(std::string("his"));
		t.add_keyword(std::string("she"));
		t.add_keyword(std::string("he"));

		auto emits = t.parse_text(std::string("ushers"));
		REQUIRE(3 == emits.size());

		auto it = emits.begin();
		check_emit(*it++, 2, 3, std::string("he"));
		check_emit(*it++, 1, 3, std::string("she"));
		check_emit(*it++, 2, 5, std::string("hers"));
	}
	SECTION("misleading test") {
		ac::trie t;
		t.add_keyword(std::string("hers"));

		auto emits = t.parse_text(std::string("h he her hers"));

		auto it = emits.begin();
		check_emit(*it++, 9, 12, std::string("hers"));
	}
	SECTION("recipes") {
		ac::trie t;
		t.add_keyword(std::string("veal"));
		t.add_keyword(std::string("cauliflower"));
		t.add_keyword(std::string("broccoli"));
		t.add_keyword(std::string("tomatoes"));

		auto emits = t.parse_text(std::string("2 cauliflowers, 3 tomatoes, 4 slices of veal, 100g broccoli"));
		REQUIRE(4 == emits.size());

		auto it = emits.begin();
		check_emit(*it++, 2, 12, std::string("cauliflower"));
		check_emit(*it++, 18, 25, std::string("tomatoes"));
		check_emit(*it++, 40, 43, std::string("veal"));
		check_emit(*it++, 51, 58, std::string("broccoli"));
	}
	SECTION("long and short overlapping match") {
		ac::trie t;
		t.add_keyword(std::string("he"));
		t.add_keyword(std::string("hehehehe"));

		auto emits = t.parse_text(std::string("hehehehehe"));
		REQUIRE(7 == emits.size());

		auto it = emits.begin();
		check_emit(*it++, 0, 1, std::string("he"));
		check_emit(*it++, 2, 3, std::string("he"));
		check_emit(*it++, 4, 5, std::string("he"));
		check_emit(*it++, 6, 7, std::string("he"));
		check_emit(*it++, 0, 7, std::string("hehehehe"));
		check_emit(*it++, 8, 9, std::string("he"));
		check_emit(*it++, 2, 9, std::string("hehehehe"));
	}
	SECTION("non overlapping") {
		ac::trie t;
		t.remove_overlaps();
		t.add_keyword(std::string("ab"));
		t.add_keyword(std::string("cba"));
		t.add_keyword(std::string("ababc"));

		auto emits = t.parse_text(std::string("ababcbab"));
		REQUIRE(3 == emits.size());

		auto it = emits.begin();
		check_emit(*it++, 0, 1, std::string("ab"));
		check_emit(*it++, 2, 3, std::string("ab"));
		check_emit(*it++, 4, 6, std::string("cba"));
	}
	SECTION("partial match") {
		ac::trie t;
		t.only_whole_words();
		t.add_keyword(std::string("sugar"));

		auto emits = t.parse_text(std::string("sugarcane sugarcane sugar canesugar"));
		REQUIRE(1 == emits.size());

		auto it = emits.begin();
		check_emit(*it, 20, 24, std::string("sugar"));
	}
	SECTION("tokenise tokens in sequence") {
		ac::trie t;
		t.add_keyword(std::string("Alpha"));
		t.add_keyword(std::string("Beta"));
		t.add_keyword(std::string("Gamma"));

		auto tokens = t.tokenise(std::string("Alpha Beta Gamma"));
		REQUIRE(5 == tokens.size());
	}
	SECTION("tokenise full sentence") {
		ac::trie t;
		t.only_whole_words();
		t.add_keyword(std::string("Alpha"));
		t.add_keyword(std::string("Beta"));
		t.add_keyword(std::string("Gamma"));

		ac::trie::string_type text("Hear: Alpha team first, Beta from the rear, Gamma in reserve");
		auto tokens = t.tokenise(text);
		REQUIRE(7 == tokens.size());

		auto it = tokens.begin();
		check_token(*it++, "Hear: ");
		check_token(*it++, "Alpha");
		check_token(*it++, " team first, ");
		check_token(*it++, "Beta");
		check_token(*it++, " from the rear, ");
		check_token(*it++, "Gamma");
		check_token(*it++, " in reserve");
	}
	SECTION("wtrie case insensitive") {
		ac::wtrie t;
		t.case_insensitive().only_whole_words();
		t.add_keyword(std::wstring(L"turning"));
		t.add_keyword(std::wstring(L"once"));
		t.add_keyword(std::wstring(L"again"));

		auto emits = t.parse_text(std::wstring(L"TurninG OnCe AgAiN"));
		REQUIRE(3 == emits.size());

		auto it = emits.begin();
		check_wemit(*it++, 0, 6, std::wstring(L"turning"));
		check_wemit(*it++, 8, 11, std::wstring(L"once"));
		check_wemit(*it++, 13, 17, std::wstring(L"again"));
	}
	SECTION("trie case insensitive") {
		ac::trie t;
		t.case_insensitive();
		t.add_keyword(std::string("turning"));
		t.add_keyword(std::string("once"));
		t.add_keyword(std::string("again"));

		auto emits = t.parse_text(std::string("TurninG OnCe AgAiN"));
		REQUIRE(3 == emits.size());

		auto it = emits.begin();
		check_emit(*it++, 0, 6, std::string("turning"));
		check_emit(*it++, 8, 11, std::string("once"));
		check_emit(*it++, 13, 17, std::string("again"));
	}
}