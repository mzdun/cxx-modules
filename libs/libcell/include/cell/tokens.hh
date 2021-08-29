#pragma once
#include <cstdint>
#include <string_view>
#include <array>

namespace cell {
	namespace fnv {
		// src: https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function#FNV_hash_parameters
		template <size_t bytes_in_size_t> struct consts {};
		template <> struct consts<4> {
			constexpr static const uint_fast32_t prime = 16777619u;
			constexpr static const uint_fast32_t offset = 2166136261u;
		};
		template <> struct consts<8> {
			constexpr static const uint_fast64_t prime = 1099511628211llu;
			constexpr static const uint_fast64_t offset = 14695981039346656037llu;
		};

		constexpr inline size_t ce_1a_hash(std::string_view view) {
			using consts = fnv::consts<sizeof(size_t)>;
			constexpr auto FNV_prime = static_cast<size_t>(consts::prime);
			constexpr auto FNV_offset = static_cast<size_t>(consts::offset);
			size_t hash = FNV_offset;
			for (auto sc : view) {
				const auto uc = static_cast<size_t>(static_cast<unsigned char>(sc));
				hash ^= uc;
				hash *= FNV_prime;
			}
			return hash;
		}
	}

	template <size_t N, typename TreeEntry>
	struct token_tree {
		using element_type = TreeEntry;
		std::array<element_type, N> items;

		template <typename ... Entry>
		constexpr token_tree(Entry&& ... name)
			: items{ element_type(std::forward<Entry>(name))... }
		{
			static_assert(N == sizeof...(Entry), "All entries must be initialized");
			sort_impl(0, N);
		}

		auto begin() const { return items.begin(); }
		auto end() const { return items.end(); }

		bool has(std::string_view tested) const {
			return find(tested) != end();
		}

		bool has(const element_type& probe) const {
			return find(probe) != end();
		}

		auto find(std::string_view tested) const {
			return find(element_type(tested));
		}

		auto find(const element_type& probe) const {
			auto e__ = end();
			auto it = std::lower_bound(begin(), e__, probe);
			while (it != e__
				&& it->hashed == probe.hashed
				&& it->key != probe.key)
			{
				++it;
			}
			return (it != e__ && it->hashed == probe.hashed) ? it : e__;
		}
	private:
		template<class T>
		static constexpr void swap(T& l, T& r)
		{
			T tmp = std::move(l);
			l = std::move(r);
			r = std::move(tmp);
		}
		constexpr void sort_impl(size_t left, size_t right)
		{
			if (left < right)
			{
				size_t m = left;

				for (size_t i = left + 1; i < right; i++)
					if (items[i] < items[left])
						swap(items[++m], items[i]);

				swap(items[left], items[m]);

				sort_impl(left, m);
				sort_impl(m + 1, right);
			}
		}
	};

	struct set_entry {
		size_t hashed{};
		std::string_view key{};
		constexpr explicit set_entry(std::string_view v)
			: hashed(fnv::ce_1a_hash(v)), key(v)
		{}
		set_entry() = default;
		set_entry(set_entry&&) = default;
		set_entry& operator=(set_entry&&) = default;
		constexpr bool operator<(const set_entry& rhs) const {
			if (hashed == rhs.hashed)
				return key < rhs.key;
			return hashed < rhs.hashed;
		}
	};

	template <size_t N>
	struct token_set : token_tree<N, set_entry> {
		using token_tree<N, set_entry>::token_tree;
	};

	template<class... Items>
	token_set(Items...) -> token_set<sizeof...(Items)>;

	template <typename Payload>
	struct map_entry : set_entry {
		Payload value{};
		constexpr explicit map_entry(std::string_view v, Payload payload)
			: set_entry(v), value(std::move(payload))
		{}
		constexpr explicit map_entry(std::string_view v): set_entry(v)
		{}
		map_entry() = default;
		map_entry(map_entry&&) = default;
		map_entry& operator=(map_entry&&) = default;
	};

	template <typename Payload>
	struct remove_map_entry { using type = Payload; };

	template <typename Payload>
	struct remove_map_entry<map_entry<Payload>> : remove_map_entry<Payload> {};

	template <typename Payload>
	struct remove_map_entry<const Payload> : remove_map_entry<Payload> {};

	template <typename Payload>
	struct remove_map_entry<volatile Payload> : remove_map_entry<Payload> {};

	template <size_t N, typename Payload>
	struct token_map : token_tree<N, map_entry<Payload>> {
		using token_tree<N, map_entry<Payload>>::token_tree;
	};

	template<class _First, class... _Rest>
	token_map(_First, _Rest...)
		->token_map<1 + sizeof...(_Rest), typename remove_map_entry<_First>::type>;
}