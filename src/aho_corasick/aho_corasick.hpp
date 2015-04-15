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

#ifndef AHO_CORASICK_HPP
#define AHO_CORASICK_HPP

#include <algorithm>
#include <cctype>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <queue>
#include <vector>

namespace aho_corasick {

	// interval_i interface
	class interval_i {
	public:
		virtual size_t get_start() const = 0;
		virtual size_t get_end() const = 0;
		virtual size_t size() const = 0;

		bool operator !=(const interval_i& other) const {
			return get_start() != other.get_start() || get_end() != other.get_end();
		}

		bool operator ==(const interval_i& other) const {
			return get_start() == other.get_start() && get_end() == other.get_end();
		}
	};

	// class interval
	class interval: public interval_i {
		size_t d_start;
		size_t d_end;

	public:
		interval(size_t start, size_t end)
			: d_start(start)
			, d_end(end) {}

		size_t get_start() const override { return d_start; }
		size_t get_end() const override { return d_end; }
		size_t size() const override { return d_end - d_start + 1; }

		bool overlaps_with(const interval& other) const {
			return d_start <= other.d_end && d_end >= other.d_start;
		}

		bool overlaps_with(size_t point) const {
			return d_start <= point && point <= d_end;
		}

		bool operator <(const interval_i& other) const {
			return get_start() < other.get_start();
		}
	};

	typedef std::vector<interval> interval_list;

	// class interval_node
	class interval_node {
		enum direction {
			LEFT, RIGHT
		};

		using interval_node_ptr = std::unique_ptr < interval_node > ;

		interval_node_ptr d_left;
		interval_node_ptr d_right;
		size_t            d_point;
		interval_list     d_intervals;

	public:
		interval_node(const interval_list& intervals)
			: d_left(nullptr)
			, d_right(nullptr)
			, d_point(0)
			, d_intervals() {
			d_point = determine_median(intervals);
			interval_list to_left, to_right;
			for (const auto& i : intervals) {
				if (i.get_end() < d_point) {
					to_left.push_back(i);
				} else if (i.get_start() > d_point) {
					to_right.push_back(i);
				} else {
					d_intervals.push_back(i);
				}
			}
			if (to_left.size() > 0) {
				d_left.reset(new interval_node(to_left));
			}
			if (to_right.size() > 0) {
				d_right.reset(new interval_node(to_right));
			}
		}

		virtual ~interval_node() = default;

		size_t determine_median(const interval_list& intervals) const {
			size_t start = -1;
			size_t end = -1;
			for (const auto& i : intervals) {
				size_t cur_start = i.get_start();
				size_t cur_end = i.get_end();
				if (start == -1 || cur_start < start) {
					start = cur_start;
				}
				if (end == -1 || cur_end > end) {
					end = cur_end;
				}
			}
			return (start + end) / 2;
		}

		interval_list find_overlaps(const interval_i& i) {
			interval_list overlaps;
			if (d_point < i.get_start()) {
				add_to_overlaps(i, overlaps, find_overlapping_ranges(d_right, i));
				add_to_overlaps(i, overlaps, check_right_overlaps(i));
			} else if (d_point > i.get_end()) {
				add_to_overlaps(i, overlaps, find_overlapping_ranges(d_left, i));
				add_to_overlaps(i, overlaps, check_left_overlaps(i));
			} else {
				add_to_overlaps(i, overlaps, d_intervals);
				add_to_overlaps(i, overlaps, find_overlapping_ranges(d_left, i));
				add_to_overlaps(i, overlaps, find_overlapping_ranges(d_right, i));
			}
			return interval_list(overlaps);
		}

	protected:
		void add_to_overlaps(const interval_i& i, interval_list& overlaps, interval_list new_overlaps) const {
			for (const auto& cur : new_overlaps) {
				if (cur != i) {
					overlaps.push_back(cur);
				}
			}
		}

		interval_list check_left_overlaps(const interval_i& i) const {
			return interval_list(check_overlaps(i, LEFT));
		}

		interval_list check_right_overlaps(const interval_i& i) const {
			return interval_list(check_overlaps(i, RIGHT));
		}

		interval_list check_overlaps(const interval_i& i, direction d) const {
			interval_list overlaps;
			for (const auto& cur : d_intervals) {
				switch (d) {
				case LEFT:
					if (cur.get_start() <= i.get_end()) {
						overlaps.push_back(cur);
					}
					break;
				case RIGHT:
					if (cur.get_end() >= i.get_start()) {
						overlaps.push_back(cur);
					}
					break;
				}
			}
			return interval_list(overlaps);
		}

		interval_list find_overlapping_ranges(interval_node_ptr& node, const interval_i& i) const {
			if (node) {
				return interval_list(node->find_overlaps(i));
			}
			return interval_list();
		}
	};

	// class interval_tree
	class interval_tree {
		interval_node d_root;

	public:
		interval_tree(const interval_list& intervals)
			: d_root(intervals) {}

		interval_list remove_overlaps(const interval_list& intervals) {
			interval_list result(intervals.begin(), intervals.end());
			std::sort(result.begin(), result.end(), [](const interval_i& a, const interval_i& b) -> bool {
				return a.get_start() > b.get_start();
			});
			std::set<interval> remove_tmp;
			for (const auto& i : result) {
				if (remove_tmp.find(i) != remove_tmp.end()) {
					continue;
				}
				auto overlaps = find_overlaps(i);
				for (const auto& overlap : overlaps) {
					remove_tmp.insert(overlap);
				}
			}
			for (const auto& i : remove_tmp) {
				result.erase(
					std::find(result.begin(), result.end(), i)
					);
			}
			std::sort(result.begin(), result.end(), [](const interval_i& a, const interval_i& b) -> bool {
				if (b.size() - a.size() == 0) {
					return a.get_start() > b.get_start();
				}
				return a.size() > b.size();
			});
			return interval_list(result);
		}

		interval_list find_overlaps(const interval_i& i) {
			return interval_list(d_root.find_overlaps(i));
		}
	};

	// trie

	// class emit
	template<typename CharType>
	class emit: public interval {
	public:
		typedef std::basic_string<CharType>  string_type;
		typedef std::basic_string<CharType>& string_ref_type;

		typedef emit<CharType>*              ptr;
		typedef emit<CharType>&              reference;

	private:
		string_type d_keyword;

	public:
		emit(size_t start, size_t end, const string_ref_type keyword)
			: interval(start, end)
			, d_keyword(keyword) {}

		string_type get_keyword() const { return string_type(d_keyword); }
	};

	// class token
	template<typename CharType>
	class token {
	public:
		using string_type = std::basic_string < CharType > ;
		using string_ref_type = std::basic_string<CharType>&;
		using emit_ptr = std::unique_ptr < emit<CharType> > ;

		typedef token<CharType> *ptr;
		typedef token<CharType> &reference;

	private:
		string_type d_fragment;
		emit_ptr    d_emit;

	public:
		token(const string_ref_type fragment)
			: d_fragment(fragment)
			, d_emit(nullptr) {}

		token(const string_ref_type fragment, emit_ptr e)
			: d_fragment(fragment)
			, d_emit(std::move(e)) {}

		string_type get_fragment() const { return string_type(d_fragment); }
		emit_ptr get_emit() const { return d_emit; }

		virtual bool is_match() const = 0;
	};

	// class fragment_token
	template<typename CharType>
	class fragment_token: public token < CharType > {
	public:
		fragment_token(const typename token<CharType>::string_ref_type fragment)
			: token<CharType>(fragment) {}
		virtual bool is_match() const override { return false; }
	};

	// class match_token
	template<typename CharType>
	class match_token: public token < CharType > {
	public:
		match_token(const typename token<CharType>::string_ref_type fragment, typename token<CharType>::emit_ptr e)
			: token<CharType>(fragment, std::move(e)) {}
		virtual bool is_match() const override { return true; }
	};

	// class state
	template<typename CharType>
	class state {
	public:
		typedef state<CharType>*             ptr;
		typedef state<CharType>&             reference;

		typedef std::basic_string<CharType>  string_type;
		typedef std::basic_string<CharType>& string_ref_type;

		typedef std::set<string_type>        string_collection;
		typedef std::vector<ptr>             state_collection;
		typedef std::vector<CharType>        transition_collection;

	private:
		size_t                     d_depth;
		state<CharType>           *d_root;
		std::map<CharType, ptr>    d_success;
		state<CharType>           *d_failure;
		string_collection          d_emits;

	public:
		state(): state(0) {}

		state(size_t depth)
			: d_depth(depth)
			, d_root(depth == 0 ? this : nullptr)
			, d_success()
			, d_failure(nullptr)
			, d_emits() {}

		//~state() {
		//	d_emits.clear();
		//	d_failure = nullptr;
		//	for (auto s : d_success) {
		//		delete s.second;
		//	}
		//	d_success.clear();
		//	delete d_root;
		//	d_root = nullptr;
		//	d_depth = 0;
		//}

		ptr next_state(CharType character) const {
			return next_state(character, false);
		}

		ptr next_state_ignore_root_state(CharType character) const {
			return next_state(character, true);
		}

		ptr add_state(CharType character) {
			auto next = next_state_ignore_root_state(character);
			if (next == nullptr) {
				next = new state<CharType>(d_depth + 1);
				d_success[character] = next;
			}
			return next;
		}

		size_t get_depth() const { return d_depth; }

		void add_emit(const string_ref_type keyword) {
			d_emits.insert(keyword);
		}

		void add_emit(const string_collection& emits) {
			for (const auto& e : emits) {
				string_type str(e);
				add_emit(str);
			}
		}

		string_collection get_emits() const { return d_emits; }

		ptr failure() const { return d_failure; }

		void set_failure(ptr fail_state) { d_failure = fail_state; }

		state_collection get_states() const {
			state_collection result;
			for (auto it = d_success.cbegin(); it != d_success.cend(); ++it) {
				result.push_back(it->second);
			}
			return state_collection(result);
		}

		transition_collection get_transitions() const {
			transition_collection result;
			for (auto it = d_success.cbegin(); it != d_success.cend(); ++it) {
				result.push_back(it->first);
			}
			return transition_collection(result);
		}

	private:
		ptr next_state(CharType character, bool ignore_root_state) const {
			ptr result = nullptr;
			auto found = d_success.find(character);
			if (found != d_success.end()) {
				result = found->second;
			} else if (!ignore_root_state && d_root != nullptr) {
				result = d_root;
			}
			return result;
		}
	};

	template<typename CharType = wchar_t>
	class basic_trie {
	public:
		using string_type = std::basic_string < CharType > ;
		using string_ref_type = std::basic_string<CharType>&;

		typedef state<CharType>              state_type;
		typedef state<CharType>*             state_ptr_type;
		typedef token<CharType>              token_type;
		typedef token<CharType>*             token_ptr_type;
		typedef emit<CharType>               emit_type;
		typedef emit<CharType>*              emit_ptr_type;
		typedef std::vector<token_ptr_type>  token_collection; // todo: change to unique_ptrs?
		typedef std::vector<emit_ptr_type>   emit_collection;  // todo: change to unique_ptrs?

		class config {
			bool d_allow_overlaps;
			bool d_only_whole_words;
			bool d_case_insensitive;

		public:
			config()
				: d_allow_overlaps(true)
				, d_only_whole_words(false)
				, d_case_insensitive(false) {}

			bool is_allow_overlaps() const { return d_allow_overlaps; }
			void set_allow_overlaps(bool val) { d_allow_overlaps = val; }

			bool is_only_whole_words() const { return d_only_whole_words; }
			void set_only_whole_words(bool val) { d_only_whole_words = val; }

			bool is_case_insensitive() const { return d_case_insensitive; }
			void set_case_insensitive(bool val) { d_case_insensitive = val; }
		};

	private:
		state_ptr_type   d_root;
		config           d_config;
		bool             d_constructed_failure_states;

	public:
		basic_trie(): basic_trie(config()) {}

		basic_trie(const config& c)
			: d_root(new state_type())
			, d_config(c)
			, d_constructed_failure_states(false) {}

		~basic_trie() {
			delete d_root;
			d_root = nullptr;
		}

		basic_trie& case_insensitive() {
			d_config.set_case_insensitive(true);
			return (*this);
		}

		basic_trie& remove_overlaps() {
			d_config.set_allow_overlaps(false);
			return (*this);
		}

		basic_trie& only_whole_words() {
			d_config.set_only_whole_words(true);
			return (*this);
		}

		void add_keyword(const string_ref_type keyword) {
			if (keyword.empty())
				return;
			state_ptr_type cur_state = d_root;
			for (const auto& c : keyword) {
				cur_state = cur_state->add_state(c);
			}
			cur_state->add_emit(keyword);
		}

		token_collection tokenise(const string_ref_type text) {
			token_collection tokens;
			auto collected_emits = parse_text(text);
			size_t last_pos = -1;
			for (const auto& e : collected_emits) {
				if (e->get_start() - last_pos > 1) {
					tokens.push_back(create_fragment(e, text, last_pos));
				}
				tokens.push_back(create_match(e, text));
				last_pos = e->get_end();
			}
			if (text.size() - last_pos > 1) {
				tokens.push_back(create_fragment(nullptr, text, last_pos));
			}
			return token_collection(tokens);
		}

		emit_collection parse_text(const string_ref_type text) {
			check_construct_failure_states();
			size_t pos = 0;
			state_ptr_type cur_state = d_root;
			emit_collection collected_emits;
			for (auto c : text) {
				if (d_config.is_case_insensitive()) {
					c = std::tolower(c);
				}
				cur_state = get_state(cur_state, c);
				store_emits(pos, cur_state, collected_emits);
				pos++;
			}
			if (d_config.is_only_whole_words()) {
				remove_partial_matches(text, collected_emits);
			}
			if (!d_config.is_allow_overlaps()) {
				//interval_tree tree(collected_emits);
				//tree.remove_overlaps(collected_emits);
			}
			return emit_collection(collected_emits);
		}

	private:
		token_ptr_type create_fragment(const emit_ptr_type e, const string_ref_type text, size_t last_pos) const {
			typename fragment_token<CharType>::string_type substr(text.substr(last_pos + 1, (e == nullptr) ? text.size() : e->get_start()));
			return new fragment_token<CharType>(substr);
		}

		token_ptr_type create_match(emit_ptr_type e, const string_ref_type text) const {
			typename match_token<CharType>::string_type substr(text.substr(e->get_start(), e->get_end() + 1));
			return new match_token<CharType>(substr, typename token<CharType>::emit_ptr(e));
		}

		void remove_partial_matches(const string_ref_type search_text, emit_collection& collected_emits) const {
			size_t size = search_text.size();
			emit_collection remove_emits;
			for (const auto& e : collected_emits) {
				if ((e->get_start() == 0 || !std::isalpha(search_text.at(e->get_start() - 1))) &&
					(e->get_end() + 1 == size || !std::isalpha(search_text.at(e->get_end() + 1)))
					) {
					continue;
				}
				remove_emits.push_back(e);
			}
			for (auto& e : remove_emits) {
				collected_emits.erase(
					std::find(collected_emits.begin(), collected_emits.end(), e)
					);
			}
		}

		state_ptr_type get_state(state_ptr_type cur_state, char c) const {
			state_ptr_type result = cur_state->next_state(c);
			while (result == nullptr) {
				cur_state = cur_state->failure();
				result = cur_state->next_state(c);
			}
			return result;
		}

		void check_construct_failure_states() {
			if (!d_constructed_failure_states) {
				construct_failure_states();
			}
		}

		void construct_failure_states() {
			std::queue<state_ptr_type> q;
			for (auto& depth_one_state : d_root->get_states()) {
				depth_one_state->set_failure(d_root);
				q.push(depth_one_state);
			}
			d_constructed_failure_states = true;

			while (!q.empty()) {
				auto cur_state = q.front();
				for (const auto& transition : cur_state->get_transitions()) {
					state_ptr_type target_state = cur_state->next_state(transition);
					q.push(target_state);

					state_ptr_type trace_failure_state = cur_state->failure();
					while (trace_failure_state->next_state(transition) == nullptr) {
						trace_failure_state = trace_failure_state->failure();
					}
					state_ptr_type new_failure_state = trace_failure_state->next_state(transition);
					target_state->set_failure(new_failure_state);
					target_state->add_emit(new_failure_state->get_emits());
				}
				q.pop();
			}
		}

		void store_emits(size_t pos, state_ptr_type cur_state, emit_collection& collected_emits) const {
			auto emits = cur_state->get_emits();
			if (!emits.empty()) {
				for (const auto& str : emits) {
					auto emit_str = typename emit<CharType>::string_type(str);
					collected_emits.push_back(new emit<CharType>(pos - emit_str.size() + 1, pos, emit_str));
				}
			}
		}
	};

	typedef basic_trie<char>     trie;
	typedef basic_trie<wchar_t>  wtrie;
	typedef basic_trie<char16_t> u16trie;
	typedef basic_trie<char32_t> u32trie;

} // namespace aho_corasick

#endif // AHO_CORASICK_HPP