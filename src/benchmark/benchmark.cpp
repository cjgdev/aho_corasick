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

#include "aho_corasick/aho_corasick.hpp"
#include <chrono>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace ac = aho_corasick;
using trie = ac::trie;

using namespace std;

string gen_str(size_t len) {
	static const char alphanum[] =
//			"0123456789"
//			"!@#$%^&*"
//			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz";

	string str;
	for (int i = 0; i < len; ++i) {
		str.append(1, alphanum[rand() % (sizeof(alphanum) - 1)]);
	}
	return string(str);
}

size_t bench_naive(string text, vector<string> patterns) {
	size_t count = 0;
	for (auto& pattern : patterns) {
		size_t pos = text.find(pattern);
		while (pos != text.npos) {
			pos = text.find(pattern, pos + 1);
			count++;
		}
	}
	return count;
}

size_t bench_aho_corasick(string text, trie& t) {
	size_t count = 0;
	auto matches = t.parse_text(text);
	count += matches.size();
	return count;
}

int main(int argc, char** argv) {
	size_t max_sentences = 10;
	size_t max_patterns = 1000000;

	cout << "*** Aho-Corasick Benchmark ***" << endl;

	cout << "Generating input text ...";
	set<string> input_strings;
	while (input_strings.size() < max_sentences) {
		input_strings.insert(gen_str(256));
	}
	vector<string> input_vector(input_strings.begin(), input_strings.end());
	cout << " done" << endl;

	cout << "Generating search patterns ...";
	set<string> patterns;
	while (patterns.size() < max_patterns) {
		patterns.insert(gen_str(6));
	}
	vector<string> pattern_vector(patterns.begin(), patterns.end());
	cout << " done" << endl;

	cout << "Generating trie ...";
	trie t;
	for (auto& pattern : patterns) {
		t.insert(pattern);
	}
	cout << " done" << endl;

	map<size_t, tuple<
			chrono::high_resolution_clock::duration,
			chrono::high_resolution_clock::duration,
			size_t>> timings;

	cout << "Running ";
	for (size_t i = 0; i < max_sentences; i++) {
		cout << ".";
		auto start_time = chrono::high_resolution_clock::now();
		size_t count_1 = bench_naive(input_vector[i], pattern_vector);
		auto end_time = chrono::high_resolution_clock::now();
		auto time_1 = end_time - start_time;

		start_time = chrono::high_resolution_clock::now();
		size_t count_2 = bench_aho_corasick(input_vector[i], t);
		end_time = chrono::high_resolution_clock::now();
		auto time_2 = end_time - start_time;

		if (count_1 != count_2) {
			cout << "failed" << endl;
		}

		timings[i] = make_tuple(time_1, time_2, count_1 & count_2);
	}
	cout << " done" << endl;

	cout << "Results: " << endl;
	for (auto& i : timings) {
		cout << "  sentence #" << i.first + 1;
		cout << ", matched: " << get<2>(i.second) << "/" << max_patterns;
		cout << ", naive: " << chrono::duration_cast<chrono::milliseconds>(get<0>(i.second)).count();
		cout << "ms, ac: " << chrono::duration_cast<chrono::milliseconds>(get<1>(i.second)).count() << "ms";
		cout << endl;
	}

	return 0;
}