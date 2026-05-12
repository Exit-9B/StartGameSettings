#pragma once

namespace Settings
{
	using group_t = std::int64_t;
	using GroupControl =
		std::variant<std::monostate, group_t, std::vector<std::optional<group_t>>>;

	using GroupControlStore = std::vector<std::pair<group_t, bool>>;

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

	using GroupCondition = std::variant<
		group_t,
		std::vector<group_t>,
		std::unique_ptr<struct GroupCondition_OR>,
		std::unique_ptr<struct GroupCondition_AND>,
		std::unique_ptr<struct GroupCondition_ONLY>,
		std::unique_ptr<struct GroupCondition_NOT>>;

	struct GroupCondition_OR
	{
		GroupCondition OR;

		GroupCondition const& Inner() const { return OR; }

		struct Checker
		{
			GroupControlStore const& store;

			bool operator()(group_t group) const { return IsGroupActive(store, group); }

			bool operator()(std::vector<group_t> const& groups) const
			{
				return std::ranges::any_of(
					groups,
					[&](auto group)
					{
						return IsGroupActive(store, group);
					});
			}

			template <typename T>
			bool operator()(std::unique_ptr<T> const& subtree) const
			{
				return std::visit(typename T::Checker{ store }, subtree->Inner());
			}
		};
	};

	struct GroupCondition_AND
	{
		GroupCondition AND;

		GroupCondition const& Inner() const { return AND; }

		struct Checker
		{
			GroupControlStore const& store;

			bool operator()(group_t const& group) const { return IsGroupActive(store, group); }

			bool operator()(std::vector<group_t> const& groups) const
			{
				return std::ranges::all_of(
					groups,
					[&](auto group)
					{
						return IsGroupActive(store, group);
					});
			}

			template <typename T>
			bool operator()(std::unique_ptr<T> const& subtree) const
			{
				return std::visit(typename T::Checker{ store }, subtree->Inner());
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

			bool operator()(group_t const& group) const
			{
				bool result = true;
				ForEachGroup(
					store,
					[&](group_t g, bool s)
					{
						result &= s == (g == group);
					});
				return result;
			}

			bool operator()(std::vector<group_t> const& groups) const
			{
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

			bool operator()(group_t const& group) const { return !IsGroupActive(store, group); }

			bool operator()(std::vector<group_t> const& groups) const
			{
				return std::ranges::none_of(
					groups,
					[&](auto group)
					{
						return IsGroupActive(store, group);
					});
			}

			template <typename T>
			bool operator()(std::unique_ptr<T> const& subtree) const
			{
				return !std::visit(typename T::Checker{ store }, subtree->Inner());
			}
		};
	};

	using GroupConditionChecker = GroupCondition_OR::Checker;
}
