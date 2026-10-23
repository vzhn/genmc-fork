/*
 * GenMC -- Generic Model Checking.
 *
 * This project is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 *
 * Apache License 2.0:
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * MIT License:
 *     https://opensource.org/licenses/MIT
 */


/*
 * The trie of the instruction cache (ADT/Trie.hpp; Scheduler keys the
 * labels it caches by the values a thread read).
 */

#include "genmc/ADT/Trie.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

using Seq = std::vector<int>;
using IntTrie = Trie<Seq, std::string>;

auto add(IntTrie &t, const Seq &s, std::string data) -> bool { return t.addSeq(s, std::move(data)); }

} // namespace

TEST(Trie, TheEmptySequenceIsTheRoot)
{
	IntTrie t(std::string("root"));
	ASSERT_NE(t.lookup({}), nullptr);
	EXPECT_EQ(*t.lookup({}), "root");
	EXPECT_FALSE(add(t, {}, "x"));
	EXPECT_EQ(t.findLongestCommonPrefix({}), 0U);
}

TEST(Trie, ASequenceIsFoundWithItsData)
{
	IntTrie t;
	EXPECT_TRUE(add(t, {1, 2, 3}, "a"));
	ASSERT_NE(t.lookup({1, 2, 3}), nullptr);
	EXPECT_EQ(*t.lookup({1, 2, 3}), "a");
	EXPECT_EQ(t.lookup({1, 2}), nullptr);	   /* a prefix of a label */
	EXPECT_EQ(t.lookup({1, 2, 3, 4}), nullptr); /* longer */
	EXPECT_EQ(t.lookup({2}), nullptr);	   /* no edge */
	EXPECT_EQ(t.lookup({1, 5}), nullptr);	   /* diverges inside a label */
}

TEST(Trie, ASequenceIsAddedOnce)
{
	IntTrie t;
	EXPECT_TRUE(add(t, {1, 2}, "a"));
	EXPECT_FALSE(add(t, {1, 2}, "b"));
	EXPECT_EQ(*t.lookup({1, 2}), "a");
	EXPECT_TRUE(t.setData({1, 2}, "b"));
	EXPECT_EQ(*t.lookup({1, 2}), "b");
	EXPECT_FALSE(t.setData({1, 3}, "c"));
}

TEST(Trie, APrefixSplitsALabel)
{
	IntTrie t;
	add(t, {1, 2, 3, 4}, "long");
	EXPECT_TRUE(add(t, {1, 2}, "prefix"));
	EXPECT_EQ(*t.lookup({1, 2}), "prefix");
	EXPECT_EQ(*t.lookup({1, 2, 3, 4}), "long");
	EXPECT_EQ(t.lookup({1, 2, 3}), nullptr);
}

TEST(Trie, ADivergingSequenceBranchesInsideALabel)
{
	IntTrie t;
	add(t, {1, 2, 3}, "a");
	EXPECT_TRUE(add(t, {1, 2, 7, 8}, "b"));
	EXPECT_TRUE(add(t, {1, 2, 3, 9}, "c")); /* past the end of a's label */
	EXPECT_TRUE(add(t, {0}, "d"));		/* a new first edge, in order */
	EXPECT_EQ(*t.lookup({1, 2, 3}), "a");
	EXPECT_EQ(*t.lookup({1, 2, 7, 8}), "b");
	EXPECT_EQ(*t.lookup({1, 2, 3, 9}), "c");
	EXPECT_EQ(*t.lookup({0}), "d");
	ASSERT_NE(t.lookup({1, 2}), nullptr); /* the split node, with the default data */
	EXPECT_EQ(*t.lookup({1, 2}), "");
}

TEST(Trie, TheLongestCommonPrefixOfASequence)
{
	IntTrie t;
	add(t, {1, 2, 3}, "a");
	add(t, {1, 2, 7, 8}, "b");
	EXPECT_EQ(t.findLongestCommonPrefix({5}), 0U);		  /* no edge */
	EXPECT_EQ(t.findLongestCommonPrefix({1}), 1U);		  /* inside a label */
	EXPECT_EQ(t.findLongestCommonPrefix({1, 2, 9}), 2U);	  /* diverges at a node */
	EXPECT_EQ(t.findLongestCommonPrefix({1, 2, 7, 5}), 3U);	  /* inside the second label */
	EXPECT_EQ(t.findLongestCommonPrefix({1, 2, 3, 4, 5}), 3U); /* past a sequence */
}

TEST(Trie, TheLongestCommonPrefixOfASequenceInTheTrieIsTheSequence)
{
	IntTrie t;
	add(t, {1, 2, 3}, "a");
	add(t, {1, 2, 7, 8}, "b");
	EXPECT_EQ(t.findLongestCommonPrefix({1, 2, 3}), 3U);
	EXPECT_EQ(t.findLongestCommonPrefix({1, 2, 7, 8}), 4U);
}
