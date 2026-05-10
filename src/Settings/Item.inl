#pragma once

#include "Settings/Collection.hpp"

namespace Settings
{
	[[nodiscard]] inline double FetchFrom(const RE::Setting* setting)
	{
		assert(setting);

		switch (setting->GetType()) {
		case RE::Setting::Type::kBool:
			return setting->GetBool();
		case RE::Setting::Type::kFloat:
			return setting->GetFloat();
		case RE::Setting::Type::kSignedInteger:
			return setting->GetSInt();
		case RE::Setting::Type::kUnsignedInteger:
			return setting->GetUInt();
		}

		return 0.0;
	}

	inline void StoreTo(RE::Setting* setting, double value)
	{
		assert(setting);

		switch (setting->GetType()) {
		case RE::Setting::Type::kBool:
			setting->data.b = static_cast<bool>(value);
			break;
		case RE::Setting::Type::kFloat:
			setting->data.f = static_cast<float>(value);
			break;
		case RE::Setting::Type::kSignedInteger:
			setting->data.i = static_cast<std::int32_t>(value);
			break;
		case RE::Setting::Type::kUnsignedInteger:
			setting->data.u = static_cast<std::uint32_t>(value);
			break;
		}
	}

	template <Type TYPE>
	inline double ItemA<TYPE>::FetchValue() const
	{
		if (!valueOptions)
			return 0.0;

		switch (valueOptions->sourceType) {
		case SourceType::GlobalValue:
		{
			if (const auto global = valueOptions->sourceForm
					? valueOptions->sourceForm->As<RE::TESGlobal>()
					: nullptr)
			{
				return global->value;
			}
		} break;
		case SourceType::GameSetting:
		{
			if (id) {
				if (const auto collection = RE::GameSettingCollection::GetSingleton()) {
					if (const auto setting = collection->GetSetting(id->data())) {
						return FetchFrom(setting);
					}
				}
			}
		} break;
		case SourceType::INIPrefSetting:
		{
			if (id) {
				if (const auto collection = RE::INIPrefSettingCollection::GetSingleton()) {
					if (const auto setting = collection->GetSetting(id->data())) {
						return FetchFrom(setting);
					}
				}
			}
		} break;
		}

		return valueOptions->defaultValue;
	}

	template <Type TYPE>
	inline void ItemA<TYPE>::StoreValue(double value) const
	{
		if (!valueOptions)
			return;

		switch (valueOptions->sourceType) {
		case SourceType::GlobalValue:
		{
			if (const auto global = valueOptions->sourceForm
					? valueOptions->sourceForm->As<RE::TESGlobal>()
					: nullptr)
			{
				global->value = static_cast<float>(value);
			}
		} break;
		case SourceType::GameSetting:
		{
			if (id) {
				if (const auto collection = RE::GameSettingCollection::GetSingleton()) {
					if (const auto setting = collection->GetSetting(id->data())) {
						StoreTo(setting, value);
					}
				}
			}
		} break;
		case SourceType::INIPrefSetting:
		{
			if (id) {
				if (const auto collection = RE::INIPrefSettingCollection::GetSingleton()) {
					if (const auto setting = collection->GetSetting(id->data())) {
						StoreTo(setting, value);
					}

					static constexpr std::string_view iDifficulty = "iDifficulty:GamePlay"sv;
					if (_strnicmp(id->data(), iDifficulty.data(), iDifficulty.size()) == 0) {
						if (const auto player = RE::PlayerCharacter::GetSingleton()) {
							player->difficulty =
								std::clamp(static_cast<std::int32_t>(value), 0, 5);
						}
					}
				}
			}
		} break;
		}
	}
}
