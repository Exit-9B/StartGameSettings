#pragma once

#include <glaze/glaze.hpp>

namespace Settings
{
	using group_t = std::int64_t;
	using GroupControl =
		std::variant<std::monostate, group_t, std::vector<std::optional<group_t>>>;

	using GroupControlStore = std::vector<std::pair<group_t, bool>>;

	inline void InsertGroup(GroupControlStore& store, group_t group, bool active)
	{
		const auto it = std::ranges::find(
			store,
			group,
			[](auto&& pair)
			{
				return pair.first;
			});

		if (it == std::ranges::end(store)) {
			store.emplace_back(group, active);
		}
		else {
			it->second |= active;
		}
	}

	[[nodiscard]] inline bool IsGroupActive(const GroupControlStore& store, group_t group)
	{
		const auto it = std::ranges::find_if(
			store,
			[group](const auto& pair)
			{
				return pair.first == group;
			});
		return it != std::ranges::end(store) && it->second;
	}

	inline void ForEachGroup(const GroupControlStore& store, auto&& op)
	{
		for (const auto& [group, value] : store) {
			op(group, value);
		}
	}

	struct GroupCondition
	{
		using type = std::variant<
			group_t,
			std::vector<struct GroupCondition>,
			std::unique_ptr<struct GroupCondition_OR>,
			std::unique_ptr<struct GroupCondition_AND>,
			std::unique_ptr<struct GroupCondition_ONLY>,
			std::unique_ptr<struct GroupCondition_NOT>>;

		type variant;
	};

	struct GroupCondition_OR
	{
		GroupCondition OR;

		GroupCondition const& Inner() const { return OR; }

		struct Checker
		{
			GroupControlStore const& store;

			bool operator()(group_t group) const { return IsGroupActive(store, group); }

			bool operator()(std::vector<GroupCondition> const& subtrees) const
			{
				return std::ranges::any_of(
					subtrees,
					[&](const auto& subtree)
					{
						return std::visit(*this, subtree.variant);
					});
			}

			template <typename T>
			bool operator()(std::unique_ptr<T> const& subtree) const
			{
				return std::visit(typename T::Checker{ store }, subtree->Inner().variant);
			}
		};
	};

	using GroupConditionChecker = GroupCondition_OR::Checker;

	struct GroupCondition_AND
	{
		GroupCondition AND;

		GroupCondition const& Inner() const { return AND; }

		struct Checker
		{
			GroupControlStore const& store;

			bool operator()(group_t group) const { return IsGroupActive(store, group); }

			bool operator()(std::vector<GroupCondition> const& subtrees) const
			{
				return std::ranges::all_of(
					subtrees,
					[&](const auto& subtree)
					{
						return std::visit(GroupConditionChecker{ store }, subtree.variant);
					});
			}

			template <typename T>
			bool operator()(std::unique_ptr<T> const& subtree) const
			{
				return std::visit(typename T::Checker{ store }, subtree->Inner().variant);
			}
		};
	};

	struct GroupCondition_ONLY
	{
		GroupCondition ONLY;

		GroupCondition const& Inner() const { return ONLY; }

		struct Checker
		{
			GroupControlStore const& store;

			bool operator()(group_t group) const
			{
				bool result = true;
				ForEachGroup(
					store,
					[&result, group](group_t g, bool s)
					{
						result &= s == (g == group);
					});
				return result;
			}

			bool operator()(std::vector<GroupCondition> const& subtrees) const
			{
				std::vector<group_t> groups;
				groups.reserve(subtrees.size());
				for (const auto& subtree : subtrees) {
					if (std::holds_alternative<group_t>(subtree.variant)) {
						groups.emplace_back(std::get<group_t>(subtree.variant));
					}
					else {
						return false;
					}
				}

				bool result = true;
				ForEachGroup(
					store,
					[&](group_t g, bool s)
					{
						result &= s == (std::ranges::find(groups, g) != std::ranges::end(groups));
					});
				return result;
			}

			template <typename T>
			bool operator()(std::unique_ptr<T> const&) const
			{
				return false;
			}
		};
	};

	struct GroupCondition_NOT
	{
		GroupCondition NOT;

		GroupCondition const& Inner() const { return NOT; }

		struct Checker
		{
			GroupControlStore const& store;

			bool operator()(group_t group) const { return !IsGroupActive(store, group); }

			bool operator()(std::vector<GroupCondition> const& subtrees) const
			{
				return std::ranges::none_of(
					subtrees,
					[&](const auto& subtree)
					{
						return std::visit(GroupConditionChecker{ store }, subtree.variant);
					});
			}

			template <typename T>
			bool operator()(std::unique_ptr<T> const& subtree) const
			{
				return !std::visit(typename T::Checker{ store }, subtree->Inner().variant);
			}
		};
	};
}

namespace glz
{
	template <>
	struct meta<Settings::GroupCondition>
	{
		using mimic = Settings::GroupCondition::type;
		static constexpr auto value = &Settings::GroupCondition::variant;
	};
}
