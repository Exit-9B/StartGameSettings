#pragma once

#include "Settings/GroupCondition.hpp"

#include <enchantum/enchantum.hpp>
#include <glaze/glaze.hpp>

namespace Settings
{
	enum struct SourceType
	{
		GlobalValue,
		GameSetting,
		INIPrefSetting,
	};

	enum struct Type
	{
		slider,
		stepper,
		toggle,
	};

	using Help = std::variant<std::monostate, std::string, std::vector<std::string>>;

	template <Type TYPE>
	struct ValueOptionsA;

	template <>
	struct ValueOptionsA<Type::slider>
	{
		SourceType sourceType;
		RE::TESForm* sourceForm;
		double defaultValue;

		double min;
		double max;
		double step;
	};

	template <>
	struct ValueOptionsA<Type::stepper>
	{
		SourceType sourceType;
		RE::TESForm* sourceForm;
		double defaultValue;

		std::vector<std::string> options;
	};

	template <>
	struct ValueOptionsA<Type::toggle>
	{
		SourceType sourceType;
		RE::TESForm* sourceForm;
		double defaultValue;
	};

	template <Type TYPE>
	struct ItemA
	{
		std::optional<std::string> id;
		std::string text;
		Help help;
		GroupControl groupControl;
		std::optional<GroupCondition> groupCondition;
		std::optional<ValueOptionsA<TYPE>> valueOptions;

		[[nodiscard]] inline double FetchValue() const;

		inline void StoreValue(double value) const;
	};

	struct Item
	{
		using type = decltype(
			[]<std::size_t... I>(std::index_sequence<I...>)
				 -> std::variant<ItemA<*enchantum::index_to_enum<Type>(I)>...>
			{
				return {};
			}(std::make_index_sequence<enchantum::count<Type>>()));

		type variant;

		[[nodiscard]] inline GroupControl const& groupControl() const;

		[[nodiscard]] inline std::optional<SourceType> sourceType() const;

		[[nodiscard]] inline double FetchValue() const;

		inline void StoreValue(double value) const;

		[[nodiscard]] inline bool IsActive(const GroupControlStore& store) const;
	};

	struct Collection
	{
		std::vector<Item> NewGame;
	};
}

namespace glz
{
	template <typename T>
		requires std::is_enum_v<T>
	struct meta<T>
	{
		static constexpr auto keys = enchantum::names<T>;
		static constexpr auto value = enchantum::values<T>;
	};

	template <>
	struct from<JSON, RE::TESForm*>
	{
		template <auto Opts>
		static void op(RE::TESForm*& value, is_context auto&& ctx, auto&& it, auto&& end)
		{
			std::string identifier;
			parse<JSON>::op<Opts>(identifier, ctx, it, end);

			std::istringstream ss{ identifier };
			std::string plugin, id;

			std::getline(ss, plugin, '|');
			std::getline(ss, id);
			RE::FormID relativeID;
			std::istringstream(id) >> std::hex >> relativeID;
			const auto dataHandler = RE::TESDataHandler::GetSingleton();
			const auto form = dataHandler ? dataHandler->LookupForm(relativeID, plugin) : nullptr;

			value = form;
		}
	};

	template <>
	struct to<JSON, RE::TESForm*>
	{
		template <auto Opts>
		static void op(const RE::TESForm*& value, is_context auto&& ctx, auto&& it, auto&& end)
		{
			if (!value) {
				return;
			}

			const auto file = value->GetFile();
			if (!file) {
				return;
			}

			const auto plugin = file->fileName;

			const RE::FormID formID = value->GetFormID();
			RE::FormID relativeID = formID & 0x00FFFFFF;

			if (file && file->IsLight()) {
				relativeID &= 0x00000FFF;
			}

			std::ostringstream ss;
			ss << plugin << "|" << std::hex << relativeID;
			const auto identifier = ss.str();

			serialize<JSON>::op<Opts>(identifier, ctx, it, end);
		}
	};

	template <>
	struct meta<Settings::Item::type>
	{
		static constexpr std::string_view tag = "type"sv;
		static constexpr auto ids = enchantum::names<Settings::Type>;
	};

	template <>
	struct meta<Settings::Item>
	{
		using mimic = Settings::Item::type;
		static constexpr auto value = &Settings::Item::variant;
	};

	template <>
	struct meta<Settings::Collection>
	{
		static constexpr auto value =
			object("NewGame"sv, append_arrays<&Settings::Collection::NewGame>);
	};
}

#include "Settings/Item.inl"
