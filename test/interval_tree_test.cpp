/*
 * Copyright (C) 2018 Christopher Gilbert.
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
#include <vector>

namespace ac = aho_corasick;

TEST_CASE("interval_tree works as required", "[interval_tree]") {
	const auto assert_interval = [](const ac::interval& i, size_t expect_start, size_t expect_end) -> void {
		REQUIRE(expect_start == i.get_start());
		REQUIRE(expect_end == i.get_end());
	};
	SECTION("find overlaps") {
		const std::vector<ac::interval> intervals {
			ac::interval(0, 2),
			ac::interval(1, 3),
			ac::interval(2, 4),
			ac::interval(3, 5),
			ac::interval(4, 6),
			ac::interval(5, 7),
		};
		ac::interval_tree<ac::interval> tree(intervals);
		auto overlaps = tree.find_overlaps(ac::interval(1, 3));
		REQUIRE(3 == overlaps.size());
		auto it = overlaps.begin();
		assert_interval(*it++, 2, 4);
		assert_interval(*it++, 3, 5);
		assert_interval(*it++, 0, 2);
	}
	SECTION("remove overlaps") {
		const std::vector<ac::interval> intervals {
			ac::interval(0, 2),
			ac::interval(4, 5),
			ac::interval(2, 10),
			ac::interval(6, 13),
			ac::interval(9, 15),
			ac::interval(12, 16),
		};
		ac::interval_tree<ac::interval> tree(intervals);
		const auto result = tree.remove_overlaps(intervals);
		REQUIRE(2 == result.size());
	}
}